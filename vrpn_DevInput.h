#ifndef VRPN_DEV_INPUT_H
#define VRPN_DEV_INPUT_H

///////////////////////////////////////////////////////////////////////////
// This file contains a distillation of the various DevInput classes that had
// been spread throughout VRPN.  The interfaces have been rationalized, so
// that they are the same between operating systems and are factored into
// independent interfaces.
///////////////////////////////////////////////////////////////////////////

/* file:	vrpn_DevInput.h
 * author:	Mike Weiblen mew@mew.cx 2004-01-14
 * copyright:	(C) 2003,2004 Michael Weiblen
 * license:	Released to the Public Domain.
 * depends:	gpm 1.19.6, VRPN 06_04
 * tested on:	Linux w/ gcc 2.95.4
 * references:  http://mew.cx/ http://vrpn.org/
 *              http://linux.schottelius.org/gpm/
*/

///////////////////////////////////////////////////////////////////////////
// vrpn_DevInput is a VRPN server class to publish events from the PC's input.
// It provides a 2-channel vrpn_Analog for X & Y input motion, and a
// 3-channel vrpn_Button for the input buttons.
//
// vrpn_DevInput makes it easy to use the diverse array of commodity input
// devices that masquerade as a input, such as PS/2 trackballs, gyroscopic
// free-space pointers, and force-sensing touchpads.
//
// This version includes a Linux-specific implementation that leverages the Linux GPM
// (General Purpose DevInput) server to handle the low-level hardware interfaces
// and device protocols.  GPM is commonly included in Linux distributions.
// The GPM homepage is http://linux.schottelius.org/gpm/
//
// It also includes a Windows interface to the input. (XXX Really?)
//
// The interface reports input position in fraction of the screen.
// The previous version of the Windows implementation had reported them
// in pixels, but this has been changed to match on both platforms.
// 
// vrpn_DevInput must be run on a Linux console, not an xterm.  Rationale:
// 1) Since the console environment doesn't presume the existance of a input,
//    it avoids issues about mapping input events to window focus, etc.
// 2) With the input movement controlled by a different user, it's really
//    not possible to also use a input-based user interface anyway.
// 3) My VRPN server machine is headless, and doesn't even have an X server.
///////////////////////////////////////////////////////////////////////////

#include "vrpn_Analog.h"
#include "vrpn_Button.h"

#ifdef VRPN_USE_DEV_INPUT

class VRPN_API vrpn_DevInput :
	public vrpn_Analog,
	public vrpn_Button_Filter
{
    enum DEVICE_TYPE { DEVICE_KEYBOARD, DEVICE_MOUSE_RELATIVE, DEVICE_MOUSE_ABSOLUTE } d_type;

public:
    vrpn_DevInput( const char* name, vrpn_Connection* cxn, const char *device, const char *type, int mouse_length );
    virtual ~vrpn_DevInput();

    virtual void mainloop();

protected:  // methods
    /// Try to read reports from the device.
    /// Returns 1 if msg received, or 0 if none received.
    virtual int get_report();

    /// send report iff changed
    virtual void report_changes( vrpn_uint32 class_of_service
		    = vrpn_CONNECTION_LOW_LATENCY );

    /// send report whether or not changed
    virtual void report( vrpn_uint32 class_of_service
		    = vrpn_CONNECTION_LOW_LATENCY );

protected:  // data
    struct timeval timestamp;	///< time of last report from device

private:  // disable unwanted default methods
    vrpn_DevInput();
    vrpn_DevInput(const vrpn_DevInput&);
    const vrpn_DevInput& operator=(const vrpn_DevInput&);

 private:
    int d_fileDescriptor;
    vrpn_float64 d_absolute_min;
    vrpn_float64 d_absolute_range;
};

#endif

#endif
