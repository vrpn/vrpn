#include "vrpn_FileController.h"
#include "vrpn_Connection.h"

#ifndef	_WIN32
#include <netinet/in.h>
#endif

vrpn_File_Controller::vrpn_File_Controller (vrpn_Connection * c) :
    d_connection (c) {

  if (!c) return;

  d_myId =
        c->register_sender("vrpn File Controller");

  d_set_replay_rate_type =
        c->register_message_type("vrpn File set replay rate");
  d_reset_type =
        c->register_message_type("vrpn File reset");
}

vrpn_File_Controller::~vrpn_File_Controller (void) {

}

void vrpn_File_Controller::set_replay_rate (float rate) {
  struct timeval now;

#if (defined(sgi) || defined(hpux) || defined(sparc))
  float netValue = rate;
#else
  float temp = rate;
  int netValue = htonl(*(int *) &temp);
#endif

  gettimeofday(&now, NULL);
  d_connection->pack_message(sizeof(netValue), now,
                d_set_replay_rate_type, d_myId, (const char *) &netValue,
                vrpn_CONNECTION_RELIABLE);  // | vrpn_CONNECTION_LOCAL_ONLY
};

void vrpn_File_Controller::reset (void) {
  struct timeval now;

  gettimeofday(&now, NULL);
  d_connection->pack_message(0, now,
                d_reset_type, d_myId, NULL,
                vrpn_CONNECTION_RELIABLE);  // | vrpn_CONNECTION_LOCAL_ONLY
};




