#include "vrpn_RBDFlystick.h"
#include <cstring>
#include <cstdio>

#if defined(VRPN_USE_WINSOCK2)
    #include <winsock2.h>
    #include <ws2tcpip.h>
#elif defined(_WIN32)
    #include <winsock.h>
#else
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
    #include <sys/select.h>
    #include <netdb.h>
#endif

vrpn_RBDFlystick::vrpn_RBDFlystick(const char* name, vrpn_Connection* c,
                                   ConnectionType type, const char* comport, int baud_or_udp_port)
    : vrpn_Analog(name, c)
    , vrpn_Button_Filter(name, c)
    , connection_type(type)
    , buffer_pos(0)
    , serial_fd(-1)
    , udp_socket(0)
    , expected_characters(0)
    , status(0)
    , connected(0)
{
    // 设置设备参数
    vrpn_Button_Filter::num_buttons = 6;  // 6个按钮
    vrpn_Analog::num_channel = 2;         // X和Y轴

    // 初始化时间戳
    vrpn_gettimeofday(&timestamp, NULL);

    // 根据连接类型初始化
    if (type == SERIAL) {
        if (!init_serial_port(comport, baud_or_udp_port)) {
            fprintf(stderr, "vrpn_RBDFlystick: Failed to initialize serial port\n");
            status = 0;
            return;
        }
    } else {  // UDP
        if (!init_udp_socket(baud_or_udp_port)) {
            fprintf(stderr, "vrpn_RBDFlystick: Failed to initialize UDP socket\n");
            status = 0;
            return;
        }
    }

    // 初始化成功
    status = 1;
    connected = 1;
}

vrpn_RBDFlystick::~vrpn_RBDFlystick() {
    if (serial_fd >= 0) {
        vrpn_close_commport(serial_fd);
    }
    if (udp_socket) {
#ifdef _WIN32
        closesocket(udp_socket);
#else
        close(udp_socket);
#endif
    }
}

void vrpn_RBDFlystick::mainloop() {
    // 获取当前时间戳
    struct timeval current_time;
    vrpn_gettimeofday(&current_time, NULL);
    timestamp = current_time;
    server_mainloop();

    // 检查连接状态
    if (!status) {
        return;
    }

    // 处理数据
    if (connection_type == SERIAL && serial_fd >= 0) {
        handle_serial_data();
    } else if (connection_type == UDP && udp_socket) {
        handle_udp_data();
    }

    // 更新基类的时间戳
    vrpn_Button::timestamp = timestamp;
    vrpn_Analog::timestamp = timestamp;

    // finish main loop:

    vrpn_Analog::report_changes(); // report any analog event;
    vrpn_Button::report_changes(); // report any button event;
}

bool vrpn_RBDFlystick::init_serial_port(const char* port, int baud_rate) {
    char port_name[256];

#ifdef _WIN32
    // 检查是否是 COM10 或更高的端口号
    int port_num;
    if (sscanf(port, "COM%d", &port_num) == 1 && port_num >= 10) {
        snprintf(port_name, sizeof(port_name), "\\\\.\\%s", port);
    } else {
        strncpy(port_name, port, sizeof(port_name));
        port_name[sizeof(port_name) - 1] = '\0';
    }
#else
    strncpy(port_name, port, sizeof(port_name));
    port_name[sizeof(port_name) - 1] = '\0';
#endif

    fprintf(stderr, "vrpn_RBDFlystick: Attempting to open serial port %s at %d baud\n", port_name, baud_rate);

    // 尝试打开端口，同时设置串口参数：8数据位，无校验，1停止位
    serial_fd = vrpn_open_commport(port_name, baud_rate, 8, vrpn_SER_PARITY_NONE, false);  // 启用RTS流控
    if (serial_fd == -1) {
#ifdef _WIN32
        DWORD error = GetLastError();
        char error_msg[1024];
        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            error_msg,
            sizeof(error_msg),
            NULL
        );
        fprintf(stderr, "vrpn_RBDFlystick: Failed to open serial port %s: Error %lu - %s\n", 
                port_name, error, error_msg);
#else
        fprintf(stderr, "Failed to open serial port %s: %s\n", 
                port_name, strerror(errno));
#endif
        return false;
    }

    // 清空缓冲区
    vrpn_flush_input_buffer(serial_fd);
    vrpn_flush_output_buffer(serial_fd);

    fprintf(stderr, "vrpn_RBDFlystick: Successfully opened serial port %s\n", port_name);
    return true;
}

bool vrpn_RBDFlystick::init_udp_socket(int port) {
    struct sockaddr_in addr;

    // 创建 socket
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1) {
        return false;
    }

    // 绑定地址
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<unsigned short>(port));
    fprintf(stderr, "vrpn_RBDFlystick: Attempting to bind UDP at %d\n", port);
    if (bind(udp_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        return false;
    }
    fprintf(stderr, "vrpn_RBDFlystick: Succeed to bind UDP at %d\n", port);

    return true;
}

