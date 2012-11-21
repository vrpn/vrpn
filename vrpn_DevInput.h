#ifndef VRPN_DEV_INPUT_H
#define VRPN_DEV_INPUT_H

///////////////////////////////////////////////////////////////////////////
// This file contains a distillation of the various DevInput classes that had
// been spread throughout VRPN.  The interfaces have been rationalized, so
// that they are the same between operating systems and are factored into
// independent interfaces.
///////////////////////////////////////////////////////////////////////////

/* file:    vrpn_DevInput.h
 * author:    Damien Touraine vrpn-python@limsi.fr 2012-06-20
 * copyright:    (C) 2012 Damien TOUraine
 * license:    Released to the Public Domain.
 * depends:    VRPN 07_30
 * tested on:    Linux w/ gcc 4.6.1
 * references:
*/

///////////////////////////////////////////////////////////////////////////
// vrpn_DevInput is a VRPN server class to publish events from the PC's input.
// It provides a many channels or buttons provided by mice, keyboards and whatever HID device that can be connected found in the /dev/input/event*.
//
// vrpn_DevInput makes it easy to use the diverse array of commodity input
// devices that are managed by Linux through /proc/bus/input/devices
//
// vrpn_DevInput can be run from X-window term or console.
//
// Beware that keyboards are not mapped in locale setting (ie. regional keyboards)
///////////////////////////////////////////////////////////////////////////

#include "vrpn_Analog.h"                // for vrpn_Analog
#include "vrpn_Button.h"                // for vrpn_Button_Filter
#include "vrpn_Configure.h"             // for VRPN_API, etc
#include "vrpn_Connection.h"            // for vrpn_CONNECTION_LOW_LATENCY, etc
#include "vrpn_Shared.h"                // for timeval
#include "vrpn_Types.h"                 // for vrpn_float64, vrpn_uint32

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
