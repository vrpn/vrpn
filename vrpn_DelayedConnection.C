#include "vrpn_DelayedConnection.h"

vrpn_DelayedEndpoint::vrpn_DelayedEndpoint
                  (timeval delay,
                   vrpn_TypeDispatcher * dispatcher,
                   vrpn_int32 * connectedEndpointCounter) :
    vrpn_Endpoint (dispatcher, connectedEndpointCounter),
    d_delay (delay)  {

  // TODO
}

// virtual
vrpn_DelayedEndpoint::~vrpn_DelayedEndpoint (void) {

}


// virtual
int vrpn_DelayedEndpoint::send_pending_reports (void) {
  vrpn_Endpoint::send_pending_reports();

  // TODO
}


// virtual
int vrpn_DelayedEndpoint::dispatch
      (vrpn_int32 type, vrpn_int32 sender, timeval time,
       vrpn_uint32 len, char * buffer) {

  // TODO
}

// virtual
int vrpn_DelayedEndpoint::dispatchPending (void) {

  // TODO
}




vrpn_DelayedConnection::vrpn_DelayedConnection
              (timeval delay,
               unsigned short listen_port_no ,
               const char * local_logfile_name ,
               long local_log_mode ,
               const char * NIC_IPaddress ) :
    vrpn_Synchronized_Connection (listen_port_no, local_logfile_name,
                                  local_log_mode, NIC_IPaddress),
    d_delay (delay) {

}

vrpn_DelayedConnection::vrpn_DelayedConnection
              (timeval delay,
               const char * server_name,
               int port = vrpn_DEFAULT_LISTEN_PORT_NO,
               const char * local_logfile_name = NULL,
               long local_log_mode = vrpn_LOG_NONE,
               const char * remote_logfile_name = NULL,
               long remote_log_mode = vrpn_LOG_NONE,
               double dFreq = 4.0, 
               int cOffsetWindow = 2,
               const char * NIC_IPaddress = NULL) :
    vrpn_Synchronized_Connection (server_name, port, local_logfile_name,
                                  local_log_mode, remote_logfile_name,
                                  remote_log_mode, d_Freq, cOffsetWindow,
                                  NIC_IPaddress),
    d_delay (delay) {

}

// virtual
vrpn_DelayedConnection::~vrpn_DelayedConnection (void) {

}


// virtual
vrpn_Endpoint * vrpn_DelayedConnection::allocateEndpoint (vrpn_int32 * e) {
  return new vrpn_DelayedEndpoint (d_delay, d_dispatcher, e);
}


