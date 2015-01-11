#include <stdio.h> // for fprintf, stderr

#include "vrpn_HumanInterface.h"

#if defined(VRPN_USE_HID)

#ifdef VRPN_USE_LOCAL_HIDAPI
#include "./submodules/hidapi/hidapi/hidapi.h" // for hid_device
#else
#include "hidapi.h"
#endif

// Accessor for USB vendor ID of connected device
vrpn_uint16 vrpn_HidInterface::vendor() const { return _vendor; }

// Accessor for USB product ID of connected device
vrpn_uint16 vrpn_HidInterface::product() const { return _product; }

// Accessor for USB interface number of connected device
int vrpn_HidInterface::interface_number() const { return _interface; }

// Returns true iff everything was working last time we checked
bool vrpn_HidInterface::connected() const { return _working; }

vrpn_HidInterface::vrpn_HidInterface(vrpn_HidAcceptor *acceptor)
    : _acceptor(acceptor)
    , _device(NULL)
    , _working(false)
    , _vendor(0)
    , _product(0)
    , _interface(0)
{
    if (_acceptor == NULL) {
        fprintf(stderr,
                "vrpn_HidInterface::vrpn_HidInterface(): NULL acceptor\n");
        return;
    }

    // Reset the acceptor and then attempt to connect to a device.
    _acceptor->reset();
    reconnect();
}

vrpn_HidInterface::~vrpn_HidInterface()
{
    if (_device) {
        hid_close(_device);
        _device = NULL;
    }
}

// Reconnects the device I/O for the first acceptable device
// Called automatically by constructor, but userland code can
// use it to reacquire a hotplugged device.
bool vrpn_HidInterface::reconnect()
{
    // Enumerate all devices and pass each one to the acceptor to see if it
    // is the one that we want.
    struct hid_device_info *devs = hid_enumerate(0, 0);
    struct hid_device_info *loop = devs;
    bool found = false;
    const wchar_t *serial;
    const char *path;
    while ((loop != NULL) && !found) {
        vrpn_HIDDEVINFO device_info;
        device_info.vendor = loop->vendor_id;
        device_info.product = loop->product_id;
        device_info.serial_number = loop->serial_number;
        device_info.manufacturer_string = loop->manufacturer_string;
        device_info.product_string = loop->product_string;
        device_info.interface_number = loop->interface_number;
        // printf("XXX Found vendor 0x%x, product 0x%x, interface %d\n",
        // (unsigned)(loop->vendor_id), (unsigned)(loop->product_id),
        // (int)(loop->interface_number) );

        if (_acceptor->accept(device_info)) {
            _vendor = loop->vendor_id;
            _product = loop->product_id;
            _interface = loop->interface_number;
            serial = loop->serial_number;
            path = loop->path;
            found = true;
#ifdef VRPN_HID_DEBUGGING
            fprintf(stderr, "vrpn_HidInterface::reconnect(): Found %ls %ls "
                            "(%04hx:%04hx) at path %s - will attempt to "
                            "open.\n",
                    loop->manufacturer_string, loop->product_string, _vendor,
                    _product, loop->path);
#endif
        }
        loop = loop->next;
    }
    if (!found) {
        fprintf(stderr, "vrpn_HidInterface::reconnect(): Device not found\n");
        hid_free_enumeration(devs);
        devs = NULL;
        return false;
    }

    // Initialize the HID interface and open the device.
    _device = hid_open_path(path);
    if (_device == NULL) {
        fprintf(stderr,
                "vrpn_HidInterface::reconnect(): Could not open device %s\n",
                path);
#ifdef linux
        fprintf(stderr, "   (Did you remember to run as root?)\n");
#endif
        hid_free_enumeration(devs);
        devs = NULL;
        return false;
    }

    // We cannot do this before the call to open because the serial number
    // is a pointer to a string down in there, which forms a race condition.
    // This will be a memory leak if the device fails to open.
    if (devs != NULL) {
        hid_free_enumeration(devs);
        devs = NULL;
    }

    // Set the device to non-blocking mode.
    if (hid_set_nonblocking(_device, 1) != 0) {
        fprintf(stderr, "vrpn_HidInterface::reconnect(): Could not set device "
                        "to nonblocking\n");
        return false;
    }

#ifdef VRPN_HID_DEBUGGING
    fprintf(stderr,
            "vrpn_HidInterface::reconnect(): Device successfully opened.\n");
#endif
    _working = true;
    return _working;
}

