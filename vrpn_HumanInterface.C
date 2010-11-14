#include "vrpn_HumanInterface.h"
#if defined(VRPN_USE_LIBHID)
#include <usb.h>
#include <errno.h>
#endif

#if !defined(VRPN_USE_LIBHID) && ( defined(_WIN32) || defined(__CYGWIN__) )

// SetupAPI provides the device enumeration stuff
#pragma comment(lib, "setupapi.lib")
#include <setupapi.h>

// Include some of the relevant definitions from hidsdi.h
// You're free to use the real hidsdi.h from the Windows DDK if
// you want to, but it requires a lot of compiler magic to get
// all the security checks working. This doesn't require a CD
// download from Microsoft either...
#ifndef _HIDSDI_H
#define _HIDSDI_H

typedef USHORT USAGE;

typedef struct {
	ULONG Size;
	USHORT VendorID;
	USHORT ProductID;
	USHORT VersionNumber;
} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;

typedef struct _HIDP_CAPS
{
	USAGE    Usage;
	USAGE    UsagePage;
	USHORT   InputReportByteLength;
	USHORT   OutputReportByteLength;
	USHORT   FeatureReportByteLength;
	USHORT   Reserved[17];

	USHORT   NumberLinkCollectionNodes;

	USHORT   NumberInputButtonCaps;
	USHORT   NumberInputValueCaps;
	USHORT   NumberInputDataIndices;

	USHORT   NumberOutputButtonCaps;
	USHORT   NumberOutputValueCaps;
	USHORT   NumberOutputDataIndices;

	USHORT   NumberFeatureButtonCaps;
	USHORT   NumberFeatureValueCaps;
	USHORT   NumberFeatureDataIndices;
} HIDP_CAPS, *PHIDP_CAPS;

// getProc: Quick-and-dirty wrapper around LoadLibrary/GetProcAddress
// - entryPoint: Name of exported function (decorated if __dllexport)
static FARPROC getProc(const char *entryPoint) {
	static HMODULE hdll = NULL;

	if (!hdll)
		hdll = LoadLibrary("HID.DLL"); 

	FARPROC p = GetProcAddress(hdll, entryPoint);
	if (!p) {
		throw __FILE__ ": GetProcAddress failed!";	// Should never happen in here
	}
	return p;
}

// Macro to avoid retyping HID.dll function signatures
// Also ensures that we HAVE our function pointers before we need them
#define HID_FUNC(name, type, args) type (__stdcall *name) args = (type (__stdcall *) args) getProc(#name)

HID_FUNC(HidD_GetHidGuid, void, (LPGUID));
HID_FUNC(HidD_GetPreparsedData, BOOLEAN, (HANDLE, void *));
HID_FUNC(HidD_FreePreparsedData, BOOLEAN, (void *));
HID_FUNC(HidD_GetAttributes, BOOLEAN, (HANDLE, PHIDD_ATTRIBUTES));
HID_FUNC(HidD_GetManufacturerString, BOOLEAN, (HANDLE, PVOID, ULONG));
HID_FUNC(HidD_GetProductString, BOOLEAN, (HANDLE, PVOID, ULONG));
HID_FUNC(HidD_GetSerialNumberString, BOOLEAN, (HANDLE, PVOID, ULONG));
HID_FUNC(HidP_GetCaps, LONG, (void *, PHIDP_CAPS));

#endif // _HIDSDI_H (Windows DDK)

// createUnnamedEvent creates a manual-reset unnamed event object.
// This is just a wrapper around CreateEvent.
static HANDLE createUnnamedEvent() {
	return CreateEvent(
		NULL /* security */, TRUE /* manual-reset */, 
		TRUE /* signaled at start */, NULL /* no name needed */
		);
}

