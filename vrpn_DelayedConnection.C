#include "vrpn_DelayedConnection.h"


// This really is the *same* thing as vrpn_File_Connection but
// that isn't written to be reused / recently refactored along the
// lines of vrpn_Connection.

vrpn_DelayedEndpoint::vrpn_DelayedEndpoint
                  (vrpn_TypeDispatcher * dispatcher,
                   vrpn_int32 * connectedEndpointCounter) :
    vrpn_Endpoint (dispatcher, connectedEndpointCounter),
    d_delayListHead (NULL),
    d_delayListEnd (NULL),
    d_delayAllTypes (vrpn_FALSE) {

  int i;

  d_delay.tv_sec = 0L;
  d_delay.tv_usec = 0L;

  for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
    d_typeIsDelayed[i] = vrpn_FALSE;
  }
}

// virtual
vrpn_DelayedEndpoint::~vrpn_DelayedEndpoint (void) {

}


/**
 * After we send things that need to go out, also see if there
 * are any delayed callbacks that need to be dispatched locally.
 */

// virtual
int vrpn_DelayedEndpoint::send_pending_reports (void) {
  vrpn_Endpoint::send_pending_reports();
  dispatchPending();

  return 0;
}

void vrpn_DelayedEndpoint::setDelay (timeval length) {
  d_delay = length;
}

void vrpn_DelayedEndpoint::delayAllTypes (vrpn_bool on) {
  d_delayAllTypes = on;
}

void vrpn_DelayedEndpoint::delayType (vrpn_int32 type, vrpn_bool on) {
  d_typeIsDelayed[type] = on;
}


/**
 * Take incoming messages & stick them in a queue with a valid date
 * sometime in the future (now + d_delay).
 */

// virtual
int vrpn_DelayedEndpoint::dispatch
      (vrpn_int32 type, vrpn_int32 sender, timeval time,
       vrpn_uint32 len, char * buffer) {

  vrpn_DELAYLIST * p;
  timeval now;

  // Are we really going to delay this message?

  if (type <= 0) {
    return vrpn_Endpoint::dispatch(type, sender, time, len, buffer);
  }

  if (!d_delayAllTypes && !d_typeIsDelayed[type]) {
    return vrpn_Endpoint::dispatch(type, sender, time, len, buffer);
  }

  // Yes!

  gettimeofday(&now, NULL);

  enqueue(vrpn_TimevalSum(now, d_delay), type, sender, time, len, buffer);

  return 0;
}


/**
 * Dispatch local callbacks for any delay messages that have now
 * become valid.
 */
// virtual
int vrpn_DelayedEndpoint::dispatchPending (void) {

  vrpn_DELAYLIST * p, * dp;
  timeval now;

  gettimeofday(&now, NULL);
  for (p = d_delayListHead; p && vrpn_TimevalGreater(now, p->valid);
       p = (vrpn_DELAYLIST *) p->next) {
    vrpn_Endpoint::dispatch(p->data.type, p->data.sender, p->data.msg_time,
                            p->data.payload_len, (char *) p->data.buffer);
  }

  if (p != d_delayListHead) {
    dp = d_delayListHead;
    d_delayListHead = p;
    if (!p) {
      d_delayListEnd = NULL;
    }

    deletePending(dp, p);
  }

  return 0;
}

void vrpn_DelayedEndpoint::deletePending (vrpn_DELAYLIST * start,
                                         vrpn_DELAYLIST * end) {
  vrpn_DELAYLIST * p, * ps;

  for (p = start; p && p != end; ) {
    ps = (vrpn_DELAYLIST *) p->next;
    if (ps) {
      ps->prev = p->prev;
    }
    if (p->prev) {
      p->prev->next = ps;
    }
    delete p;
    p = ps;
  }
}

void vrpn_DelayedEndpoint::enqueue (timeval validTime, vrpn_int32 type,
                                    vrpn_int32 sender, timeval time,
                                    vrpn_uint32 len, char * buffer) {
  vrpn_DELAYLIST * p;

  p = new vrpn_DELAYLIST;
  if (!p) {
    fprintf(stderr, "vrpn_DelayedEndpoint::enqueue:  Out of memory.\n");
    return;
  }

  p->data.type = type;
  p->data.sender = sender;
  p->data.msg_time = time;
  p->data.payload_len = len;
  p->data.buffer = buffer;
  p->valid = validTime;

  if (d_delayListEnd) {
    d_delayListEnd->next = p;
    p->prev = d_delayListEnd;
  } else {
    p->prev = NULL;
  }
  p->next = NULL;

  if (!d_delayListHead) {
    d_delayListHead = p;
  }

  d_delayListEnd = p;
}


vrpn_DelayedConnection::vrpn_DelayedConnection
              (timeval delay,
               unsigned short listen_port_no ,
               const char * local_logfile_name ,
               long local_log_mode ,
               const char * NIC_IPaddress,
               vrpn_Endpoint * (* epa) (vrpn_Connection *, vrpn_int32 *)) :
    vrpn_Synchronized_Connection (listen_port_no, local_logfile_name,
                                  local_log_mode, NIC_IPaddress,
                                  allocateEndpoint),
    d_delay (delay),
    d_delayAllTypes (vrpn_FALSE) {

  int i;
  for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
    d_typeIsDelayed[i] = vrpn_FALSE;
  }
}