void vrpn_RBDFlystick::handle_serial_data() {
    unsigned char buffer[1024];
    int bytes_read = vrpn_read_available_characters(serial_fd, buffer, sizeof(buffer));

    if (bytes_read > 0) {
        ////打印接收到的原始数据
        //fprintf(stderr, "Received %d bytes: ", bytes_read);
        //for (int i = 0; i < bytes_read; i++) {
        //    fprintf(stderr, "%02X ", buffer[i]);
        //}
        //fprintf(stderr, "\nAs text: ");
        //for (int i = 0; i < bytes_read; i++) {
        //    if (buffer[i] >= 32 && buffer[i] <= 126) {
        //        fprintf(stderr, "%c", buffer[i]);
        //    } else {
        //        fprintf(stderr, ".");
        //    }
        //}
        //fprintf(stderr, "\n");

        for (int i = 0; i < bytes_read; i++) {
            if (buffer_pos < sizeof(read_buffer) - 1) {
                if (buffer[i] == '\r' || buffer[i] == '\n') {
                    if (buffer_pos > 0) {
                        read_buffer[buffer_pos] = '\0';
                        //fprintf(stderr, "Parsing line: %s\n", read_buffer);
                        parse_line(read_buffer);
                        buffer_pos = 0;
                    }
                } else {
                    read_buffer[buffer_pos++] = buffer[i];
                }
            } else {
                buffer_pos = 0;  // 缓冲区溢出保护
            }
        }
    }
}

void vrpn_RBDFlystick::handle_udp_data() {
    fd_set readset;
    struct timeval timeout;
    unsigned char buffer[1024];
    
    FD_ZERO(&readset);
    FD_SET(udp_socket, &readset);
    
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;  // 500ms timeout
    
    if (select(static_cast<int>(udp_socket + 1), &readset, NULL, NULL, &timeout) > 0) {
        struct sockaddr_in client_addr;
#ifdef _WIN32
        int addr_len = sizeof(client_addr);
#else
        socklen_t addr_len = sizeof(client_addr);
#endif
        
        int bytes_read = recvfrom(udp_socket, (char*)buffer, sizeof(buffer)-1, 0,
                                 (struct sockaddr*)&client_addr, &addr_len);
        
        //if (bytes_read > 0) {
        //    buffer[bytes_read] = '\0';
        //    parse_line((char*)buffer);
        //}
        for (int i = 0; i < bytes_read; i++) {
            if (buffer_pos < sizeof(read_buffer) - 1) {
                if (buffer[i] == '\r' || buffer[i] == '\n') {
                    if (buffer_pos > 0) {
                        read_buffer[buffer_pos] = '\0';
                        // fprintf(stderr, "Parsing line: %s\n", read_buffer);
                        parse_line(read_buffer);
                        buffer_pos = 0;
                    }
                }
                else {
                    read_buffer[buffer_pos++] = buffer[i];
                }
            }
            else {
                buffer_pos = 0; // 缓冲区溢出保护
            }
        }
    }
}

void vrpn_RBDFlystick::parse_line(const char* line) {
    if (!line || !*line) {
        return;
    }

    //fprintf(stderr, "Processing line: %s\n", line);

    if (strncmp(line, "Key", 3) == 0 && strlen(line) >= 6) {
        int button_num = line[3] - '0';
        if (button_num > 0 && button_num <= vrpn_Button::num_buttons) {
            buttons[button_num - 1] = (strstr(line, "Pressed") != NULL) ? 1 : 0;
            vrpn_Button::timestamp = timestamp;
            //fprintf(stderr, "Button %d %s\n", button_num, 
            //        buttons[button_num - 1] ? "pressed" : "released");
            vrpn_Button::report_changes(); // report any button event;
        }
    } else if (strncmp(line, "Joystick_", 9) == 0) {
        int value = 0;
        if (sscanf(line + 11, "%d", &value) == 1) {
            if (line[9] == 'X') {
                channel[0] = map_joystick_value(value);
                vrpn_Analog::timestamp = timestamp;
                //fprintf(stderr, "Joystick X: %f\n", channel[0]);
            } else if (line[9] == 'Y') {
                channel[1] = map_joystick_value(value);
                vrpn_Analog::timestamp = timestamp;
                //fprintf(stderr, "Joystick Y: %f\n", channel[1]);
            }
            vrpn_Analog::report_changes(); // report any analog event;
        }
    }
}

double vrpn_RBDFlystick::map_joystick_value(int value) {
    if (value < -100) value = -100;
    if (value > 100) value = 100;
    return static_cast<double>(value) / 100.0;
} 