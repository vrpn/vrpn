/*****************************************************************************\
  vrpn_NewClockClient.cpp
  --
  Description : an example of how to use a vrpn_Clock_Remote object to 
                provide cross-network time synchronization.

	        See vrpn_Clock.[Ch] and vrpn_NewClockServer.cpp for more
		details.

  ----------------------------------------------------------------------------
  Author: weberh
  Created: Wed Dec 10 16:01:46 1997
  Revised: Mon Dec 15 15:09:18 1997 by weberh
\*****************************************************************************/


#include <vrpn_Connection.h>
#include <vrpn_Clock.h>
#include <iostream.h>

#include <stdlib.h>

void printClockOffset( void *userdata, const vrpn_CLOCKCB& info ) {
  cerr << "clock offset is " << timevalMsecs(info.tvClockOffset) 
       << " msecs (used round trip which took " 
       << 2*timevalMsecs(info.tvHalfRoundTrip) << " msecs)." << endl;
}

main(int argc, char *argv[]) {

  if (argc<2) {
    cerr << "Usage: " << argv[0] << " sdi_name_of_server_station/host" 
	 << " [freq in hz -- if <0, then do a full sync]" 
	 << endl;
    return -1;
  }
  
#ifdef WIN32
  // init windows socket dll, version 1.1
  WSADATA wsaData; 
  int status;
  if ((status = WSAStartup(MAKEWORD(1,1), &wsaData)) != 0) {
    cerr << "WSAStartup failed with " << status << endl;
  } else {
    cerr << "WSAStartup success" << endl;
  }
#endif
  vrpn_Clock_Remote *clock;

  // attempt to connect to a remote clock
  if (argc==3) {
    // specify a quick sync rate (or no quick sync if negative)
    clock=new vrpn_Clock_Remote(argv[1], atof(argv[2]));
  } else {
    // use default quick sync rate
    clock=new vrpn_Clock_Remote(argv[1]);
  }

  clock->register_clock_sync_handler( NULL, printClockOffset );

  if (argc>2) {
    if (atof(argv[2])<0) {
      clock->fullSync();
    }
  }

  while (1) {
    clock->mainloop();
  }

  return 0;
}

/*****************************************************************************\
  $Log: clock_client.C,v $
  Revision 1.2  2000/08/28 16:25:18  taylorr
  2000-08-24  Russell M. Taylor II  <taylorr@cs.unc.edu>

          * Makefile : Added AIX compile

  Revision 1.1.1.1  2000/02/22 18:45:49  ddj
  VRPN 4.11 beta 2 sources

  Revision 1.1  1997/12/15 21:37:29  weberh
  demonstrates the use of the clock class as a client

  Revision 1.3  1997/12/13 07:37:16  weberh
  nice clock sync server

  Revision 1.1  1997/12/11 10:12:28  weberh
  Initial revision

  Revision 1.1  1997/12/11 09:41:24  weberh
  Initial revision

\*****************************************************************************/






