#include "vrpn_ForwarderController.h"

#include <sys/types.h>
#include <netinet/in.h>  // for ntohl()/htonl()

#include "vrpn_Forwarder.h"

vrpn_Forwarder_Brain::vrpn_Forwarder_Brain (vrpn_Connection * c) :
    d_connection (c),
    d_myId (-1) {

  if (!c)
    return;

  d_myId = c->register_sender("vrpn Forwarder Brain");

  d_start_forwarding_type =
      c->register_message_type("vrpn Forwarder Brain start");
  d_forward_type = 
      c->register_message_type("vrpn Forwarder Brain forward");
}


vrpn_Forwarder_Brain::~vrpn_Forwarder_Brain (void) {

}

// static
char * vrpn_Forwarder_Brain::encode_start_remote_forwarding
            (int * length, int remote_port) {
  char * outbuf;

  int nPort;

  *length = sizeof(int);
  outbuf = new char [*length];
  if (!outbuf) {
    *length = 0;
    return NULL;
  }

  nPort = htonl(remote_port);
  memcpy(outbuf, &nPort, sizeof(int));

  return outbuf;
}

// static
void vrpn_Forwarder_Brain::decode_start_remote_forwarding
          (const char * buffer, int * remote_port) {
  int port;

  if (!buffer || !remote_port) return;

  memcpy(&port, buffer, sizeof(int));
  *remote_port = ntohl(port);
}

// static
char * vrpn_Forwarder_Brain::encode_forward_message_type
            (int * length, int remote_port, const char * service_name,
             const char * message_type) {
  char * outbuf;

  int nPort;
  int nSLen;
  int nTLen;

  *length = 3 * sizeof(int) + strlen(service_name) + strlen(message_type);
  outbuf = new char [*length];
  if (!outbuf) {
    *length = 0;
    return NULL;
  }

//fprintf(stderr, "Encoding forward %s of %s on port %d.\n",
//message_type, service_name, remote_port);

  // Put all the char [] at the end of the message so we don't have to
  // worry about padding and alignment.

  nPort = htonl(remote_port);
  nSLen = htonl(strlen(service_name));
  nTLen = htonl(strlen(message_type));
  memcpy(outbuf, &nPort, sizeof(int));
  memcpy(outbuf + sizeof(int), &nSLen, sizeof(int));
  memcpy(outbuf + 2 * sizeof(int), &nTLen, sizeof(int));
  strcpy(outbuf + 3 * sizeof(int), service_name);
  strcpy(outbuf + 3 * sizeof(int) + strlen(service_name), message_type);

  return outbuf;
}

// static
void vrpn_Forwarder_Brain::decode_forward_message_type
          (const char * buffer, int * remote_port, char ** service_name,
           char ** message_type) {
  int port;
  int Slength;
  int Tlength;
  char * Soutbuf;
  char * Toutbuf;

  if (!buffer || !remote_port || !message_type) return;

  // All the char [] are at the end of the message so we don't have to
  // worry about padding and alignment.

  memcpy(&port, buffer, sizeof(int));
  *remote_port = ntohl(port);
  memcpy(&Slength, buffer + sizeof(int), sizeof(int));
  Slength = ntohl(Slength);
  Soutbuf = new char [1 + Slength];
  memcpy(&Tlength, buffer + 2 * sizeof(int), sizeof(int));
  Tlength = ntohl(Tlength);
  Toutbuf = new char [1 + Tlength];
  if (!Soutbuf || !Toutbuf) {
    *remote_port = -1;
    *service_name = NULL;
    *message_type = NULL;
    return;
  }
  strncpy(Soutbuf, buffer + 3 * sizeof(int), Slength);
  Soutbuf[Slength] = '\0';
  *service_name = Soutbuf;
  strncpy(Toutbuf, buffer + 3 * sizeof(int) + Slength, Tlength);
  Toutbuf[Tlength] = '\0';
  *message_type = Toutbuf;
}


vrpn_Forwarder_Server::vrpn_Forwarder_Server (vrpn_Connection * c) :
    vrpn_Forwarder_Brain (c) {

  if (!c) return;

  c->register_handler(d_start_forwarding_type, handle_start, this,
                      d_myId);
  c->register_handler(d_forward_type, handle_forward, this,
                      d_myId);
}

vrpn_Forwarder_Server::~vrpn_Forwarder_Server (void) {

  if (!d_connection) return;

  d_connection->unregister_handler
         (d_start_forwarding_type, handle_start, this, d_myId);
  d_connection->unregister_handler
         (d_forward_type, handle_forward, this, d_myId);
}

