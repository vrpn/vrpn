/*****************************************************************************\
  vrpn_Nidaq.h
  --

  Description : This class reads from a National Instruments D/A Card
		(NIDAQ).  To compile this class, you must have the
		following directories in your include path:
		  ~tracker/hiball/nidaq/
		  ~tracker/hiball/src/libs/libgb (for uptime.h)
		  ~tracker/hiball/src/hybrid/ (for daq.h)
		And you must link in:
		  ~tracker/hiball/src/libs/libgb/uptime.cpp
		  ~tracker/hiball/src/hybrid/daq.cpp
		  ~tracker/hiball/nidaq/nidaq32.lib

  ----------------------------------------------------------------------------
  Author: weberh
  Created: Fri Jan 29 10:00:00 1999
  Revised: Fri Mar 19 14:45:55 1999 by weberh
  $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_Nidaq.h,v $
  $Locker:  $
  $Revision: 1.9 $
\*****************************************************************************/

#ifndef VRPN_NIDAQ
#define VRPN_NIDAQ
#if defined(_WIN32) || defined(WIN32)
#include "vrpn_Analog.h"
#include <daq.h>
#include <windows.h>

class vrpn_Nidaq : public vrpn_Analog {
public:
	// see daq.h for more info on the args
	// fNice says whether these threads should use 100% of the cpu or
	// whether they should sleep for 1 ms each time thru their loops
	// (the net effect is that they add 1 ms of uncertainty to the
	// existing 1 or 1/2 ms of uncertainty in time-stamps across a
	// synchronized vrpn connection).  If fNice is set, then 
  // the max theoretical reporting rate is 1000 hz.
	vrpn_Nidaq(char * pchName, vrpn_Connection * pConnection,
	     double dSamplingRate=100.0, double dInterChannelRate=100000.0, 
	     short sDeviceNumber=DAQ::DEF_DEVICE, int cChannels=10, 
	     short rgsChan[]=DAQ::DEF_CHANS_DIFF, 
	     short rgsGain[]=DAQ::DEF_GAINS,
	     short sInputMode=DAQ::DIFFERENTIAL, 
	     short sPolarity=DAQ::BIPOLAR,
		   int fNice=0);

  ~vrpn_Nidaq();
  void mainloop(const struct timeval * timeout = NULL);
  int doing_okay();

protected:
  void report_changes();
  
private:
  DAQSample daqSample;
  DAQ *pDAQ;
  // value to add to UpTime calls to get into gettimeofday timeframe
  struct timeval tvOffset;
  
  // Data, threadshell, and function used by extra daq getSample thread.
  // the crit section is used to protect the analog buffer from simultaneous
  // access by the two nidaq threads.
  CRITICAL_SECTION csAnalogBuffer;
  HANDLE hDAQThread;
  static unsigned __stdcall vrpn_Nidaq::runThread(void *pVrpnNidaq);
  void runNidaq();
  int fStop;
  int fNewData;
  double dSampleTime;

  // this controls whether we give up about 1 ms worth of timestamp accuracy
  // in exchange for cpu utilization going from 100% to 4% (or so)
  int fNice;
};

#endif // def(_WIN32) || def(WIN32)
#endif // ndef(VRPN_NIDAQ)

/*****************************************************************************\
  $Log: vrpn_Nidaq.h,v $
  Revision 1.9  1999/03/19 23:05:49  weberh
  modified the client and server classes so that they work with
  tom's changes to allow blocking vrpn mainloop calls (ie, mainloop
  signature changed).

  Revision 1.8  1999/03/17 22:30:14  weberh
  added new comment re: max rate with fNice.

  Revision 1.7  1999/02/15 16:09:08  weberh
  updated comments to reflect necessary files to compile this component.

  Revision 1.6  1999/02/11 20:17:34  weberh
  cleaned up problems with cname alloc/dealloc, etc.

  Revision 1.5  1999/02/01 20:53:08  weberh
  cleaned up and added nice and multithreading for practical usage.

  Revision 1.4  1999/01/29 22:18:28  weberh
  cleaned up analog.C and added class to support national instruments a/d card

  Revision 1.3  1999/01/29 19:48:28  weberh
  *** empty log message ***

  Revision 1.2  1999/01/29 19:47:34  weberh
  *** empty log message ***

\*****************************************************************************/