// HID constructor
// Automatically initializes the first device OKed by the acceptor
vrpn_HidInterface::vrpn_HidInterface(vrpn_HidAcceptor *acceptor)
: _acceptor(acceptor), _working(false), _readEvent(NULL), _writeEvent(NULL),
_vendor(0), _product(0), _device(0) {
	_readEvent = createUnnamedEvent();
	if (!_readEvent) {
		fprintf(stderr, "vrpn_HidInterface: Unable to create read event (GLE %u)\n", GetLastError());
		return;
	}
	
	_writeEvent = createUnnamedEvent();
	if (!_writeEvent) {
		fprintf(stderr, "vrpn_HidInterface: Unable to create writing event (GLE %u)\n", GetLastError());
		return;
	}

	reconnect();
}

// HID destructor
// Cleans up any pending I/O requests and closes all handles
vrpn_HidInterface::~vrpn_HidInterface() {
	CancelIo(_device);
	CloseHandle(_readEvent);
	CloseHandle(_writeEvent);
	CloseHandle(_device);
}
#endif // Windows

#if defined(__APPLE__) && !defined(VRPN_USE_LIBHID)
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/errno.h>
#include <sysexits.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <CoreFoundation/CoreFoundation.h>
#endif // Apple

#if defined(_WIN32) || defined(__CYGWIN__) || defined(__APPLE__) || defined(VRPN_USE_LIBHID)
// Accessor for USB vendor ID of connected device
vrpn_uint16 vrpn_HidInterface::vendor() const {
	return _vendor;
}

// Accessor for USB product ID of connected device
vrpn_uint16 vrpn_HidInterface::product() const {
	return _product;
}

// Returns true iff everything was working last time we checked
bool vrpn_HidInterface::connected() const {
	return _working;
}
#endif // any interface

#if !defined(VRPN_USE_LIBHID) && ( defined(_WIN32) || defined(__CYGWIN__) )
// Reconnects the device I/O for the first acceptable device
// Called automatically by constructor, but userland code can
// use it to reacquire a hotplugged device.
void vrpn_HidInterface::reconnect() {
	// Clean up after our old selves
	CancelIo(_device);
	CloseHandle(_device);
	_device = NULL;

	// Variable initialization
	_acceptor->reset();
	_working = false;

	// Get a "device information set" of connected HID drivers
	GUID hidGuid;
	HDEVINFO devInfoSet;
	HidD_GetHidGuid(&hidGuid);
	devInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE | DIGCF_DEVICEINTERFACE);
	if (devInfoSet == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "vrpn_HidInterface: Unable to open device information set (GLE %u)\n", GetLastError());
		return;
	}

	// Loop through all devices searching for a match
	DWORD deviceIndex = 0;
	SP_DEVICE_INTERFACE_DATA ifData = { sizeof(SP_DEVICE_INTERFACE_DATA) };
	while (SetupDiEnumDeviceInterfaces(devInfoSet, NULL, &hidGuid, deviceIndex++, &ifData)) {
		void *preparsed = 0;
		HIDD_ATTRIBUTES hidAttrs = {sizeof(HIDD_ATTRIBUTES)};
		PSP_DEVICE_INTERFACE_DETAIL_DATA detailData;
		SP_DEVINFO_DATA infoData = { sizeof(SP_DEVINFO_DATA) };
		HIDP_CAPS hidCaps = {0};
		vrpn_HIDDEVINFO vrpnInfo = {0};
		
		// Figure out how much storage we'll need
		DWORD neededSize = 0;
		SetupDiGetDeviceInterfaceDetail(devInfoSet, &ifData, NULL, 0, &neededSize, NULL);

		detailData = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>( LocalAlloc(LPTR, neededSize) );
		detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		// Ensure we can retrieve the device path
		if (!SetupDiGetDeviceInterfaceDetail(devInfoSet, &ifData, detailData, neededSize, NULL, &infoData)) {
			goto enumLoopClose;
		}

		// Try to open the device; if we can't, skip it
		_device = CreateFile(detailData->DevicePath, GENERIC_WRITE | GENERIC_READ, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		if (!_device || (_device == INVALID_HANDLE_VALUE)) {
			goto enumLoopClose;
		}

		// Get the HID-specific attributes
		if (!HidD_GetAttributes(_device, &hidAttrs)) {
			goto enumLoopCloseDevice;
		}

		// Use the Windows preparsed data to find the usage page info
		if (!HidD_GetPreparsedData(_device, &preparsed) || !preparsed) {
			goto enumLoopCloseDevice;
		}

		// Get capabilities of the device and then clean up some
		HidP_GetCaps(preparsed, &hidCaps);
		HidD_FreePreparsedData(preparsed);
		
		// Prepare a vrpn_HIDDEVINFO structure
		vrpnInfo.vendor = hidAttrs.VendorID;
		vrpnInfo.product = hidAttrs.ProductID;
		vrpnInfo.version = hidAttrs.VersionNumber;
		vrpnInfo.usagePage = hidCaps.UsagePage;
		vrpnInfo.usage = hidCaps.Usage;
		vrpnInfo.devicePath = detailData->DevicePath;

		// Finally we can test for device acceptance
		if (_acceptor->accept(vrpnInfo)) {
			// Eureka!
			// Clean everything up and return success
			// Keep the device handle open, though!
			LocalFree(detailData);
			SetupDiDestroyDeviceInfoList(devInfoSet);

			// Store the vendor and product ID for later use
			_vendor = hidAttrs.VendorID;
			_product = hidAttrs.ProductID;

			start_io();
			return;
		}

enumLoopCloseDevice:
		CloseHandle(_device);
		_device = NULL;
enumLoopClose:
		LocalFree(detailData);
	}

	// Oops, we ran out of devices somehow
	fprintf(stderr, "vrpn_HidInterface: Couldn't find any acceptable devices!\n");

	SetupDiDestroyDeviceInfoList(devInfoSet);
}

