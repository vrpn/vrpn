#ifndef VRPN_RBDFLYSTICK_H
#define VRPN_RBDFLYSTICK_H

#include "vrpn_Configure.h"
#include "vrpn_Button.h"
#include "vrpn_Analog.h"
#include "vrpn_Serial.h"
#include "vrpn_BaseClass.h"
#include "vrpn_Connection.h"
#include "vrpn_Shared.h"

class VRPN_API vrpn_RBDFlystick: public vrpn_Analog, 
                                public vrpn_Button_Filter 
{
public:
    enum ConnectionType {
        SERIAL,
        UDP
    };

    vrpn_RBDFlystick(const char* name, vrpn_Connection* c,
                     ConnectionType type, const char* port, int baud_or_udp_port);
    
    virtual ~vrpn_RBDFlystick();

    virtual void mainloop();

protected:
    ConnectionType connection_type;
    int serial_fd;
    vrpn_SOCKET udp_socket;
    char read_buffer[1024]; 
    int buffer_pos;         

    bool init_serial_port(const char* port, int baud_rate);
    bool init_udp_socket(int port);

    void handle_serial_data();
    void handle_udp_data();
    void parse_line(const char* line);
    double map_joystick_value(int value);

    int expected_characters;
    unsigned char status;
    struct timeval timestamp;
    int connected;
};

#endif // VRPN_RBDFLYSTICK_H 