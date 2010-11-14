#include "vrpn_HumanInterface.h"
#if defined(VRPN_USE_LIBHID)
#include <usb.h>
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
	// Initialize the HID interface and open the device.
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

// Reconnects the device I/O for the first acceptable device
// Called automatically by constructor, but userland code can
// use it to reacquire a hotplugged device.
void vrpn_HidInterface::reconnect() {
	// Clean up after our old selves
	// XXX CancelIo(_device);
	if (_hid->dev_handle) {
		usb_close(_hid->dev_handle);
		_hid->dev_handle = NULL;
		_hid->device = NULL;
	}

	// Variable initialization
	_acceptor->reset();
	_working = false;

	// Go through the list of USB busses and devices on each bus.  This code is
	// pulled from the hid_find_usb_device() function within libhid.
	// For each device, check against the acceptor function.
	// If it matches, then open it (using code pulled from hid_force_open().
	struct usb_bus	*usbbus = usb_get_busses();
	struct usb_device *usbdev;

	// In at least one Linux implementation, the linked list of buses went
	// right back to the first device on the list, rather than having a NULL
	// next pointer.  So we need to check if we're revisiting the first one
	// and stop if we are.
	bool saw_first_bus = false;
	for (;
	     (usbbus != NULL) && (!saw_first_bus || (usbbus != usb_get_busses()));
	     usbbus->next) {
		// We've checked the first one now.  We're done if we
		// see it again.
		saw_first_bus = true;

		// In at least one Linux implementation, the linked list of devices went
		// right back to the first device on the list, rather than having a NULL
		// next pointer.  So we need to check if we're revisiting the first one
		// and stop if we are.
		bool saw_first_device = false;
		for (usbdev = usbbus->devices;
		     (usbdev != NULL) && (!saw_first_device || (usbdev != usbbus->devices));
		     usbdev = usbdev->next) {
			// We've checked the first one now.  We're done if we
			// see it again.
			saw_first_device = true;

			// Get a handle for the device.
			usb_dev_handle *usbdev_h = usb_open(usbdev);
			if (!usb_claim_interface(usbdev_h, _hid->interface)) {
				fprintf(stderr,"vrpn_HidInterface::reconnect(): Cannot claim interface\n");
				continue;
			}

			// Prepare a vrpn_HIDDEVINFO structure
			vrpn_HIDDEVINFO vrpnInfo;
			vrpnInfo.vendor = usbdev->descriptor.idVendor;
			vrpnInfo.product = usbdev->descriptor.idProduct;
			vrpnInfo.version = 0;	// XXX Don't know where to get this info from.
			// According to test_libhid.c, the usage information can be found by
			// using command-line queries to the device.
			vrpnInfo.usagePage = 0;	// XXX Don't know where to get this info from.
			vrpnInfo.usage = 0;	// XXX Don't know where to get this info from.
			vrpnInfo.devicePath = usbdev->filename;

			// Check to see if it matches.  If so, set the device to
			// this one, mark things as okay, and return.  If not, keep on looking.
			if (_acceptor->accept(vrpnInfo)) {
				if (hid_is_opened(_hid)) {
					fprintf(stderr,"vrpn_HidInterface::reconnect(): Device %s already open\n", _hid->id);
					return;
				}
				_vendor = vrpnInfo.vendor;
				_product = vrpnInfo.product;
				_hid->dev_handle = usbdev_h;
				_hid->device = usbdev;
				//XXX Start IO
				_working = true;
				return;
			}

		}
	}

	// Oops, we ran out of devices somehow
	fprintf(stderr, "vrpn_HidInterface: Couldn't find any acceptable devices!\n");
}

void vrpn_HidInterface::update() {
// XXX		on_data_received(_gotsize, _buffer);
}

#endif