// start_io: If one isn't already pending, starts an asynchronous
// read on the connected USB device
void vrpn_HidInterface::start_io() {
	// Do we need to cancel I/O requests to avoid fragmented reports
	// near the end of our 512-byte reads?
	//
	// Currently this section is disabled for testing.
	// So far everything works, but I can't guarantee it either way. -- CMV
	//
#if 0
	CancelIo(_device);
	Sleep(10);
#endif

	ZeroMemory(&_readOverlap, sizeof(OVERLAPPED));
	_readOverlap.hEvent = _readEvent;
	
	// Latter check added because pending I/O is OK by us
	if (!ReadFile(_device, _readBuffer, 512, NULL, &_readOverlap)) {
		switch (GetLastError()) {
			case ERROR_IO_PENDING:
				// Expected condition; do nothing
				break;
			case ERROR_DEVICE_NOT_CONNECTED:
				fprintf(stderr, "vrpn_HidInterface: Device removed unexpectedly\n");
				_working = false;
				return;
			default:
				fprintf(stderr, "vrpn_HidInterface: ReadFile couldn't start async input (GLE %u)\n", GetLastError());
				_working = false;
				return;
		}
	}

	_working = true;
}

// update: Poll the device for updates.
// If any data reports have been received, this will fire the
// associated on_data_received callback.
// Multiple data reports waiting = one big on_data_received.
void vrpn_HidInterface::update() {
	DWORD rv = WaitForSingleObject(_readEvent, 0);
	if (rv == WAIT_OBJECT_0) {
		// Data received from device
		DWORD numBytes = 0;
		rv = GetOverlappedResult(_device, &_readOverlap, &numBytes, TRUE);
		if (rv) {
			on_data_received(numBytes, _readBuffer);
		}
		else {
			_working = false;
			if (GetLastError() == ERROR_DEVICE_NOT_CONNECTED) {
				fprintf(stderr, "vrpn_HidInterface: Device removed unexpectedly\n");
				ResetEvent(_readEvent);
				return;
			}
			else {
				fprintf(stderr, "vrpn_HidInterface: GetOverlappedResult returned error status (GLE %u)\n", GetLastError());
			}
		}
		ResetEvent(_readEvent);
		start_io();
	}
	else if (rv == WAIT_FAILED) {
		fprintf(stderr, "vrpn_HidInterface: WaitForSingleObject returned error status (GLE %u)\n", GetLastError());
	}
}

