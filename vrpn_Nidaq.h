/*****************************************************************************\
  vrpn_Nidaq.h
  --

  Description : This class reads from a National Instruments D/A Card
		(NIDAQ).  To compile this class, you must have the
		following directories in your include path:
		  ~tracker/hiball/nidaq/
		  ~tracker/hiball/src/libs/libgb (for uptime.h)
		  ~tracker/hiball/src/libs/tracker (for daq.h)
		And you must link in:
		  ~tracker/hiball/src/libs/libgb/uptime.cpp
		  ~tracker/hiball/src/libs/tracker/daq.cpp
		  ~tracker/hiball/nidaq/nidaq32.lib

  ----------------------------------------------------------------------------
  Author: weberh
  Created: Fri Jan 29 10:00:00 1999
  Revised: Fri Jan 29 16:28:30 1999 by weberh
  $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_Nidaq.h,v $
  $Locker:  $
  $Revision: 1.4 $
\*****************************************************************************/

#ifndef VRPN_NIDAQ
#define VRPN_NIDAQ

#if defined(_WIN32) || defined(WIN32)
#include "vrpn_Analog.h"
#include <daq.h>

class vrpn_Nidaq : public vrpn_Analog {
public:
  // see daq.h for more info on the args
  vrpn_Nidaq(char * pchName, vrpn_Connection * pConnection,
	     double dSamplingRate=-1, double dInterChannelRate=100000.0, 
	     short sDeviceNumber=DAQ::DEF_DEVICE, int cChannels=10, 
	     short rgsChan[]=DAQ::DEF_CHANS_DIFF, 
	     short rgsGain[]=DAQ::DEF_GAINS,
	     short sInputMode=DAQ::DIFFERENTIAL, 
	     short sPolarity=DAQ::BIPOLAR);

  ~vrpn_Nidaq();
  void mainloop();
  int doing_okay();

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

/*****************************************************************************\
  $Log: vrpn_Nidaq.h,v $
  Revision 1.4  1999/01/29 22:18:28  weberh
  cleaned up analog.C and added class to support national instruments a/d card

  Revision 1.3  1999/01/29 19:48:28  weberh
  *** empty log message ***

  Revision 1.2  1999/01/29 19:47:34  weberh
  *** empty log message ***

\*****************************************************************************/
