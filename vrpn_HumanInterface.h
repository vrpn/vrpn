// vrpn_HumanInterface.h: Generic USB HID driver I/O interface.
//    This implementation uses the cross-platform HIDAPI library
//    from http://www.signal11.us/oss/hidapi/ to handle the cross-
//    platform HID device interface.  The older version had a different
//    implementation for each platform; it has been discontinued as
//    of version 7.29.

/* For looking at the types of inforamtion that a device can send, we can use
   the following (from a post at microchip.com/forums)

 mcuee@Ubuntu804:~$ lsusb
 Bus 002 Device 010: ID 04f2:0760 Chicony Electronics Co., Ltd
 Bus 002 Device 007: ID ffff:0005
 Bus 002 Device 005: ID 046d:c054 Logitech, Inc.
 Bus 002 Device 004: ID 14c0:0008
 Bus 002 Device 003: ID 1947:0033
 Bus 002 Device 002: ID 058f:9360 Alcor Micro Corp. 8-in-1 Media Card Reader
 Bus 002 Device 001: ID 0000:0000
 Bus 001 Device 001: ID 0000:0000
 mcuee@Ubuntu804:~$ sudo libhid-detach-device 04f2:0760
 Trying to detach HID with IDs 04f2:0760... done.
 mcuee@Ubuntu804:~$ sudo lsusb -vvv | more

 Bus 002 Device 010: ID 04f2:0760 Chicony Electronics Co., Ltd
 Device Descriptor:
   bLength                18
   bDescriptorType         1
*/

#ifndef VRPN_HUMANINTERFACE_H
#define VRPN_HUMANINTERFACE_H

#include <stddef.h> // for size_t
#include <wchar.h>  // for wcscmp

#include "vrpn_Configure.h" // for VRPN_API, VRPN_USE_HID, etc
#include "vrpn_Types.h"     // for vrpn_uint16, vrpn_uint8
#include "vrpn_OwningPtr.h"

struct vrpn_HIDDEVINFO {
    vrpn_uint16 vendor;     // USB Vendor ID
    vrpn_uint16 product;    // USB Product ID
    wchar_t *serial_number; // USB device serial number
    wchar_t *manufacturer_string;
    wchar_t *product_string;
    int interface_number;
    const char *path;
};

// General interface for device enumeration:
// vrpn_HidInterface will connect to the first device your class accepts.
// The reset() method will be called before starting a new enumueration, in
// case you want to connect to the second or third device found that matches
// some criterion.  There are some example HID acceptors at the end of
// this file.
class VRPN_API vrpn_HidAcceptor {
public:
    virtual ~vrpn_HidAcceptor() {}
    virtual bool accept(const vrpn_HIDDEVINFO &device) = 0;
    virtual void reset() {}
};

#if defined(VRPN_USE_HID)

// Forward declarations for hid_device
struct hid_device_;
typedef struct hid_device_ hid_device;

// Main VRPN API for HID devices
class VRPN_API vrpn_HidInterface {
public:
    /// Constructor
    /// If we already have a HID device from some other source, it can be passed
    /// and we'll take ownership: still need the acceptor for reconnect, we just
    /// won't do it right away.
    vrpn_HidInterface(
        /// Determines which device we want
        vrpn_HidAcceptor *acceptor,
        /// Default vendor is "any"
        vrpn_uint16 vendor = 0,
        /// Default product is "any"
        vrpn_uint16 product = 0,
        /// Optional underlying HID device to manage and take ownership of
        hid_device *device = NULL);

    /// Simplified constructor that just takes an acceptor and an underlying HID
    /// device (both non-optional).
    ///
    /// Designed for passing in a device (May look at you funny if you pass a
    /// NULL device, but will work just as if you had called the other
    /// constructor with a null device) but handles NULL devices just fine.
    vrpn_HidInterface(
        /// Determines which device we want
        vrpn_HidAcceptor *acceptor,
        /// Underlying HID device to manage and take ownership of
        hid_device *device);

