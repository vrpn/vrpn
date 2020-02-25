#include <stdio.h>  // for fprintf, stderr, fclose, etc
#include <string.h> // for NULL, memcpy

#include "vrpn_RedundantTransmission.h"

struct timeval;

vrpn_RedundantTransmission::vrpn_RedundantTransmission(vrpn_Connection *c)
    : d_connection(c)
    , d_messageList(NULL)
    , d_numMessagesQueued(0)
    , d_numTransmissions(0)
    , d_isEnabled(VRPN_FALSE)
{

    d_transmissionInterval.tv_sec = 0L;
    d_transmissionInterval.tv_usec = 0L;

    if (d_connection) {
        d_connection->addReference();
    }
}

vrpn_RedundantTransmission::~vrpn_RedundantTransmission(void)
{

    if (d_connection) {
        d_connection->removeReference();
    }
}

vrpn_uint32 vrpn_RedundantTransmission::defaultRetransmissions(void) const
{
    return d_numTransmissions;
}

timeval vrpn_RedundantTransmission::defaultInterval(void) const
{
    return d_transmissionInterval;
}

vrpn_bool vrpn_RedundantTransmission::isEnabled(void) const
{
    return d_isEnabled;
}

// virtual
void vrpn_RedundantTransmission::mainloop(void)
{

    queuedMessage *qm;
    queuedMessage **snitch;
    timeval now;

    if (!d_connection) {
        return;
    }

    // fprintf(stderr, "mainloop:  %d messages queued.\n", d_numMessagesQueued);

    vrpn_gettimeofday(&now, NULL);
    for (qm = d_messageList; qm; qm = qm->next) {
        if ((qm->remainingTransmissions > 0) &&
            vrpn_TimevalGreater(now, qm->nextValidTime)) {
            d_connection->pack_message(qm->p.payload_len, qm->p.msg_time,
                                       qm->p.type, qm->p.sender, qm->p.buffer,
                                       vrpn_CONNECTION_LOW_LATENCY);
            qm->nextValidTime = vrpn_TimevalSum(now, qm->transmissionInterval);
            qm->remainingTransmissions--;
            // fprintf(stderr, "Sending message;  "
            //"%d transmissions remaining at %d.%d int.\n",
            // qm->remainingTransmissions, qm->transmissionInterval.tv_sec,
            // qm->transmissionInterval.tv_usec);
        }
    }

    snitch = &d_messageList;
    qm = *snitch;

    while (qm) {
        if (!qm->remainingTransmissions) {
            *snitch = qm->next;
            delete[]qm -> p.buffer;
            delete qm;
            qm = *snitch;
            d_numMessagesQueued--;
        }
        else {
            snitch = &qm->next;
            qm = *snitch;
        }
    }

    if ((d_numMessagesQueued && !d_messageList) ||
        (!d_numMessagesQueued && d_messageList)) {
        fprintf(stderr, "vrpn_RedundantTransmission::mainloop():  "
                        "serious internal error.\n");
        d_numMessagesQueued = 0;
        d_messageList = NULL;
    }
}

void vrpn_RedundantTransmission::enable(vrpn_bool on)
{
    d_isEnabled = on;

    // fprintf(stderr, "vrpn_RedundantTransmission::enable(%s)\n",
    // on ? "on" : "off");
}

void vrpn_RedundantTransmission::setDefaults(vrpn_uint32 numTransmissions,
                                             timeval transmissionInterval)
{

    // fprintf(stderr, "vrpn_RedundantTransmission::setDefaults:  "
    //"to %d, %d.%d\n", numTransmissions, transmissionInterval.tv_sec,
    // transmissionInterval.tv_usec);

    d_numTransmissions = numTransmissions;
    d_transmissionInterval = transmissionInterval;
}

