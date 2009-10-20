#include "vrpn_TypeDispatcher.h"

#include <stdio.h>
#include <string.h>  // for strcmp(), strncpy()


vrpn_TypeDispatcher::vrpn_TypeDispatcher (void) :
    d_numTypes (0),
    d_numSenders (0),
    d_genericCallbacks (NULL)
{
  clear();
}

vrpn_TypeDispatcher::~vrpn_TypeDispatcher (void) {
  vrpnMsgCallbackEntry * pVMCB, * pVMCB_Del;
  int i;

  for (i = 0; i < d_numTypes; i++) {
    if (d_types[i].name) {
      delete [] d_types[i].name;
    }
    pVMCB = d_types[i].who_cares;
    while (pVMCB) {
      pVMCB_Del = pVMCB;
      pVMCB = pVMCB_Del->next;
      delete pVMCB_Del;
    }
  }

  pVMCB = d_genericCallbacks;

  while (pVMCB) {
    pVMCB_Del = pVMCB;
    pVMCB = pVMCB_Del->next;
    delete pVMCB_Del;
  }

}

int vrpn_TypeDispatcher::numTypes (void) const {
  return d_numTypes;
}

const char * vrpn_TypeDispatcher::typeName (int i) const {
  if ((i < 0) || (i >= d_numTypes)) {
    return NULL;
  }
  return d_types[i].name;
}

vrpn_int32 vrpn_TypeDispatcher::getTypeID (const char * name) {
  vrpn_int32 i;

  for (i = 0; i < d_numTypes; i++) {
    if (!strcmp(name, d_types[i].name)) {
      return i;
    }
  }

  return -1;
}

int vrpn_TypeDispatcher::numSenders (void) const {
  return d_numSenders;
}

const char * vrpn_TypeDispatcher::senderName (int i) const {
  if ((i < 0) || (i >= d_numSenders)) {
    return NULL;
  }
  return d_senders[i];
}

vrpn_int32 vrpn_TypeDispatcher::getSenderID (const char * name) {
  vrpn_int32 i;

  for (i = 0; i < d_numSenders; i++) {
    if (!strcmp(name, d_senders[i])) {
      return i;
    }
  }

  return -1;
}

vrpn_int32 vrpn_TypeDispatcher::addType (const char * name) {

  // See if there are too many on the list.  If so, return -1.
  if (d_numTypes >= vrpn_CONNECTION_MAX_TYPES) {
    fprintf(stderr, "vrpn_TypeDispatcher::addType:  "
                    "Too many! (%d)\n", d_numTypes);
    return -1;
  }

  if (!d_types[d_numTypes].name) {
    d_types[d_numTypes].name = new cName;
       if (!d_types[d_numTypes].name) {
         fprintf(stderr, "vrpn_TypeDispatcher::addType:  "
                         "Can't allocate memory for new record.\n");
       return -1;
    }
  }

  // Add this one into the list and return its index
  strncpy(d_types[d_numTypes].name, name, sizeof(cName) - 1);
  d_types[d_numTypes].who_cares = NULL;
  d_types[d_numTypes].cCares = 0;

  d_numTypes++;

  return d_numTypes - 1;
}

vrpn_int32 vrpn_TypeDispatcher::addSender (const char * name) {

  // See if there are too many on the list.  If so, return -1.
  if (d_numSenders >= vrpn_CONNECTION_MAX_SENDERS) {
    fprintf(stderr, "vrpn_TypeDispatcher::addSender:  "
                    "Too many! (%d).\n", d_numSenders);
    return -1;
  }

  if (!d_senders[d_numSenders]) {

    //  fprintf(stderr, "Allocating a new name entry\n");

    d_senders[d_numSenders] = new cName;
    if (!d_senders[d_numSenders]) {
      fprintf(stderr, "vrpn_TypeDispatcher::addSender:  "
                      "Can't allocate memory for new record\n");
      return -1;
    }
  }

  // Add this one into the list
  strncpy(d_senders[d_numSenders], name, sizeof(cName) - 1);
  d_numSenders++;

  // One more in place -- return its index
  return d_numSenders - 1;
}

