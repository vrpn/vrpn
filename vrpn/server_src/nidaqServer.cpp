/*****************************************************************************\
  nidaqServer.cpp
  --
  Description : you need to link to the vrpnServer library to compile this
                executable (this will require linking with daq.cpp from 
		~tracker/hiball/src/libs/tracker, uptime.cpp from 
		~tracker/hiball/src/libs/libgb, and the nidaq headers and
		nidaq32.lib from ~tracker/hiball/nidaq).
  ----------------------------------------------------------------------------
  Author: weberh
  Created: Mon Apr 27 14:26:09 1998
  Revised: Tue Feb  9 13:43:09 1999 by weberh
\*****************************************************************************/
#if defined(WIN32) || defined(_WIN32)

#include <conio.h>
#include "vrpn_Nidaq.h"
#include "getOptClean.h"

#include <windows.h>
//#define MEM_DBG

#if defined(MEM_DBG)
#define _MEM_DBG_SET
#include <crtdbg.h>
#define _CRTDBG_MAP_ALLOC
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

// NEED TO DO: I should install a signal handler to shut down the nidaq.
// for now, I provide a kbhit and query for q to provide a graceful shutdown.

void usage(char *pch) {
	cerr << "Usage: " << pch << endl;
	cerr << "\t-channels <int>\tspecify the num of chans to read\n";
	cerr << "\t-rate <double> \tspecify the sampling rate\n";
	//cerr << "\t-nice		  \tdon't use 100% cpu (2 ms timestamp accuracy)\n";
	cerr << "\t-not_nice	  \tuse 100% cpu (1 ms timestamp accuracy)\n";
	cerr << "\t-port <int>    \tuse port <int>\n";
	cerr << "The defaults are 10 channels, the 100 hz (say -1 for fastest possible),\n"
		<< "2 ms timestamp accuracy, and the default vrpn port ("
		<< vrpn_DEFAULT_LISTEN_PORT_NO << ")\n";
	cerr << endl;
}

int main (int argc, char * argv[])
{
  vrpn_Connection *pConnection;
  vrpn_Nidaq *pNidaq;

  #if defined(_DEBUG) && defined(MEM_DBG)
  // Get current flag
  int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
  
  // Turn on leak-checking bit
  tmpFlag |= _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF |
    _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF;
    
  // don't check c runtime libs
  // | _CRTDBG_CHECK_CRT_DF;
  
  // Set flag to the new value
  _CrtSetDbgFlag( tmpFlag );
#endif

  GetOpt go(argc, argv, usage);
  if (go.error) {
	  go.report(cerr);
	  return -1;
  }

  int cChannels=10;
  double dSamplingRate=100;
  int fNice=1;
  int iVrpnPort=vrpn_DEFAULT_LISTEN_PORT_NO;

  go.integer("channels", &cChannels);
  go.real("rate", &dSamplingRate);
  go.boolean("nice",&fNice);
  fNice=!go.boolean("not_nice");
  go.integer("port",&iVrpnPort);

  go.cleanUp();
  if (go.error) {
	  usage(argv[0]);
	  return -1;
  }

  // Need to have a global pointer to it so we can shut it down
  // in the signal handler (so we can close any open logfiles).
  // In this case, it is so we can shut down the nidaq server and 
  // avoid blue screening.
  pConnection = new vrpn_Synchronized_Connection(iVrpnPort);


  // You should use an even number of channels even if you
  // really only need one less.  If you don't, the daq will
  // only report at about half the rate (some wierd double-buffering
  // side-effect).

  // xbow acc are 0->5v, tokin rg are +-1v, systron rg +-5
  // a/d is +-10, so we have the following gains (since 
  // our device is set to +-10 v by default
  short rgsGain[] = { 2, 2, 2, 10, 10, 10, 2, 2, 2, 1 };
    

  pNidaq = new vrpn_Nidaq("Nidaq0", pConnection, dSamplingRate, 100000, 
			  DAQ::DEF_DEVICE, cChannels, DAQ::DEF_CHANS_DIFF,
			  rgsGain, DAQ::DIFFERENTIAL, DAQ::BIPOLAR, fNice);
  if (!pNidaq->doing_okay()) {
	cerr << "NidaqServer: problems initializing the nidaq card. Shutting down ..."
		<< endl;
	delete pNidaq;
	delete pConnection;
	return -1;
  }

  cerr << "Nidaq VRPN Server: opened on port " << iVrpnPort <<
	  ", press 'q' to quit." << endl;
  // Loop forever calling the mainloop()s for all devices
  while (1) {
    pNidaq->mainloop();
    
    // Send and receive all messages

	  pConnection->mainloop();
    if (!pConnection->doing_okay() || 
	!pNidaq->doing_okay() ||
	(_kbhit() && (_getch()=='q'))) {
      break;
    }
  }
  
  delete pNidaq;
  delete pConnection;
  return 0;
}
#endif // defined(WIN32) || defined(_WIN32)

