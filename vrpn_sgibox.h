/*                               -*- Mode: C -*- 
 * 
 * 
 * Author          : Ruigang Yang
 * Created On      : Wed Apr 29 16:06:25 1998
 * Last Modified By: Ruigang Yang
 * Last Modified On: Thu May 14 10:31:53 1998
 * Update Count    : 13
 * 
 * $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_sgibox.h,v $
 * $Date: 1998/06/26 15:49:03 $
 * $Author: hudson $
 * $Revision: 1.3 $
 * 
 * $Log: vrpn_sgibox.h,v $
 * Revision 1.3  1998/06/26 15:49:03  hudson
 * Wrote vrpn_FileConnection.
 * Changed connection naming convention.
 * Changed all the base classes to reflect the new naming convention.
 * Added #ifdef sgi around vrpn_sgibox.
 *
 * Revision 1.2  1998/05/14 14:45:26  ryang
 * modified vrpn_sgibox, so output is clamped to +-0.5, max 2 rounds
 *
 * Revision 1.1  1998/05/06 18:00:41  ryang
 * v0.1 of vrpn_sgibox
 *
 * SCCS Status     : %W%	%G%
 * 
 * HISTORY
 */

#ifndef VRPN_SGIBOX
#define VRPN_SGIBOX

#ifdef sgi

#include "vrpn_Analog.h"
#include "vrpn_Button.h"
#include <gl/gl.h>
#include <gl/device.h>

/* Number of buttons and number of dials on sgi button/dial boxes */
#define NUM_BUTTONS (32)
#define NUM_DIALS   (8)
#define NUMDEVS (NUM_BUTTONS+NUM_DIALS)

class vrpn_SGIBox :public vrpn_Analog, public vrpn_Button {
public:
  vrpn_SGIBox(char * name, vrpn_Connection * c);
  void mainloop();
  void reset();

protected:
  void get_report();

  
private:
  double resetval[vrpn_CHANNEL_MAX];
  long MAX_TIME_INTERVAL;

  unsigned long btstat;           /* status of of on/off buttons */
  unsigned long bs1, bs2;         /* status of all buttons */
  unsigned long *bpA, *bpB, *bpT; /* ptrs to above */
  Device  devs[NUMDEVS];          /* device array */
  short   vals1[NUMDEVS],
    vals2[NUMDEVS];         /* two values arrays */
  int	dial_changed[NUM_DIALS];
  int  mid_values[NUM_DIALS];
  int winid;
  int sid;// server id;
};

#endif  // sgi

#endif  // VRPN_SGIBOX
