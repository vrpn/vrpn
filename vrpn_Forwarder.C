#include "vrpn_Forwarder.h"


vrpn_ConnectionForwarder::vrpn_ConnectionForwarder 
               (vrpn_Connection * source,
                vrpn_Connection * destination) :
  d_source (source),
  d_destination (destination),
  d_list (NULL) {

}

vrpn_ConnectionForwarder::~vrpn_ConnectionForwarder (void) {

  vrpn_CONNECTIONFORWARDERRECORD * dlp;

  while (d_list) {
    dlp = d_list->next;

    if (d_source)
      d_source->unregister_handler(d_list->sourceId, handle_message,
                                   this, d_list->sourceServiceId);

    delete d_list;
    d_list = dlp;
  }

}

int vrpn_ConnectionForwarder::forward
                (const char * sourceName,
                 const char * sourceServiceId,
                 const char * destinationName,
                 const char * destinationServiceId,
                 unsigned long classOfService) {
  
  vrpn_CONNECTIONFORWARDERRECORD * newList =
    new vrpn_CONNECTIONFORWARDERRECORD (d_source, d_destination,
             sourceName, sourceServiceId, destinationName,
             destinationServiceId, classOfService);

  if (!newList)
    return -1;

  newList->next = d_list;
  d_list = newList;

  d_source->register_handler(newList->sourceId, handle_message,
                             this, newList->sourceServiceId);

  return 0;
}

int vrpn_ConnectionForwarder::unforward
                  (const char * sourceName,
                   const char * sourceServiceId,
                   const char * destinationName,
                   const char * destinationServiceId,
                   unsigned long classOfService) {

  // TODO

  return -1;
}

// static
int vrpn_ConnectionForwarder::handle_message (void * userdata,
                                              vrpn_HANDLERPARAM p) {

  vrpn_ConnectionForwarder * me = (vrpn_ConnectionForwarder *) userdata;

  long id = p.type;
  long serviceId = p.sender;
  unsigned long serviceClass;

  int retval;

  // Convert from source's representation for type & sender ID to
  // that of destination;  look up service class to use (defaulted to
  // vrpn_CONNECTION_RELIABLE).

  retval = me->map(&id, &serviceId, &serviceClass);
  if (retval)
    return -1;

  me->d_destination->pack_message(p.payload_len, p.msg_time,
                                  id, serviceId, p.buffer,
                                  vrpn_CONNECTION_RELIABLE);
    // Not knowing what service discipline we arrived on, we
    // forward everything reliably.
    // TODO:  add optional to forward() to allow this to be
    // stored and looked up by map()

  // HACK:  should we have this here?
  me->d_destination->mainloop();

  return 0;
}

int vrpn_ConnectionForwarder::map (long * id, long * serviceId,
                                   unsigned long * classOfService) {

  vrpn_CONNECTIONFORWARDERRECORD * dlp;

  for (dlp = d_list; dlp; dlp = dlp->next)
    if ((*id == dlp->sourceId) && (*serviceId == dlp->sourceServiceId)) {
      *id = dlp->destinationId;
      *serviceId = dlp->destinationServiceId;
      *classOfService = dlp->classOfService;
      return 0;
    }

  return -1;
}

// Build a mapping from the source Connection's types to the destination
// Connection's.  Store the class of service to be used on forwarding.

vrpn_ConnectionForwarder::vrpn_CONNECTIONFORWARDERRECORD::vrpn_CONNECTIONFORWARDERRECORD 
  (vrpn_Connection * source, vrpn_Connection * dest,
   const char * iSourceId, const char * iSourceServiceId,
   const char * iDestId, const char * iDestServiceId,
   unsigned long cos) :
      sourceId (source->register_message_type(iSourceId)),
      sourceServiceId (source->register_sender(iSourceServiceId)),
      destinationId (dest->register_message_type(iDestId)),
      destinationServiceId (dest->register_sender(iDestServiceId)),
      classOfService (cos),
      next (NULL) {

}






vrpn_StreamForwarder::vrpn_StreamForwarder 
               (vrpn_Connection * source,
                const char * sourceServiceName,
                vrpn_Connection * destination,
                const char * destinationServiceName) :
  d_source (source),
  d_sourceService (source->register_sender(sourceServiceName)),
  d_destination (destination),
  d_destinationService (destination->register_sender(destinationServiceName)),
  d_list (NULL) {

}

vrpn_StreamForwarder::~vrpn_StreamForwarder (void) {

  vrpn_STREAMFORWARDERRECORD * dlp;

  while (d_list) {
    dlp = d_list->next;

    if (d_source)
      d_source->unregister_handler(d_list->sourceId, handle_message,
                                   this, d_sourceService);

    delete d_list;
    d_list = dlp;
  }

}

int vrpn_StreamForwarder::forward
                (const char * sourceName,
                 const char * destinationName,
                 unsigned long classOfService) {
  
  vrpn_STREAMFORWARDERRECORD * newList =
    new vrpn_STREAMFORWARDERRECORD (d_source, d_destination,
             sourceName, destinationName, classOfService);

  if (!newList)
    return -1;

  newList->next = d_list;
  d_list = newList;

  if (d_source)
    d_source->register_handler(newList->sourceId, handle_message,
                               this, d_sourceService);

  return 0;
}

int vrpn_StreamForwarder::unforward
                  (const char * sourceName,
                   const char * destinationName,
                   unsigned long classOfService) {

  // TODO

  return -1;
}

// static
int vrpn_StreamForwarder::handle_message (void * userdata,
                                          vrpn_HANDLERPARAM p) {

  vrpn_StreamForwarder * me = (vrpn_StreamForwarder *) userdata;

  long id = p.type;
  unsigned long serviceClass;

  int retval;

  // Convert from source's representation for type & sender ID to
  // that of destination;  look up service class to use (defaulted to
  // vrpn_CONNECTION_RELIABLE).

  retval = me->map(&id, &serviceClass);
  if (retval) return -1;

  if (me->d_destination) {
    me->d_destination->pack_message(p.payload_len, p.msg_time,
                                    id, me->d_destinationService, p.buffer,
                                    serviceClass);
    // Not knowing what service discipline we arrived on, we
    // forward everything reliably.
    // TODO:  add optional to forward() to allow this to be
    // stored and looked up by map()

    // HACK:  should we have this here?
    me->d_destination->mainloop();
  }

  return 0;
}

int vrpn_StreamForwarder::map (long * id,
                               unsigned long * classOfService) {

  vrpn_STREAMFORWARDERRECORD * dlp;

  for (dlp = d_list; dlp; dlp = dlp->next)
    if (*id == dlp->sourceId) {
      *id = dlp->destinationId;
      *classOfService = dlp->classOfService;
      return 0;
    }

  return -1;
}

// Build a mapping from the source Connection's types to the destination
// Connection's.  Store the class of service to be used on forwarding.

vrpn_StreamForwarder::vrpn_STREAMFORWARDERRECORD::vrpn_STREAMFORWARDERRECORD 
  (vrpn_Connection * source, vrpn_Connection * dest,
   const char * iSourceId,
   const char * iDestId,
   unsigned long cos) :
      sourceId (source->register_message_type(iSourceId)),
      destinationId (dest->register_message_type(iDestId)),
      classOfService (cos),
      next (NULL) {

}