// virtual
int vrpn_RedundantTransmission::pack_message(
    vrpn_uint32 len, timeval time, vrpn_uint32 type, vrpn_uint32 sender,
    const char *buffer, vrpn_uint32 class_of_service,
    vrpn_int32 numTransmissions, timeval *transmissionInterval)
{
    queuedMessage *qm;
    int ret;
    int i;

    if (!d_connection) {
        fprintf(stderr, "vrpn_RedundantTransmission::pack_message:  "
                        "Connection not defined!\n");
        return -1;
    }

    if (!d_isEnabled) {
        return d_connection->pack_message(len, time, type, sender, buffer,
                                          class_of_service);
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

    // fprintf(stderr, "In pack message with %d xmit at %d.%d\n",
    // numTransmissions, transmissionInterval->tv_sec,
    // transmissionInterval->tv_usec);

    if (!numTransmissions) {
        return ret;
    }

    // Special case - if transmissionInterval is 0, we send them all right
    // away, but force VRPN to use separate network packets.
    if (!transmissionInterval->tv_sec && !transmissionInterval->tv_usec) {
        for (i = 0; i < numTransmissions; i++) {
            d_connection->send_pending_reports();
            ret = d_connection->pack_message(len, time, type, sender, buffer,
                                             vrpn_CONNECTION_LOW_LATENCY);
            // TODO:  check ret
        }
        d_connection->send_pending_reports();
        return 0;
    }

    try { qm = new queuedMessage; }
    catch (...) {
        fprintf(stderr,
                "vrpn_RedundantTransmission::pack_message:  "
                "Out of memory;  can't queue message for retransmission.\n");
        return -1;
    }

    qm->p.payload_len = len;
    qm->p.msg_time = time;
    qm->p.type = type;
    qm->p.sender = sender;
    try { qm->p.buffer = new char[len]; }
    catch (...) {
        fprintf(stderr,
                "vrpn_RedundantTransmission::pack_message:  "
                "Out of memory;  can't queue message for retransmission.\n");
        return -1;
    }
    memcpy(const_cast<char *>(qm->p.buffer), buffer, len);

    qm->remainingTransmissions = numTransmissions;
    qm->transmissionInterval = *transmissionInterval;
    qm->nextValidTime = vrpn_TimevalSum(time, *transmissionInterval);
    qm->next = d_messageList;

    d_numMessagesQueued++;

    // timeval now;
    // vrpn_gettimeofday(&now, NULL);
    // fprintf(stderr, "  Queued message to go at %d.%d (now is %d.%d)\n",
    // qm->nextValidTime.tv_sec, qm->nextValidTime.tv_usec,
    // now.tv_sec, now.tv_usec);

    d_messageList = qm;

    return ret;
}

char *vrpn_RedundantController_Protocol::encode_set(int *len, vrpn_uint32 num,
                                                    timeval interval)
{
    char *buffer;
    char *bp;
    int buflen;

    buflen = sizeof(vrpn_uint32) + sizeof(timeval);
    *len = buflen;
    try { buffer = new char[buflen]; }
    catch (...) {
        fprintf(stderr, "vrpn_RedundantController_Protocol::encode_set:  "
                        "Out of memory.\n");
        return NULL;
    }

    bp = buffer;
    vrpn_buffer(&bp, &buflen, num);
    vrpn_buffer(&bp, &buflen, interval);

    return buffer;
}

void vrpn_RedundantController_Protocol::decode_set(const char **buf,
                                                   vrpn_uint32 *num,
                                                   timeval *interval)
{
    vrpn_unbuffer(buf, num);
    vrpn_unbuffer(buf, interval);
}

char *vrpn_RedundantController_Protocol::encode_enable(int *len, vrpn_bool on)
{
    char *buffer;
    char *bp;
    int buflen;

    buflen = sizeof(vrpn_bool);
    *len = buflen;
    try { buffer = new char[buflen]; }
    catch (...) {
        fprintf(stderr, "vrpn_RedundantController_Protocol::encode_enable:  "
                        "Out of memory.\n");
        return NULL;
    }

    bp = buffer;
    vrpn_buffer(&bp, &buflen, on);

    return buffer;
}

void vrpn_RedundantController_Protocol::decode_enable(const char **buf,
                                                      vrpn_bool *on)
{
    vrpn_unbuffer(buf, on);
}

void vrpn_RedundantController_Protocol::register_types(vrpn_Connection *c)
{
    d_set_type = c->register_message_type("vrpn_Red_Xmit_Ctrl set");
    d_enable_type = c->register_message_type("vrpn_Red_Xmit_Ctrl enable");
}

vrpn_RedundantController::vrpn_RedundantController(
    vrpn_RedundantTransmission *r, vrpn_Connection *c)
    : vrpn_BaseClass("vrpn Redundant Transmission Controller", c)
    , d_object(r)
{

    vrpn_BaseClass::init();

    // fprintf(stderr, "Registering set handler with type %d.\n",
    // d_protocol.d_set_type);

    register_autodeleted_handler(d_protocol.d_set_type, handle_set, this);
    register_autodeleted_handler(d_protocol.d_enable_type, handle_enable, this);
}

vrpn_RedundantController::~vrpn_RedundantController(void) {}

void vrpn_RedundantController::mainloop(void)
{
    server_mainloop();

    // do nothing
}

int vrpn_RedundantController::register_types(void)
{
    d_protocol.register_types(d_connection);

    return 0;
}

// static
int vrpn_RedundantController::handle_set(void *ud, vrpn_HANDLERPARAM p)
{
    vrpn_RedundantController *me = (vrpn_RedundantController *)ud;
    const char **bp = &p.buffer;
    vrpn_uint32 num;
    timeval interval;

    me->d_protocol.decode_set(bp, &num, &interval);
    // fprintf(stderr, "vrpn_RedundantController::handle_set:  %d, %d.%d\n",
    // num, interval.tv_sec, interval.tv_usec);
    me->d_object->setDefaults(num, interval);

    return 0;
}

int vrpn_RedundantController::handle_enable(void *ud, vrpn_HANDLERPARAM p)
{
    vrpn_RedundantController *me = (vrpn_RedundantController *)ud;
    const char **bp = &p.buffer;
    vrpn_bool on;

    me->d_protocol.decode_enable(bp, &on);
    me->d_object->enable(on);

    return 0;
}

vrpn_RedundantRemote::vrpn_RedundantRemote(vrpn_Connection *c)
    : vrpn_BaseClass("vrpn Redundant Transmission Controller", c)
{
    vrpn_BaseClass::init();
    d_protocol.d_enable_type = 0;
    d_protocol.d_set_type = 0;
}

vrpn_RedundantRemote::~vrpn_RedundantRemote(void) {}

void vrpn_RedundantRemote::mainloop(void)
{
    client_mainloop();

    // do nothing
}

void vrpn_RedundantRemote::set(int num, timeval interval)
{
    char *buf;
    int len = 0;
    timeval now;

    buf = d_protocol.encode_set(&len, num, interval);
    if (!buf) {
        return;
    }
    // fprintf(stderr, "vrpn_RedundantRemote::set:  %d, %d.%d\n",
    // num, interval.tv_sec, interval.tv_usec);

    vrpn_gettimeofday(&now, NULL);
    d_connection->pack_message(len, now, d_protocol.d_set_type, d_sender_id,
                               buf, vrpn_CONNECTION_RELIABLE);
}

void vrpn_RedundantRemote::enable(vrpn_bool on)
{
    char *buf;
    int len = 0;
    timeval now;

    buf = d_protocol.encode_enable(&len, on);
    if (!buf) {
        return;
    }

    vrpn_gettimeofday(&now, NULL);
    d_connection->pack_message(len, now, d_protocol.d_enable_type, d_sender_id,
                               buf, vrpn_CONNECTION_RELIABLE);
}

int vrpn_RedundantRemote::register_types(void)
{
    d_protocol.register_types(d_connection);

    return 0;
}

vrpn_RedundantReceiver::RRRecord::RRRecord(void)
    : nextTimestampToReplace(0)
    , cb(NULL)
    , handlerIsRegistered(vrpn_FALSE)
{
    timeval zero;
    int i;

    zero.tv_sec = 0;
    zero.tv_usec = 0;

    for (i = 0; i < VRPN_RR_LENGTH; i++) {
        timestampSeen[i] = zero;
        numSeen[i] = 0;
    }
}

vrpn_RedundantReceiver::vrpn_RedundantReceiver(vrpn_Connection *c)
    : d_connection(c)
    , d_memory(NULL)
    , d_lastMemory(NULL)
    , d_record(VRPN_FALSE)
{

    if (d_connection) {
        d_connection->addReference();
    }
}

vrpn_RedundantReceiver::~vrpn_RedundantReceiver(void)
{
    vrpnMsgCallbackEntry *pVMCB, *pVMCB_Del;
    int i;

    for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
        pVMCB = d_records[i].cb;
        while (pVMCB) {
            pVMCB_Del = pVMCB;
            pVMCB = pVMCB_Del->next;
            try {
              delete pVMCB_Del;
            } catch (...) {
              fprintf(stderr, "vrpn_RedundantReceiver::~vrpn_RedundantReceiver(): delete failed\n");
              return;
            }
        }
    }

    pVMCB = d_generic.cb;

    while (pVMCB) {
        pVMCB_Del = pVMCB;
        pVMCB = pVMCB_Del->next;
        try {
          delete pVMCB_Del;
        } catch (...) {
          fprintf(stderr, "vrpn_RedundantReceiver::~vrpn_RedundantReceiver(): delete failed\n");
          return;
        }
    }

    if (d_connection) {
        d_connection->removeReference();
    }
}

