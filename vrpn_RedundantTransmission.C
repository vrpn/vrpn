#include "vrpn_RedundantTransmission.h"

#include <string.h>  // for memcpy() on solaris

vrpn_RedundantTransmission::vrpn_RedundantTransmission (vrpn_Connection * c) :
    d_connection (c),
    d_messageList (NULL),
    d_numMessagesQueued (0),
    d_numTransmissions (0) {

  d_transmissionInterval.tv_sec = 0L;
  d_transmissionInterval.tv_usec = 0L;
}

vrpn_RedundantTransmission::~vrpn_RedundantTransmission (void) {

}

vrpn_uint32 vrpn_RedundantTransmission::defaultRetransmissions (void) const {
  return d_numTransmissions;
}

timeval vrpn_RedundantTransmission::defaultInterval (void) const {
  return d_transmissionInterval;
}


// virtual
void vrpn_RedundantTransmission::mainloop (void) {

  queuedMessage * qm;
  queuedMessage ** snitch;
  timeval now;

  if (!d_connection) {
    return;
  }

//fprintf(stderr, "mainloop:  %d messages queued.\n", d_numMessagesQueued);

  gettimeofday(&now, NULL);
  for (qm = d_messageList; qm; qm = qm->next) {
    if ((qm->remainingTransmissions > 0) &&
        vrpn_TimevalGreater(now, qm->nextValidTime)) {
      d_connection->pack_message(qm->p.payload_len, qm->p.msg_time,
                                 qm->p.type, qm->p.sender, qm->p.buffer,
                                 vrpn_CONNECTION_LOW_LATENCY);
      qm->nextValidTime = vrpn_TimevalSum(now, qm->transmissionInterval);
      qm->remainingTransmissions--;
//fprintf(stderr, "Sending message;  "
//"%d transmissions remaining at %d.%d int.\n",
//qm->remainingTransmissions, qm->transmissionInterval.tv_sec,
//qm->transmissionInterval.tv_usec);
    }
  }
  for (snitch = &d_messageList, qm = *snitch; qm;
       snitch = &qm->next, qm = *snitch) {
    while (qm && !qm->remainingTransmissions) {
      *snitch = qm->next;
      delete [] (void *) qm->p.buffer;
      delete qm;
      qm = *snitch;
      d_numMessagesQueued--;
    }
  }
}

void vrpn_RedundantTransmission::setDefaults
      (vrpn_uint32 numTransmissions, timeval transmissionInterval) {
//fprintf(stderr, "vrpn_RedundantTransmission::setDefaults:  to %d, %d.%d\n",
//numTransmissions, transmissionInterval.tv_sec, transmissionInterval.tv_usec);
  d_numTransmissions = numTransmissions;
  d_transmissionInterval = transmissionInterval;
}

// virtual
int vrpn_RedundantTransmission::pack_message
      (vrpn_uint32 len, timeval time, vrpn_uint32 type,
       vrpn_uint32 sender, const char * buffer,
       vrpn_int32 numTransmissions, timeval * transmissionInterval) {
  queuedMessage * qm;
  int ret;

  if (!d_connection) {
    fprintf(stderr, "vrpn_RedundantTransmission::pack_message:  "
            "Connection not defined!\n");
    return -1;
  }

  ret = d_connection->pack_message(len, time, type, sender, buffer,
                                   vrpn_CONNECTION_LOW_LATENCY);

  // TODO:  check ret

  // use defaults?
  if (numTransmissions < 0) {
    numTransmissions = d_numTransmissions;
  }
  if (!transmissionInterval) {
    transmissionInterval = &d_transmissionInterval;
  }

//fprintf(stderr, "In pack message with %d xmit at %d.%d\n",
//numTransmissions, transmissionInterval->tv_sec,
//transmissionInterval->tv_usec);
  if (!numTransmissions) {
    return ret;
  }

  qm = new queuedMessage;
  if (!qm) {
    fprintf(stderr, "vrpn_RedundantTransmission::pack_message:  "
            "Out of memory;  can't queue message for retransmission.\n");
    return ret;
  }

  qm->p.payload_len = len;
  qm->p.msg_time = time;
  qm->p.type = type;
  qm->p.sender = sender;
  qm->p.buffer = new char [len];
  if (!qm->p.buffer) {
    fprintf(stderr, "vrpn_RedundantTransmission::pack_message:  "
            "Out of memory;  can't queue message for retransmission.\n");
    return ret;
  }
  memcpy((char *) qm->p.buffer, buffer, len);

  qm->remainingTransmissions = numTransmissions;
  qm->transmissionInterval = *transmissionInterval;
  qm->nextValidTime = vrpn_TimevalSum(time, *transmissionInterval);
  qm->next = d_messageList;

  d_numMessagesQueued++;

//timeval now;
//gettimeofday(&now, NULL);
//fprintf(stderr, "  Queued message to go at %d.%d (now is %d.%d)\n",
//qm->nextValidTime.tv_sec, qm->nextValidTime.tv_usec,
//now.tv_sec, now.tv_usec);
  d_messageList = qm;

  return ret;
}





