#ifndef VRPN_WANDA
#define VRPN_WANDA
#include "vrpn_Analog.h"                // for vrpn_CHANNEL_MAX, etc
#include "vrpn_Button.h"                // for vrpn_Button_Filter
#include "vrpn_Configure.h"             // for VRPN_API

class VRPN_API vrpn_Connection;

// This is a driver for the Wanda device, which is an analog and
// button device.  You can find out more at http://home.att.net/~glenmurray/
// This driver was written at Brown University.

class VRPN_API vrpn_Wanda :public vrpn_Serial_Analog, public vrpn_Button_Filter {
public:
  vrpn_Wanda(char * name, vrpn_Connection * c, char * portname,int
	     baud, double);

  void mainloop(void);

protected:
  void report_new_button_info();
  void report_new_valuator_info();

private:
  double last_val_timestamp;
  double resetval[vrpn_CHANNEL_MAX];
  long MAX_TIME_INTERVAL;
  int  bytesread;
  int  first;
  int  index;
  static int dbug_wanda;
};


#endif
