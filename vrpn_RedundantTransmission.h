#ifndef VRPN_REDUNDANT_TRANSMISSION_H
#define VRPN_REDUNDANT_TRANSMISSION_H

/// @class vrpn_RedundantTransmission
/// Helper class for vrpn_Connection that automates redundant transmission
/// for unreliable (low-latency) messages.  Call pack_messages() here instead
/// of on your connection, and call mainloop() here before calling mainloop()
/// on your connection.

#include <vrpn_Shared.h>  // for timeval, types
#include <vrpn_BaseClass.h>
#include <vrpn_Connection.h>  // for vrpn_HANDLERPARAM, vrpn_Connection

class vrpn_RedundantTransmission {

  public:

    vrpn_RedundantTransmission (vrpn_Connection * c);
    ~vrpn_RedundantTransmission (void);

    vrpn_uint32 defaultRetransmissions (void) const;
    timeval defaultInterval (void) const;

    virtual void mainloop (void);
      ///< Determines which messages need to be resent and queues
      ///< them up on the connection for transmission.

    virtual void setDefaults (vrpn_uint32 numRetransmissions,
                              timeval transmissionInterval);
      ///< Set default values for future calls to pack_message().

    virtual int pack_message
      (vrpn_uint32 len, timeval time, vrpn_uint32 type,
       vrpn_uint32 sender, const char * buffer,
       vrpn_int32 numRetransmissions = -1,
       timeval * transmissionInterval = NULL);
      ///< Specify -1 and NULL to use default values.
      ///< Bug - running at user level, we don't know when the message
      ///< was sent, only when it was packed.  If the message is packed
      ///< long before it's sent, one or more replicas will be sent along
      ///< with the initial transmission.

  protected:

    vrpn_Connection * d_connection;

    struct queuedMessage {
      vrpn_HANDLERPARAM p;
      vrpn_uint32 remainingTransmissions;
      timeval transmissionInterval;
      timeval nextValidTime;
      queuedMessage * next;
    };

    queuedMessage * d_messageList;
    vrpn_uint32 d_numMessagesQueued;
      ///< For debugging, mostly.

    // Default values.

    vrpn_uint32 d_numTransmissions;
    timeval d_transmissionInterval;


};


struct vrpn_RedundantController_Protocol {

  char * encode_set (int * len, vrpn_uint32 num, timeval interval);
  void decode_set (const char ** buf, vrpn_uint32 * num, timeval * interval);

  void register_types (vrpn_Connection *);

  vrpn_int32 d_set_type;
};


/// @class vrpn_RedundantController
/// Accepts commands over a connection to control a local
/// vrpn_RedundantTransmission's default parameters.
  
class vrpn_RedundantController : public vrpn_BaseClass {

  public:

    vrpn_RedundantController (vrpn_RedundantTransmission *, vrpn_Connection *);
    ~vrpn_RedundantController (void);

    void mainloop (void);
      // Do nothing;  vrpn_BaseClass requires this.

  protected:

    virtual int register_types (void);

    vrpn_RedundantController_Protocol d_protocol;

    static int handle_set (void *, vrpn_HANDLERPARAM);

    vrpn_RedundantTransmission * d_object;
};

class vrpn_RedundantRemote : public vrpn_BaseClass {

  public:

    vrpn_RedundantRemote (vrpn_Connection *);
    ~vrpn_RedundantRemote (void);

    void mainloop (void);
      // Do nothing;  vrpn_BaseClass requires this.

    void set (int numRetransmissions, timeval transmissionInterval);

  protected:

    int register_types (void);

    vrpn_RedundantController_Protocol d_protocol;
};

#endif  // VRPN_REDUNDANT_TRANSMISSION_H


