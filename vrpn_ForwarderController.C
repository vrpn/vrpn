#include <stdio.h>  // for fprintf, stderr
#include <string.h> // for memcpy, strlen, NULL, etc

#include "vrpn_Connection.h" // for vrpn_Connection, etc
#include "vrpn_ForwarderController.h"
#include "vrpn_Shared.h" // for timeval, vrpn_gettimeofday
#if !(defined(_WIN32) && defined(VRPN_USE_WINSOCK_SOCKETS))
#include <netinet/in.h> // for htonl, ntohl
#endif

#include "vrpn_Forwarder.h" // for vrpn_ConnectionForwarder

vrpn_Forwarder_Brain::vrpn_Forwarder_Brain(vrpn_Connection *c)
    : d_connection(c)
    , d_myId(-1)
{

    if (!c) return;

    d_connection->addReference();

    d_myId = c->register_sender("vrpn_Forwarder_Brain");

    d_start_forwarding_type =
        c->register_message_type("vrpn_Forwarder_Brain start_forwarding");
    d_forward_type = c->register_message_type("vrpn_Forwarder_Brain forward");
}

vrpn_Forwarder_Brain::~vrpn_Forwarder_Brain(void)
{

    if (d_connection) {
        d_connection->removeReference();
    }
}

// static
char *
vrpn_Forwarder_Brain::encode_start_remote_forwarding(vrpn_int32 *length,
                                                     vrpn_int32 remote_port)
{
    char *outbuf;

    vrpn_int32 nPort;

    *length = sizeof(vrpn_int32);
    try { outbuf = new char[*length]; }
    catch (...) {
        *length = 0;
        return NULL;
    }

    nPort = htonl(remote_port);
    memcpy(outbuf, &nPort, sizeof(vrpn_int32));

    return outbuf;
}

// static
void vrpn_Forwarder_Brain::decode_start_remote_forwarding(
    const char *buffer, vrpn_int32 *remote_port)
{
    vrpn_int32 port;

    if (!buffer || !remote_port) return;

    memcpy(&port, buffer, sizeof(vrpn_int32));
    *remote_port = ntohl(port);
}

// static
char *vrpn_Forwarder_Brain::encode_forward_message_type(
    vrpn_int32 *length, vrpn_int32 remote_port, const char *service_name,
    const char *message_type)
{
    char *outbuf;

    vrpn_int32 nPort;
    vrpn_int32 nSLen;
    vrpn_int32 nTLen;

    if (!service_name || !message_type) { *length = 0; return NULL; }
    *length = static_cast<int>(3 * sizeof(vrpn_int32) + strlen(service_name) +
                               strlen(message_type));
    try { outbuf = new char[*length]; }
    catch (...) {
        *length = 0;
        return NULL;
    }

    // fprintf(stderr, "Encoding forward %s of %s on port %d.\n",
    // message_type, service_name, remote_port);

    // Put all the char [] at the end of the message so we don't have to
    // worry about padding and alignment.

    nPort = htonl(remote_port);
    nSLen = htonl(static_cast<vrpn_int32>(strlen(service_name)));
    nTLen = htonl(static_cast<vrpn_int32>(strlen(message_type)));
    memcpy(outbuf, &nPort, sizeof(vrpn_int32));
    memcpy(outbuf + sizeof(vrpn_int32), &nSLen, sizeof(vrpn_int32));
    memcpy(outbuf + 2 * sizeof(vrpn_int32), &nTLen, sizeof(vrpn_int32));
    strcpy(outbuf + 3 * sizeof(vrpn_int32), service_name);
    strcpy(outbuf + 3 * sizeof(vrpn_int32) + strlen(service_name),
           message_type);

    return outbuf;
}