    /// Constructor
    /// If we already know the path to the device we want, we can pass it in and
    /// open it directly: still need the acceptor for reconnect enumeration, we
    /// just won't do it right away.
    vrpn_HidInterface(
        /// Platform-specific device path, suitable to be passed to HIDAPI's
        /// hid_open_path
        const char *device_path,
        /// Determines which device we want
        vrpn_HidAcceptor *acceptor,
        /// Default vendor is "any"
        vrpn_uint16 vendor = 0,
        /// Default product is "any"
        vrpn_uint16 product = 0);

    virtual ~vrpn_HidInterface();

    /// Returns true iff the last device I/O succeeded
    virtual bool connected() const;

    /// Polls the device buffers and causes on_data_received callbacks if
    /// appropriate
    /// You NEED to call this frequently to ensure the OS doesn't drop data
    virtual void update();

    /// Tries to reconnect to an acceptable device.
    /// Call this if you suspect a hotplug event has occurred.
    virtual bool reconnect();

    /// Returns USB vendor ID of connected device
    /// May not contain valid if an already-open device was provided to the
    /// constructor.
    vrpn_uint16 vendor() const;

    /// Returns USB product ID of connected device
    /// May not contain valid if an already-open device was provided to the
    /// constructor.
    vrpn_uint16 product() const;

    /// Returns the USB interface number of connected device
    /// May not contain valid information on all platforms or if an already-open
    /// device was provided to the constructor.
    int interface_number() const;

protected:
    /** @brief Derived class reimplements this callback.  It is called whenever
       a read returns some data.

        WARNING!  The data returned by this function differs when the device
        sends multiple report types and when it only has one.  When it
        can have more than one, the report type is sent as the first byte.
        When it only has one, the report type is NOT included.  This is
        the behavior of the HIDAPI library we are using.  It is surprising
        to me, but that's how it behaves.
    */
    virtual void on_data_received(size_t bytes, vrpn_uint8 *buffer) = 0;

    /// Call this to send data to the device
    void send_data(size_t bytes, const vrpn_uint8 *buffer);

    /// Call this to send a feature report to the device - first byte must be
    /// Report ID (or 0x0 for devices without numbered reports)
    void send_feature_report(size_t bytes, const vrpn_uint8 *buffer);

    /// Call this to get a feature report from the device - first byte must be
    /// Report ID (or 0x0 for devices without numbered reports)
    /// @return Number of bytes received, or -1 on error
    int get_feature_report(size_t bytes, vrpn_uint8 *buffer);

    /** @brief This is the HidAcceptor we use when reconnecting.

        We do not take ownership of the pointer; it is the user's
       responsibility. Using a stack-allocated acceptor is a really good way to
       get a segfault when calling reconnect()--there won't be an acceptor there
       any longer!
       Thus, always use "new" to make your acceptors.
    */
    vrpn_HidAcceptor *m_acceptor;

    bool m_working;
    vrpn_uint16 m_vendor;
    vrpn_uint16 m_product;
    int m_interface;

    // These are passed in the constructor and kept for use by the
    // reconnect() method.  Strictly speaking, they are redundant
    // because you can put an acceptor that only matches the given
    // vendor and product ID.  However, at least one HID device on at
    // least one server times out when repeatedly polled for its
    // info during hid_enumerate() if we don't check for the correct
    // vendor and product ID and ignore it, so we made this an optional
    // setting in the constructor.
    vrpn_uint16 m_vendor_sought;  //!< What vendor we want
    vrpn_uint16 m_product_sought; //!< What product we want

private:
    // finishing setup of a device, either passed in or recently opened.
    bool finish_setup();
    // Report an error message, which will be augmented with a HIDAPI error
    // message if one is available and desired
    void print_error(const char *function, const char *msg,
                     bool askHIDAPI = true) const;
    // Performs just the HIDAPI portion of print_error.
    void print_hidapi_error(const char *function) const;
    hid_device *m_device; ///< The HID device to use.
};

#endif // VRPN_USE_HID

// Some sample acceptors

/// Always accepts the first device passed. Pointless by itself except for
/// testing.
class VRPN_API vrpn_HidAlwaysAcceptor : public vrpn_HidAcceptor {
public:
    bool accept(const vrpn_HIDDEVINFO &) { return true; }
};

