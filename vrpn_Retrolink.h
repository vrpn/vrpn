#pragma once

#include "vrpn_Analog.h"                // for vrpn_Analog
#include "vrpn_Button.h"                // for vrpn_Button_Filter
#include "vrpn_HumanInterface.h"        // for vrpn_HidAcceptor (ptr only), etc
#include <stddef.h>                     // for size_t

#if defined(VRPN_USE_HID)

// Device drivers for the Retrolink Classic Controller USB line of products
// Currently supported: GameCube
//
// Exposes two major VRPN device classes: Button, Analog.  See vrpn.cfg for
// a description of which buttons and control map where.
//

class vrpn_Retrolink: public vrpn_BaseClass, protected vrpn_HidInterface {
public:
	vrpn_Retrolink(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c = 0,
        vrpn_uint16 vendor = 0, vrpn_uint16 product = 0);
	virtual ~vrpn_Retrolink(void);

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

  // No actual types to register, derived classes will be buttons, analogs
  int register_types(void) { return 0; }
};

//--------------------------------------------------------------------------------
// For GameCube :
//	Analog channel assignments :
// 0 = Left joystick X axis; -1 = left, 1 = right
// 1 = Left joystick Y axis; -1 = up, 1 = down
// 2 = Right joystick X axis; -1 = left, 1 = right
// 3 = Right joystick Y axis; -1 = up, 1 = down
// 4 = Left rocker switch angle in degrees(-1 if nothing is pressed)
//	Button number assignments :
// 0 = Y
// 1 = X
// 2 = A
// 3 = B
// 4 = left trigger
// 5 = right trigger
// 6 = Z
// 7 = Start / pause
// Buttons 8 - 11 are duplicate mappings for the rocker - switch; both
// these and the analog angle in degrees will change as they are pressed
// 8 = up
// 9 = right
// 10 = down
// 11 = left

class vrpn_Retrolink_GameCube : protected vrpn_Retrolink, public vrpn_Analog, public vrpn_Button_Filter {
public:
	vrpn_Retrolink_GameCube(const char *name, vrpn_Connection *c = 0);
	virtual ~vrpn_Retrolink_GameCube(void) {};

  virtual void mainloop(void);

protected:
  // Send report iff changed
  void report_changes (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
  // Send report whether or not changed
  void report (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);

  void decodePacket(size_t bytes, vrpn_uint8 *buffer);
};

//--------------------------------------------------------------------------------
// For Genesis :
//	Analog channel assignments :
// 0 = Rocker switch angle in degrees(-1 if nothing is pressed)
//	Button number assignments :
// 0 = A
// 1 = B
// 2 = C
// 3 = X
// 4 = Y
// 5 = Z
// 6 = Mode
// 7 = Start
// Buttons 8 - 11 are duplicate mappings for the rocker - switch; both
// these and the analog angle in degrees will change as they are pressed
// 8 = up
// 9 = right
// 10 = down
// 11 = left

class vrpn_Retrolink_Genesis : protected vrpn_Retrolink, public vrpn_Analog, public vrpn_Button_Filter {
public:
	vrpn_Retrolink_Genesis(const char *name, vrpn_Connection *c = 0);
	virtual ~vrpn_Retrolink_Genesis(void) {};

  virtual void mainloop(void);

protected:
  // Send report iff changed
  void report_changes (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
  // Send report whether or not changed
  void report (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);

  void decodePacket(size_t bytes, vrpn_uint8 *buffer);
};

// end of VRPN_USE_HID
#else
class VRPN_API vrpn_Retrolink;
class VRPN_API vrpn_Retrolink_GameCube;
class VRPN_API vrpn_Retrolink_Genesis;
#endif
