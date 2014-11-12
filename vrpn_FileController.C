#include <stddef.h> // for NULL

#include "vrpn_Connection.h" // for vrpn_Connection, etc
#include "vrpn_FileController.h"
// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h
// and netinet/in.h and ...
#include "vrpn_Shared.h" // for timeval, vrpn_gettimeofday, etc

vrpn_File_Controller::vrpn_File_Controller(vrpn_Connection *c)
    : d_connection(c)
{

    if (!c) return;

    d_myId = c->register_sender("vrpn File Controller");

    d_set_replay_rate_type =
        c->register_message_type("vrpn_File set_replay_rate");
    d_reset_type = c->register_message_type("vrpn_File reset");
    d_play_to_time_type = c->register_message_type("vrpn_File play_to_time");
}

vrpn_File_Controller::~vrpn_File_Controller(void) {}

void vrpn_File_Controller::set_replay_rate(vrpn_float32 rate)
{
    struct timeval now;

    char buf[sizeof(vrpn_float32)];

    vrpn_int32 bufLen = sizeof(vrpn_float32);
    char *bufPtr = buf;

    if (vrpn_buffer(&bufPtr, &bufLen, rate)) {
        return;
    }
    vrpn_gettimeofday(&now, NULL);
    d_connection->pack_message(
        sizeof(vrpn_float32), now, d_set_replay_rate_type, d_myId, buf,
        vrpn_CONNECTION_RELIABLE); // | vrpn_CONNECTION_LOCAL_ONLY
};

void vrpn_File_Controller::reset(void)
{
    struct timeval now;

    vrpn_gettimeofday(&now, NULL);
    d_connection->pack_message(
        0, now, d_reset_type, d_myId, NULL,
        vrpn_CONNECTION_RELIABLE); // | vrpn_CONNECTION_LOCAL_ONLY
};

void vrpn_File_Controller::play_to_time(struct timeval t)
{
    struct timeval now;

    vrpn_gettimeofday(&now, NULL);
    d_connection->pack_message(
        sizeof(struct timeval), now, d_play_to_time_type, d_myId,
        (const char *)&t,
        vrpn_CONNECTION_RELIABLE); // | vrpn_CONNECTION_LOCAL_ONLY
};
