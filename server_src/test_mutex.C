
#include <stdlib.h>
#include <vrpn_Mutex.h>


int main (int argc, char ** argv) {

  vrpn_Synchronized_Connection * c;
  vrpn_Mutex_Server * me;
  int portno = 4500;

  if (argc > 2) {
    portno = atoi(argv[2]);
  }

  c = new vrpn_Synchronized_Connection (portno);
  me = new vrpn_Mutex_Server (argv[1], c);

  while (1) {
    me->mainloop();
  }

}


