/*****************************************************************************\
  nidaqClient.cpp
  --
  Description : you need to link to the vrpnClient library to compile this
                executable.

  ----------------------------------------------------------------------------
  Author: weberh
  Created: Mon Apr 27 14:26:09 1998
  Revised: Fri Jan 29 17:09:15 1999 by weberh
\*****************************************************************************/
#include "vrpn_Shared.h"

#include <mmsystem.h>
// link with winmm.lib

#include <iostream>
using std::endl;
using std::cerr;

#include "vrpn_Analog.h"
void VRPN_CALLBACK printNidaq( void *pvTVZero, const vrpn_ANALOGCB cbInfo ) {
  cerr << vrpn_TimevalMsecs( vrpn_TimevalDiff(cbInfo.msg_time,
					      (*(struct timeval *)pvTVZero)))
    *0.001 << " Voltages: ";
  for (int i=0;i<cbInfo.num_channel;i++) {
    cerr << cbInfo.channel[i] << "\t";
  }
  cerr << endl;
}

int main( int argc, char *argv[] ) {
  struct timeval tvZero;
  vrpn_Analog_Remote *pNidaq;

  // hires timer for sleep
	  MMRESULT res = timeBeginPeriod(1);
	  if (res != TIMERR_NOERROR) {
		  cerr << "NidaqServer: timeBeginPeriod() failed!!!\n";
	  }                       

  //  pNidaq = new vrpn_Analog_Remote("Nidaq0@helium.cs.unc.edu");
  pNidaq = new vrpn_Analog_Remote("Nidaq0@trackerpc-cs.cs.unc.edu");

  vrpn_gettimeofday(&tvZero,NULL);
  pNidaq->register_change_handler(&tvZero, printNidaq);
  
  while (1) {
    pNidaq->mainloop();
    Sleep(1);
  }
  timeEndPeriod(1);
  return 0;
}
