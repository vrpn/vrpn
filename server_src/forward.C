#include <vrpn_Connection.h>
#include <vrpn_Forwarder.h>

#include <stdio.h>  // fprintf()
#include <stdlib.h>  // atoi()
#include <signal.h>  // for signal()

// forward.C

// This program is a toy use of the vrpn_Forwarder class
// (actually vrpn_StreamForwarder, one of the two alternatives).

// global so our signal handler can see them

vrpn_Connection * server_connection;
vrpn_Connection * client_connection;
vrpn_StreamForwarder * forwarder;


void Usage (char * s) {
  fprintf(stderr, "Usage:  %s location service port\n", s);
  exit(0);
}

// Catch control-C and shut down our network connections nicely.

void sighandler (int /*signal*/) {
  delete forwarder;
  delete server_connection;
  delete client_connection;
  exit(0);
}

// Install the signal handler.  Isn't it annoying that this code is
// longer than the handler itself?

void install_handler (void) {

#ifndef WIN32
#ifdef sgi

  sigset( SIGINT, sighandler );
  sigset( SIGKILL, sighandler );
  sigset( SIGTERM, sighandler );
  sigset( SIGPIPE, sighandler );

#else

  signal( SIGINT, sighandler );
  signal( SIGKILL, sighandler );
  signal( SIGTERM, sighandler );
  signal( SIGPIPE, sighandler );

#endif  // sgi
#endif  // WIN32

}


int main (int argc, char ** argv) {

  char * source_location;
  char * source_name;
  char * client_name;
  int port = 4510;
  int retval;

  if ((argc < 3) || (argc > 4))
    Usage(argv[0]);

  // Expect that source_name and client_name are both ???
  // Port is the port that our client will connect to us on

  source_location = argv[1];
  source_name = argv[2];
  client_name = argv[2];
  if (argc == 4)
    port = atoi(argv[3]);

  // Connect to the server.

  server_connection = vrpn_get_connection_by_name (source_location);

  // Open a port for our client to connect to us.

  client_connection = vrpn_create_server_connection(port);

  // Put a forwarder on that port.

  forwarder = new vrpn_StreamForwarder
    (server_connection, source_name, client_connection, client_name);

  // Tell the forwarder to send Tracker Pos/Quat messages through,
  // using the same name on both sides.

  retval = forwarder->forward("Tracker Pos/Quat", "Tracker Pos/Quat");

  if (retval)
    fprintf(stderr, "forwarder->forward(\"Tracker Pos/Quat\") failed.\n");
  else
    fprintf(stderr, "forwarder->forward(\"Tracker Pos/Quat\") succeeded.\n");

  // Set up a signal handler to shut down cleanly on control-C.

  install_handler();

  // Do the dirty work.

  while (1) {

    // Get any received messages and queue them up for transmission
    // to the client.

    server_connection->mainloop();

    // Send them.

    client_connection->mainloop();

  }

}


