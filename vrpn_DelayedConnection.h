#ifndef VRPN_DELAYED_CONNECTION_H
#define VRPN_DELAYED_CONNECTION_H

#include "vrpn_Connection.h"

// We'd like to reuse bits of Log or FileController here to manage
// a list of messages-to-be-dispatched but there isn't an obvious easy way.

// List of messages, each one with a wall-clock time at which it becomes
// "valid".  The connection will not trigger any callbacks for it until
// that time.  That time = time of receipt + the delay to which the
// connection is set.

struct vrpn_DELAYLIST : public vrpn_LOGLIST {
  timeval valid;
};

/// @class vrpn_DelayedEndpoint
/// Adds a user-controlled latency to the delivery of every message.

class vrpn_DelayedEndpoint : public vrpn_Endpoint {

  public:

    vrpn_DelayedEndpoint
                  (vrpn_TypeDispatcher * dispatcher,
                   vrpn_int32 * connectedEndpointCounter);
    virtual ~vrpn_DelayedEndpoint (void);

    virtual int send_pending_reports (void);
      ///< After we send things that need to go out, also see if there
      ///< are any delayed callbacks that need to be dispatched locally.

    void setDelay (timeval);

    void delayAllTypes (vrpn_bool);

    void delayType (vrpn_int32, vrpn_bool);

  protected:

    virtual int dispatch (vrpn_int32, vrpn_int32, timeval,
                          vrpn_uint32, char *);
      ///< Take incoming messages & stick them in a queue with a valid date
      ///< sometime in the future (now + d_delay).

    virtual int dispatchPending (void);
      ///< Dispatch local callbacks for any delayed messages that have now
      ///< become valid.

    virtual void deletePending (vrpn_DELAYLIST * start, vrpn_DELAYLIST * end);
      ///< Delete the list of pending messages beginning at <start> and ending
      ///< just before <end>.

    virtual void enqueue (timeval validTime, vrpn_int32, vrpn_int32, timeval,
                          vrpn_uint32, char *);

    timeval d_delay;
    vrpn_DELAYLIST * d_delayListHead;
    vrpn_DELAYLIST * d_delayListEnd;

    vrpn_bool d_delayAllTypes;
    vrpn_bool d_typeIsDelayed [vrpn_CONNECTION_MAX_TYPES];
};


///< @class vrpn_DelayedConnection
///< Adds a user-controlled latency to the delivery of every message.

class vrpn_DelayedConnection : public vrpn_Synchronized_Connection {

  public:

    vrpn_DelayedConnection
              (timeval delay,
               unsigned short listen_port_no = vrpn_DEFAULT_LISTEN_PORT_NO,
               const char * local_logfile_name = NULL,
               long local_log_mode = vrpn_LOG_NONE,
               const char * NIC_IPaddress = NULL,
               vrpn_Endpoint * (* epa) (vrpn_Connection *, vrpn_int32 *)
                 = allocateEndpoint);
      ///< Server constructor.

    vrpn_DelayedConnection
              (timeval delay,
               const char * server_name,
               int port = vrpn_DEFAULT_LISTEN_PORT_NO,
               const char * local_logfile_name = NULL,
               long local_log_mode = vrpn_LOG_NONE,
               const char * remote_logfile_name = NULL,
               long remote_log_mode = vrpn_LOG_NONE,
               double dFreq = 4.0, 
               int cOffsetWindow = 2,
               const char * NIC_IPaddress = NULL,
               vrpn_Endpoint * (* epa) (vrpn_Connection *, vrpn_int32 *)
                 = allocateEndpoint);
      ///< Client constructor.

    virtual ~vrpn_DelayedConnection (void);


    // MANIPULATORS


    void setDelay (timeval);

    void delayAllTypes (vrpn_bool);

    void delayType (vrpn_int32, vrpn_bool);

  protected:

    virtual void updateEndpoints (void);
    static vrpn_Endpoint * allocateEndpoint (vrpn_Connection *, vrpn_int32 *);

    timeval d_delay;
    vrpn_bool d_delayAllTypes;
    vrpn_bool d_typeIsDelayed [vrpn_CONNECTION_MAX_TYPES];

};

/// @class vrpn_DelayedEndpoint
/// Adds a user-controlled latency to the delivery of every message.
/// Delay is the message's timestamp - measured RTT + d_delay;
/// if the server sends a reply back to us with the same timestamp
/// we sent on the request (instead of restamping it with the time
/// transmitted) this attempts to put a constant end-to-end latency on the
/// connection.  (If timestamp - measured RTT + d_delay < now
/// it should play out immediately instead of queueing until the
/// next mainloop() call.)

class vrpn_RelativeDelayEndpoint : public vrpn_DelayedEndpoint {

  public:

    vrpn_RelativeDelayEndpoint
                  (vrpn_TypeDispatcher * dispatcher,
                   vrpn_int32 * connectedEndpointCounter);
    virtual ~vrpn_RelativeDelayEndpoint (void);

  protected:

    virtual int dispatch (vrpn_int32, vrpn_int32, timeval,
                          vrpn_uint32, char *);
      ///< Take incoming messages & stick them in a queue with a valid date
      ///< sometime in the future (timestamp - RTT + d_delay).

};

class vrpn_RelativeDelayConnection : public vrpn_DelayedConnection {

  public:

    vrpn_RelativeDelayConnection
              (timeval delay,
               unsigned short listen_port_no = vrpn_DEFAULT_LISTEN_PORT_NO,
               const char * local_logfile_name = NULL,
               long local_log_mode = vrpn_LOG_NONE,
               const char * NIC_IPaddress = NULL);
      ///< Server constructor.

    vrpn_RelativeDelayConnection
              (timeval delay,
               const char * server_name,
               int port = vrpn_DEFAULT_LISTEN_PORT_NO,
               const char * local_logfile_name = NULL,
               long local_log_mode = vrpn_LOG_NONE,
               const char * remote_logfile_name = NULL,
               long remote_log_mode = vrpn_LOG_NONE,
               double dFreq = 4.0, 
               int cOffsetWindow = 2,
               const char * NIC_IPaddress = NULL);
      ///< Client constructor.

    virtual ~vrpn_RelativeDelayConnection (void);

    virtual void updateEndpoints (void);
    static vrpn_Endpoint * allocateEndpoint (vrpn_Connection *, vrpn_int32 *);

};

#endif  // VRPN_DELAYED_CONNECTION_H