// Send data to the connected device.
void vrpn_HidInterface::send_data(size_t bytes, vrpn_uint8 *buffer) {
	OVERLAPPED overlap = {0};

	overlap.hEvent = _writeEvent;

	ResetEvent(_writeEvent);
	if (!WriteFile(_device, buffer, static_cast<DWORD>(bytes), NULL, &overlap)) {
		if (GetLastError() == ERROR_IO_PENDING) {
			// Do nothing--expected condition
		}
		else {
			fprintf(stderr, "vrpn_HidInterface: Unable to start asynchronous write (GLE %u)\n", GetLastError());
			_working = false;
		}
	}
	
	// Wait for write operation to complete
	WaitForSingleObject(_writeEvent, INFINITE);
}
#endif // Windows

#if defined(__APPLE__) && !defined(VRPN_USE_LIBHID)
vrpn_HidInterface::vrpn_HidInterface(vrpn_HidAcceptor *acceptor)
	: _acceptor(acceptor)
	, _ioObject(IO_OBJECT_NULL)
	, _interface(0)
	, _gotdata(0)
	, _gotsize(0)
	, _working(false)
	, _vendor(0)
	, _product(0)
{
	// _buffer
	_acceptor->reset();
	reconnect();
}

vrpn_HidInterface::~vrpn_HidInterface()
{
	// TODO
}

void ReaderReportCallback(
	void *target, IOReturn result, void *refcon, void *sender, MACOSX_HID_UINT32T size
	)
{
	vrpn_HidInterface* vrpnHidInterface = (vrpn_HidInterface*) target;
	unsigned char* buffer = vrpnHidInterface->getBuffer();
//	printf("ReaderReportCallback, got %d bytes for report type %d\n", size, buffer[0]);
/*
	if (size == 2)
	{
		printf("ReaderReportCallback, got %d bytes %d %d\n", size, buffer[0], buffer[1]);
	}
*/
	vrpnHidInterface->gotData(size);
}

