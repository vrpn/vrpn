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
  d_updateFromRemote_type (-1),
  d_callbacks (NULL) {

  if (name) {
    strcpy(d_name, name);
  }
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

//printf ("My name is %s;  myId %d, ufs type %d, ufr type %d.\n",
//buffer, d_myId, d_updateFromServer_type, d_updateFromRemote_type);
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

void vrpn_Shared_int32::unregister_handler (vrpnSharedIntCallback cb,
                                          void * userdata) {
  callbackEntry * e, ** snitch;

  snitch = &d_callbacks;
  e = *snitch;
  while (e && (e->handler != cb) && (e->userdata != userdata)) {
    snitch = &(e->next);
    e = *snitch;
  }
  if (!e) {
    fprintf(stderr, "vrpn_Shared_int32::unregister_handler:  "
                    "Handler not found.\n");
    return;
  }

  *snitch = e->next;
  delete e;
}

void vrpn_Shared_int32::encode (char ** buffer, vrpn_int32 * len) const {
  vrpn_buffer(buffer, len, d_value);
}
void vrpn_Shared_int32::decode (const char ** buffer, vrpn_int32 *) {
  vrpn_unbuffer(buffer, &d_value);
}

int vrpn_Shared_int32::yankCallbacks (void) {
  callbackEntry * e;

  for (e = d_callbacks;  e;  e = e->next) {
    if ((*e->handler)(e->userdata, d_value)) {
      return -1;
    }
  }

  return 0;
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
                                      (vrpn_int32 newValue) {

//fprintf(stderr, "vrpn_Shared_int32_Server::operator = (%d)\n", newValue);

  d_value = newValue;
  yankCallbacks();

  if (d_connection) {
    struct timeval now;
    char buffer [32];
    vrpn_int32 buflen = 32;
    char * bp = buffer;

    gettimeofday(&now, NULL);
    encode(&bp, &buflen);
    d_connection->pack_message(32 - buflen, now,
                               d_updateFromServer_type, d_myId,
                               buffer, vrpn_CONNECTION_RELIABLE);
//fprintf(stderr, "vrpn_Shared_int32_Server::operator =:  packed message\n");
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
  s->yankCallbacks();

#ifdef VRPN_SO_DEFER_UPDATES
  // MUST send an update notification back to the originator (if
  // we accept their update) so that they can reflect it.
  if (s->d_connection) {
    timeval now;
    char buffer [32];
    vrpn_int32 buflen = 32;
    char * bp = buffer;

    gettimeofday(&now, NULL);
    s->encode(&bp, &buflen);
    s->d_connection->pack_message(32 - buflen, now,
                                  s->d_updateFromServer_type, s->d_myId,
                                  buffer, vrpn_CONNECTION_RELIABLE);
  }
#endif

//fprintf(stderr, "vrpn_Shared_int32_Server::handle_updateFromRemote done\n");
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
                                            (vrpn_int32 newValue) {

#ifndef VRPN_SO_DEFER_UPDATES
  // If we're not deferring updates until the server verifies them,
  // update our local value immediately.
//fprintf(stderr, "vrpn_Shared_int32_Remote::operator = (%d)\n", newValue);
  d_value = newValue;
  yankCallbacks();
#endif

  if (d_connection) {
    struct timeval now;
    char buffer [32];
    vrpn_int32 buflen = 32;
    char * bp = buffer;

    gettimeofday(&now, NULL);
    encode(&bp, &buflen);
    d_connection->pack_message(32 - buflen, now,
                               d_updateFromRemote_type, d_myId,
                               buffer, vrpn_CONNECTION_RELIABLE);
//fprintf(stderr, "vrpn_Shared_int32_Remote::operator =:  packed message\n");
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
  s->yankCallbacks();
//fprintf(stderr, "vrpn_Shared_int32_Remote::handle_updateFromServer done\n");

  return 0;
}




