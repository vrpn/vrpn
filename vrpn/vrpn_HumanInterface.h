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

#if defined(__APPLE__)
#include <stdio.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <CoreFoundation/CoreFoundation.h>

// If using CMake, the correct type name here will be detected.  If not
// using CMake, we have to just guess and pick one of the two that seem
// to be possible and mutually exclusive.
// Chose UInt32 as our fallback (over uint32_t) based on:
// http://developer.apple.com/hardwaredrivers/customusbdrivers.html
// retrieved 16 Dec 2009
#if !defined(MACOSX_HID_UINT32T)
#define MACOSX_HID_UINT32T UInt32
#endif


#endif // Apple

#if defined(_WIN32) || defined(__CYGWIN__) || defined(__APPLE__)

#if defined(__CYGWIN__)
// This will cause _WIN32 to be defined, which will cause us trouble later on.
// It is needed to define the HANDLE type used below.
#include <windows.h>
#undef _WIN32
#endif // Cygwin

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

#if defined(__APPLE__)
	void gotData(int size) {_gotdata = true; _gotsize = size; }
	unsigned char* getBuffer() { return _buffer; }
#endif // Apple

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

private:
#if defined(_WIN32) || defined(__CYGWIN__)
	void start_io();

	HANDLE _device;
	HANDLE _readEvent;
	HANDLE _writeEvent;
	OVERLAPPED _readOverlap;
	BYTE _readBuffer[512];
#endif // Windows
#if defined(__APPLE__)
	io_object_t _ioObject;
	IOHIDDeviceInterface122 **_interface;
	bool _gotdata;
	int _gotsize;
	unsigned char _buffer[512];
#endif // Apple
	bool _working;
	vrpn_uint16 _vendor;
	vrpn_uint16 _product;
};

#endif // Windows or Apple

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
	vrpn_uint16 product, vendor;
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
	size_t target, found;
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

