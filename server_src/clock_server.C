/*****************************************************************************\
  vrpn_NewClockServer.cpp
  --
  Description : shows how to use a vrpn_Clock object to provide for cross-
                network time synchronization.

		See vrpn_Clock.[Ch] and vrpn_NewClockClient.cpp for more 
		details.

  ----------------------------------------------------------------------------
  Author: weberh
  Created: Wed Dec 10 16:01:46 1997
  Revised: Mon Dec 15 18:24:22 1997 by weberh
  $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/server_src/clock_server.C,v $
  $Locker:  $
  $Revision: 1.4 $
\*****************************************************************************/


#include <vrpn_Connection.h>
#include <vrpn_Clock.h>
#ifdef  VRPN_USE_OLD_STREAMS
        #include <iostream.h>
#else
        #include <iostream>
        using namespace std;
#endif

main(int argc, char *argv[]) {


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

  // create a vrpn server with no devices
  // Need only one per station (with multiple devices on it)
  vrpn_Connection c;

  // add a clock device to the server
  vrpn_Clock_Server *clock = new vrpn_Clock_Server(&c);

  while(1) {
    clock->mainloop();
    c.mainloop();
  }

  return 0;
}

/*****************************************************************************\
  $Log: clock_server.C,v $
  Revision 1.4  2004/04/08 15:33:48  taylorr
  2004-04-08  Russell M. Taylor II  <taylorr@cs.unc.edu>

          * Makefile : Adding flag to SGI compile in the hope that it would
                  fix things and let us use the new streams libraries.  It
                  got us part of the way there on my system.
          * NIUtil.cpp : Same.
          * clock_server.C : Same.
          * nidaqClient.cpp : Same.

  Revision 1.3  2000/08/28 16:25:36  taylorr
  2000-08-24  Russell M. Taylor II  <taylorr@cs.unc.edu>

          * Makefile : Added AIX compile

  Revision 1.1.1.1  2000/02/22 18:45:49  ddj
  VRPN 4.11 beta 2 sources

  Revision 1.2  1998/01/08 23:34:11  weberh
  clock_server.C has sample code for a clock server/client (sync connection
  class takes care of all this)

  vrpn.C has winsock startup call and sync connection

  Revision 1.3  1997/12/13 07:37:16  weberh
  nice clock sync server

  Revision 1.1  1997/12/11 10:12:28  weberh
  Initial revision

  Revision 1.1  1997/12/11 09:41:24  weberh
  Initial revision

\*****************************************************************************/






