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
    const char* name, vrpn_Connection* c, float updateRate)
    : vrpn_Tracker(name, c)
    , m_update_rate(updateRate)
{
    // Generic VRPN stuff
    num_sensors = 1;
    register_server_handlers();

    if (d_connection) {
      // Register a handler for the update change callback
      if (register_autodeleted_handler(
              update_rate_id, handle_update_rate_request, this, d_sender_id))
          fprintf(stderr, "vrpn_Tracker_OccipitalStructureCore: Can't register update-rate handler\n");
    }

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

    // Get current pose if it has been long enough since the last
    // report.
    if (vrpn_TimevalDurationSeconds(now, m_last_report) > 1/m_update_rate) {
        get_report();
        m_last_report = now;
    }
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

    // Record the velocity, which is reported in world space.
    vel[0] = pose.linearVelocity[0];
    vel[1] = pose.linearVelocity[1];
    vel[2] = pose.linearVelocity[2];

    // Record the angular velocity, converting to a differential
    // Quaternion with 1/100th of second dt.
    // The angular velocity is reported in the local coordinate
    // system of the device, not in world space.  Its first entry
    // is rotation about X, second Y, and third Z.  They are in radians
    // per second.
    // We need to first transform from the local coordinate system
    // to the global, and also pack in the order expected by Quatlib, and
    // the scale to convert to a differential Quaternion with dt
    // time scale so we never wrap around.
    double dt = 1 / 10.0;

    // Convert rotation from local to global coordinates by transforming the
    // XYZ Euler angles into world space.
    q_vec_type dRot;
    dRot[Q_X] = pose.angularVelocity[Q_X] * dt;
    dRot[Q_Y] = pose.angularVelocity[Q_Y] * dt;
    dRot[Q_Z] = pose.angularVelocity[Q_Z] * dt;
    q_xform(dRot, d_quat, dRot);

    // Convert from an XYZ vector into a Roll, Pitch Yaw vector
    // in Quatlib order.  Then convert this into a Quaternion.
    q_vec_type euler;
    euler[Q_ROLL] = dRot[Q_X] * dt;
    euler[Q_PITCH] = dRot[Q_Y] * dt;
    euler[Q_YAW] = dRot[Q_Z] * dt;
    q_type q;
    q_from_euler(q, euler[Q_YAW], euler[Q_PITCH], euler[Q_ROLL]);

    // Put into the VRPN structure.
    vel_quat[Q_X] = q[Q_X];
    vel_quat[Q_Y] = q[Q_Y];
    vel_quat[Q_Z] = q[Q_Z];
    vel_quat[Q_W] = q[Q_W];

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
            VRPN_MSG_ERROR("vrpn_Tracker_OccipitalStructureCore::send_report(): cannot write pose message: tossing");
	}
        len = encode_vel_to(msgbuf);
        if (d_connection->pack_message(len, timestamp, velocity_m_id,
                                       d_sender_id, msgbuf,
                                       vrpn_CONNECTION_LOW_LATENCY)) {
            VRPN_MSG_ERROR("vrpn_Tracker_OccipitalStructureCore::send_report():"
                           " cannot write velocity message: tossing");
        }
    }
}

int vrpn_Tracker_OccipitalStructureCore::handle_update_rate_request(
    void* userdata,
                                                        vrpn_HANDLERPARAM p)
{
    vrpn_Tracker_OccipitalStructureCore* thistracker =
        (vrpn_Tracker_OccipitalStructureCore*)userdata;
    vrpn_float64 update_rate = 0;
    vrpn_unbuffer(&p.buffer, &update_rate);
    thistracker->m_update_rate = update_rate;
    return 0;
}

#endif
