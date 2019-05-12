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
#include <quat.h>

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
    // Disconnect if we were already tracking
    if (m_session.isTrackingRunning()) {
        m_session.stopTracking();
        m_session.disconnectFromServer();
    }

    // Start a new connection
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

    // Mark that we have a good connection to the tracker.
    vrpn_gettimeofday(&timestamp, NULL);
}

vrpn_Tracker_OccipitalStructureCore::~vrpn_Tracker_OccipitalStructureCore()
{
    m_session.stopTracking();
    m_session.disconnectFromServer();
}

void vrpn_Tracker_OccipitalStructureCore::mainloop()
{
    // Reset if we haven't heard anything in too long
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    if (vrpn_TimevalDurationSeconds(now, timestamp) > 3) {
        VRPN_MSG_ERROR("vrpn_Tracker_OccipitalStructureCore::mainloop(): "
                       "Timeout talking to tracker, resetting");
        reset();
        // Record that we tried even if the reset failed
        timestamp = now;
    }

    // Nothing to do if tracking is not running
    if (!m_session.isTrackingRunning()) {
        return;
    }

    // Call the generic server mainloop, since we are a server
    server_mainloop();

    // Get latest data
    get_report();
}

void vrpn_Tracker_OccipitalStructureCore::get_report()
{
    vrpn_gettimeofday(&timestamp, NULL);
    ST::XRPose pose;
    ST::XRFrameOfReference ref = ST::XRFrameOfReference::VisibleCamera;
    m_session.predictWorldFromCameraPose(ref, m_session.currentTime(), pose);

    // Record the pose, converting from matrix to pos/quat
    q_xyz_quat_type qPose;
    q_matrix_type matrix;
    for (size_t x = 0; x < 4; x++) {
        for (size_t y = 0; y < 4; y++) {
            matrix[x][y] = pose.matrix.m[y + x * 4];
        }
    }
    q_row_matrix_to_xyz_quat(&qPose, matrix);
    pos[0] = qPose.xyz[Q_X];
    pos[1] = qPose.xyz[Q_Y];
    pos[2] = qPose.xyz[Q_Z];
    d_quat[Q_X] = qPose.quat[Q_X];
    d_quat[Q_Y] = qPose.quat[Q_Y];
    d_quat[Q_Z] = qPose.quat[Q_Z];
    d_quat[Q_W] = qPose.quat[Q_W];

    // Record the velocity
    /// @todo Ensure that the coordinate system is correct
    vel[0] = vel[1] = vel[2] = 0;
    /*
    vel[0] = pose.linearVelocity[0];
    vel[1] = pose.linearVelocity[1];
    vel[2] = pose.linearVelocity[2];
    */

    // Record the angular velocity and units
    /// @todo Convert to proper format
    vel_quat[Q_X] = vel_quat[Q_Y] = vel_quat[Q_Z] = 0;
    vel_quat[Q_W] = 1;
    vel_quat_dt = 1;
    /*
    vel_quat[Q_X] = pose.angularVelocity[0];
    vel_quat[Q_Y] = pose.angularVelocity[1];
    vel_quat[Q_Z] = pose.angularVelocity[2];
    */

    // Send the report for sensor 0
    d_sensor = 0;
    send_report();
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
