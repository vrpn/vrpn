#pragma once

#include <stddef.h>                     // for size_t

#include "vrpn_Analog.h"                // for vrpn_Analog
#include "vrpn_BaseClass.h"             // for vrpn_BaseClass
#include "vrpn_Button.h"                // for vrpn_Button_Filter
#include "vrpn_Configure.h"             // for VRPN_CALLBACK, VRPN_USE_HID
#include "vrpn_Connection.h"            // for vrpn_CONNECTION_LOW_LATENCY, etc
#include "vrpn_Dial.h"                  // for vrpn_Dial
#include "vrpn_HumanInterface.h"        // for vrpn_HidAcceptor (ptr only), etc
#include "vrpn_Shared.h"                // for timeval
#include "vrpn_Types.h"                 // for vrpn_uint8, vrpn_uint32

#if defined(VRPN_USE_HID)

// USB vendor and product IDs for the models we support
static const vrpn_uint16 MICROSOFT_VENDOR = 0x045e;
static const vrpn_uint16 SIDEWINDER_PRECISION_2 = 0x0038;
static const vrpn_uint16 SIDEWINDER = 0x003c;
static const vrpn_uint16 XBOX_S = 0x0289;
static const vrpn_uint16 XBOX_360 = 0x028e;
//static const vrpn_uint16 XBOX_360_WIRELESS = 0x028f;	// does not seem to be an HID-compliant device

// and generic controllers that act the same as the above
static const vrpn_uint16 AFTERGLOW_VENDOR = 0x0e6f;
static const vrpn_uint16 AX1_FOR_XBOX_360 = 0x0213;

// Device drivers for the Microsoft Controller Raw USB line of products
// Currently supported: Xbox Controller S, Xbox 360 Controller
//
// Exposes three major VRPN device classes: Button, Analog, Dial (as appropriate).
// All models expose Buttons for the keys on the device.
// Button 0 is the programming switch; it is set if the switch is in the "red" position.
//

class vrpn_Microsoft_Controller_Raw : public vrpn_BaseClass, protected vrpn_HidInterface
{
public:
	vrpn_Microsoft_Controller_Raw(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c = 0);
	virtual ~vrpn_Microsoft_Controller_Raw(void);

	virtual void mainloop(void) = 0;
protected:
	// Set up message handlers, etc.
	void init_hid(void);
	void on_data_received(size_t bytes, vrpn_uint8 *buffer);

	static int VRPN_CALLBACK on_connect(void *thisPtr, vrpn_HANDLERPARAM p);
	static int VRPN_CALLBACK on_last_disconnect(void *thisPtr, vrpn_HANDLERPARAM p);

	virtual void decodePacket(size_t bytes, vrpn_uint8 *buffer) = 0;	
	struct timeval _timestamp;
	vrpn_HidAcceptor *_filter;

	// No actual types to register, derived classes will be buttons, analogs, and/or dials
	int register_types(void) { return (0); }
};

class vrpn_Microsoft_SideWinder_Precision_2: protected vrpn_Microsoft_Controller_Raw, public vrpn_Analog, public vrpn_Button_Filter, public vrpn_Dial
{
public:
	vrpn_Microsoft_SideWinder_Precision_2(const char *name, vrpn_Connection *c = 0);
	virtual ~vrpn_Microsoft_SideWinder_Precision_2(void) {};

	virtual void mainloop(void);
protected:
	// Send report iff changed
	void report_changes (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
	// Send report whether or not changed
	void report (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);

	void decodePacket(size_t bytes, vrpn_uint8 *buffer);
};

class vrpn_Microsoft_SideWinder: protected vrpn_Microsoft_Controller_Raw, public vrpn_Analog, public vrpn_Button_Filter, public vrpn_Dial
{
public:
	vrpn_Microsoft_SideWinder(const char *name, vrpn_Connection *c = 0);
	virtual ~vrpn_Microsoft_SideWinder(void) {};

	virtual void mainloop(void);
protected:
	// Send report iff changed
	void report_changes (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
	// Send report whether or not changed
	void report (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);

	void decodePacket(size_t bytes, vrpn_uint8 *buffer);
};

class vrpn_Microsoft_Controller_Raw_Xbox_S : protected vrpn_Microsoft_Controller_Raw, public vrpn_Analog, public vrpn_Button_Filter, public vrpn_Dial
{
public:
	vrpn_Microsoft_Controller_Raw_Xbox_S(const char *name, vrpn_Connection *c = 0);
	virtual ~vrpn_Microsoft_Controller_Raw_Xbox_S(void) {};

	virtual void mainloop(void);
protected:
	// Send report iff changed
	void report_changes (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
	// Send report whether or not changed
	void report (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);

	void decodePacket(size_t bytes, vrpn_uint8 *buffer);

	// Previous dial value, used to determine delta to send when it changes.
	vrpn_uint8 _lastDial;
};

class vrpn_Microsoft_Controller_Raw_Xbox_360: protected vrpn_Microsoft_Controller_Raw, public vrpn_Analog, public vrpn_Button_Filter, public vrpn_Dial
{
public:
	vrpn_Microsoft_Controller_Raw_Xbox_360(const char *name, vrpn_Connection *c = 0, vrpn_uint16 vendorId = MICROSOFT_VENDOR, vrpn_uint16 productId = XBOX_360);
	virtual ~vrpn_Microsoft_Controller_Raw_Xbox_360(void) {};

	virtual void mainloop(void);
protected:
	// Send report iff changed
	void report_changes (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
	// Send report whether or not changed
	void report (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);

	void decodePacket(size_t bytes, vrpn_uint8 *buffer);

	// Previous dial value, used to determine delta to send when it changes.
	vrpn_uint8 _lastDial;
};

// Generic Xbox 360, same as actual Xbox 360, but other IDs
class vrpn_Afterglow_Ax1_For_Xbox_360 : public vrpn_Microsoft_Controller_Raw_Xbox_360
{
public:
	vrpn_Afterglow_Ax1_For_Xbox_360(const char *name, vrpn_Connection *c) : vrpn_Microsoft_Controller_Raw_Xbox_360(name, c, AFTERGLOW_VENDOR, AX1_FOR_XBOX_360) {};
	virtual ~vrpn_Afterglow_Ax1_For_Xbox_360(void) {};
};

// end of VRPN_USE_HID
#else
class VRPN_API vrpn_Microsoft_Controller_Raw_Xbox_S;
class VRPN_API vrpn_Microsoft_Controller_Raw_Xbox_360;
class VRPN_API vrpn_Afterglow_Ax1_For_Xbox_360;
class VRPN_API vrpn_Microsoft_SideWinder_Precision_2;
class VRPN_API vrpn_Microsoft_SideWinder;
#endif
