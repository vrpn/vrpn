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

// Device drivers for the X-Keys USB line of products from P.I. Engineering
// Currently supported: X-Keys Desktop, X-Keys Jog & Shuttle Pro.
// Theoretically working but untested: X-Keys Professional, X-Keys Joystick Pro.
//
// Exposes three major VRPN device classes: Button, Analog, Dial (as appropriate).
// All models expose Buttons for the keys on the device.
// Button 0 is the programming switch; it is set if the switch is in the "red" position.
//
// For the X-Keys Jog & Shuttle (58-button version):
// Analog channel 0 is the shuttle position (-1 to 1). There are 15 levels.
// Analog channel 1 is the dial orientation (0 to 255).
// Dial channel 0 sends deltas on the dial (in single-tick values, XXX should be 1/10th revs).
//
// For the X-Keys Jog & Shuttle (12- and 68-button versions):
// Analog channel 0 is the shuttle position (-1 to 1). There are 15 levels.
// Analog channel 1 is the dial orientation (total revolutions turned).
// Dial channel 0 sends deltas on the dial (in 1/10th revolution increments).
//      (Note that moving the dial too fast can miss updates.)
//
// For the X-Keys Joystick Pro and 12-button version:
// Analog channel 0 is the joystick X axis (-1 to 1).
// Analog channel 1 is the joystick Y axis (-1 to 1).
// Analog channel 2 is the joystick RZ axis (unbounded, since it can spin freely).
// The Z axis of the joystick is also exported as a dial (a single tick is 1/255th revolution).
class vrpn_Xkeys: public vrpn_BaseClass, protected vrpn_HidInterface {
public:
  vrpn_Xkeys(vrpn_HidAcceptor *filter, const char *name,
      vrpn_Connection *c = 0, vrpn_uint16 vendor = 0, vrpn_uint16 product = 0,
      bool toggle_light = true);
  virtual ~vrpn_Xkeys();

  virtual void mainloop() = 0;

protected:
  // Set up message handlers, etc.
  void init_hid();
  void on_data_received(size_t bytes, vrpn_uint8 *buffer);

  static int VRPN_CALLBACK on_connect(void *thisPtr, vrpn_HANDLERPARAM p);
  static int VRPN_CALLBACK on_last_disconnect(void *thisPtr, vrpn_HANDLERPARAM p);

  // Decode the packet of data in a way that is appropriate to the
  // actual device.
  virtual void decodePacket(size_t bytes, vrpn_uint8 *buffer) = 0;

  // Set the LEDs on the device in a way that is appropriate for the
  // particular device.
  typedef enum { Off, On, Flash } LED_STATE;
  virtual void setLEDs(LED_STATE red, LED_STATE green) = 0;

  struct timeval _timestamp;
  vrpn_HidAcceptor *_filter;
  bool		_toggle_light;

  // No actual types to register, derived classes will be buttons, analogs, and/or dials
  int register_types(void) { return 0; }
};

// Xkeys devices that have no LEDs.  To avoid confusing them, we don't send them
// any commands.
class vrpn_Xkeys_noLEDs: public vrpn_Xkeys {
public:
  vrpn_Xkeys_noLEDs(vrpn_HidAcceptor *filter, const char *name,
      vrpn_Connection *c = 0, vrpn_uint16 vendor = 0, vrpn_uint16 product = 0,
      bool toggle_light = true)
    : vrpn_Xkeys(filter, name, c, vendor, product, toggle_light) { };

protected:
  virtual void setLEDs(LED_STATE, LED_STATE) {};
};

// Original devices that used one method to turn the LEDs on and off.
class vrpn_Xkeys_v1: public vrpn_Xkeys {
public:
  vrpn_Xkeys_v1(vrpn_HidAcceptor *filter, const char *name,
      vrpn_Connection *c = 0, vrpn_uint16 vendor = 0, vrpn_uint16 product = 0,
      bool toggle_light = true)
      : vrpn_Xkeys(filter, name, c, vendor, product, toggle_light) {
      // Indicate we're waiting for a connection by turning on the red LED
      if (_toggle_light) { setLEDs(On, Off); }
    };

  ~vrpn_Xkeys_v1() { if (_toggle_light) setLEDs(Off, Off); };

protected:
  virtual void setLEDs(LED_STATE red, LED_STATE green);
};

// Next generation devices that used another method to turn the LEDs on and off.
class vrpn_Xkeys_v2: public vrpn_Xkeys {
public:
  vrpn_Xkeys_v2(vrpn_HidAcceptor *filter, const char *name,
      vrpn_Connection *c = 0, vrpn_uint16 vendor = 0, vrpn_uint16 product = 0,
      bool toggle_light = true)
      : vrpn_Xkeys(filter, name, c, vendor, product, toggle_light) {
      // Indicate we're waiting for a connection by turning on the red LED
      if (_toggle_light) { setLEDs(On, Off); }
    };

  ~vrpn_Xkeys_v2() { if (_toggle_light) setLEDs(Off, Off); };

protected:
  virtual void setLEDs(LED_STATE red, LED_STATE green);
};

