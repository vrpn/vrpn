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
#include <windows.h>
#include <mmsystem.h>
// link with winmm.lib

#include "vrpn_Analog.h"
void printNidaq( void *pvTVZero, const vrpn_ANALOGCB cbInfo ) {
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

	  // If I don't care about specific parts of the synchronization, then
  // I can just call analog_remote with no connection arg (default is null)
  // and let it set up the connection.
  //  Connection *pConnection;

  //  pConnection = new vrpn_Synchronized_Connection();

  //  pNidaq = new vrpn_Analog_Remote("Nidaq0@helium.cs.unc.edu", pConnection);
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