vrpn_DelayedConnection::vrpn_DelayedConnection
              (timeval delay,
               const char * server_name,
               int port,
               const char * local_logfile_name,
               long local_log_mode,
               const char * remote_logfile_name,
               long remote_log_mode,
               double dFreq,
               int cOffsetWindow,
               const char * NIC_IPaddress,
               vrpn_Endpoint * (* epa) (vrpn_Connection *, vrpn_int32 *)) :
    vrpn_Synchronized_Connection (server_name, port, local_logfile_name,
                                  local_log_mode, remote_logfile_name,
                                  remote_log_mode, dFreq, cOffsetWindow,
                                  NIC_IPaddress,
                                  allocateEndpoint),
    d_delay (delay),
    d_delayAllTypes (vrpn_FALSE) {

  int i;
  for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
    d_typeIsDelayed[i] = vrpn_FALSE;
  }
}

// virtual
vrpn_DelayedConnection::~vrpn_DelayedConnection (void) {

}

void vrpn_DelayedConnection::setDelay (timeval length) {
  int i;
  d_delay = length;
  for (i = 0; i < d_numEndpoints; i++) {
    if (d_endpoints[i]) {
      ((vrpn_DelayedEndpoint *) d_endpoints[i])->setDelay(length);
    }
  }
}

void vrpn_DelayedConnection::delayAllTypes (vrpn_bool on) {
  int i;

  d_delayAllTypes = on;
  for (i = 0; i < d_numEndpoints; i++) {
    if (d_endpoints[i]) {
      ((vrpn_DelayedEndpoint *) d_endpoints[i])->delayAllTypes(on);
    }
  }
}

void vrpn_DelayedConnection::delayType (vrpn_int32 type, vrpn_bool on) {
  int i;

  d_typeIsDelayed[type] = on;
  for (i = 0; i < d_numEndpoints; i++) {
    if (d_endpoints[i]) {
      ((vrpn_DelayedEndpoint *) d_endpoints[i])->delayType(type, on);
    }
  }
}

// virtual
void vrpn_DelayedConnection::updateEndpoints (void) {
  int i, j;

  setDelay(d_delay);
  delayAllTypes(d_delayAllTypes);

  for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
    if (d_typeIsDelayed[i]) {
      for (j = 0; j < d_numEndpoints; j++) {
        if (d_endpoints[j]) {
          ((vrpn_DelayedEndpoint *) d_endpoints[j])->
                                     delayType(i, vrpn_TRUE);
        }
      }
    }
  }
}

// static
vrpn_Endpoint * vrpn_DelayedConnection::allocateEndpoint
                         (vrpn_Connection * me, vrpn_int32 * ep) {
  return new vrpn_DelayedEndpoint (me->d_dispatcher, ep);
}


vrpn_RelativeDelayEndpoint::vrpn_RelativeDelayEndpoint
                  (vrpn_TypeDispatcher * dispatcher,
                   vrpn_int32 * connectedEndpointCounter) :
    vrpn_DelayedEndpoint (dispatcher, connectedEndpointCounter) {

}

// virtual
vrpn_RelativeDelayEndpoint::~vrpn_RelativeDelayEndpoint (void) {

}


/**
 * Take incoming messages & stick them in a queue with a valid date
 * sometime in the future (timestamp - RTT + d_delay).
 */

// virtual
int vrpn_RelativeDelayEndpoint::dispatch
      (vrpn_int32 type, vrpn_int32 sender, timeval time,
       vrpn_uint32 len, char * buffer) {

  timeval validTime;
  timeval now;

  // Are we really going to delay this message?

  if (!d_delayAllTypes && !d_typeIsDelayed[type]) {
    return vrpn_Endpoint::dispatch(type, sender, time, len, buffer);
  }

  // Is it already past its valid time?

  validTime = vrpn_TimevalSum(time, d_delay);

  // tvClockOffset is HALF round-trip time, so subtract it twice.
  validTime = vrpn_TimevalDiff(validTime, tvClockOffset);
  validTime = vrpn_TimevalDiff(validTime, tvClockOffset);

  gettimeofday(&now, NULL);

  if (vrpn_TimevalGreater(now, validTime)) {
    return vrpn_Endpoint::dispatch(type, sender, time, len, buffer);
  }

  // No!

  enqueue(validTime, type, sender, time, len, buffer);

  return 0;
}

vrpn_RelativeDelayConnection::vrpn_RelativeDelayConnection
              (timeval delay,
               unsigned short listen_port_no,
               const char * local_logfile_name,
               long local_log_mode,
               const char * NIC_IPaddress) :
    vrpn_DelayedConnection (delay, listen_port_no, local_logfile_name,
                            local_log_mode, NIC_IPaddress) {

}

vrpn_RelativeDelayConnection::vrpn_RelativeDelayConnection
              (timeval delay,
               const char * server_name,
               int port,
               const char * local_logfile_name,
               long local_log_mode,
               const char * remote_logfile_name,
               long remote_log_mode,
               double dFreq,
               int cOffsetWindow,
               const char * NIC_IPaddress) :
    vrpn_DelayedConnection (delay, server_name, port, local_logfile_name,
                            local_log_mode, remote_logfile_name,
                            remote_log_mode, dFreq, cOffsetWindow,
                            NIC_IPaddress) {

}

// virtual
vrpn_RelativeDelayConnection::~vrpn_RelativeDelayConnection (void) {

}

// virtual
void vrpn_RelativeDelayConnection::updateEndpoints (void) {
  vrpn_DelayedConnection::updateEndpoints();

}

// static
vrpn_Endpoint * vrpn_RelativeDelayConnection::allocateEndpoint
                        (vrpn_Connection * me, vrpn_int32 * e) {
  return new vrpn_RelativeDelayEndpoint(me->d_dispatcher, e);
}