void vrpn_HidInterface::reconnect()
{
	io_iterator_t hidObjectIterator = 0;
	io_object_t hidDevice = IO_OBJECT_NULL;
	CFMutableDictionaryRef hidMatchDictionary = IOServiceMatching(kIOHIDDeviceKey);
	IOReturn result = IOServiceGetMatchingServices(kIOMasterPortDefault, hidMatchDictionary, &hidObjectIterator);
	if ((result != kIOReturnSuccess) || (hidObjectIterator == 0))
	{
		fprintf(stderr, "%s\n", "No matching HID class devices found.");
		return;
	}

	if (hidObjectIterator)
	{
		while ((hidDevice = IOIteratorNext(hidObjectIterator)))
		{
			CFMutableDictionaryRef hidProperties = 0;
			int vendorID = 0 , productID = 0 , version = 0 , usage = 0 , usagePage = 0;
			char path[512] = "", manufacturer[256] = "", product[256] = "";
			result = IORegistryEntryCreateCFProperties(hidDevice, &hidProperties, kCFAllocatorDefault, kNilOptions);
			if ((result == KERN_SUCCESS) && hidProperties)
			{
				CFNumberRef vendorIDRef, productIDRef, versionRef, usageRef, usagePageRef;
				CFStringRef manufacturerRef, productRef;

				vendorIDRef = (CFNumberRef)CFDictionaryGetValue(hidProperties, CFSTR(kIOHIDVendorIDKey));
				if (vendorIDRef)
				{
					CFNumberGetValue(vendorIDRef, kCFNumberIntType, &vendorID);
					CFRelease(vendorIDRef);
				}
				productIDRef = (CFNumberRef)CFDictionaryGetValue(hidProperties, CFSTR(kIOHIDProductIDKey));
				if (productIDRef)
				{
					CFNumberGetValue(productIDRef, kCFNumberIntType, &productID);
					CFRelease(productIDRef);
				}
				versionRef = (CFNumberRef)CFDictionaryGetValue(hidProperties, CFSTR(kIOHIDVersionNumberKey));
				if (versionRef)
				{
					CFNumberGetValue(versionRef, kCFNumberIntType, &version);
					CFRelease(versionRef);
				}
				usageRef = (CFNumberRef)CFDictionaryGetValue(hidProperties, CFSTR(kIOHIDPrimaryUsageKey));
				if (usageRef)
				{
					CFNumberGetValue(usageRef, kCFNumberIntType, &usage);
					CFRelease(usageRef);
				}
				usagePageRef = (CFNumberRef)CFDictionaryGetValue(hidProperties, CFSTR(kIOHIDPrimaryUsagePageKey));
				if (usagePageRef)
				{
					CFNumberGetValue(usagePageRef, kCFNumberIntType, &usagePage);
					CFRelease(usagePageRef);
				}
				manufacturerRef = (CFStringRef)CFDictionaryGetValue(hidProperties, CFSTR(kIOHIDManufacturerKey));
				if (manufacturerRef)
				{
					CFStringGetCString(manufacturerRef, manufacturer, 256, CFStringGetSystemEncoding() );
					CFRelease(manufacturerRef);
				}
				productRef = (CFStringRef)CFDictionaryGetValue(hidProperties, CFSTR(kIOHIDProductKey));
				if (productRef)
				{
					CFStringGetCString(productRef, product, 256, CFStringGetSystemEncoding() );
					CFRelease(productRef);
				}
				result = IORegistryEntryGetPath(hidDevice, kIOServicePlane, path);
				// TODO handle result
			}

			vrpn_HIDDEVINFO vrpnInfo = {0};
			vrpnInfo.vendor = vendorID;
			vrpnInfo.product = productID;
			vrpnInfo.version = version;
			vrpnInfo.usagePage = usagePage;
			vrpnInfo.usage =  usage;
			vrpnInfo.devicePath = path;
			if (_acceptor->accept(vrpnInfo))
			{
				fprintf(stderr, "vrpn_HidInterface: found %04x:%04x - %s - %s\n",
					vendorID,
					productID,
					manufacturer,
					product);
				_vendor = vendorID;
				_product = productID;
				_ioObject = hidDevice;
				break;
			}
#if 0
			else
			{
				fprintf(stderr, "vrpn_HidInterface: ignoring device %04x:%04x - %s - %s\n",
					vendorID,
					productID,
					manufacturer,
					product);
			}
#endif
			IOObjectRelease(hidDevice);
		}
		IOObjectRelease(hidObjectIterator);
	}
	if (_ioObject)
	{
		IOCFPlugInInterface **plugInInterface;
		SInt32 score;
		result = IOCreatePlugInInterfaceForService(hidDevice, kIOHIDDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
		if (result != kIOReturnSuccess)
		{
			fprintf(stderr, "vrpn_HidInterface: Unable to IOCreatePlugInInterfaceForService\n");
		}
		IOHIDDeviceInterface122** hidDeviceInterface = 0;
		(*plugInInterface)->QueryInterface(plugInInterface,
										   CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID),
										   (LPVOID *) &(_interface));
		(*plugInInterface)->Release(plugInInterface);
		// we have the _interface

		CFRunLoopSourceRef eventSource;
		mach_port_t port;
		result = (*_interface)->open(_interface, 0);
		if (result != kIOReturnSuccess)
		{
			fprintf(stderr, "vrpn_HidInterface: Unable to open interface\n");
		}
		result = (*_interface)->createAsyncPort(_interface, &port);
		if (result != kIOReturnSuccess)
		{
			fprintf(stderr, "vrpn_HidInterface: Unable to createAsyncPort\n");
		}
		result = (*_interface)->createAsyncEventSource(_interface, &eventSource);
		if (result != kIOReturnSuccess)
		{
			fprintf(stderr, "vrpn_HidInterface: Unable to createAsyncEventSource\n");
		}
		result = (*_interface)->setInterruptReportHandlerCallback(_interface,
														 _buffer, 512,
														 ReaderReportCallback,
														 this, 0);
		if (result != kIOReturnSuccess)
		{
			fprintf(stderr, "vrpn_HidInterface: Unable to setInterruptReportHandlerCallback\n");
		}
		result = (*_interface)->startAllQueues(_interface);
		if (result != kIOReturnSuccess)
		{
			fprintf(stderr, "vrpn_HidInterface: Unable to startAllQueues\n");
		}

		CFRunLoopAddSource(CFRunLoopGetCurrent(), eventSource, kCFRunLoopCommonModes);
		if (CFRunLoopContainsSource(CFRunLoopGetCurrent(), eventSource,
									kCFRunLoopCommonModes) != true)
		{
			fprintf(stderr, "vrpn_HidInterface: Unable to CFRunLoopAddSource\n");
		}
	}
}

