// vrpn_Tracker_DeadReckoning.C
//

#include "vrpn_Tracker_DeadReckoning.h"
#include "vrpn_SendTextMessageStreamProxy.h"
#include <iostream>
#include <quat.h>

vrpn_Tracker_DeadReckoning_Rotation::vrpn_Tracker_DeadReckoning_Rotation(
    std::string myName
    , vrpn_Connection *c
    , std::string origTrackerName
    , vrpn_int32 numSensors
    , vrpn_float64 predictionTime
    ) : vrpn_Tracker_Server(myName.c_str(), c, numSensors)
{
    // Do the things all tracker servers need to do.
    num_sensors = numSensors;
    register_server_handlers();

    // Set values that don't need to be set in the list above, so that we
    // don't worry about out-of-order warnings from the compiler in case we
    // adjust the order in the header file in the future.
    d_predictionTime = predictionTime;
    d_numSensors = numSensors;

    // If the name of the tracker we're using starts with a '*' character,
    // we use our own connection to talk with it.  Otherwise, we open a remote
    // tracker with the specified name.
    if (origTrackerName[0] == '*') {
        d_origTracker = new vrpn_Tracker_Remote(&(origTrackerName.c_str()[1]), c);
    }
    else {
        d_origTracker = new vrpn_Tracker_Remote(origTrackerName.c_str());
    }

    // Initialize the rotational state of each sensor.  There has not bee
    // an orientation report, so we set its time to 0.
    // Set up callback handlers for the pose and pose velocity messages,
    // from which we will extract the orientation and orientation velocity
    // information.
    for (vrpn_int32 i = 0; i < numSensors; i++) {
        q_type no_rotation = { 0, 0, 0, 1 };
        RotationState rs;
        q_copy(rs.d_rotationAmount, no_rotation);
        rs.d_rotationInterval = 1;
        rs.d_receivedAngularVelocityReport = false;
        rs.d_lastReportTime.tv_sec = 0;
        rs.d_lastReportTime.tv_usec = 0;
        d_rotationStates.push_back(rs);
    }

    // Register handler for all sensors, so we'll be told the ID
    d_origTracker->register_change_handler(this, handle_tracker_report);
    d_origTracker->register_change_handler(this, handle_tracker_velocity_report);
}

void vrpn_Tracker_DeadReckoning_Rotation::mainloop()
{
    // Call the generic server mainloop routine, since this is a server
    server_mainloop();
}

vrpn_Tracker_DeadReckoning_Rotation::~vrpn_Tracker_DeadReckoning_Rotation()
{
    delete d_origTracker;
}

void vrpn_Tracker_DeadReckoning_Rotation::sendNewPrediction(vrpn_int32 sensor)
{
    //========================================================================
    // Figure out which rotation state we're supposed to use.
    if (sensor >= d_numSensors) {
        send_text_message(vrpn_TEXT_WARNING)
            << "sendNewPrediction: Asked for sensor " << sensor
            << " but I only have " << d_numSensors
            << "sensors.  Discarding.";
        return;
    }
    vrpn_Tracker_DeadReckoning_Rotation::RotationState &state =
        d_rotationStates[sensor];

    //========================================================================
    // If we haven't had a tracker report yet, nothing to send.
    if (state.d_lastReportTime.tv_sec == 0) {
        return;
    }

    //========================================================================
    // Estimate the future orientation based on the current angular velocity
    // estimate and the last reported orientation.  Predict it into the future
    // the amount we've been asked to.

    // Start with the previous orientation.
    q_type newOrientation;
    q_copy(newOrientation, state.d_lastOrientation);

    // Rotate it by the amount to rotate once for every integral multiple
    // of the rotation time we've been asked to go.
    double remaining = d_predictionTime;
    while (remaining > state.d_rotationInterval) {
        q_mult(newOrientation, state.d_rotationAmount, newOrientation);
        remaining -= state.d_rotationInterval;
    }

    // Then rotate it by the remaining fractional amount.
    double fractionTime = remaining / state.d_rotationInterval;
    q_type identity = { 0, 0, 0, 1 };
    q_type fractionRotation;
    q_slerp(fractionRotation, identity, state.d_rotationAmount, fractionTime);
    q_mult(newOrientation, fractionRotation, newOrientation);

    //========================================================================
    // Find out the future time for which we will be predicting by adding the
    // prediction interval to our last report time.
    struct timeval future_time;
    struct timeval delta;
    delta.tv_sec = static_cast<unsigned long>(d_predictionTime);
    double remainder = d_predictionTime - delta.tv_sec;
    delta.tv_usec = static_cast<unsigned long>(remainder * 1e6);
    future_time = vrpn_TimevalSum(delta, state.d_lastReportTime);

    //========================================================================
    // Pack our predicted tracker report for this future time.
    // Use the position portion of the report unaltered.
    report_pose(sensor, future_time, state.d_lastPosition, newOrientation);
}