// virtual
int vrpn_RedundantReceiver::register_handler(vrpn_int32 type,
                                             vrpn_MESSAGEHANDLER handler,
                                             void *userdata, vrpn_int32 sender)
{
    vrpnMsgCallbackEntry *ce = NULL;
    try { ce = new vrpnMsgCallbackEntry; }
    catch (...) {
      fprintf(stderr, "vrpn_RedundantReceiver::register_handler:  "
        "Out of memory.\n");
      return -1;
    }
    ce->handler = handler;
    ce->userdata = userdata;
    ce->sender = sender;

    if (type == vrpn_ANY_TYPE) {
        ce->next = d_generic.cb;
        d_generic.cb = ce;
        return 0;
    } else if (type < 0) {
        fprintf(stderr, "vrpn_RedundantReceiver::register_handler:  "
                        "Negative type passed in.\n");
        try {
          delete ce;
        } catch (...) {
          fprintf(stderr, "vrpn_RedundantReceiver::register_handler(): delete failed\n");
          return -1;
        }
        return -1;
    } else {
        ce->next = d_records[type].cb;
        d_records[type].cb = ce;
    }

    if (!d_records[type].handlerIsRegistered) {
        d_connection->register_handler(type, handle_possiblyRedundantMessage,
                                       this, sender);
        d_records[type].handlerIsRegistered = VRPN_TRUE;
    }

    return 0;
}

