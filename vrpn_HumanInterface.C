#include <stdio.h> // for fprintf, stderr

#include "vrpn_HumanInterface.h"

#if defined(VRPN_USE_HID)

#ifdef VRPN_USE_LOCAL_HIDAPI
#include "./submodules/hidapi/hidapi/hidapi.h" // for hid_device
#else
#include "hidapi.h"
#endif

// Accessor for USB vendor ID of connected device
vrpn_uint16 vrpn_HidInterface::vendor() const { return m_vendor; }

// Accessor for USB product ID of connected device
vrpn_uint16 vrpn_HidInterface::product() const { return m_product; }

// Accessor for USB interface number of connected device
int vrpn_HidInterface::interface_number() const { return m_interface; }

// Returns true iff everything was working last time we checked
bool vrpn_HidInterface::connected() const { return m_working; }

vrpn_HidInterface::vrpn_HidInterface(vrpn_HidAcceptor *acceptor,
                                     hid_device *device)
    : m_acceptor(acceptor)
    , m_working(false)
    , m_vendor(0)
    , m_product(0)
    , m_interface(0)
    , m_vendor_sought(0 /*vendor*/)
    , m_product_sought(0 /*product*/)
    , m_device(device)
{
    if (!m_acceptor) {
        print_error("vrpn_HidInterface", "NULL acceptor", false);
        return;
    }
    if (!m_device) {
        /// Not given an already-open device, hmm.
        m_acceptor->reset();
        reconnect();
        return;
    }
    // We'll let this method set m_working for us, if it is, in fact, working.
    finish_setup();
}

vrpn_HidInterface::vrpn_HidInterface(vrpn_HidAcceptor *acceptor,
                                     vrpn_uint16 vendor, vrpn_uint16 product,
                                     hid_device *device)
    : m_acceptor(acceptor)
    , m_working(false)
    , m_vendor(0)
    , m_product(0)
    , m_interface(0)
    , m_vendor_sought(vendor)
    , m_product_sought(product)
    , m_device(device)
{
    if (!m_acceptor) {
        print_error("vrpn_HidInterface", "NULL acceptor", false);
        return;
    }
    if (!m_device) {
        /// Not given an already-open device.
        m_acceptor->reset();
        reconnect();
        return;
    }
    // We'll let this method set m_working for us, if it is, in fact, working.
    finish_setup();
}

vrpn_HidInterface::vrpn_HidInterface(const char *device_path,
                                     vrpn_HidAcceptor *acceptor,
                                     vrpn_uint16 vendor, vrpn_uint16 product)
    : m_acceptor(acceptor)
    , m_working(false)
    , m_vendor(0)
    , m_product(0)
    , m_interface(0)
    , m_vendor_sought(vendor)
    , m_product_sought(product)
    , m_device(NULL)
{
    if (!m_acceptor) {
        print_error("vrpn_HidInterface", "NULL acceptor", false);
        return;
    }
    if (!device_path || !(device_path[0])) {
        /// Given a null/empty path.
        m_acceptor->reset();
        reconnect();
        return;
    }

    // Initialize the HID interface and open the device.
    m_device = hid_open_path(device_path);
    if (m_device == NULL) {
        fprintf(stderr, "vrpn_HidInterface::vrpn_HidInterface(): Could not "
                        "open device %s\n",
                device_path);
#ifdef linux
        fprintf(stderr, "   (Did you remember to run as root or otherwise set "
                        "permissions?)\n");
#endif
        print_hidapi_error("vrpn_HidInterface");
    }
    // We'll let this method set m_working for us, if it is, in fact, working.
    finish_setup();
}

vrpn_HidInterface::~vrpn_HidInterface()
{
    if (m_device) {
        hid_close(m_device);
        m_device = NULL;
    }
}

namespace {
    /// RAII class to automatically free the enumeration.
    class EnumerationFreer {
    public:
        typedef struct hid_device_info *EnumerationType;
        explicit EnumerationFreer(EnumerationType &devs)
            : devs_(devs)
        {
        }
        ~EnumerationFreer()
        {
            hid_free_enumeration(devs_);
            devs_ = NULL;
        }

    private:
        EnumerationType &devs_;
    };
} // namespace

