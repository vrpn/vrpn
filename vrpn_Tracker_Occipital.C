///////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:        vrpn_Tracker_Occipital.h
//
// Author:      Russell Taylor
//              ReliaSolve working for Vality
//
// Description: VRPN tracker class for Occipital Structure Core using PerceptionEngine library
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include "vrpn_Tracker_Occipital.h"

#ifdef VRPN_USE_STRUCTUREPERCEPTIONENGINE
#include <vrpn_MessageMacros.h>
#include <string>

vrpn_Tracker_OccipitalStructureCore::vrpn_Tracker_OccipitalStructureCore(
    const char* name, vrpn_Connection* c)
    : vrpn_Tracker(name, c)
{
    // Generic VRPN stuff
    num_sensors = 1;
    register_server_handlers();

    // Occipital-specific stuff
    reset();
}

void vrpn_Tracker_OccipitalStructureCore::reset()
{
    if (m_session.connectToServer() != ST::XRStatus::Good) {
        std::string errMsg = "vrpn_Tracker_OccipitalStructureCore: Unable to "
                             "connect to server: ";
        errMsg += m_session.lastSessionError();
        VRPN_MSG_ERROR(errMsg.c_str());
        return;
    }

    ST::XRSessionSettings settings;
    if (m_session.startTracking(settings) != ST::XRStatus::Good) {
        std::string errMsg = "vrpn_Tracker_OccipitalStructureCore: Unable to "
                             "start tracking: ";
        errMsg += m_session.lastSessionError();
        VRPN_MSG_ERROR(errMsg.c_str());
        return;
    }
}

vrpn_Tracker_OccipitalStructureCore::~vrpn_Tracker_OccipitalStructureCore()
{
    /// @todo
}

void vrpn_Tracker_OccipitalStructureCore::mainloop()
{
    // Nothing to do if tracking is not running
    if (!m_session.isTrackingRunning()) {
        return;
    }

    // Call the generic server mainloop, since we are a server
    server_mainloop();

    // Reset if we haven't heard anything in too long
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    if (vrpn_TimevalDurationSeconds(now, timestamp) > 1) {
        VRPN_MSG_ERROR("vrpn_Tracker_OccipitalStructureCore::mainloop(): "
                       "Timeout talking to traker, resetting");
        reset();
    }

    // Get latest data
    get_report();
}

void vrpn_Tracker_OccipitalStructureCore::get_report()
{
    vrpn_gettimeofday(&timestamp, NULL);

    /// @todo

    for (int i = 0; i < num_sensors; i++) {
        /// @todo

        // The sensor number
        d_sensor = i;

        // No need to fill in position, as we don´t get position information
        /// @todo Fill in d_pos and d_quat

        // Send the data
        send_report();
    }
}

void vrpn_Tracker_OccipitalStructureCore::send_report()
{
    if (d_connection) {
        char msgbuf[1000];
        int len = encode_to(msgbuf);
        if (d_connection->pack_message(len, timestamp, position_m_id, d_sender_id, msgbuf, 
                                       vrpn_CONNECTION_LOW_LATENCY)) {
            VRPN_MSG_ERROR("vrpn_Tracker_OccipitalStructureCore::send_report(): cannot write message: tossing");
		}
    }
}

#endif
