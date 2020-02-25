#include <stddef.h> // for NULL

#include "vrpn_Forwarder.h"

vrpn_ConnectionForwarder::vrpn_ConnectionForwarder(vrpn_Connection *source,
                                                   vrpn_Connection *destination)
    : d_source(source)
    , d_destination(destination)
    , d_list(NULL)
{

    if (d_source) {
        d_source->addReference();
    }
    if (d_destination) {
        d_destination->addReference();
    }
}

vrpn_ConnectionForwarder::~vrpn_ConnectionForwarder(void)
{
    vrpn_CONNECTIONFORWARDERRECORD *dlp;

    while (d_list) {
        dlp = d_list->next;

        if (d_source)
            d_source->unregister_handler(d_list->sourceId, handle_message, this,
                                         d_list->sourceServiceId);
        try {
          delete d_list;
        } catch (...) {
          fprintf(stderr, "vrpn_ConnectionForwarder::~vrpn_ConnectionForwarder(): delete failed\n");
          return;
        }
        d_list = dlp;
    }

    if (d_source) {
        d_source->removeReference();
    }
    if (d_destination) {
        d_destination->removeReference();
    }
}

int vrpn_ConnectionForwarder::forward(const char *sourceName,
                                      const char *sourceServiceId,
                                      const char *destinationName,
                                      const char *destinationServiceId,
                                      vrpn_uint32 classOfService)
{
    vrpn_CONNECTIONFORWARDERRECORD *newList = NULL;
    try { newList =
        new vrpn_CONNECTIONFORWARDERRECORD(
            d_source, d_destination, sourceName, sourceServiceId,
            destinationName, destinationServiceId, classOfService);
    } catch (...) { return -1; }

    newList->next = d_list;
    d_list = newList;

    if (d_source)
        d_source->register_handler(newList->sourceId, handle_message, this,
                                   newList->sourceServiceId);

    return 0;
}

int vrpn_ConnectionForwarder::unforward(const char *sourceName,
                                        const char *sourceServiceId,
                                        const char *destinationName,
                                        const char *destinationServiceId,
                                        vrpn_uint32 classOfService)
{

    vrpn_CONNECTIONFORWARDERRECORD **snitch;
    vrpn_CONNECTIONFORWARDERRECORD *victim;

    vrpn_int32 st, ss, dt, ds;

    st = d_source->register_message_type(sourceName);
    ss = d_source->register_sender(sourceServiceId);
    dt = d_destination->register_message_type(destinationName);
    ds = d_source->register_sender(destinationServiceId);

    for (snitch = &d_list, victim = *snitch; victim;
         snitch = &(victim->next), victim = *snitch) {

        if ((victim->sourceId == st) && (victim->sourceServiceId == ss) &&
            (victim->destinationId == dt) &&
            (victim->destinationServiceId == ds) &&
            (victim->classOfService == classOfService)) {
            (*snitch)->next = victim->next;
            try {
              delete victim;
            } catch (...) {
              fprintf(stderr, "vrpn_ConnectionForwarder::unforward(): delete failed\n");
              return -1;
            }
            victim = *snitch;
        }
    }

    return 0;
}

// static
int vrpn_ConnectionForwarder::handle_message(void *userdata,
                                             vrpn_HANDLERPARAM p)
{

    vrpn_ConnectionForwarder *me = (vrpn_ConnectionForwarder *)userdata;

    vrpn_int32 id = p.type;
    vrpn_int32 serviceId = p.sender;
    vrpn_uint32 serviceClass;

    int retval;

    // Convert from source's representation for type & sender ID to
    // that of destination;  look up service class to use (defaulted to
    // vrpn_CONNECTION_RELIABLE).

    retval = me->map(&id, &serviceId, &serviceClass);
    if (retval) return -1;

    if (me->d_destination) {
        me->d_destination->pack_message(p.payload_len, p.msg_time, id,
                                        serviceId, p.buffer, serviceClass);

        // HACK:  should we have this here?
        me->d_destination->mainloop();
    }

    return 0;
}

