#include "vrpn_DelayedConnection.h"


// This really is the *same* thing as vrpn_File_Connection but
// that isn't written to be reused / recently refactored along the
// lines of vrpn_Connection.

vrpn_DelayedEndpoint::vrpn_DelayedEndpoint
                  (timeval delay,
                   vrpn_TypeDispatcher * dispatcher,
                   vrpn_int32 * connectedEndpointCounter) :
    vrpn_Endpoint (dispatcher, connectedEndpointCounter),
    d_delay (delay),
    d_delayListHead (NULL) {

  // TODO
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

  gettimeofday(&now, NULL);
  p = new vrpn_DELAYLIST;
  if (!p) {
    fprintf(stderr, "vrpn_DelayedEndpoint::dispatch:  Out of memory.\n");
    return -1;
  }

  p->data.type = type;
  p->data.sender = sender;
  p->data.msg_time = time;
  p->data.payload_len = len;
  p->data.buffer = buffer;
  p->valid = vrpn_TimevalSum(now, d_delay);
  p->next = d_delayListHead;
  p->prev = NULL;

  d_delayListHead = p;

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



vrpn_DelayedConnection::vrpn_DelayedConnection
              (timeval delay,
               unsigned short listen_port_no ,
               const char * local_logfile_name ,
               long local_log_mode ,
               const char * NIC_IPaddress) :
    vrpn_Synchronized_Connection (listen_port_no, local_logfile_name,
                                  local_log_mode, NIC_IPaddress),
    d_delay (delay) {

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
               const char * NIC_IPaddress ) :
    vrpn_Synchronized_Connection (server_name, port, local_logfile_name,
                                  local_log_mode, remote_logfile_name,
                                  remote_log_mode, dFreq, cOffsetWindow,
                                  NIC_IPaddress),
    d_delay (delay) {

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


// virtual
vrpn_Endpoint * vrpn_DelayedConnection::allocateEndpoint (vrpn_int32 * e) {
  return new vrpn_DelayedEndpoint (d_delay, d_dispatcher, e);
}


