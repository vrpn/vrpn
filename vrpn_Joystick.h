#ifndef VRPN_JOYSTICK
#define VRPN_JOYSTICK
#include "vrpn_Analog.h"
#include "vrpn_Button.h"

class vrpn_Joystick :public vrpn_Serial_Analog, public vrpn_Button {
public:
  vrpn_Joystick(char * name, vrpn_Connection * c, char * portname,int
		baud, double);

  void mainloop(const struct timeval * timeout = NULL);

protected:
  void get_report();
  void reset();
  void parse(int);
private:
  double resetval[vrpn_CHANNEL_MAX];
  long MAX_TIME_INTERVAL;
};


#endif
