/*****************************************************************************\
  vrpn_Nidaq.C
  --
  Description :

  ----------------------------------------------------------------------------
  Author: weberh
  Created: Fri Jan 29 10:00:00 1999
  Revised: Fri Jan 29 16:22:55 1999 by weberh
  $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_Nidaq.C,v $
  $Locker:  $
  $Revision: 1.5 $
\*****************************************************************************/

#include "vrpn_Nidaq.h"
#include <uptime.h>

// for fNice stuff
#include <windows.h>
#include <mmsystem.h>
// link with winmm.lib
#include <process.h>
// must link to multithreaded libs

#if defined(WIN32) || defined(_WIN32)
//#define VERBOSE
vrpn_Nidaq::vrpn_Nidaq(char *pchName, vrpn_Connection *pConnection,
		       double dSamplingRate, double dInterChannelRate, 
		       short sDeviceNumber, int cChannels, 
		       short rgsChan[], short rgsGain[],
		       short sInputMode, short sPolarity, int fNice)
  : vrpn_Analog(pchName, pConnection), pDAQ(NULL), fNice(fNice), fStop(0),
  fNewData(0), dSampleTime(0) {

	if (cChannels>vrpn_CHANNEL_MAX) {
      cerr << "vrpn_Nidaq::vrpn_Nidaq: vrpn_Analog allows only " 
	   << vrpn_CHANNEL_MAX << " channels (" << cChannels 
	   << " requested). DAQ not initialized." << endl;
      return;
    }
    
	if (fNice) {
		MMRESULT res = timeBeginPeriod(1);
		if (res != TIMERR_NOERROR) {
			cerr << "NidaqServer: timeBeginPeriod() failed!!!\n";
		}                       
	}

    num_channel = cChannels;
    daqSample.resize(cChannels);
    
    // calc the approximate offset between the clock the daq class uses
    // and the clock vrpn uses.
    
    // call each to get them in cache
    struct timeval tv, tvUpTime;
    double dTime1, dTime2;
    gettimeofday(&tv, NULL);
    gettimeofday(&tv, NULL);
    UpTime::Now();
    UpTime::Now();
    
    // Now calc offset
    dTime1=UpTime::Now();
    gettimeofday(&tv, NULL);
    dTime2=UpTime::Now();
    
    dTime1 = (dTime1 + dTime2)/2.0;
    tvUpTime = vrpn_MsecsTimeval(dTime1*1000.0);
    tvOffset = vrpn_TimevalDiff(tv, tvUpTime);

    // later, add this to tvUpTime to get into gettimeofday time frame

    // alloc the daq (which also starts it up)
    // args are:
    // rate for set of channels
    // rate between channels
    // which device num the daq has been set up as
    // the number of channels to read off of it
    // the array of channels to read
    // the gain to apply to each
    // differential or single ended
    // bipolar (+/-) or just unipolar (+)
    pDAQ = new DAQ(dSamplingRate, dInterChannelRate, sDeviceNumber, cChannels,
		   rgsChan, rgsGain, sInputMode, sPolarity );
	
	// start the DAQ-only thread
	InitializeCriticalSection(&csAnalogBuffer);
	hDAQThread = (HANDLE) _beginthreadex(NULL, 0, runThread, this, 0, NULL);
}

// threadshell for code which actually runs "Inertials"
unsigned __stdcall vrpn_Nidaq::runThread(void *pVrpnNidaq) {
  ((vrpn_Nidaq *)pVrpnNidaq)->runNidaq();
  return 0;
}

// here is what Inertials does in its run thread
void vrpn_Nidaq::runNidaq() {
	// always service the nidaq, but only pack messages if there is 
	// a new report and we have a connection.
	
	// getSample will fill in the report with most recent valid
	// data and the time of that data.
	// return value is the number of reports processed by
	// the a/d card since the last getSample call.
	// (if > 1, then we missed a report; if 0, then no new data)
	// if gfAllInertial is filled in, then we will grab the intervening
	// reports as well (note: gfAllInertial does not work properly as 
	// of 1/29/99 weberh).
	while (!fStop) {
		if (pDAQ->getSample(&daqSample)) {
			// there is a reading and a connection ... so package it
			
			// copy daq channels to analog class data
			EnterCriticalSection(&csAnalogBuffer);
			for (int i=0;i<daqSample.cChannels;i++) {
				channel[i]=daqSample.rgd[i];
			}
			fNewData=1;
			dSampleTime=daqSample.dTime;
			LeaveCriticalSection(&csAnalogBuffer);
		}
		if (fNice) {
			Sleep(1);
		}
	}
}

vrpn_Nidaq::~vrpn_Nidaq() {
  fStop=1;
	WaitForSingleObject(hDAQThread,INFINITE);
	if (fNice) {
	  timeEndPeriod(1);
  }
  delete pDAQ;
}

void vrpn_Nidaq::mainloop() {
	// lower cpu utilization, but do so before report_changes so
	// that when connection->mainloop is called after nidaq->mainloop
	// the messages will be sent immediately (rather than having an 
	// intervening sleep
	if (fNice) {
		Sleep(1);
	}
	report_changes();
}

int vrpn_Nidaq::doing_okay() {
  return (pDAQ->status()==DAQ::RUNNING);
}

void vrpn_Nidaq::report_changes() {
	// always service the nidaq, but only pack messages if there is 
	// a new report and we have a connection.
	
	// getSample will fill in the report with most recent valid
	// data and the time of that data.
	// return value is the number of reports processed by
	// the a/d card since the last getSample call.
	// (if > 1, then we missed a report; if 0, then no new data)
	// if gfAllInertial is filled in, then we will grab the intervening
	// reports as well (note: gfAllInertial does not work properly as 
	// of 1/29/99 weberh).
#ifdef VERBOSE
	int fHadNew=0;
#endif
	if (connection) {
		// there is a reading and a connection ... so package it
		
		EnterCriticalSection(&csAnalogBuffer);
		
#ifdef VERBOSE
		fHadNew=fNewData;
#endif
		if (fNewData) {
			fNewData=0;
			// It will actually be sent out when the server calls 
			// mainloop() on the connection object this device uses.
			char rgch[1000];
			int	cChars = vrpn_Analog::encode_to(rgch);
			double dTime = dSampleTime;
			LeaveCriticalSection(&csAnalogBuffer);
			
			struct timeval tv;
			tv = vrpn_TimevalSum(vrpn_MsecsTimeval(dTime*1000.0), 
				tvOffset);
			
			if (connection->pack_message(cChars, tv, channel_m_id, my_id, rgch,
				vrpn_CONNECTION_LOW_LATENCY)) {
				cerr << "vrpn_Nidaq::report_changes: cannot write message: tossing.\n";
			}
		} else {
			LeaveCriticalSection(&csAnalogBuffer);
		}
#ifdef VERBOSE
		if (fHadNew) {
			print();
		}
#endif
		
	} else {
		cerr << "vrpn_Nidaq::report_changes: no valid connection.\n";
	}
}
#endif  // def(WIN32) || def(_WIN32)

/*****************************************************************************\
  $Log: vrpn_Nidaq.C,v $
  Revision 1.5  1999/02/11 21:47:49  weberh
  *** empty log message ***

  Revision 1.4  1999/02/01 20:53:07  weberh
  cleaned up and added nice and multithreading for practical usage.

  Revision 1.3  1999/01/29 22:18:27  weberh
  cleaned up analog.C and added class to support national instruments a/d card

  Revision 1.2  1999/01/29 19:47:33  weberh
  *** empty log message ***

\*****************************************************************************/

