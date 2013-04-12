/*
# Linux Joystick. Interface to the Linux Joystick driver by Vojtech Pavlik
# included in several Linux distributions. The server code has been tested 
# with Linux Joystick driver version 1.2.14. Yet, there is no way how to
# map a typical joystick's zillion buttons and axes on few buttons and axes
# really used. Unfortunately, even joysticks of the same kind can have 
# different button mappings from one to another.  Driver written by Harald
# Barth (haba@pdc.kth.se).
*/

#ifndef VRPN_JOYLIN
#define VRPN_JOYLIN
#include "vrpn_Analog.h"                // for vrpn_Analog
#include "vrpn_Button.h"                // for vrpn_Button_Filter
#include "vrpn_Configure.h"             // for VRPN_API

class VRPN_API vrpn_Connection;


class VRPN_API vrpn_Joylin :public vrpn_Analog, public vrpn_Button_Filter {
public:
  vrpn_Joylin(char * name, vrpn_Connection * c, char * portname);
  ~vrpn_Joylin();

  void mainloop(void);

#ifdef VRPN_USE_JOYLIN
protected:
  int init();
#endif
private:
  int namelen;
  int fd;
  int version;
  char *devname;
  char *device;
};


#endif
