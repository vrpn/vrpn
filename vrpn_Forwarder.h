#ifndef VRPN_FORWARDER_H
#define VRPN_FORWARDER_H

#include "vrpn_Connection.h"  // for vrpn_HANDLERPARAM


// vrpn_Forwarder
// Tom Hudson, August 1998
//
// Class to take messages from one VRPN connection and send them out
// on another.

// Design decisions:
//   Scale of forwarding:
//     Could write a forwarder per stream (serviceName per instantiation)
//     or per connection (serviceName per forward() call).  Latter is
//     more flexible, but takes up more memory if few distinct streams need
//     to be forwarded, has a clunkier syntax, ...
//   Flexibility of naming:
//     We allow users to take in a message of one name and send it out
//     with another name;  this is useful and dangerous.

// Faults:
//   There is currently no way to specify vrpn_SENDER_ANY as a source.
// If we do, it isn't clear what sender to specify to the destination.

class vrpn_ConnectionForwarder {

  public:

    // Set up to forward messages from <source> to <destination>
    vrpn_ConnectionForwarder (vrpn_Connection * source,
                              vrpn_Connection * destination);

    ~vrpn_ConnectionForwarder (void);



    // Begins forwarding of a message type.
    // Forwards messages of type <sourceName> and sender <sourceServiceName>,
    // sending them out as type <destinationName> from sender
    // <destinationServiceName>.
    // Return nonzero on failure.
    int forward (const char * sourceName,
                 const char * sourceServiceName,
                 const char * destinationName,
                 const char * destinationServiceName,
                 unsigned long classOfService = vrpn_CONNECTION_RELIABLE);

    // Stops forwarding of a message type.
    // Return nonzero on failure.
    int unforward (const char * sourceName,
                   const char * sourceServiceName,
                   const char * destinationName,
                   const char * destinationServiceName,
                   unsigned long classOfService = vrpn_CONNECTION_RELIABLE);

  private:

    static int handle_message (void *, vrpn_HANDLERPARAM);

    // Translates (id, serviceId) from source to destination
    // and looks up intended class of service.
    // Returns nonzero if lookup fails.
    int map (long * id, long * serviceId, unsigned long * serviceClass);

    vrpn_Connection * d_source;
    vrpn_Connection * d_destination;

    struct vrpn_CONNECTIONFORWARDERRECORD {

      vrpn_CONNECTIONFORWARDERRECORD (vrpn_Connection *, vrpn_Connection *,
           const char *, const char *, const char *, const char *,
           unsigned long);

      long sourceId;              // source's type id
      long sourceServiceId;       // source's sender id
      long destinationId;         // destination's type id
      long destinationServiceId;  // destination's sender id
      unsigned long classOfService;  // class of service to send

      vrpn_CONNECTIONFORWARDERRECORD * next;
    };

    vrpn_CONNECTIONFORWARDERRECORD * d_list;

};

class vrpn_StreamForwarder {

  public:

    // Set up to forward messages from sender <sourceServiceName> on <source>
    // to <destination>, as if from sender <destinationServiceName>
    vrpn_StreamForwarder
        (vrpn_Connection * source,
         const char * sourceServiceName,
         vrpn_Connection * destination,
         const char * destinationServiceName);

    ~vrpn_StreamForwarder (void);



    // Begins forwarding of a message type.
    // Return nonzero on failure.
    int forward (const char * sourceName,
                 const char * destinationName,
                 unsigned long classOfService = vrpn_CONNECTION_RELIABLE);

    // Stops forwarding of a message type.
    // Return nonzero on failure.
    int unforward (const char * sourceName,
                   const char * destinationName,
                   unsigned long classOfService = vrpn_CONNECTION_RELIABLE);

  private:

    static int handle_message (void *, vrpn_HANDLERPARAM);

    // Translates (id, serviceId) from source to destination
    // and looks up intended class of service.
    // Returns nonzero if lookup fails.
    int map (long * id, unsigned long * serviceClass);

    vrpn_Connection * d_source;
    long d_sourceService;
    vrpn_Connection * d_destination;
    long d_destinationService;

    struct vrpn_STREAMFORWARDERRECORD {

      vrpn_STREAMFORWARDERRECORD (vrpn_Connection *, vrpn_Connection *,
           const char *, const char *, unsigned long);

      long sourceId;       // source's type id
      long destinationId;  // destination's type id
      unsigned long classOfService;  // class of service to send

      vrpn_STREAMFORWARDERRECORD * next;
    };

    vrpn_STREAMFORWARDERRECORD * d_list;

};


#endif  // VRPN_FORWARDER_H