// Reconnects the device I/O for the first acceptable device
// Called automatically by constructor, but userland code can
// use it to reacquire a hotplugged device.
bool vrpn_HidInterface::reconnect()
{
    // Enumerate all devices and pass each one to the acceptor to see if it
    // is the one that we want.
    struct hid_device_info *devs =
        hid_enumerate(m_vendor_sought, m_product_sought);
    struct hid_device_info *loop = devs;

    // This will free the enumeration when it goes out of scope: has to stick
    // around until after the call to open. The serial number (path?) is a
    // pointer to a
    // string down in there.
    EnumerationFreer freeEnumeration(devs);

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
        device_info.path = loop->path;
        // printf("XXX Found vendor 0x%x, product 0x%x, interface %d\n",
        // (unsigned)(loop->vendor_id), (unsigned)(loop->product_id),
        // (int)(loop->interface_number) );

        if (m_acceptor->accept(device_info)) {
            m_vendor = loop->vendor_id;
            m_product = loop->product_id;
            m_interface = loop->interface_number;
            serial = loop->serial_number;
            path = loop->path;
            found = true;
#ifdef VRPN_HID_DEBUGGING
            fprintf(stderr, "vrpn_HidInterface::reconnect(): Found %ls %ls "
                            "(%04hx:%04hx) at path %s - will attempt to "
                            "open.\n",
                    loop->manufacturer_string, loop->product_string, m_vendor,
                    m_product, loop->path);
#endif
        }
        loop = loop->next;
    }
    if (!found) {
        // fprintf(stderr, "vrpn_HidInterface::reconnect(): Device not
        // found\n");
        return false;
    }

    // Initialize the HID interface and open the device.
    m_device = hid_open_path(path);
    if (m_device == NULL) {
        fprintf(stderr,
                "vrpn_HidInterface::reconnect(): Could not open device %s\n",
                path);
#ifdef linux
        fprintf(stderr, "   (Did you remember to run as root or otherwise set "
                        "permissions?)\n");
#endif
        print_hidapi_error("reconnect");
        return false;
    }

    return finish_setup();
}

bool vrpn_HidInterface::finish_setup()
{
    if (!m_device) {
        m_working = false;
        return false;
    }
    // Set the device to non-blocking mode.
    if (hid_set_nonblocking(m_device, 1) != 0) {
        print_error("finish_setup", "Could not set device to nonblocking");
        return false;
    }

#ifdef VRPN_HID_DEBUGGING
    fprintf(stderr,
            "vrpn_HidInterface::reconnect(): Device successfully opened.\n");
#endif
    m_working = true;
    return m_working;
}

// Check for incoming characters.  If we get some, pass them on to the handler
// code.

void vrpn_HidInterface::update()
{
    if (!m_working) {
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
        ret = hid_read(m_device, inbuf, sizeof(inbuf));
        if (ret < 0) {
            print_error("update", "Read error");
            m_working = false;
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
    if (!m_working) {
        print_error("send_data", "Interface not currently working", false);
        return;
    }
    int ret;
    if ((ret = hid_write(m_device, const_cast<vrpn_uint8 *>(buffer), bytes)) !=
        static_cast<int>(bytes)) {
        print_error("send_data", "hid_write failed");
    }
}

void vrpn_HidInterface::send_feature_report(size_t bytes,
                                            const vrpn_uint8 *buffer)
{
    if (!m_working) {
        print_error("get_feature_report", "Interface not currently working",
                    false);
        return;
    }

    int ret = hid_send_feature_report(m_device, buffer, bytes);
    if (ret == -1) {
        print_error("send_feature_report", "failed to send feature report");
    }
    else {
        // fprintf(stderr, "vrpn_HidInterface::send_feature_report(): sent
        // feature report, %d bytes\n", static_cast<int>(bytes));
    }
}

int vrpn_HidInterface::get_feature_report(size_t bytes, vrpn_uint8 *buffer)
{
    if (!m_working) {
        print_error("get_feature_report", "Interface not currently working",
                    false);
        return -1;
    }

    int ret = hid_get_feature_report(m_device, buffer, bytes);
    if (ret == -1) {
        print_error("get_feature_report", "failed to get feature report");
    }
    else {
        // fprintf(stderr, "vrpn_HidInterface::get_feature_report(): got feature
        // report, %d bytes\n", static_cast<int>(bytes));
    }
    return ret;
}

void vrpn_HidInterface::print_error(const char *function, const char *msg,
                                    bool askHIDAPI) const
{
    fprintf(stderr, "vrpn_HidInterface::%s(): %s\n", function, msg);
    if (!askHIDAPI || !m_device) {
        return;
    }
    print_hidapi_error(function);
}

void vrpn_HidInterface::print_hidapi_error(const char *function) const
{
    const wchar_t *errmsg = hid_error(m_device);
    if (!errmsg) {
        return;
    }
    fprintf(stderr, "vrpn_HidInterface::%s(): error message: %ls\n", function,
            errmsg);
}

#endif