// static
void vrpn_Forwarder_Brain::decode_forward_message_type(const char *buffer,
                                                       vrpn_int32 *remote_port,
                                                       char **service_name,
                                                       char **message_type)
{
    vrpn_int32 port;
    vrpn_int32 Slength;
    vrpn_int32 Tlength;
    char *Soutbuf;
    char *Toutbuf;

    if (!buffer || !remote_port || !message_type) return;

    // All the char [] are at the end of the message so we don't have to
    // worry about padding and alignment.

    memcpy(&port, buffer, sizeof(vrpn_int32));
    *remote_port = ntohl(port);
    memcpy(&Slength, buffer + sizeof(vrpn_int32), sizeof(vrpn_int32));
    Slength = ntohl(Slength);
    try { Soutbuf = new char[1 + Slength]; }
    catch (...) {
      *remote_port = -1;
      *service_name = NULL;
      *message_type = NULL;
      return;
    }
    memcpy(&Tlength, buffer + 2 * sizeof(vrpn_int32), sizeof(vrpn_int32));
    Tlength = ntohl(Tlength);
    try { Toutbuf = new char[1 + Tlength]; }
    catch (...) {
        *remote_port = -1;
        *service_name = NULL;
        *message_type = NULL;
        return;
    }
    strncpy(Soutbuf, buffer + 3 * sizeof(vrpn_int32), Slength);
    Soutbuf[Slength] = '\0';
    *service_name = Soutbuf;
    strncpy(Toutbuf, buffer + 3 * sizeof(vrpn_int32) + Slength, Tlength);
    Toutbuf[Tlength] = '\0';
    *message_type = Toutbuf;
}

vrpn_Forwarder_Server::vrpn_Forwarder_Server(vrpn_Connection *c)
    : vrpn_Forwarder_Brain(c)
    , d_myForwarders(NULL)
{

    if (!c) return;

    c->register_handler(d_start_forwarding_type, handle_start, this, d_myId);
    c->register_handler(d_forward_type, handle_forward, this, d_myId);
}

vrpn_Forwarder_Server::~vrpn_Forwarder_Server(void)
{

    if (!d_connection) return;

    d_connection->unregister_handler(d_start_forwarding_type, handle_start,
                                     this, d_myId);
    d_connection->unregister_handler(d_forward_type, handle_forward, this,
                                     d_myId);

    // Destroy my list of forwarders
    vrpn_Forwarder_List *fp;
    for (fp = d_myForwarders; fp; fp = fp->next) {
      if (fp->connection) {
        try {
          delete fp->connection;
        } catch (...) {
          fprintf(stderr, "vrpn_Forwarder_Server::~vrpn_Forwarder_Server(): delete failed\n");
          return;
        }
      }
      if (fp->forwarder) {
        try {
          delete fp->forwarder;
        } catch (...) {
          fprintf(stderr, "vrpn_Forwarder_Server::~vrpn_Forwarder_Server(): delete failed\n");
          return;
        }
      }
    }
}

void vrpn_Forwarder_Server::mainloop(void)
{

    vrpn_Forwarder_List *fp;

    for (fp = d_myForwarders; fp; fp = fp->next)
        if (fp->connection) fp->connection->mainloop();
}

vrpn_bool vrpn_Forwarder_Server::start_remote_forwarding(vrpn_int32 remote_port)
{
    vrpn_Forwarder_List *fp;

    // Make sure it isn't already there

    for (fp = d_myForwarders; fp; fp = fp->next)
        if (fp->port == remote_port) {
            fprintf(stderr, "vrpn_Forwarder_Server::start_remote_forwarding:  "
                            "Already open on port %d.\n",
                    remote_port);
            return false;
        }

    // Create it and add it to the list

    try { fp = new vrpn_Forwarder_List; }
    catch (...) {
      fprintf(stderr, "vrpn_Forwarder_Server::start_remote_forwarding:  "
        "Out of memory.\n");
      return false;
    }

    fp->port = remote_port;
    fp->connection = vrpn_create_server_connection(remote_port);
    try { fp->forwarder = new vrpn_ConnectionForwarder(d_connection, fp->connection); }
    catch (...) {
      fprintf(stderr, "vrpn_Forwarder_Server::start_remote_forwarding:  "
        "Out of memory.\n");
      return false;
    }

    fp->next = d_myForwarders;
    d_myForwarders = fp;

    // fprintf(stderr, "vrpn_Forwarder_Server::start_remote_forwarding:  "
    //"On port %d.\n", remote_port);
    return true;
}

