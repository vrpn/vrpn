/*
 *  CheckMacHIDAPI.cpp
 *  
 *
 *  Created by Ryan Pavlik on 11/29/09.
 *
 */


#if defined(__APPLE__)

#include <stdio.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <CoreFoundation/CoreFoundation.h>
void ReaderReportCallback(
						  void *target, IOReturn result, void *refcon, void *sender, MACOSX_HID_UINT32T size
						  )
						  {}
#endif

int main(int argc, char* argv[]) {
#if defined(__APPLE__)
	io_object_t _ioObject;
	IOHIDDeviceInterface122 **_interface;
	bool _gotdata;
	int _gotsize;
	unsigned char _buffer[512];	
	IOReturn result = (*_interface)->setInterruptReportHandlerCallback(_interface,
															  _buffer, 512,
															  ReaderReportCallback,
																	   NULL, 0);
#endif
	return 0;
}