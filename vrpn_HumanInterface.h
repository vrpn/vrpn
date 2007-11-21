// vrpn_HumanInterface.h: Generic USB HID driver I/O interface.

#ifndef VRPN_HUMANINTERFACE_H
#define VRPN_HUMANINTERFACE_H

#include "vrpn_Configure.h"
#include "vrpn_BaseClass.h"
#include <string.h>

#ifdef _WIN64
#pragma message("Windows 64-bit compatibility is EXPERIMENTAL!")
#endif

struct vrpn_HIDDEVINFO {
	vrpn_uint16 vendor;		// USB Vendor ID
	vrpn_uint16 product;		// USB Product ID
	vrpn_uint16 usagePage;		// HID Usage Page (major)
	vrpn_uint16 usage;		// HID Usage (minor)
	vrpn_uint16 version;		// HID Version Number (vendor-specific)
	const char *devicePath;		// Platform-specific path to device
};

// General interface for device enumeration:
// vrpn_HidInterface will connect to the first device your class accepts.
// The reset() method will be called before starting a new enumueration, in
// case you want to connect to the second or third device found that matches
// some criterion.  There are some example HID acceptors at the end of
// this file.
class VRPN_API vrpn_HidAcceptor {
public:
	virtual bool accept(const vrpn_HIDDEVINFO &device) = 0;
	virtual void reset() { }
};

#ifdef _WIN32

// Main VRPN API for HID devices
class VRPN_API vrpn_HidInterface {
public:
	vrpn_HidInterface(vrpn_HidAcceptor *acceptor);
	virtual ~vrpn_HidInterface();

	// Returns true iff the last device I/O succeeded
	virtual bool connected() const;

	// Polls the device buffers and causes on_data_received callbacks if appropriate
	// You NEED to call this frequently to ensure the OS doesn't drop data
	// Note that ReadFile() is buffered by default on Windows, so it doesn't have to
	// be called every 60th of a second unless you've got a really verbose device.
	virtual void update();

	// Tries to reconnect to an acceptable device.
	// Call this if you suspect a hotplug event has occurred.
	virtual void reconnect();

	// Returns USB vendor ID of connected device
	vrpn_uint16 vendor() const;

	// Returns USB product ID of connected device
	vrpn_uint16 product() const;

protected:
	// User reimplements this callback
	virtual void on_data_received(size_t bytes, vrpn_uint8 *buffer) = 0;
	
	// Call this to send data to the device
	void send_data(size_t bytes, vrpn_uint8 *buffer);

	// This is the HidAcceptor we use when reconnecting.
	// We do not take ownership of the pointer; it is the user's responsibility.
	// Using a stack-allocated acceptor is a really good way to get a segfault when
	// calling reconnect()--there won't be an acceptor there any longer!
	vrpn_HidAcceptor *_acceptor;

	// Now the Windows-specific implementation details.
	// Other operating systems will require their own blocks.
#ifdef _WIN32
private:
	void start_io();

	HANDLE _device;
	HANDLE _readEvent;
	HANDLE _writeEvent;
	bool _working;
	OVERLAPPED _readOverlap;

	BYTE _readBuffer[512];

	vrpn_uint16 _vendor;
	vrpn_uint16 _product;
#endif
};

#endif // _WIN32

// Some sample acceptors

// Always accepts the first device passed. Pointless by itself except for testing.
class VRPN_API vrpn_HidAlwaysAcceptor: public vrpn_HidAcceptor {
public:
	bool accept(const vrpn_HIDDEVINFO &) { return true; }
};

// Accepts any device with the given vendor and product IDs.
class VRPN_API vrpn_HidProductAcceptor: public vrpn_HidAcceptor {
public:
	vrpn_HidProductAcceptor(vrpn_uint16 vendorId, vrpn_uint16 productId): product(productId), vendor(vendorId) { }
	bool accept(const vrpn_HIDDEVINFO &device) { return (device.vendor == vendor) && (device.product == product); }
private:
	vrpn_uint16 vendor, product;
};

// Accepts any device with a particular node filename.
// Note that filenames tend to change upon hotplug/replug events.
class VRPN_API vrpn_HidFilenameAcceptor: public vrpn_HidAcceptor {
public:
	vrpn_HidFilenameAcceptor(const char *filename) : devPath(filename) { }
	bool accept(const vrpn_HIDDEVINFO &device) { return !strcmp(devPath, device.devicePath); }
private:
	const char *devPath;
};

// Accepts the Nth device matching a given acceptor.
// This demonstrates the composition of acceptors, allowing the user/programmer
// to create a more specific Hid object without needing to duplicate code all over
// the place.
class VRPN_API vrpn_HidNthMatchAcceptor: public vrpn_HidAcceptor {
public:
	vrpn_HidNthMatchAcceptor(size_t index, vrpn_HidAcceptor *acceptor)
	: target(index), found(0), delegate(acceptor) { }
	bool accept(const vrpn_HIDDEVINFO &device) { return delegate->accept(device) && (found++ == target); }
	void reset() { found = 0; delegate->reset(); }
private:
	size_t found, target;
	vrpn_HidAcceptor *delegate;
};

// Accepts only devices meeting two criteria. NOT SHORT-CIRCUIT.
// Another demonstration of acceptor composition.
class VRPN_API vrpn_HidBooleanAndAcceptor: public vrpn_HidAcceptor {
public:
	vrpn_HidBooleanAndAcceptor(vrpn_HidAcceptor *p, vrpn_HidAcceptor *q)
		: first(p), second(q) { }
	bool accept(const vrpn_HIDDEVINFO &device) {
		bool p = first->accept(device);
		bool q = second->accept(device);
		return p && q;
	}
	void reset() {
		first->reset();
		second->reset();
	}
private:
	vrpn_HidAcceptor *first, *second;
};

#endif // VRPN_HUMANINTERFACE_H