// Check for incoming characters.  If we get some, pass them on to the handler
// code.

void vrpn_HidInterface::update()
{
    if (!_working) {
        // fprintf(stderr,"vrpn_HidInterface::update(): Interface not currently
        // working\n");
        return;
    }

    // Maximum packet size for USB is 512 characters.
    vrpn_uint8 inbuf[512];

    // Continue reading until we eat up all of the reports.  If we only handle
    // one report per loop cycle, we can accumulate latency.
    int ret;
    do {
        ret = hid_read(_device, inbuf, sizeof(inbuf));
        if (ret < 0) {
            fprintf(stderr, "vrpn_HidInterface::update(): Read error\n");
#if !defined(_WIN32) && !defined(__APPLE__)
            fprintf(stderr, "  (On one version of Red Hat Linux, this was from "
                            "not having libusb-devel installed when "
                            "configuring in CMake.)\n");
#endif
            const wchar_t *errmsg = hid_error(_device);
            if (errmsg) {
                fprintf(
                    stderr,
                    "vrpn_HidInterface::update(): error message: %ls\n",
                    errmsg);
            }
            return;
        }

        // Handle any data we got.  This can include fewer bytes than we
        // asked for.
        if (ret > 0) {
            vrpn_uint8 *data =
                static_cast<vrpn_uint8 *>(static_cast<void *>(inbuf));
            on_data_received(ret, data);
        }
    } while (ret > 0);
}

// This is based on sample code from UMinn Duluth at
// http://www.d.umn.edu/~cprince/PubRes/Hardware/LinuxUSB/PICDEM/tutorial1.c
// It has not yet been tested.

void vrpn_HidInterface::send_data(size_t bytes, const vrpn_uint8 *buffer)
{
    if (!_working) {
        fprintf(stderr, "vrpn_HidInterface::send_data(): Interface not "
                        "currently working\n");
        return;
    }
    int ret;
    if ((ret = hid_write(_device, const_cast<vrpn_uint8 *>(buffer), bytes)) !=
        bytes) {
        fprintf(stderr, "vrpn_HidInterface::send_data(): hid_interrupt_write() "
                        "failed with code %d\n",
                ret);
    }
}

void vrpn_HidInterface::send_feature_report(size_t bytes,
                                            const vrpn_uint8 *buffer)
{
    if (!_working) {
        fprintf(stderr, "vrpn_HidInterface::send_feature_report(): Interface "
                        "not currently working\n");
        return;
    }

    int ret = hid_send_feature_report(_device, buffer, bytes);
    if (ret == -1) {
        fprintf(stderr, "vrpn_HidInterface::send_feature_report(): failed to "
                        "send feature report\n");
        const wchar_t *errmsg = hid_error(_device);
        if (errmsg) {
            fprintf(stderr, "vrpn_HidInterface::send_feature_report(): error "
                            "message: %ls\n",
                    errmsg);
        }
    }
    else {
        // fprintf(stderr, "vrpn_HidInterface::send_feature_report(): sent
        // feature report, %d bytes\n", static_cast<int>(bytes));
    }
}

int vrpn_HidInterface::get_feature_report(size_t bytes, vrpn_uint8 *buffer)
{
    if (!_working) {
        fprintf(stderr, "vrpn_HidInterface::get_feature_report(): Interface "
                        "not currently working\n");
        return -1;
    }

    int ret = hid_get_feature_report(_device, buffer, bytes);
    if (ret == -1) {
        fprintf(stderr, "vrpn_HidInterface::get_feature_report(): failed to "
                        "get feature report\n");
        const wchar_t *errmsg = hid_error(_device);
        if (errmsg) {
            fprintf(
                stderr,
                "vrpn_HidInterface::get_feature_report(): error message: %ls\n",
                errmsg);
        }
    }
    else {
        // fprintf(stderr, "vrpn_HidInterface::get_feature_report(): got feature
        // report, %d bytes\n", static_cast<int>(bytes));
    }
    return ret;
}

#endif // any interface
