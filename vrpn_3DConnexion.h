#ifndef VRPN_3DCONNEXION_H
#define VRPN_3DCONNEXION_H

#include <stddef.h>                     // for size_t

#include "vrpn_Analog.h"                // for vrpn_Analog
#include "vrpn_Button.h"                // for vrpn_Button_Filter
#include "vrpn_Configure.h"             // for VRPN_API, VRPN_USE_HID
#include "vrpn_Connection.h"            // for vrpn_Connection (ptr only), etc
#include "vrpn_HumanInterface.h"        // for vrpn_HidAcceptor (ptr only), etc
#include "vrpn_Shared.h"                // for timeval
#include "vrpn_Types.h"                 // for vrpn_uint32, vrpn_uint8

// Device drivers for the 3DConnexion SpaceNavigator and SpaceTraveler
// SpaceExplorer, SpaceMouse, SpaceMousePro, SpaceMouseCompact, Spaceball5000,
// SpacePilot devices, connecting to them as HID devices (USB).

// Exposes two VRPN device classes: Button and Analog.
// Analogs are mapped to the six channels, each in the range (-1..1).

// This is the base driver for the devices.  The Navigator has
// only two buttons and has product ID 50726, the traveler has 8
// buttons and ID 50723.  The derived classes just construct with
// the appropriate number of buttons and an acceptor for the proper
// product ID; the baseclass does all the work.

#if defined(VRPN_USE_HID)
class VRPN_API vrpn_3DConnexion: public vrpn_Button_Filter, public vrpn_Analog, protected vrpn_HidInterface {
public:
  vrpn_3DConnexion(vrpn_HidAcceptor *filter, unsigned num_buttons,
      const char *name, vrpn_Connection *c = 0,
      vrpn_uint16 vendor = 0, vrpn_uint16 product = 0);
  virtual ~vrpn_3DConnexion();

  virtual void mainloop();

protected:
  // Set up message handlers, etc.
  void on_data_received(size_t bytes, vrpn_uint8 *buffer);

  virtual void decodePacket(size_t bytes, vrpn_uint8 *buffer);
  struct timeval _timestamp;
  vrpn_HidAcceptor *_filter;

  // Send report iff changed
  void report_changes (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
  // Send report whether or not changed
  void report (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
  // NOTE:  class_of_service is only applied to vrpn_Analog
  //  values, not vrpn_Button or vrpn_Dial
};
#else   // not VRPN_USE_HID
class VRPN_API vrpn_3DConnexion: public vrpn_Button_Filter, public vrpn_Analog {
public:
  vrpn_3DConnexion(vrpn_HidAcceptor *filter, unsigned num_buttons,
      const char *name, vrpn_Connection *c = 0,
      vrpn_uint16 vendor = 0, vrpn_uint16 product = 0);
  virtual ~vrpn_3DConnexion();

  virtual void mainloop();

protected:
  struct timeval _timestamp;
  vrpn_HidAcceptor *_filter;
  int fd;

  // Send report iff changed
  void report_changes(vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
  // Send report whether or not changed
  void report(vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
  // NOTE:  class_of_service is only applied to vrpn_Analog
  //  values, not vrpn_Button or vrpn_Dial

// There is a non-HID Linux-based driver for this device that has a capability
// not implemented in the HID interface.
#if defined(linux) && !defined(VRPN_USE_HID)
  int set_led(int led_state);
#endif
};
#endif  // not VRPN_USE_HID

class VRPN_API vrpn_3DConnexion_Navigator: public vrpn_3DConnexion {
public:
  vrpn_3DConnexion_Navigator(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_3DConnexion_Navigator() {};

protected:
};

class VRPN_API vrpn_3DConnexion_Navigator_for_Notebooks: public vrpn_3DConnexion {
public:
  vrpn_3DConnexion_Navigator_for_Notebooks(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_3DConnexion_Navigator_for_Notebooks() {};

protected:
};

class VRPN_API vrpn_3DConnexion_Traveler: public vrpn_3DConnexion {
public:
  vrpn_3DConnexion_Traveler(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_3DConnexion_Traveler() {};

protected:
};

class VRPN_API vrpn_3DConnexion_SpaceMouse: public vrpn_3DConnexion {
public:
  vrpn_3DConnexion_SpaceMouse(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_3DConnexion_SpaceMouse() {};

protected:
};

/*
The button numbers are labeled as follows (the ones similar to <x> have a graphic on the button and are referred to the text enclosed text in the help):
0=Menu
1=Fit
2=<T>
4=<R>
5=<F>
8=<Roll+>
12=1
13=2
14=3
15=4
22=Esc
23=Alt
24=Shift
25=Ctrl
26=<Rot>
*/

class VRPN_API vrpn_3DConnexion_SpaceMousePro: public vrpn_3DConnexion {
public:
	vrpn_3DConnexion_SpaceMousePro(const char *name, vrpn_Connection *c = 0);
	virtual ~vrpn_3DConnexion_SpaceMousePro() {};

protected:
};

class VRPN_API vrpn_3DConnexion_SpaceMouseCompact : public vrpn_3DConnexion {
public:
	vrpn_3DConnexion_SpaceMouseCompact(const char *name, vrpn_Connection *c = 0);
	virtual ~vrpn_3DConnexion_SpaceMouseCompact() {};

protected:
};

class VRPN_API vrpn_3DConnexion_SpaceMouseWireless : public vrpn_3DConnexion {
public:
	vrpn_3DConnexion_SpaceMouseWireless(const char *name, vrpn_Connection *c = 0);
	virtual ~vrpn_3DConnexion_SpaceMouseWireless() {};

protected:
};

class VRPN_API vrpn_3DConnexion_SpaceExplorer : public vrpn_3DConnexion {
public:
  vrpn_3DConnexion_SpaceExplorer(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_3DConnexion_SpaceExplorer() {};

protected:
};

class VRPN_API vrpn_3DConnexion_SpaceBall5000: public vrpn_3DConnexion {
public:
  vrpn_3DConnexion_SpaceBall5000(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_3DConnexion_SpaceBall5000() {};

protected:
};

class VRPN_API vrpn_3DConnexion_SpacePilot: public vrpn_3DConnexion {
public:
  vrpn_3DConnexion_SpacePilot(const char *name, vrpn_Connection *c = 0);
  virtual ~vrpn_3DConnexion_SpacePilot() {};

protected:
};

class VRPN_API vrpn_3DConnexion_SpacePilotPro : public vrpn_3DConnexion {
public:
    vrpn_3DConnexion_SpacePilotPro(const char *name, vrpn_Connection *c = 0);
    virtual ~vrpn_3DConnexion_SpacePilotPro(){};

protected:
};

class VRPN_API vrpn_3DConnexion_SpaceMouseProWireless : public vrpn_3DConnexion {
public:
	vrpn_3DConnexion_SpaceMouseProWireless(const char *name, vrpn_Connection *c = 0);
	virtual ~vrpn_3DConnexion_SpaceMouseProWireless() {};

protected:
	void decodePacket(size_t bytes, vrpn_uint8 *buffer);
};

// end of VRPN_3DCONNEXION_H
#endif
