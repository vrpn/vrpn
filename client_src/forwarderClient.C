#include <vrpn_Connection.h>
#include <vrpn_ForwarderController.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// This program connects to a vrpn (Tracker) server and can be used
// to tell it to open ports for other clients to listen to and to
// start forwarding Tracker messages to them.

// Tom Hudson, October 1998

vrpn_Connection * connection;
vrpn_Forwarder_Controller * controller;

void Usage (char * s) {
  fprintf(stderr, "Usage:  %s location\n", s);
  exit(0);
}

int main (int argc, char ** argv) {

  char ib [1000];
  char sb [75], tb [75];
  char * qp;
  char * source_location;
  int port;
  int len;

  if (argc != 2) Usage(argv[0]);

  source_location = argv[1];

  connection = vrpn_get_connection_by_name (source_location);

  controller = new vrpn_Forwarder_Controller (connection);

  printf("Commands:\n"
         "  quit\n"
         "  open <port>\n"
         "  forward <port> \"<service name>\" \"<message type>\"\n");
  do {
    if (fgets(ib, 1000, stdin) == NULL) {
	perror("Could not read line");
	return -1;
    }
    fprintf(stderr, "Got:  >%s<\n", ib);
    if (!strncmp(ib, "open", 4)) {
      port = atoi(ib + 5);
      controller->start_remote_forwarding(port);
      fprintf(stderr, "Opening %d.\n", port);
    }
    if (!strncmp(ib, "forward", 7)) {
      port = atoi(ib + 8);
      qp = strchr(ib, '\"');  // start of service name
      qp++;
      len = static_cast<int>(strcspn(qp, "\""));  fprintf(stderr, "Prefix len %d.\n", len);
      strncpy(sb, qp, len);
      sb[len] = '\0';
      qp = strchr(qp, '\"');  // end of service name
      qp++;
      qp = strchr(qp, '\"');  // start of message type name
      qp++;
      len = static_cast<int>(strcspn(qp, "\""));  fprintf(stderr, "Prefix len %d.\n", len);
      strncpy(tb, qp, len);
      tb[len] = '\0';
      qp = strchr(qp, '\"');  // end of message type name

      controller->forward_message_type(port, sb, tb);
fprintf(stderr, "Forwarding %s of %s on %d.\n", tb, sb, port);
    }
    connection->mainloop();
  } while (strncmp(ib, "quit", 4));
  
}