char * vrpn_RedundantController_Protocol::encode_set
              (int * len, vrpn_uint32 num, timeval interval) {
  char * buffer;
  char * bp;
  int buflen;

  buflen = sizeof(vrpn_uint32) + sizeof(timeval);
  *len = buflen;
  buffer = new char [buflen];
  if (!buffer) {
    fprintf(stderr, "vrpn_RedundantController_Protocol::encode_set:  "
                    "Out of memory.\n");
    return NULL;
  }

  bp = buffer;
  vrpn_buffer(&bp, &buflen, num);
  vrpn_buffer(&bp, &buflen, interval);

  return buffer;
}

void vrpn_RedundantController_Protocol::decode_set
            (const char ** buf, vrpn_uint32 * num, timeval * interval) {
  vrpn_unbuffer(buf, num);
  vrpn_unbuffer(buf, interval);
}

void vrpn_RedundantController_Protocol::register_types (vrpn_Connection * c) {
  d_set_type = c->register_message_type("vrpn Red Xmit Ctrl set");
}

vrpn_RedundantController::vrpn_RedundantController
       (vrpn_RedundantTransmission * r, vrpn_Connection * c) :
  vrpn_BaseClass ("vrpn Redundant Transmission Controller", c),
  d_object (r) {

  vrpn_BaseClass::init();

//fprintf(stderr, "Registering set handler with type %d.\n",
//d_protocol.d_set_type);

  register_autodeleted_handler(d_protocol.d_set_type, handle_set, this);
}

vrpn_RedundantController::~vrpn_RedundantController (void) {

}

void vrpn_RedundantController::mainloop (void) {
  server_mainloop();

  // do nothing
}

int vrpn_RedundantController::register_types (void) {
  d_protocol.register_types(d_connection);

  return 0;
}

// static
int vrpn_RedundantController::handle_set (void * ud, vrpn_HANDLERPARAM p) {
  vrpn_RedundantController * me = (vrpn_RedundantController *) ud;
  const char ** bp = &p.buffer;
  vrpn_uint32 num;
  timeval interval;

  me->d_protocol.decode_set(bp, &num, &interval);
//fprintf(stderr, "vrpn_RedundantController::handle_set:  %d, %d.%d\n",
//num, interval.tv_sec, interval.tv_usec);
  me->d_object->setDefaults(num, interval);

  return 0;
}

vrpn_RedundantRemote::vrpn_RedundantRemote (vrpn_Connection * c) :
  vrpn_BaseClass ("vrpn Redundant Transmission Controller", c) {

  vrpn_BaseClass::init();
}

vrpn_RedundantRemote::~vrpn_RedundantRemote (void) {

}

void vrpn_RedundantRemote::mainloop (void) {
  client_mainloop();

  // do nothing
}

void vrpn_RedundantRemote::set (int num, timeval interval) {
  char * buf;
  int len = 0;
  timeval now;

  buf = d_protocol.encode_set(&len, num, interval);
  if (!buf) {
    return;
  }
//fprintf(stderr, "vrpn_RedundantRemote::set:  %d, %d.%d\n",
//num, interval.tv_sec, interval.tv_usec);

  gettimeofday(&now, NULL);
  d_connection->pack_message(len, now, d_protocol.d_set_type, d_sender_id,
                             buf, vrpn_CONNECTION_RELIABLE);
}

int vrpn_RedundantRemote::register_types (void) {
  d_protocol.register_types(d_connection);

  return 0;
}

