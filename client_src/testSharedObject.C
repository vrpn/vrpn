#include <stdlib.h>  // for exit()

#include <vrpn_Connection.h>
#include <vrpn_SharedObject.h>

int noteChange (void * userdata, vrpn_int32 newValue, vrpn_bool isLocal) {
  vrpn_Shared_int32_Remote * ip;

  ip = (vrpn_Shared_int32_Remote *) userdata;
  printf("Remote %s set to %d.\n", ip->name(), newValue);

  return 0;
}

int main (int argc, char ** argv) {

  vrpn_Connection * c;

  c = vrpn_get_connection_by_name(argv[1]);

  if (!c) {
    exit(0);
  }

  vrpn_Shared_int32_Remote a ("a", 0, VRPN_SO_DEFER_UPDATES);

  a.bindConnection(c);
  a.register_handler(noteChange, &a);

  c->mainloop();

  printf("a = %d.\n", a.value());

  c->mainloop();

  a = 3;

  c->mainloop();

  printf("a = %d.\n", a.value());

  c->mainloop();

  a = -3;

  c->mainloop();

  printf("a = %d.\n", a.value());

  while (1) {
    c->mainloop();
  }

}