void vrpn_HidInterface::update() {
	SInt32 reason = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true);
	if (reason == kCFRunLoopRunHandledSource)
	{
//		printf("kCFRunLoopRunHandledSource\n");
	}

	if (_gotdata)
	{
//		printf("call on_data_received\n");
		on_data_received(_gotsize, _buffer);
		_gotdata = false;
		_gotsize = 0;
	}
}

void vrpn_HidInterface::send_data(size_t bytes, vrpn_uint8 *buffer) {
	// XXX TODO
}

#endif // Apple

#if defined(VRPN_USE_LIBHID)

vrpn_HidInterface::vrpn_HidInterface(vrpn_HidAcceptor *acceptor)
	: _acceptor(acceptor)
	, _hid(NULL)
	, _working(false)
	, _vendor(0)
	, _product(0)
{
	if (acceptor == NULL) {
		fprintf(stderr,"vrpn_HidInterface::vrpn_HidInterface(): NULL acceptor\n");
		return;
	}

	// Initialize the HID interface and open the device.
//	hid_set_debug(HID_DEBUG_ALL);
//	hid_set_debug_stream(stderr);
	hid_set_usb_debug(0);
	hid_return ret;
	if ( (ret = hid_init()) != HID_RET_SUCCESS) {
		fprintf(stderr,"vrpn_HidInterface::vrpn_HidInterface(): hid_init() failed with code %d\n", ret);
		return;
	}
	if ( (_hid = hid_new_HIDInterface()) == NULL) {
		fprintf(stderr,"vrpn_HidInterface::vrpn_HidInterface(): hid_new_Interface() failed\n");
		return;
	}

	// Reset the acceptor and then attempt to connect to a device.
	_acceptor->reset();
	reconnect();
}

vrpn_HidInterface::~vrpn_HidInterface()
{
	hid_return ret;
	if (_hid) {
		// XXX CancelIo(_device);

		hid_return ret;
		if ( (ret = hid_close(_hid)) != HID_RET_SUCCESS) {
			fprintf(stderr,"vrpn_HidInterface::~vrpn_HidInterface(): hid_close() failed with error %d\n", ret);
		}
		_hid->dev_handle = NULL;
		_hid->device = NULL;
		hid_delete_HIDInterface(&_hid);
		_hid = NULL;
	}
	if ( (ret = hid_cleanup()) != HID_RET_SUCCESS) {
		fprintf(stderr,"vrpn_HidInterface::~vrpn_HidInterface(): hid_cleanup() failed with error %d\n", ret);
	}
}

