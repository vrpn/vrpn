/*****************************************************************************\
  $FILE$
  --
  Description :

  ----------------------------------------------------------------------------
  $AUTHOR$
  $DATE$
  $REVISED$
  $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_Nidaq.h,v $
  $Locker:  $
  $Revision: 1.1 $
\*****************************************************************************/

#ifndef VRPN_NIDAQ
#define VRPN_NIDAQ

#if defined(_WIN32) || defined(WIN32)
#include "vrpn_Analog.h"

class vrpn_Nidaq : public vrpn_Analog {
public:
  vrpn_Nidaq(char * pchName, vrpn_Connection * pConnection);
  ~vrpn_Nidaq();
  void mainloop();

protected:
  void report_changes();

private:
  DAQSample daqSample;
  DAQ *pDAQ;
  // value to add to UpTime calls to get into gettimeofday timeframe
  struct timeval tvOffset;
};

#endif // def(_WIN32) || def(WIN32)
#endif // ndef(VRPN_NIDAQ)
