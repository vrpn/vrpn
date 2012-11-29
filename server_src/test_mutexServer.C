
#include <stdlib.h>
#include <string.h>
#include <vrpn_Connection.h>
#include <vrpn_Mutex.h>


int main (int argc, char ** argv) {

  vrpn_Connection * c;
  vrpn_Mutex_Server * me;
  int portno = vrpn_DEFAULT_LISTEN_PORT_NO;

  if (argc > 2) {
    portno = atoi(argv[2]);
  }

  char con_name[512];
  sprintf(con_name, "localhost:%d", portno);
  c = vrpn_create_server_connection(con_name);
  me = new vrpn_Mutex_Server (argv[1], c);

  while (1) {
    me->mainloop();
  }

}