// Wrapper for the libhid match function that pulls needed information
// from the device and then calls the VRPN match function.  It gets a
// pointer to the vrpn_HidInterface object as its custom pointer.

bool vrpn_HidInterface:: match_wrapper(const struct usb_dev_handle *usbdev, void *custom,
					unsigned int /* unused */)
{
	vrpn_HidInterface	*hid_interface = static_cast<vrpn_HidInterface *>(custom);

	// Horrible const() casts happen here because we had to pass a const usb_dev_handle
	// into this function (based on its rquired prototype) but then we need to convert
	// it to its device instance so that we can query things.  But the converter
	// requires a non-const variable.

	// Prepare a vrpn_HIDDEVINFO structure
	vrpn_HIDDEVINFO vrpnInfo;
	vrpnInfo.vendor = usb_device(const_cast<struct usb_dev_handle *>(usbdev))->descriptor.idVendor;
	vrpnInfo.product = usb_device(const_cast<struct usb_dev_handle *>(usbdev))->descriptor.idProduct;
	vrpnInfo.version = 0;	// XXX Don't know where to get this info from.
	// According to test_libhid.c, the usage information can be found by
	// using command-line queries to the device.
	vrpnInfo.usagePage = 0;	// XXX Don't know where to get this info from.
	vrpnInfo.usage = 0;	// XXX Don't know where to get this info from.
	vrpnInfo.devicePath = usb_device(const_cast<struct usb_dev_handle *>(usbdev))->filename;

	// Ask if this is the one.  Return a bool saying whether it is or not.
	return hid_interface->_acceptor->accept(vrpnInfo);
}


// Reconnects the device I/O for the first acceptable device
// Called automatically by constructor, but userland code can
// use it to reacquire a hotplugged device.
void vrpn_HidInterface::reconnect() {
	hid_return ret;
	if (!_hid) {
		return;
	}

	// Clean up after our old selves
	// XXX CancelIo(_device);
	if (_hid->dev_handle) {
		hid_close(_hid);
	}

	// Variable initialization
	_acceptor->reset();
	_working = false;

	// Open the device.  We use a static method to pass to the acceptor function
	// in libHid.  This will be used as the custom acceptor function.  We put in
	// 0 for the vendor and product tag, so that these will always match and
	// the custom function will be the arbiter (it can match on either of these
	// or both, but it gets to decide).
	// NOTE: You need to be root on Linux for this command to complete correctly.
	HIDInterfaceMatcher match = { 0, 0, match_wrapper, this, 0 };
	if ( (ret = hid_open(_hid, 0, &match)) != HID_RET_SUCCESS) {
		fprintf(stderr,"vrpn_HidInterface::reconnect(): Cannot open device (code %d)\n", ret);
		fprintf(stderr,"   (make sure you're running as root so you can open /dev/bus/usb devices for writing)\n");
		return;
	}

	_working = true;
	// XXX Start an asynchronous read I/O transfer.
}

// Check for incoming characters.  If we get some, pass them on to the handler code.
// This is based on the source code for the hid_interrupt_read() function; it goes
// down to the libusb interface directly to get any available characters.

