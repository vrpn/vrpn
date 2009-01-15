// NOTE: This currently only compiles under Windows.

#include "vrpn_HumanInterface.h"

#if defined(_WIN32) || defined(__CYGWIN__)

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

#endif

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

// End of Windows implementation
#endif
