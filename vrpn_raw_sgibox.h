/*					vrpn_raw_sgibox.h
 *	
 *	This file describes the interface to an SGI dial & button box that
 * is connected through a serial interface.  This allows the control of
 * the boxes without going through the SGI GL library, rather using the
 * serial interface to connect with the device.
 */

#ifndef VRPN_RAW_SGIBOX
#define VRPN_RAW_SGIBOX

#include "vrpn_Analog.h"
#include "vrpn_Button.h"
#ifndef _WIN32 
#include <netinet/in.h>
#include <sys/ioctl.h>
#endif

/* Number of buttons and number of dials on sgi button/dial boxes */
#define NUM_BUTTONS (32)
#define NUM_DIALS   (8)
#define NUMDEVS (NUM_BUTTONS+NUM_DIALS)

class vrpn_raw_SGIBox :public vrpn_Analog, public vrpn_Button_Filter {
public:
  vrpn_raw_SGIBox(char * name, vrpn_Connection * c, char *serialDevName);
  void mainloop();
  int reset();
  int send_light_command();

protected:
  void get_report();
  void check_press_bank(int base_button, unsigned char base_command,
	  unsigned char command);
  void check_release_bank(int base_button, unsigned char base_command,
	  unsigned char command);
  
private:
  int	serialfd;		// Serial port that has been opened
  unsigned long btstat;           /* status of of on/off buttons */
  unsigned long bs1, bs2;         /* status of all buttons */
  short   vals1[NUMDEVS];	// Value array?
  int	dial_changed[NUM_DIALS];
  int	mid_values[NUM_DIALS];
};

#endif  // VRPN_RAW_SGIBOX
