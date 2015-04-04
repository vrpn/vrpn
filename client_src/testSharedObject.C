#include <stdio.h>                      // for printf
#include <stdlib.h>                     // for exit
#include <vrpn_Connection.h>            // for vrpn_Connection, etc
#include <vrpn_SharedObject.h>          // for vrpn_Shared_int32_Remote, etc

#include "vrpn_Configure.h"             // for VRPN_CALLBACK
#include "vrpn_Shared.h"                // for timeval
#include "vrpn_Types.h"                 // for vrpn_bool, vrpn_int32

int VRPN_CALLBACK noteChange (void *userdata, vrpn_int32 newValue, vrpn_bool) {
  vrpn_Shared_int32_Remote * ip;

  ip = (vrpn_Shared_int32_Remote *) userdata;
  printf("Remote %s set to %d.\n", ip->name(), newValue);

  return 0;
}

int main (int, char ** argv) {

  vrpn_Connection * c;
  timeval qsec;

  qsec.tv_sec = 0L;
  qsec.tv_usec = 250000L;

  c = vrpn_get_connection_by_name(argv[1]);

  if (!c) {
    exit(0);
  }

  vrpn_Shared_int32_Remote a ("a", 0, VRPN_SO_DEFER_UPDATES);

  a.bindConnection(c);
  a.register_handler(noteChange, &a);

  c->mainloop(&qsec);

  printf("a = %d.\n", a.value());

  c->mainloop(&qsec);

  a = 3;

  c->mainloop(&qsec);

  printf("a = %d.\n", a.value());

  c->mainloop(&qsec);

  a = -3;

  c->mainloop(&qsec);

  printf("a = %d.\n", a.value());

  a.becomeSerializer();
  c->mainloop(&qsec);

  c->mainloop(&qsec);

  printf("a = %d.\n", a.value());

  c->mainloop(&qsec);

  a = 3;

  c->mainloop(&qsec);

  printf("a = %d.\n", a.value());

  c->mainloop(&qsec);

  a = -3;

  c->mainloop(&qsec);

  printf("a = %d.\n", a.value());

  while (1) {
    c->mainloop();
  }

}