void vrpn_Forwarder_Server::forward_message_type(vrpn_int32 remote_port,
                                                 const char *service_name,
                                                 const char *message_type)
{

    vrpn_Forwarder_List *fp;
    vrpn_Forwarder_List *it = NULL;
    int retval;

    // Find the forwarder requested

    for (fp = d_myForwarders; fp; fp = fp->next)
        if (fp->port == remote_port) it = fp;

    if (!it) {
        fprintf(stderr,
                "vrpn_Forwarder_Server:  No forwarder open on port %d.\n",
                remote_port);
        return;
    }

    // Forward that message type from that service

    retval = it->forwarder->forward(message_type, service_name, message_type,
                                    service_name);
    if (retval) {
        fprintf(stderr, "vrpn_Forwarder_Server:  Couldn't forward messages of "
                        "type \"%s\" on port %d.\n",
                message_type, remote_port);
        return;
    }
}

// static
int vrpn_Forwarder_Server::handle_start(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Forwarder_Server *me = (vrpn_Forwarder_Server *)userdata;
    vrpn_int32 port;

    decode_start_remote_forwarding(p.buffer, &port);
    me->start_remote_forwarding(port);
    return 0;
}

// static
int vrpn_Forwarder_Server::handle_forward(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Forwarder_Server *me = (vrpn_Forwarder_Server *)userdata;
    vrpn_int32 port;
    char *typebuffer;
    char *servicebuffer;

    decode_forward_message_type(p.buffer, &port, &servicebuffer, &typebuffer);
    if (!servicebuffer || !typebuffer) return -1; // memory allocation failure
    me->forward_message_type(port, servicebuffer, typebuffer);

    try {
      delete[] servicebuffer;
      delete[] typebuffer;
    } catch (...) {
      fprintf(stderr, "vrpn_Forwarder_Server::handle_forward(): delete failed\n");
      return -1;
    }
    return 0;
}

vrpn_Forwarder_Controller::vrpn_Forwarder_Controller(vrpn_Connection *c)
    : vrpn_Forwarder_Brain(c)
{
}

vrpn_Forwarder_Controller::~vrpn_Forwarder_Controller(void) {}

vrpn_bool vrpn_Forwarder_Controller::start_remote_forwarding(vrpn_int32 remote_port)
{

    struct timeval now;
    char *buffer;
    vrpn_int32 length;

    vrpn_gettimeofday(&now, NULL);
    buffer = encode_start_remote_forwarding(&length, remote_port);

    if (!buffer) return false; // memory allocation failure

    int ret = d_connection->pack_message(length, now, d_start_forwarding_type, d_myId,
                               buffer, vrpn_CONNECTION_RELIABLE);
    try {
      delete[] buffer;
    } catch (...) {
      fprintf(stderr, "vrpn_Forwarder_Server::start_remote_forwarding(): delete failed\n");
      return false;
    }
    return ret == 0;
}

void vrpn_Forwarder_Controller::forward_message_type(vrpn_int32 remote_port,
                                                     const char *service_name,
                                                     const char *message_type)
{
    struct timeval now;
    char *buffer;
    vrpn_int32 length;

    vrpn_gettimeofday(&now, NULL);
    buffer = encode_forward_message_type(&length, remote_port, service_name,
                                         message_type);

    if (!buffer) return; // memory allocation failure

    d_connection->pack_message(length, now, d_forward_type, d_myId, buffer,
                               vrpn_CONNECTION_RELIABLE);
    try {
      delete[] buffer;
    } catch (...) {
      fprintf(stderr, "vrpn_Forwarder_Server::forward_message_type(): delete failed\n");
      return;
    }
}
