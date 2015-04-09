#include <vrpn_Connection.h>
#include <vrpn_SharedObject.h>

#include <stdio.h>  // for printf()
#include <stdlib.h>  // for atoi()

int VRPN_CALLBACK noteChange (void *userdata, vrpn_int32 newValue, vrpn_bool /*isLocal*/) {
  vrpn_Shared_int32_Server * ip;

  ip = (vrpn_Shared_int32_Server *) userdata;
  printf("Server %s set to %d.\n", ip->name(), newValue);

  return 0;
}

int main (int argc, char ** argv) {
  if (argc != 2) {
    fprintf(stderr, "Must pass a port number as the sole argument\n");
    return 1;
  }
  vrpn_Connection * c;

  c = vrpn_create_server_connection(atoi(argv[1]));

  vrpn_Shared_int32_Server a ("a", 0, VRPN_SO_DEFER_UPDATES);

  a.setSerializerPolicy(vrpn_DENY_REMOTE);

  c->mainloop();

  a.bindConnection(c);
  a.register_handler(&noteChange, &a);

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

  c->mainloop();

  while (c->doing_okay()) {
    c->mainloop();
  }

  printf("C is not OK;  shutting down.\n");

  delete c;
  return 2;
}

