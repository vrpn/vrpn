#include <stdio.h>

#include <vrpn_Connection.h>

int VRPN_CALLBACK handle_quit (void * userdata, vrpn_HANDLERPARAM) {
  int * quitNow = (int *) userdata;

  *quitNow = 1;

  return 0;  // non-error completion
}

int VRPN_CALLBACK handle_any_print (void * userdata, vrpn_HANDLERPARAM p) {
  vrpn_Connection * c = (vrpn_Connection *) userdata;

  fprintf(stderr, "Got mesage \"%s\" from \"%s\".\n",
          c->message_type_name(p.type), c->sender_name(p.sender));

  return 0;  // non-error completion
}

int main (int, char **) {

  vrpn_Connection * listen_connection;
  long myId;
  long fooType, barType, bazType, quitType;
  int quitNow = 0;

  listen_connection = vrpn_create_server_connection();
    // defaults to port vrpn_DEFAULT_LISTEN_PORT_NO

  myId = listen_connection->register_sender("Sample Server");
  fooType = listen_connection->register_message_type("Sample Foo Type");
  barType = listen_connection->register_message_type("Sample Bar Type");
  bazType = listen_connection->register_message_type("Sample Baz Type");
  quitType = listen_connection->register_message_type("Sample Quit Type");

  listen_connection->register_handler(quitType, handle_quit, &quitNow);
  listen_connection->register_handler(vrpn_ANY_TYPE, handle_any_print,
                                      listen_connection);
    // defaults to vrpn_SENDER_ANY

  while (!quitNow) {
    listen_connection->mainloop();
  }
}