void vrpn_Forwarder_Server::mainloop (void) {

  vrpn_Forwarder_List * fp;

  for (fp = d_myForwarders;  fp;  fp = fp->next)
    if (fp->connection)
      fp->connection->mainloop();

}

void vrpn_Forwarder_Server::start_remote_forwarding
                    (int remote_port) {

  vrpn_Forwarder_List * fp;

  // Make sure it isn't already there

  for (fp = d_myForwarders;  fp;  fp = fp->next)
    if (fp->port == remote_port) {
      fprintf(stderr, "vrpn_Forwarder_Server::start_remote_forwarding:  "
                      "Already open on port %d.\n", remote_port);
      return;
    }

  // Create it and add it to the list

  fp = new vrpn_Forwarder_List;

  fp->port = remote_port;
  fp->connection = new vrpn_Synchronized_Connection (remote_port);
  fp->forwarder = new vrpn_ConnectionForwarder
       (d_connection, fp->connection);

  fp->next = d_myForwarders;
  d_myForwarders = fp;

//fprintf(stderr, "vrpn_Forwarder_Server::start_remote_forwarding:  "
                //"On port %d.\n", remote_port);
  
}

void vrpn_Forwarder_Server::forward_message_type
                   (int remote_port, const char * service_name,
                    const char * message_type) {

  vrpn_Forwarder_List * fp;
  vrpn_Forwarder_List * it = NULL;
  int retval;

  // Find the forwarder requested

  for (fp = d_myForwarders;  fp;  fp = fp->next)
    if (fp->port == remote_port)
      it = fp;

  if (!it) {
    fprintf(stderr, "vrpn_Forwarder_Server:  No forwarder open on port %d.\n",
            remote_port);
    return;
  }

  // Forward that message type from that service

  retval = it->forwarder->forward(message_type, service_name,
                                  message_type, service_name);
  if (retval) {
    fprintf(stderr, "vrpn_Forwarder_Server:  Couldn't forward messages of "
                    "type \"%s\" on port %d.\n",
            message_type, remote_port);
    return;
  }

}

// static
int vrpn_Forwarder_Server::handle_start
                   (void * userdata, vrpn_HANDLERPARAM p) {
  vrpn_Forwarder_Server * me = (vrpn_Forwarder_Server *) userdata;
  int port;

  decode_start_remote_forwarding(p.buffer, &port);
  me->start_remote_forwarding(port);
  return 0;
}


// static
int vrpn_Forwarder_Server::handle_forward
                   (void * userdata, vrpn_HANDLERPARAM p) {
  vrpn_Forwarder_Server * me = (vrpn_Forwarder_Server *) userdata;
  int port;
  char * typebuffer;
  char * servicebuffer;

  decode_forward_message_type(p.buffer, &port, &servicebuffer, &typebuffer);
  if (!servicebuffer || !typebuffer)
    return -1;  // memory allocation failure
  me->forward_message_type(port, servicebuffer, typebuffer);

  delete [] servicebuffer;
  delete [] typebuffer;
  return 0;
}


vrpn_Forwarder_Controller::vrpn_Forwarder_Controller (vrpn_Connection * c) :
    vrpn_Forwarder_Brain (c) {

}

vrpn_Forwarder_Controller::~vrpn_Forwarder_Controller (void) {

}

void vrpn_Forwarder_Controller::start_remote_forwarding (int remote_port) {

  struct timeval now;
  char * buffer;
  int length;

  gettimeofday(&now, NULL);
  buffer = encode_start_remote_forwarding(&length, remote_port);

  if (!buffer)
    return;  // memory allocation failure

  d_connection->pack_message(length, now, d_start_forwarding_type,
                             d_myId, buffer, vrpn_CONNECTION_RELIABLE);
  delete [] buffer;
}

void vrpn_Forwarder_Controller::forward_message_type
                   (int remote_port, const char * service_name,
                    const char * message_type) {
  struct timeval now;
  char * buffer;
  int length;

  gettimeofday(&now, NULL);
  buffer = encode_forward_message_type(&length, remote_port, service_name,
                                       message_type);

  if (!buffer)
    return;  // memory allocation failure

  d_connection->pack_message(length, now, d_forward_type,
                             d_myId, buffer, vrpn_CONNECTION_RELIABLE);
  delete [] buffer;
}


