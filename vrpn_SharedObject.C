#include "vrpn_SharedObject.h"

#include <string.h>

#include <vrpn_Connection.h>

vrpn_Shared_int32::vrpn_Shared_int32 (const char * name,
                                      vrpn_int32 defaultValue) :
  d_value (defaultValue),
  d_name (name ? new char [1 + strlen(name)] : NULL),
  d_connection (NULL),
  d_myId (-1),
  d_updateFromServer_type (-1),
  d_updateFromRemote_type (-1) {

}

// virtual
vrpn_Shared_int32::~vrpn_Shared_int32 (void) {
  if (d_name) {
    delete [] d_name;
  }
}

vrpn_int32 vrpn_Shared_int32::value (void) const {
  return d_value;
}

vrpn_Shared_int32::operator int () const {
  return value();
}

void vrpn_Shared_int32::bindConnection (vrpn_Connection * c) {
  char buffer [101];
  if (c && d_connection) {
    fprintf(stderr, "vrpn_Shared_int32::bindConnection:  "
                    "Tried to rebind a connection to %s.\n", d_name);
    return;
  }

  d_connection = c;
  sprintf(buffer, "vrpn Shared int32 %s", d_name);
  d_myId = c->register_sender(buffer);
  d_updateFromServer_type = c->register_message_type
          ("vrpn Shared int32 update from server");
  d_updateFromRemote_type = c->register_message_type
          ("vrpn Shared int32 update from remote");
}

void vrpn_Shared_int32::register_handler (vrpnSharedIntCallback cb,
                                          void * userdata) {
  callbackEntry * e = new callbackEntry;
  if (!e) {
    fprintf(stderr, "vrpn_Shared_int32::register_handler:  "
                    "Out of memory.\n");
    return;
  }
  e->handler = cb;
  e->userdata = userdata;
  e->next = d_callbacks;
  d_callbacks = e;
}

void vrpn_Shared_int32::encode (char ** buffer, vrpn_int32 * len) const {
  vrpn_buffer(buffer, len, d_value);
}
void vrpn_Shared_int32::decode (const char ** buffer, vrpn_int32 *) {
  vrpn_unbuffer(buffer, &d_value);
}



vrpn_Shared_int32_Server::vrpn_Shared_int32_Server (const char * name,
                                                    vrpn_int32 defaultValue) :
    vrpn_Shared_int32 (name, defaultValue) {

}

// virtual
vrpn_Shared_int32_Server::~vrpn_Shared_int32_Server (void) {

  // unregister handlers
  if (d_connection) {
    d_connection->unregister_handler
           (d_updateFromRemote_type, handle_updateFromRemote, this, d_myId);
  }

}

// virtual
vrpn_Shared_int32 & vrpn_Shared_int32_Server::operator =
                                      (vrpn_int32 & newValue) {
  struct timeval now;

  d_value = newValue;

  if (d_connection) {
    char buffer [32];
    int buflen = 32;

    gettimeofday(&now, NULL);
    encode((char **) &buffer, &buflen);
    d_connection->pack_message(32 - buflen, now,
                               d_updateFromServer_type, d_myId,
                               buffer, vrpn_CONNECTION_RELIABLE);
  }

  return *this;
}

// virtual
void vrpn_Shared_int32_Server::bindConnection (vrpn_Connection * c) {

  vrpn_Shared_int32::bindConnection(c);

  if (d_connection) {
    d_connection->register_handler(d_updateFromRemote_type,
                                   handle_updateFromRemote,
                                   this, d_myId);
  }
}

// static
int vrpn_Shared_int32_Server::handle_updateFromRemote
             (void * ud, vrpn_HANDLERPARAM p) {
  vrpn_Shared_int32_Server * s = (vrpn_Shared_int32_Server *) ud;
  s->decode(&p.buffer, &p.payload_len);

  return 0;
}


vrpn_Shared_int32_Remote::vrpn_Shared_int32_Remote (const char * name,
                                                    vrpn_int32 defaultValue) :
    vrpn_Shared_int32 (name, defaultValue) {

}

// virtual
vrpn_Shared_int32_Remote::~vrpn_Shared_int32_Remote (void) {

  // unregister handlers
  if (d_connection) {
    d_connection->unregister_handler
           (d_updateFromRemote_type, handle_updateFromServer, this, d_myId);
  }

}

// virtual
vrpn_Shared_int32 & vrpn_Shared_int32_Remote::operator =
                                            (vrpn_int32 & newValue) {
  struct timeval now;

  d_value = newValue;

  if (d_connection) {
    char buffer [32];
    int buflen = 32;

    gettimeofday(&now, NULL);
    encode((char **) &buffer, &buflen);
    d_connection->pack_message(32 - buflen, now,
                               d_updateFromRemote_type, d_myId,
                               buffer, vrpn_CONNECTION_RELIABLE);
  }

  return *this;
}

// virtual
void vrpn_Shared_int32_Remote::bindConnection (vrpn_Connection * c) {

  vrpn_Shared_int32::bindConnection(c);

  if (d_connection) {
    d_connection->register_handler(d_updateFromServer_type,
                                   handle_updateFromServer,
                                   this, d_myId);
  }
}

// static
int vrpn_Shared_int32_Remote::handle_updateFromServer
             (void * ud, vrpn_HANDLERPARAM p) {
  vrpn_Shared_int32_Remote * s = (vrpn_Shared_int32_Remote *) ud;
  s->decode(&p.buffer, &p.payload_len);

  return 0;
}