vrpn_int32 vrpn_TypeDispatcher::registerType (const char * name) {
  vrpn_int32 retval;

  // See if the name is already in the list.  If so, return it.
  retval = getTypeID(name);
  if (retval != -1) {
    return retval;
  }

  return addType(name);
}

vrpn_int32 vrpn_TypeDispatcher::registerSender (const char * name) {
  vrpn_int32 retval;

  // See if the name is already in the list.  If so, return it.
  retval = getSenderID(name);
  if (retval != -1) {
    return retval;
  }

  return addSender(name);
}



int vrpn_TypeDispatcher::addHandler (vrpn_int32 type,
                                     vrpn_MESSAGEHANDLER handler,
                                     void * userdata, vrpn_int32 sender) {
  vrpnMsgCallbackEntry * new_entry;
  vrpnMsgCallbackEntry ** ptr;

  // Ensure that the type is a valid one (one that has been defined)
  //   OR that it is "any"
  if (((type < 0) || (type >= d_numTypes)) &&
      (type != vrpn_ANY_TYPE)) {
          fprintf(stderr,
                  "vrpn_TypeDispatcher::addHandler:  No such type\n");
          return -1;
  }

  // Ensure that the sender is a valid one (or "any")
  if ((sender != vrpn_ANY_SENDER) &&
      ((sender < 0) || (sender >= d_numSenders))) {
          fprintf(stderr,
                  "vrpn_TypeDispatcher::addHandler:  No such sender\n");
          return -1;
  }

  // Ensure that the handler is non-NULL
  if (handler == NULL) {
          fprintf(stderr,
                  "vrpn_TypeDispatcher::addHandler:  NULL handler\n");
          return -1;
  }

  // Allocate and initialize the new entry
  new_entry = new vrpnMsgCallbackEntry ();
  if (new_entry == NULL) {
    fprintf(stderr, "vrpn_TypeDispatcher::addHandler:  Out of memory\n");
    return -1;
  }
  new_entry->handler = handler;
  new_entry->userdata = userdata;
  new_entry->sender = sender;

  // Add this handler to the chain at the beginning (don't check to see
  // if it is already there, since duplication is okay).
#ifdef  VERBOSE
  printf("Adding user handler for type %ld, sender %ld\n",type,sender);
#endif

  // TCH June 2000 - rewrote to insert at end of list instead of beginning,
  // to make sure multiple callbacks on the same type are triggered
  // in the order registered.

  if (type == vrpn_ANY_TYPE) {
    ptr = &d_genericCallbacks;
  } else {
    ptr = &d_types[type].who_cares;
  }

  while (*ptr) {
    ptr = &((*ptr)->next);
  }
  *ptr = new_entry;
  new_entry->next = NULL;

  return 0;
}