void vrpn_Tracker_DeadReckoning_Rotation::handle_tracker_report(void *userdata,
    const vrpn_TRACKERCB info)
{
    // Find the pointer to the class that registered the callback and get a
    // reference to the RotationState we're supposed to be using.
    vrpn_Tracker_DeadReckoning_Rotation *me =
        static_cast<vrpn_Tracker_DeadReckoning_Rotation *>(userdata);
    if (info.sensor >= me->d_numSensors) {
        me->send_text_message(vrpn_TEXT_WARNING)
            << "Received tracker message from sensor " << info.sensor
            << " but I only have " << me->d_numSensors
            << "sensors.  Discarding.";
        return;
    }
    vrpn_Tracker_DeadReckoning_Rotation::RotationState &state =
        me->d_rotationStates[info.sensor];

    // If we have not received any velocity reports, then we estimate
    // the angular velocity using the last report (if any).
    if (!state.d_receivedAngularVelocityReport) {
        if (state.d_lastReportTime.tv_sec != 0) {
            q_vec_subtract(state.d_rotationAmount,
                info.quat, state.d_lastOrientation);
            state.d_rotationInterval = vrpn_TimevalDurationSeconds(
                info.msg_time, state.d_lastReportTime);
        }
    }

    // Keep track of the position, orientation and time for the next report
    q_vec_copy(state.d_lastPosition, info.pos);
    q_copy(state.d_lastOrientation, info.quat);
    state.d_lastReportTime = info.msg_time;

    // We have new data, so we send a new prediction.
    me->sendNewPrediction(info.sensor);
}

void vrpn_Tracker_DeadReckoning_Rotation::handle_tracker_velocity_report(void *userdata,
    const vrpn_TRACKERVELCB info)
{
    // Find the pointer to the class that registered the callback and get a
    // reference to the RotationState we're supposed to be using.
    vrpn_Tracker_DeadReckoning_Rotation *me =
        static_cast<vrpn_Tracker_DeadReckoning_Rotation *>(userdata);
    if (info.sensor >= me->d_numSensors) {
        me->send_text_message(vrpn_TEXT_WARNING)
            << "Received velocity message from sensor " << info.sensor
            << " but I only have " << me->d_numSensors
            << "sensors.  Discarding.";
        return;
    }
    vrpn_Tracker_DeadReckoning_Rotation::RotationState &state =
        me->d_rotationStates[info.sensor];

    // Store the rotational information and indicate that we have gotten
    // a report of the tracker velocity.
    state.d_receivedAngularVelocityReport = true;
    q_copy(state.d_rotationAmount, info.vel_quat);
    state.d_rotationInterval = info.vel_quat_dt;

    // We have new data, so we send a new prediction.
    me->sendNewPrediction(info.sensor);

    // Pass through the velocity estimate.
    me->report_pose_velocity(info.sensor, info.msg_time, info.vel,
        info.vel_quat, info.vel_quat_dt);
}

int vrpn_Tracker_DeadReckoning_Rotation::test(void)
{
    std::cerr << "vrpn_Tracker_DeadReckoning_Rotation::test(): Not yet implemented"
        << std::endl;
    return 1;
}