/// Accepts any device with the given vendor and product IDs.
class VRPN_API vrpn_HidProductAcceptor : public vrpn_HidAcceptor {
public:
    vrpn_HidProductAcceptor(vrpn_uint16 vendorId, vrpn_uint16 productId)
        : product(productId)
        , vendor(vendorId)
    {
    }
    bool accept(const vrpn_HIDDEVINFO &device)
    {
        return (device.vendor == vendor) && (device.product == product);
    }

private:
    vrpn_uint16 product, vendor;
};

/// Accepts any device with a particular serial number.
class VRPN_API vrpn_HidSerialNumberAcceptor : public vrpn_HidAcceptor {
public:
    vrpn_HidSerialNumberAcceptor(const wchar_t *serial)
        : devNum(serial)
    {
    }
    bool accept(const vrpn_HIDDEVINFO &device)
    {
        return !wcscmp(devNum, device.serial_number);
    }

private:
    const wchar_t *devNum;
};

/// Accepts any device with a particular interface number. Best in conjunction
/// with vrpn_HidBooleanAndAcceptor.
class VRPN_API vrpn_HidInterfaceNumberAcceptor : public vrpn_HidAcceptor {
public:
    vrpn_HidInterfaceNumberAcceptor(int iface)
        : _iface(iface)
    {
    }
    bool accept(const vrpn_HIDDEVINFO &device)
    {
        return _iface == device.interface_number;
    }

private:
    const int _iface;
};

/** @brief Accepts the Nth device matching a given acceptor.

    This demonstrates the composition of acceptors, allowing the user/programmer
   to create a more specific Hid object without needing to duplicate code all
   over the place.
*/
class VRPN_API vrpn_HidNthMatchAcceptor : public vrpn_HidAcceptor {
public:
    vrpn_HidNthMatchAcceptor(size_t index, vrpn_HidAcceptor *acceptor)
        : target(index)
        , found(0)
        , delegate(acceptor)
    {
    }

    virtual ~vrpn_HidNthMatchAcceptor() {}

    bool accept(const vrpn_HIDDEVINFO &device)
    {
        return delegate->accept(device) && (found++ == target);
    }
    void reset()
    {
        found = 0;
        delegate->reset();
    }

private:
    size_t target, found;
    vrpn::OwningPtr<vrpn_HidAcceptor> delegate;
};

/// Accepts only devices meeting two criteria. NOT SHORT-CIRCUIT.
/// Another demonstration of acceptor composition.
class VRPN_API vrpn_HidBooleanAndAcceptor : public vrpn_HidAcceptor {
public:
    vrpn_HidBooleanAndAcceptor(vrpn_HidAcceptor *p, vrpn_HidAcceptor *q)
        : first(p)
        , second(q)
    {
    }

    virtual ~vrpn_HidBooleanAndAcceptor() {}

    bool accept(const vrpn_HIDDEVINFO &device)
    {
        bool p = first->accept(device);
        bool q = second->accept(device);
        return p && q;
    }

    void reset()
    {
        first->reset();
        second->reset();
    }

private:
    vrpn::OwningPtr<vrpn_HidAcceptor> first;
    vrpn::OwningPtr<vrpn_HidAcceptor> second;
};

/// Accepts devices meeting at least one of two criteria. NOT SHORT-CIRCUIT.
/// Another demonstration of acceptor composition.
class VRPN_API vrpn_HidBooleanOrAcceptor : public vrpn_HidAcceptor {
public:
    vrpn_HidBooleanOrAcceptor(vrpn_HidAcceptor *p, vrpn_HidAcceptor *q)
        : first(p)
        , second(q)
    {
    }

    virtual ~vrpn_HidBooleanOrAcceptor() {}

    bool accept(const vrpn_HIDDEVINFO &device)
    {
        bool p = first->accept(device);
        bool q = second->accept(device);
        return p || q;
    }

    void reset()
    {
        first->reset();
        second->reset();
    }

private:
    vrpn::OwningPtr<vrpn_HidAcceptor> first;
    vrpn::OwningPtr<vrpn_HidAcceptor> second;
};

/// @todo Operators for these acceptors (really predicates) ?

#endif // VRPN_HUMANINTERFACE_H
