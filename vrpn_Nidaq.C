/*****************************************************************************\
  $FILE$
  --
  Description :

  ----------------------------------------------------------------------------
  $AUTHOR$
  $DATE$
  $REVISED$
  $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_Nidaq.C,v $
  $Locker:  $
  $Revision: 1.1 $
\*****************************************************************************/

#include "vrpn_Nidaq.h"
#include <uptime.h>

#if defined(WIN32) || defined(_WIN32)
#define VERBOSE
vrpn_Nidaq::vrpn_Nidaq(char *pchName, vrpn_Connection *pConnection)
  : vrpn_Analog(pchName, pConnection), pDAQ(NULL) {

    // NOTE: all of these will have to become args

    // You should use an even number of channels even if you
    // really only need one less.  If you don't, the daq will
    // only report at about half the rate (some wierd double-buffering
    // side-effect).
    int cChannels = 10;

    // differential mode, so chans/2
    if (cChannels>(DAQ::MAX_CHANS/2)) {
      cerr << "vrpn_Nidaq::vrpn_Nidaq: only " << (DAQ::MAX_CHANS/2) << 
	" allowed, " << cChannels << " requested. DAQ not initialized." endl;
      return;
    }

    if (cChannels>vrpn_CHANNEL_MAX) {
      cerr << "vrpn_Nidaq::vrpn_Nidaq: vrpn_Analog allows only " 
	   << vrpn_CHANNEL_MAX << " channels (" << cChannels 
	   << " requested). DAQ not initialized." endl;
      return;
    }
    
    short rgsChan[cMaxChannels];
    // xbow acc are 0->5v, tokin rg are +-1v, systron rg +-5
    // a/d is +-10, so we have the following gains (since 
    // our device is set to +-10 v by default
    short rgsGain[] = { 2, 2, 2, 10, 10, 10, 2, 2, 2, 1 };
    
    // max rate for 6 channels at 100khz
    double dInterSampleRate = 100000.0
    double dSamplingRate=dInterSampleRate/(double)cChannels;
    
    // set up the channel array
    int i;
    for (i=0;i<cChannels;i++) {
      // these are all differential, so they go 0-7, 16-23, 32-39, 48-55 
      // if we want single ended, then we don't do this trickery
      rgsChan[i] = i%8 + 16*(i/8);
    }
    
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
    tvUpTime.tv_secs = vrpn_MsecsTimeval(dTime1*1000.0);
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
    pDAQ = new DAQ(dSamplingRate, 100000.0, DAQ::DEF_DEVICE, cChannels,
		   rgsChan, rgsGain, DAQ::DIFFERENTIAL, DAQ::BIPOLAR );
}

vrpn_Nidaq::~vrpn_Nidaq() {
  delete pDAQ;
}

void vrpn_Nidaq::mainloop() {
  report_changes();
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

  if (pDaq->getSample(&daqSample) && pConnection) {
    // there is a reading and a connection ... so package it

    // copy daq channels to analog class data
    for (int i=0;i<daqSample.cChannels;i++) {
      channels[i]=daqSample.rgd[i];
    }

    // It will actually be sent out when the server calls 
    // mainloop() on the connection object this device uses.
    char rgch[1000];
    int	cChars = vrpn_Analog::encode_to(msgbuf);
#ifdef VERBOSE
    print();
#endif

    struct timeval tv;

    tv = vrpn_TimevalSum(vrpn_MsecsTimeval(daqSample.dTime*1000.0), 
			 tvOffset);

    if (pConnection->pack_message(cChars, tv, channel_m_id, my_id, rgch,
				  vrpn_CONNECTION_LOW_LATENCY)) {
      cerr << "vrpn_Nidaq::report_changes: cannot write message: tossing.\n";
    }
  } else {
    cerr << "vrpn_Nidaq::report_changes: no valid connection.\n";
  }
}

#endif  // def(WIN32) || def(_WIN32)


