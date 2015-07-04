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

// Device drivers for the Futaba USB line of products
// Currently supported: InterLink Elite Controller
//
// Exposes three major VRPN device classes: Button, Analog, Dial (as appropriate).
// All models expose Buttons for the keys on the device.
// Button 0 is the programming switch; it is set if the switch is in the "red" position.
//

class vrpn_Futaba: public vrpn_BaseClass, protected vrpn_HidInterface {
public:
  vrpn_Futaba(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c = 0,
      vrpn_uint16 vendor = 0, vrpn_uint16 product = 0);
  virtual ~vrpn_Futaba(void);

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
  int register_types(void) { return 0; }
};

class vrpn_Futaba_InterLink_Elite: protected vrpn_Futaba, public vrpn_Analog, public vrpn_Button_Filter, public vrpn_Dial {
public:
  vrpn_Futaba_InterLink_Elite(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_Futaba_InterLink_Elite(void) {};

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

// end of VRPN_USE_HID
#else
class VRPN_API vrpn_Futaba;
class VRPN_API vrpn_Futaba_InterLink_Elite;
#endif

