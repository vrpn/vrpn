#include "vrpn_SharedObject.h"

#include <string.h>

#include <vrpn_Connection.h>

vrpn_Shared_int32::vrpn_Shared_int32 (const char * name,
                                      vrpn_int32 defaultValue,
                                      vrpn_int32 mode) :
  d_value (defaultValue),
  d_name (name ? new char [1 + strlen(name)] : NULL),
  d_mode (mode),
  d_connection (NULL),
  d_myId (-1),
  d_updateFromServer_type (-1),
  d_updateFromRemote_type (-1),
  d_callbacks (NULL),
  d_timedCallbacks (NULL) {

  if (name) {
    strcpy(d_name, name);
  }
  gettimeofday(&d_lastUpdate, NULL);
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

vrpn_Shared_int32::operator vrpn_int32 () const {
  return value();
}

// virtual
vrpn_Shared_int32 & vrpn_Shared_int32::operator =
                                            (vrpn_int32 newValue) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return set(newValue, now);
}

void vrpn_Shared_int32::bindConnection (vrpn_Connection * c) {
  char buffer [101];
  if (c == NULL) {
      // unbind the connection
      d_connection = NULL;
      return;
  }

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

void vrpn_Shared_int32::register_handler (vrpnTimedSharedIntCallback cb,
                                          void * userdata) {
  timedCallbackEntry * e = new timedCallbackEntry;
  if (!e) {
    fprintf(stderr, "vrpn_Shared_int32::register_handler:  "
                    "Out of memory.\n");
    return;
  }
  e->handler = cb;
  e->userdata = userdata;
  e->next = d_timedCallbacks;
  d_timedCallbacks = e;
}

void vrpn_Shared_int32::unregister_handler (vrpnTimedSharedIntCallback cb,
                                            void * userdata) {
  timedCallbackEntry * e, ** snitch;

  snitch = &d_timedCallbacks;
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
void vrpn_Shared_int32::decode (const char ** buffer, vrpn_int32 *,
                                vrpn_int32 * newValue) const {
  vrpn_unbuffer(buffer, newValue);
}

void vrpn_Shared_int32::sendUpdate (vrpn_int32 msgType) {
  char buffer [32];
  vrpn_int32 buflen = 32;
  char * bp = buffer;

  if (d_connection) {
    encode(&bp, &buflen);
    d_connection->pack_message(32 - buflen, d_lastUpdate,
                               msgType, d_myId,
                               buffer, vrpn_CONNECTION_RELIABLE);
//fprintf(stderr, "vrpn_Shared_int32::sendUpdate:  packed message\n");
  }

}


int vrpn_Shared_int32::yankCallbacks (void) {
  callbackEntry * e;
  timedCallbackEntry * te;

  for (e = d_callbacks;  e;  e = e->next) {
    if ((*e->handler)(e->userdata, d_value)) {
      return -1;
    }
  }
  for (te = d_timedCallbacks;  te;  te = te->next) {
    if ((*te->handler)(te->userdata, d_value, d_lastUpdate)) {
      return -1;
    }
  }

  return 0;
}







vrpn_Shared_int32_Server::vrpn_Shared_int32_Server (const char * name,
                                                    vrpn_int32 defaultValue,
                                                    vrpn_int32 defaultMode) :
    vrpn_Shared_int32 (name, defaultValue, defaultMode) {

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
vrpn_Shared_int32 & vrpn_Shared_int32_Server::set
                                      (vrpn_int32 newValue,
                                       timeval when,
                                       vrpn_bool shouldSendUpdate) {

  // Is this "change" to the same value?
  if ((d_mode & VRPN_SO_IGNORE_IDEMPOTENT) &&
      (newValue == d_value)) {
    return *this;
  }

  // Is this "new" change older than the previous change?
  if ((d_mode & VRPN_SO_IGNORE_OLD) &&
      !vrpn_TimevalGreater(when, d_lastUpdate)) {
    return *this;
  }

//fprintf(stderr, "vrpn_Shared_int32_Server::set (%d)\n", newValue);

  d_value = newValue;
  d_lastUpdate = when;
  yankCallbacks();

  if (shouldSendUpdate) {
    sendUpdate(d_updateFromServer_type);
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
  vrpn_int32 newValue;

  s->decode(&p.buffer, &p.payload_len, &newValue);
  s->set(newValue, p.msg_time, s->d_mode & VRPN_SO_DEFER_UPDATES);
    // If VRPN_SO_DEFER_UPDATES is set, we
    // MUST send an update notification back to the originator (if
    // we accept their update) so that they can reflect it.
  //gettimeofday(&s->d_lastUpdate);
  //s->d_lastUpdate = p.msg_time;
  //s->yankCallbacks();

//fprintf(stderr, "vrpn_Shared_int32_Server::handle_updateFromRemote done\n");
  return 0;
}


vrpn_Shared_int32_Remote::vrpn_Shared_int32_Remote (const char * name,
                                                    vrpn_int32 defaultValue,
                                                    vrpn_int32 defaultMode) :
    vrpn_Shared_int32 (name, defaultValue, defaultMode) {

}

// virtual
vrpn_Shared_int32_Remote::~vrpn_Shared_int32_Remote (void) {

  // unregister handlers
  if (d_connection) {
    d_connection->unregister_handler
           (d_updateFromServer_type, handle_updateFromServer, this, d_myId);
  }

}

// virtual
vrpn_Shared_int32 & vrpn_Shared_int32_Remote::set
                                            (vrpn_int32 newValue,
                                             timeval when,
                                             vrpn_bool shouldSendUpdate) {
  if ((d_mode & VRPN_SO_IGNORE_IDEMPOTENT) &&
      (newValue == d_value)) {
    return *this;
  }

  // Is this "new" change older than the previous change?
  if ((d_mode & VRPN_SO_IGNORE_OLD) &&
      !vrpn_TimevalGreater(when, d_lastUpdate)) {
    return *this;
  }

  if (!(d_mode & VRPN_SO_DEFER_UPDATES)) {
    // If we're not deferring updates until the server verifies them,
    // update our local value immediately.
//fprintf(stderr, "vrpn_Shared_int32_Remote::set (%d)\n", newValue);
    d_value = newValue;
    d_lastUpdate = when;
    yankCallbacks();
  }

  if (shouldSendUpdate) {
    sendUpdate(d_updateFromRemote_type);
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
  vrpn_int32 newValue;

  s->decode(&p.buffer, &p.payload_len, &newValue);
  s->set(newValue, p.msg_time, vrpn_FALSE);
  //gettimeofday(&s->d_lastUpdate, NULL);
  //s->d_lastUpdate = p.msg_time;
  //s->yankCallbacks();
//fprintf(stderr, "vrpn_Shared_int32_Remote::handle_updateFromServer done\n");

  return 0;
}





vrpn_Shared_float64::vrpn_Shared_float64 (const char * name,
                                      vrpn_float64 defaultValue,
                                      vrpn_int32 mode) :
  d_value (defaultValue),
  d_name (name ? new char [1 + strlen(name)] : NULL),
  d_mode (mode),
  d_connection (NULL),
  d_myId (-1),
  d_updateFromServer_type (-1),
  d_updateFromRemote_type (-1),
  d_callbacks (NULL),
  d_timedCallbacks (NULL) {

  if (name) {
    strcpy(d_name, name);
  }
  gettimeofday(&d_lastUpdate, NULL);
}

// virtual
vrpn_Shared_float64::~vrpn_Shared_float64 (void) {
  if (d_name) {
    delete [] d_name;
  }
}

vrpn_float64 vrpn_Shared_float64::value (void) const {
  return d_value;
}

vrpn_Shared_float64::operator vrpn_float64 () const {
  return value();
}

// virtual
vrpn_Shared_float64 & vrpn_Shared_float64::operator =
                                            (vrpn_float64 newValue) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return set(newValue, now);
}

void vrpn_Shared_float64::bindConnection (vrpn_Connection * c) {
  char buffer [101];
  if (c == NULL) {
      // unbind the connection
      d_connection = NULL;
      return;
  }

  if (c && d_connection) {
    fprintf(stderr, "vrpn_Shared_float64::bindConnection:  "
                    "Tried to rebind a connection to %s.\n", d_name);
    return;
  }

  d_connection = c;
  sprintf(buffer, "vrpn Shared float64 %s", d_name);
  d_myId = c->register_sender(buffer);
  d_updateFromServer_type = c->register_message_type
          ("vrpn Shared float64 update from server");
  d_updateFromRemote_type = c->register_message_type
          ("vrpn Shared float64 update from remote");

//printf ("My name is %s;  myId %d, ufs type %d, ufr type %d.\n",
//buffer, d_myId, d_updateFromServer_type, d_updateFromRemote_type);
}

void vrpn_Shared_float64::register_handler (vrpnSharedFloatCallback cb,
                                            void * userdata) {
  callbackEntry * e = new callbackEntry;
  if (!e) {
    fprintf(stderr, "vrpn_Shared_float64::register_handler:  "
                    "Out of memory.\n");
    return;
  }
  e->handler = cb;
  e->userdata = userdata;
  e->next = d_callbacks;
  d_callbacks = e;
}

void vrpn_Shared_float64::unregister_handler (vrpnSharedFloatCallback cb,
                                              void * userdata) {
  callbackEntry * e, ** snitch;

  snitch = &d_callbacks;
  e = *snitch;
  while (e && (e->handler != cb) && (e->userdata != userdata)) {
    snitch = &(e->next);
    e = *snitch;
  }
  if (!e) {
    fprintf(stderr, "vrpn_Shared_float64::unregister_handler:  "
                    "Handler not found.\n");
    return;
  }

  *snitch = e->next;
  delete e;
}

void vrpn_Shared_float64::register_handler (vrpnTimedSharedFloatCallback cb,
                                            void * userdata) {
  timedCallbackEntry * e = new timedCallbackEntry;
  if (!e) {
    fprintf(stderr, "vrpn_Shared_float64::register_handler:  "
                    "Out of memory.\n");
    return;
  }
  e->handler = cb;
  e->userdata = userdata;
  e->next = d_timedCallbacks;
  d_timedCallbacks = e;
}

void vrpn_Shared_float64::unregister_handler (vrpnTimedSharedFloatCallback cb,
                                              void * userdata) {
  timedCallbackEntry * e, ** snitch;

  snitch = &d_timedCallbacks;
  e = *snitch;
  while (e && (e->handler != cb) && (e->userdata != userdata)) {
    snitch = &(e->next);
    e = *snitch;
  }
  if (!e) {
    fprintf(stderr, "vrpn_Shared_float64::unregister_handler:  "
                    "Handler not found.\n");
    return;
  }

  *snitch = e->next;
  delete e;
}

void vrpn_Shared_float64::encode (char ** buffer, vrpn_int32 * len) const {
  vrpn_buffer(buffer, len, d_value);
}
void vrpn_Shared_float64::decode (const char ** buffer, vrpn_int32 *,
                                vrpn_float64 * newValue) const {
  vrpn_unbuffer(buffer, newValue);
}

void vrpn_Shared_float64::sendUpdate (vrpn_int32 msgType) {
  char buffer [32];
  vrpn_int32 buflen = 32;
  char * bp = buffer;

  if (d_connection) {
    encode(&bp, &buflen);
    d_connection->pack_message(32 - buflen, d_lastUpdate,
                               msgType, d_myId,
                               buffer, vrpn_CONNECTION_RELIABLE);
//fprintf(stderr, "vrpn_Shared_float64::sendUpdate:  packed message\n");
  }

}


int vrpn_Shared_float64::yankCallbacks (void) {
  callbackEntry * e;
  timedCallbackEntry * te;

  for (e = d_callbacks;  e;  e = e->next) {
    if ((*e->handler)(e->userdata, d_value)) {
      return -1;
    }
  }
  for (te = d_timedCallbacks;  te;  te = te->next) {
    if ((*te->handler)(te->userdata, d_value, d_lastUpdate)) {
      return -1;
    }
  }

  return 0;
}







vrpn_Shared_float64_Server::vrpn_Shared_float64_Server
                                     (const char * name,
                                      vrpn_float64 defaultValue,
                                      vrpn_int32 defaultMode) :
    vrpn_Shared_float64 (name, defaultValue, defaultMode) {

}

// virtual
vrpn_Shared_float64_Server::~vrpn_Shared_float64_Server (void) {

  // unregister handlers
  if (d_connection) {
    d_connection->unregister_handler
           (d_updateFromRemote_type, handle_updateFromRemote, this, d_myId);
  }

}



// virtual
vrpn_Shared_float64 & vrpn_Shared_float64_Server::set
                                      (vrpn_float64 newValue,
                                       timeval when,
                                       vrpn_bool shouldSendUpdate) {

  // Is this "change" to the same value?
  if ((d_mode & VRPN_SO_IGNORE_IDEMPOTENT) &&
      (newValue == d_value)) {
    return *this;
  }

  // Is this "new" change older than the previous change?
  if ((d_mode & VRPN_SO_IGNORE_OLD) &&
      !vrpn_TimevalGreater(when, d_lastUpdate)) {
    return *this;
  }

//fprintf(stderr, "vrpn_Shared_float64_Server::set (%d)\n", newValue);

  d_value = newValue;
  d_lastUpdate = when;
  yankCallbacks();

  if (shouldSendUpdate) {
    sendUpdate(d_updateFromServer_type);
  }

  return *this;
}

// virtual
void vrpn_Shared_float64_Server::bindConnection (vrpn_Connection * c) {

  vrpn_Shared_float64::bindConnection(c);

  if (d_connection) {
    d_connection->register_handler(d_updateFromRemote_type,
                                   handle_updateFromRemote,
                                   this, d_myId);
  }
}

// static
int vrpn_Shared_float64_Server::handle_updateFromRemote
             (void * ud, vrpn_HANDLERPARAM p) {
  vrpn_Shared_float64_Server * s = (vrpn_Shared_float64_Server *) ud;
  vrpn_float64 newValue;

  s->decode(&p.buffer, &p.payload_len, &newValue);
  s->set(newValue, p.msg_time, s->d_mode & VRPN_SO_DEFER_UPDATES);
    // If VRPN_SO_DEFER_UPDATES is set, we
    // MUST send an update notification back to the originator (if
    // we accept their update) so that they can reflect it.
  //gettimeofday(&s->d_lastUpdate);
  //s->d_lastUpdate = p.msg_time;
  //s->yankCallbacks();

//fprintf(stderr, "vrpn_Shared_float64_Server::handle_updateFromRemote done\n");
  return 0;
}


vrpn_Shared_float64_Remote::vrpn_Shared_float64_Remote
                                  (const char * name,
                                   vrpn_float64 defaultValue,
                                   vrpn_int32 defaultMode) :
    vrpn_Shared_float64 (name, defaultValue, defaultMode) {

}

// virtual
vrpn_Shared_float64_Remote::~vrpn_Shared_float64_Remote (void) {

  // unregister handlers
  if (d_connection) {
    d_connection->unregister_handler
           (d_updateFromServer_type, handle_updateFromServer, this, d_myId);
  }

}

// virtual
vrpn_Shared_float64 & vrpn_Shared_float64_Remote::set
                                            (vrpn_float64 newValue,
                                             timeval when,
                                             vrpn_bool shouldSendUpdate) {
  if ((d_mode & VRPN_SO_IGNORE_IDEMPOTENT) &&
      (newValue == d_value)) {
    return *this;
  }

  // Is this "new" change older than the previous change?
  if ((d_mode & VRPN_SO_IGNORE_OLD) &&
      !vrpn_TimevalGreater(when, d_lastUpdate)) {
    return *this;
  }

  if (!(d_mode & VRPN_SO_DEFER_UPDATES)) {
    // If we're not deferring updates until the server verifies them,
    // update our local value immediately.
//fprintf(stderr, "vrpn_Shared_float64_Remote::set (%d)\n", newValue);
    d_value = newValue;
    d_lastUpdate = when;
    yankCallbacks();
  }

  if (shouldSendUpdate) {
    sendUpdate(d_updateFromRemote_type);
  }

  return *this;
}

// virtual
void vrpn_Shared_float64_Remote::bindConnection (vrpn_Connection * c) {

  vrpn_Shared_float64::bindConnection(c);

  if (d_connection) {
    d_connection->register_handler(d_updateFromServer_type,
                                   handle_updateFromServer,
                                   this, d_myId);
  }
}

// static
int vrpn_Shared_float64_Remote::handle_updateFromServer
             (void * ud, vrpn_HANDLERPARAM p) {
  vrpn_Shared_float64_Remote * s = (vrpn_Shared_float64_Remote *) ud;
  vrpn_float64 newValue;

  s->decode(&p.buffer, &p.payload_len, &newValue);
  s->set(newValue, p.msg_time, vrpn_FALSE);
  //gettimeofday(&s->d_lastUpdate, NULL);
  //s->d_lastUpdate = p.msg_time;
  //s->yankCallbacks();
//fprintf(stderr, "vrpn_Shared_float64_Remote::handle_updateFromServer done\n");

  return 0;
}

