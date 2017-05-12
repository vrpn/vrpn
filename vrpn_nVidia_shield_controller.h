#pragma once

#include <stddef.h>                     // for size_t

#include "vrpn_Analog.h"                // for vrpn_Analog
#include "vrpn_Button.h"                // for vrpn_Button_Filter
#include "vrpn_HumanInterface.h"        // for vrpn_HidAcceptor (ptr only), etc

#if defined(VRPN_USE_HID)

// Device drivers for the nVidia Shield controller line of products
// Currently supported: Shield controller plugged into USB

class vrpn_nVidia_shield: public vrpn_BaseClass, protected vrpn_HidInterface {
public:
  vrpn_nVidia_shield(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c = 0,
      vrpn_uint16 vendor = 0, vrpn_uint16 product = 0);
  virtual ~vrpn_nVidia_shield(void);

  virtual void mainloop(void) = 0;

protected:
  // Set up message handlers, etc.
  void init_hid(void);
  void on_data_received(size_t bytes, vrpn_uint8 *buffer);

  virtual void decodePacket(size_t bytes, vrpn_uint8 *buffer) = 0;	
  struct timeval d_timestamp;
  vrpn_HidAcceptor *d_filter;

  // No actual types to register, derived classes will be buttons or analogs
  int register_types(void) { return 0; }
};

class vrpn_nVidia_shield_USB: protected vrpn_nVidia_shield,
    public vrpn_Analog, public vrpn_Button_Filter {
public:
  vrpn_nVidia_shield_USB(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_nVidia_shield_USB(void) {};

  virtual void mainloop(void);

protected:
  // Send report iff changed
  void report_changes (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
  // Send report whether or not changed
  void report (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);

  void decodePacket(size_t bytes, vrpn_uint8 *buffer);
};

class vrpn_nVidia_shield_stealth_USB: protected vrpn_nVidia_shield,
    public vrpn_Analog, public vrpn_Button_Filter {
public:
  vrpn_nVidia_shield_stealth_USB(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_nVidia_shield_stealth_USB(void) {};

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
class VRPN_API vrpn_nVidia_shield;
class VRPN_API vrpn_nVidia_shield_USB;
class VRPN_API vrpn_nVidia_shield_stealth_USB;
#endif