void vrpn_HidInterface::update()
{
	if (!_working) {
		fprintf(stderr,"vrpn_HidInterface::update(): Interface not currently working\n");
		return;
	}

	// We read from endpoint 1.  Endpoint 0 is defined as a system endpoint.
	// It is possible to parse the information from the device to find additional
	// endpoints, but we just use endpoint 1 and hope it is right.  We OR this
	// with 0x80 to mean "input", per the documentation for hid_interrupt_read().
	int ret;
	char inbuf[256];	// Longest set of characters we can receive
	unsigned int timeout = 300;	// Milliseconds, 100 times/second if we have no data.
	// XXX If we wait long enough when calling usb_intterrupt_read(), we get 512 characters
	// but they report a bunch of
	// press/release events from a DreamCheeky that aren't happening.  But if we don't
	// wait long enough, we get no characters... so it does not seem to be sending
	// back a partial report.  This happens whether I use a bulk read or an interrupt
	// read.  It seems to get all or nothing in each case.
	// Someone posted about there being a problem if you used the max size, so I went
	// down to 511 from 512, but that didn't help.
	// DreamCheeky is working, except for the fact that we're waiting for all of
	// the bytes.
	// IF WE use hid_interrupt_read(), then we get 21 bytes of 512 requested whether
	// the timeout is 10ms or 50ms.  With a timeout of 1, we still get 21 bytes, but
	// much more often some are nonzero (3 are nonzero very frequently).  For the
	// DreamCheeky, I'd expect a multiple of 9 bytes.  When I quickly press and
	// release two buttons, I get a bunch of press/release events in the report,
	// as if we've got mis-aligned bytes.  This is a bit strange, because this just
	// goes and calls the usb_interrupt_read() call.  Of course!  hid_interrupt is
	// returning a hid_return type, not the count of characters.  So it is returning
	// a timeout error that way!  ... but them how did me pushing the buttons cause
	// reports to get generated?  It must be that the bytes ARE going in even when
	// the timeout is happening.
	// XXX There is a more subtle libusb_interrupt_transfer() function that is buried
	// down in libusb somewhere... how to get at it?

	unsigned int endpoint = 0x01 | 0x80;
	ret = usb_interrupt_read(_hid->dev_handle, endpoint, inbuf, sizeof(inbuf), timeout);
	if (ret == -ETIMEDOUT) {
		printf("XXX timeout\n");
		return;
	}
	if (ret < 0) {
		fprintf(stderr,"vrpn_HidInterface::update(): failed to read from device %s: %s\n",
			_hid->id, usb_strerror());
		_working = false;
		return;
	}

	// Handle any data we got.
	if (ret > 0) {
		vrpn_uint8 *data = static_cast<vrpn_uint8 *>(static_cast<void*>(inbuf));
		printf("XXX %d bytes of %d requested\n", ret, sizeof(inbuf));
		unsigned XXXnonzero = 0;
		unsigned XXXloop;
		for (XXXloop = 0; XXXloop < ret; XXXloop++) {
			if (data[XXXloop] != 0) { XXXnonzero++; }
		}
		if (XXXnonzero) printf("  (XXX %d were nonzero)\n", XXXnonzero);
		on_data_received(ret, data);
	}
}

// This is based on sample code from UMinn Duluth at
// http://www.google.com/url?sa=t&source=web&cd=4&ved=0CDUQFjAD&url=http%3A%2F%2Fwww.d.umn.edu%2F~cprince%2FPubRes%2FHardware%2FLinuxUSB%2FPICDEM%2Ftutorial1.c&rct=j&q=hid_interrrupt_read&ei=3C3gTKWeN4GClAeX9qCXAw&usg=AFQjCNHp94pwNFKjZTMqrgPfhV1nk7p5Lg&sig2=rG1A7PL-Up0Yv-sbvEMaCw&cad=rja
// It has not yet been tested.

void vrpn_HidInterface::send_data(size_t bytes, vrpn_uint8 *buffer)
{
	unsigned int timeout = 1000;	// Milliseconds
	unsigned int endpoint = 0x1;	// XXX No idea how to pick which one to use.
	char *charbuf = static_cast<char *>(static_cast<void*>(buffer));
	hid_return ret;
	if ( (ret = hid_interrupt_write(_hid, endpoint, charbuf, bytes, timeout)) != HID_RET_SUCCESS) {
		fprintf(stderr,"vrpn_HidInterface::send_data(): hid_interrupt_write() failed with code %d\n", ret);
	}
}

#endif