int vrpn_TypeDispatcher::removeHandler (vrpn_int32 type,
                                        vrpn_MESSAGEHANDLER handler,
                                        void * userdata, vrpn_int32 sender) {
  // The pointer at *snitch points to victim
  vrpnMsgCallbackEntry * victim, ** snitch;

  // Ensure that the type is a valid one (one that has been defined)
  //   OR that it is "any"
  if (((type < 0) || (type >= d_numTypes)) &&
      (type != vrpn_ANY_TYPE)) {
          fprintf(stderr, "vrpn_TypeDispatcher::removeHandler: No such type\n");
          return -1;
  }

  // Find a handler with this registry in the list (any one will do,
  // since all duplicates are the same).
  if (type == vrpn_ANY_TYPE) {
    snitch = &d_genericCallbacks;
  } else {
    snitch = &(d_types[type].who_cares);
  }
  victim = *snitch;
  while ( (victim != NULL) &&
          ( (victim->handler != handler) ||
            (victim->userdata != userdata) ||
            (victim->sender != sender) )){
      snitch = &( (*snitch)->next );
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
  delete victim;

  return 0;
}

void vrpn_TypeDispatcher::setSystemHandler (vrpn_int32 type,
                                           vrpn_MESSAGEHANDLER handler) {
  d_systemMessages[-type] = handler;
}



int vrpn_TypeDispatcher::doCallbacksFor
                       (vrpn_int32 type, vrpn_int32 sender,
                        timeval time, vrpn_uint32 len,
                        const char * buffer) {
  vrpnMsgCallbackEntry * who;
  vrpn_HANDLERPARAM p;

  // We don't dispatch system messages (kluge?).
  if (type < 0) {
    return 0;
  }

  if (type >= d_numTypes) {
    return -1;
  }

  // Fill in the parameter to be passed to the routines
  p.type = type;
  p.sender = sender;
  p.msg_time = time;
  p.payload_len = len;
  p.buffer = buffer;

  // Do generic callbacks (vrpn_ANY_TYPE)
  who = d_genericCallbacks;

  while (who) {   // For each callback entry
    // Verify that the sender is ANY or matches
    if ((who->sender == vrpn_ANY_SENDER) ||
        (who->sender == sender) ) {
      if (who->handler(who->userdata, p)) {
        fprintf(stderr, "vrpn_TypeDispatcher::doCallbacksFor:  "
                        "Nonzero user generic handler return.\n");
              return -1;
      }
    }

    // Next callback in list
    who = who->next;
  }

  // Find the head for the list of callbacks to call
  who = d_types[type].who_cares;

  while (who) {   // For each callback entry
    // Verify that the sender is ANY or matches
    if ((who->sender == vrpn_ANY_SENDER) ||
        (who->sender == sender) ) {
      if (who->handler(who->userdata, p)) {
        fprintf(stderr, "vrpn_TypeDispatcher::doCallbacksFor:  "
                        "Nonzero user handler return.\n");
              return -1;
      }
    }

    // Next callback in list
    who = who->next;
  }

  return 0;
}

int vrpn_TypeDispatcher::doSystemCallbacksFor
                       (vrpn_int32 type, vrpn_int32 sender,
                        timeval time, vrpn_uint32 len,
                        const char * buffer,
                        void * userdata) {
  vrpn_HANDLERPARAM p;

  if (type >= 0) {
    return 0;
  }
  if (-type >= vrpn_CONNECTION_MAX_TYPES) {
    fprintf(stderr, "vrpn_TypeDispatcher::doSystemCallbacksFor:  "
                    "Illegal type %d.\n", type);
    return -1;
  }

  if (!d_systemMessages[-type]) {
    return 0;
  }

  // Fill in the parameter to be passed to the routines
  p.type = type;
  p.sender = sender;
  p.msg_time = time;
  p.payload_len = len;
  p.buffer = buffer;

  return doSystemCallbacksFor(p, userdata);
}

int vrpn_TypeDispatcher::doSystemCallbacksFor
                       (vrpn_HANDLERPARAM p,
                        void * userdata) {
  int retval;

  if (p.type >= 0) {
    return 0;
  }
  if (-p.type >= vrpn_CONNECTION_MAX_TYPES) {
    fprintf(stderr, "vrpn_TypeDispatcher::doSystemCallbacksFor:  "
                    "Illegal type %d.\n", p.type);
    return -1;
  }

  if (!d_systemMessages[-p.type]) {
    return 0;
  }

  retval = d_systemMessages[-p.type](userdata, p);
  if (retval) {
    fprintf(stderr, "vrpn_TypeDispatcher::doSystemCallbacksFor:  "
                    "Nonzero system handler return.\n");
    return -1;
  }
  return 0;
}

void vrpn_TypeDispatcher::clear (void) {
  int i;

  for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
    d_types[i].who_cares = NULL;
    d_types[i].cCares = 0;
    d_types[i].name = NULL;

    d_systemMessages[i] = NULL;
  }

  for (i = 0; i < vrpn_CONNECTION_MAX_SENDERS; i++) {
    d_senders[i] = NULL;
  }
}