// virtual
int vrpn_RedundantReceiver::unregister_handler(vrpn_int32 type,
                                               vrpn_MESSAGEHANDLER handler,
                                               void *userdata,
                                               vrpn_int32 sender)
{
    // The pointer at *snitch points to victim
    vrpnMsgCallbackEntry *victim, **snitch;

    // Find a handler with this registry in the list (any one will do,
    // since all duplicates are the same).
    if (type == vrpn_ANY_TYPE) {
        snitch = &d_generic.cb;
    }
    else {
        snitch = &(d_records[type].cb);
    }
    victim = *snitch;
    while ((victim != NULL) &&
           ((victim->handler != handler) || (victim->userdata != userdata) ||
            (victim->sender != sender))) {
        snitch = &((*snitch)->next);
        victim = victim->next;
    }

    // Make sure we found one
    if (victim == NULL) {
        fprintf(stderr,
                "vrpn_TypeDispatcher::removeHandler: No such handler\n");
        return -1;
    }

    // Remove the entry from the list
    *snitch = victim->next;
    try {
      delete victim;
    } catch (...) {
      fprintf(stderr, "vrpn_RedundantReceiver::unregister_handler(): delete failed\n");
      return -1;
    }

    return 0;
}

void vrpn_RedundantReceiver::record(vrpn_bool on) { d_record = on; }