class vrpn_Xkeys_Desktop: protected vrpn_Xkeys_v1, public vrpn_Button_Filter {
public:
  vrpn_Xkeys_Desktop(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_Xkeys_Desktop() {};

  virtual void mainloop();

protected:
  // Send report iff changed
  void report_changes (void);
  // Send report whether or not changed
  void report (void);

  void decodePacket(size_t bytes, vrpn_uint8 *buffer);
};

class vrpn_Xkeys_Pro: protected vrpn_Xkeys_v1, public vrpn_Button_Filter {
public:
  vrpn_Xkeys_Pro(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_Xkeys_Pro() {};

  virtual void mainloop();

protected:
  // Send report iff changed
  void report_changes (void);
  // Send report whether or not changed
  void report (void);

  void decodePacket(size_t bytes, vrpn_uint8 *buffer);
};

// Original unit with 58 buttons
class vrpn_Xkeys_Joystick: protected vrpn_Xkeys_v1, public vrpn_Analog, public vrpn_Button_Filter, public vrpn_Dial {
public:
  vrpn_Xkeys_Joystick(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_Xkeys_Joystick() {};

  virtual void mainloop();

protected:
  // Send report iff changed
  void report_changes (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
  // Send report whether or not changed
  void report (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
  // NOTE:  class_of_service is only applied to vrpn_Analog
  //  values, not vrpn_Button or vrpn_Dial

  void decodePacket(size_t bytes, vrpn_uint8 *buffer);

  // Previous dial value, used to determine delta to send when it changes.
  bool _gotDial;
  vrpn_uint8 _lastDial;
};

// 12-button version of the above unit.
class vrpn_Xkeys_Joystick12 : protected vrpn_Xkeys_v2, public vrpn_Analog, public vrpn_Button_Filter, public vrpn_Dial {
public:
	vrpn_Xkeys_Joystick12(const char *name, vrpn_Connection *c = 0);
	virtual ~vrpn_Xkeys_Joystick12() {};

	virtual void mainloop();

protected:
	// Send report iff changed
	void report_changes(vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
	// Send report whether or not changed
	void report(vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
	// NOTE:  class_of_service is only applied to vrpn_Analog
	//  values, not vrpn_Button or vrpn_Dial

	void decodePacket(size_t bytes, vrpn_uint8 *buffer);

	// Previous dial value, used to determine delta to send when it changes.
	bool _gotDial;
	vrpn_uint8 _lastDial;
};

// Original unit with 58 buttons.
class vrpn_Xkeys_Jog_And_Shuttle: protected vrpn_Xkeys_v1, public vrpn_Analog, public vrpn_Button_Filter, public vrpn_Dial {
public:
  vrpn_Xkeys_Jog_And_Shuttle(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_Xkeys_Jog_And_Shuttle() {};

  virtual void mainloop();

protected:
  // Send report iff changed
  void report_changes (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
  // Send report whether or not changed
  void report (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
  // NOTE:  class_of_service is only applied to vrpn_Analog
  //  values, not vrpn_Button or vrpn_Dial

  void decodePacket(size_t bytes, vrpn_uint8 *buffer);

  // Previous dial value, used to determine delta to send when it changes.
  vrpn_uint8 _lastDial;
};

// 12-button version of the above unit.
class vrpn_Xkeys_Jog_And_Shuttle12 : protected vrpn_Xkeys_v2, public vrpn_Analog, public vrpn_Button_Filter, public vrpn_Dial {
public:
	vrpn_Xkeys_Jog_And_Shuttle12(const char *name, vrpn_Connection *c = 0);
	virtual ~vrpn_Xkeys_Jog_And_Shuttle12() {};

	virtual void mainloop();

protected:
	// Send report iff changed
	void report_changes(vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
	// Send report whether or not changed
	void report(vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
	// NOTE:  class_of_service is only applied to vrpn_Analog
	//  values, not vrpn_Button or vrpn_Dial

	void decodePacket(size_t bytes, vrpn_uint8 *buffer);

	// Previous dial value, used to determine delta to send when it changes.
	vrpn_uint8 _lastDial;
};

// 68-button version of the above unit.
class vrpn_Xkeys_Jog_And_Shuttle68 : protected vrpn_Xkeys_v2, public vrpn_Analog, public vrpn_Button_Filter, public vrpn_Dial {
public:
	vrpn_Xkeys_Jog_And_Shuttle68(const char *name, vrpn_Connection *c = 0);
	virtual ~vrpn_Xkeys_Jog_And_Shuttle68() {};

	virtual void mainloop();

protected:
	// Send report iff changed
	void report_changes(vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
	// Send report whether or not changed
	void report(vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
	// NOTE:  class_of_service is only applied to vrpn_Analog
	//  values, not vrpn_Button or vrpn_Dial

	void decodePacket(size_t bytes, vrpn_uint8 *buffer);

	// Previous dial value, used to determine delta to send when it changes.
	vrpn_uint8 _lastDial;
};

class vrpn_Xkeys_XK3 : protected vrpn_Xkeys_noLEDs, public vrpn_Button_Filter {
public:
  vrpn_Xkeys_XK3(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_Xkeys_XK3() {};

  virtual void mainloop();

protected:
  // Send report iff changed
  void report_changes (void);
  // Send report whether or not changed
  void report (void);

  void decodePacket(size_t bytes, vrpn_uint8 *buffer);
};

// end of VRPN_USE_HID
#else
class VRPN_API vrpn_Xkeys;
class VRPN_API vrpn_Xkeys_Desktop;
class VRPN_API vrpn_Xkeys_Pro;
class VRPN_API vrpn_Xkeys_Joystick;
class VRPN_API vrpn_Xkeys_Joystick12;
class VRPN_API vrpn_Xkeys_Jog_And_Shuttle;
class VRPN_API vrpn_Xkeys_Jog_And_Shuttle12;
class VRPN_API vrpn_Xkeys_Jog_And_Shuttle68;
class VRPN_API vrpn_Xkeys_XK3;
#endif

