/*****************************************************************************\
  vrpn_NewClockServer.cpp
  --
  Description : shows how to use a vrpn_Clock object to provide for cross-
                network time synchronization.

		See vrpn_Clock.[Ch] and vrpn_NewClockClient.cpp for more 
		details.

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