void vrpn_RedundantReceiver::writeMemory(const char *filename)
{
    FILE *fp;
    RRMemory *mp;

    if (!d_memory) {
        fprintf(stderr, "vrpn_RedundantReceiver::writeMemory:  "
                        "Memory is empty.\n");
        return;
    }

    // BUG:  This doesn't write out the records of the things that
    // are still in the active list!  As long as our network tests
    // are recording a few seconds beyond the end of the interesting
    // period & trimming them back that's OK, but if this code is ever
    // used for precise timings there will be slight undersampling.

    fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "vrpn_RedundantReceiver::writeMemory:  "
                        "Couldn't open %s for writing.\n",
                filename);
        return;
    }

    for (mp = d_memory; mp; mp = mp->next) {
        fprintf(fp, "%ld.%ld %d\n", mp->timestamp.tv_sec,
                static_cast<long>(mp->timestamp.tv_usec), mp->numSeen);
    }

    fclose(fp);
}

void vrpn_RedundantReceiver::clearMemory(void)
{
    RRMemory *mp;

    for (mp = d_memory; d_memory; mp = d_memory) {
        d_memory = mp->next;
        try {
          delete mp;
        } catch (...) {
          fprintf(stderr, "vrpn_RedundantReceiver::clearMemory(): delete failed\n");
          return;
        }
    }

    d_lastMemory = NULL;
}

// static
int vrpn_RedundantReceiver::handle_possiblyRedundantMessage(void *ud,
                                                            vrpn_HANDLERPARAM p)
{
    vrpn_RedundantReceiver *me = (vrpn_RedundantReceiver *)ud;
    vrpnMsgCallbackEntry *who;
    RRMemory *memory;
    int ntr;
    int i;

    for (i = 0; i < VRPN_RR_LENGTH; i++) {
        if ((p.msg_time.tv_sec ==
             me->d_records[p.type].timestampSeen[i].tv_sec) &&
            (p.msg_time.tv_usec ==
             me->d_records[p.type].timestampSeen[i].tv_usec)) {

            // We've seen this one (recently);  it's a redundant send whose
            // predecessor didn't get lost, so this one is unnecessary FEC.
            // Discard it.

            // fprintf(stderr, "Threw out redundant copy of type %d.\n",
            // p.type);

            me->d_records[p.type].numSeen[i]++;

            return 0;
        }
    }

    ntr = me->d_records[p.type].nextTimestampToReplace;

    // Store a record of how many copies of this message we received so
    // we can compute loss later (knowing what level of redundancy was being
    // used).

    if (me->d_record) {
        if (me->d_records[p.type].numSeen[ntr]) {
            try { memory = new RRMemory; }
            catch (...) {
                fprintf(stderr,
                        "vrpn_RedundantReceiver::"
                        "handle_possiblyRedundantMessage:  Out of memory.\n");
                return -1;
            }

            memory->timestamp = me->d_records[p.type].timestampSeen[ntr];
            memory->numSeen = me->d_records[p.type].numSeen[ntr];
            memory->next = NULL;

            if (me->d_lastMemory) {
                me->d_lastMemory->next = memory;
            }
            else {
                me->d_memory = memory;
            }
            me->d_lastMemory = memory;
        }
    }

    me->d_records[p.type].timestampSeen[ntr] = p.msg_time;
    me->d_records[p.type].numSeen[ntr] = 1;
    me->d_records[p.type].nextTimestampToReplace = (ntr + 1) % VRPN_RR_LENGTH;

    for (who = me->d_generic.cb; who; who = who->next) {
        if ((who->sender == vrpn_ANY_SENDER) || (who->sender == p.sender)) {
            if (who->handler(who->userdata, p)) {
                fprintf(stderr, "vrpn_RedundantReceiver::"
                                "handle_possiblyRedundantMessage:  "
                                "Nonzero user generic handler return.\n");
                return -1;
            }
        }
    }

    for (who = me->d_records[p.type].cb; who; who = who->next) {
        // Verify that the sender is ANY or matches
        if ((who->sender == vrpn_ANY_SENDER) || (who->sender == p.sender)) {
            if (who->handler(who->userdata, p)) {
                fprintf(stderr, "vrpn_RedundantReceiver::"
                                "handle_possiblyRedundantMessage:  "
                                "Nonzero user handler return.\n");
                return -1;
            }
        }
    }

    return 0;
}