vrpn_int32 vrpn_ConnectionForwarder::map(vrpn_int32 *id, vrpn_int32 *serviceId,
                                         vrpn_uint32 *classOfService)
{

    vrpn_CONNECTIONFORWARDERRECORD *dlp;

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

vrpn_ConnectionForwarder::vrpn_CONNECTIONFORWARDERRECORD::
    vrpn_CONNECTIONFORWARDERRECORD(vrpn_Connection *source,
                                   vrpn_Connection *dest, const char *iSourceId,
                                   const char *iSourceServiceId,
                                   const char *iDestId,
                                   const char *iDestServiceId, vrpn_uint32 cos)
    : sourceId(source->register_message_type(iSourceId))
    , sourceServiceId(source->register_sender(iSourceServiceId))
    , destinationId(dest->register_message_type(iDestId))
    , destinationServiceId(dest->register_sender(iDestServiceId))
    , classOfService(cos)
    , next(NULL)
{
}

vrpn_StreamForwarder::vrpn_StreamForwarder(vrpn_Connection *source,
                                           const char *sourceServiceName,
                                           vrpn_Connection *destination,
                                           const char *destinationServiceName)
    : d_source(source)
    , d_sourceService(source->register_sender(sourceServiceName))
    , d_destination(destination)
    , d_destinationService(destination->register_sender(destinationServiceName))
    , d_list(NULL)
{

    if (d_source) {
        d_source->addReference();
    }
    if (d_destination) {
        d_destination->addReference();
    }
}

vrpn_StreamForwarder::~vrpn_StreamForwarder(void)
{

    vrpn_STREAMFORWARDERRECORD *dlp;

    while (d_list) {
        dlp = d_list->next;

        if (d_source)
            d_source->unregister_handler(d_list->sourceId, handle_message, this,
                                         d_sourceService);

        try {
          delete d_list;
        } catch (...) {
          fprintf(stderr, "vrpn_StreamForwarder::~vrpn_StreamForwarder(): delete failed\n");
          return;
        }
        d_list = dlp;
    }

    if (d_source) {
        d_source->removeReference();
    }
    if (d_destination) {
        d_destination->removeReference();
    }
}

int vrpn_StreamForwarder::forward(const char *sourceName,
                                  const char *destinationName,
                                  vrpn_uint32 classOfService)
{
    vrpn_STREAMFORWARDERRECORD *newList = NULL;
    try { newList = new vrpn_STREAMFORWARDERRECORD(
        d_source, d_destination, sourceName, destinationName, classOfService);
    } catch (...) { return -1; }

    newList->next = d_list;
    d_list = newList;

    if (d_source)
        d_source->register_handler(newList->sourceId, handle_message, this,
                                   d_sourceService);

    return 0;
}

int vrpn_StreamForwarder::unforward(const char *sourceName,
                                    const char *destinationName,
                                    vrpn_uint32 classOfService)
{

    vrpn_STREAMFORWARDERRECORD **snitch;
    vrpn_STREAMFORWARDERRECORD *victim;

    vrpn_int32 st, dt;

    st = d_source->register_message_type(sourceName);
    dt = d_destination->register_message_type(destinationName);

    for (snitch = &d_list, victim = *snitch; victim;
         snitch = &(victim->next), victim = *snitch) {

        if ((victim->sourceId == st) && (victim->destinationId == dt) &&
            (victim->classOfService == classOfService)) {
            (*snitch)->next = victim->next;
            try {
              delete victim;
            } catch (...) {
              fprintf(stderr, "vrpn_StreamForwarder::unforward(): delete failed\n");
              return -1;
            }
            victim = *snitch;
        }
    }

    return 0;
}

// static
int vrpn_StreamForwarder::handle_message(void *userdata, vrpn_HANDLERPARAM p)
{

    vrpn_StreamForwarder *me = (vrpn_StreamForwarder *)userdata;

    vrpn_int32 id = p.type;
    vrpn_uint32 serviceClass;

    int retval;

    // Convert from source's representation for type & sender ID to
    // that of destination;  look up service class to use (defaulted to
    // vrpn_CONNECTION_RELIABLE).

    retval = me->map(&id, &serviceClass);
    if (retval) return -1;

    if (me->d_destination) {
        me->d_destination->pack_message(p.payload_len, p.msg_time, id,
                                        me->d_destinationService, p.buffer,
                                        serviceClass);

        // HACK:  should we have this here?
        me->d_destination->mainloop();
    }

    return 0;
}

vrpn_int32 vrpn_StreamForwarder::map(vrpn_int32 *id,
                                     vrpn_uint32 *classOfService)
{

    vrpn_STREAMFORWARDERRECORD *dlp;

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

vrpn_StreamForwarder::vrpn_STREAMFORWARDERRECORD::vrpn_STREAMFORWARDERRECORD(
    vrpn_Connection *source, vrpn_Connection *dest, const char *iSourceId,
    const char *iDestId, vrpn_uint32 cos)
    : sourceId(source->register_message_type(iSourceId))
    , destinationId(dest->register_message_type(iDestId))
    , classOfService(cos)
    , next(NULL)
{
}
