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
  Revised: Mon Dec 15 12:53:59 1997 by weberh
  $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/server_src/clock_server.C,v $
  $Locker:  $
  $Revision: 1.1 $
\*****************************************************************************/


#include <vrpn_Connection.h>
#include <vrpn_Clock.h>
#include <iostream.h>

main(int argc, char *argv[]) {
  if (argc!=2) {
    cerr << "Usage: " << argv[0] << " sdi_name_of_server_station/host" << endl;
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

  // create a vrpn server with no devices
  // Need only one per station (with multiple devices on it)
  vrpn_Connection c;

  // add a clock device to the server
  vrpn_Clock_Server *clock = new vrpn_Clock_Server(argv[1], &c);

  while(1) {
    clock->mainloop();
    c.mainloop();
  }

  return 0;
}

/*****************************************************************************\
  $Log: clock_server.C,v $
  Revision 1.1  1997/12/15 21:37:45  weberh
  demonstrates the use of the clock class as a server

  Revision 1.3  1997/12/13 07:37:16  weberh
  nice clock sync server

  Revision 1.1  1997/12/11 10:12:28  weberh
  Initial revision

  Revision 1.1  1997/12/11 09:41:24  weberh
  Initial revision

\*****************************************************************************/






