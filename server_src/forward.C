#include <vrpn_Connection.h>
#include <vrpn_Forwarder.h>

#include <stdio.h>  // fprintf()
#include <stdlib.h>  // atoi()
#include <signal.h>  // for signal()


// global so our signal handler can see them

vrpn_Connection * server_connection;
vrpn_Connection * client_connection;
vrpn_StreamForwarder * forwarder;


void Usage (char * s) {
  fprintf(stderr, "Usage:  %s location service port\n", s);
  exit(0);
}

void sighandler (int signal) {
  delete forwarder;
  delete server_connection;
  delete client_connection;
  exit(0);
}



int main (int argc, char ** argv) {

  char * source_location;
  char * source_name;
  char * client_name;
  int port = 4510;
  int retval;

  if ((argc < 3) || (argc > 4))
    Usage(argv[0]);

  source_location = argv[1];
  source_name = argv[2];
  client_name = argv[2];
  if (argc == 4)
    port = atoi(argv[3]);

  server_connection = vrpn_get_connection_by_name (source_location);
  client_connection = new vrpn_Synchronized_Connection (port);
  forwarder = new vrpn_StreamForwarder
    (server_connection, source_name, client_connection, client_name);

  retval = forwarder->forward("Tracker Pos/Quat", "Tracker Pos/Quat");

  if (retval)
    fprintf(stderr, "forwarder->forward(\"Tracker Pos/Quat\") failed.\n");
  else
    fprintf(stderr, "forwarder->forward(\"Tracker Pos/Quat\") succeeded.\n");


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

  // Do the dirty work.

  while (1) {
    server_connection->mainloop();
    client_connection->mainloop();
  }

}


