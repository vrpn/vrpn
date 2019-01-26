// Internal Includes
#include "vrpn_Tracker_Filter.h"
#include "vrpn_SendTextMessageStreamProxy.h"  // for operator<<, etc

// Library/third-party includes
// - none

// Standard includes
#include <iostream>
#include <sstream>                      // for operator<<, basic_ostream, etc
#include <string>                       // for char_traits, basic_string, etc
#include <stddef.h>                     // for size_t
#include <stdio.h>                      // for fprintf, NULL, stderr
#include <string.h>                     // for memset
#include <math.h>

void VRPN_CALLBACK vrpn_Tracker_FilterOneEuro::handle_tracker_update(void *userdata, const vrpn_TRACKERCB info)
{
  // Get pointer to the object we're dealing with.
  vrpn_Tracker_FilterOneEuro *me = static_cast<vrpn_Tracker_FilterOneEuro *>(userdata);

  // See if this sensor is within our range.  If not, we ignore it.
  if (info.sensor >= me->d_channels) {
    return;
  }

  // Filter the position and orientation and then report the filtered value
  // for this channel.  Keep track of the delta-time, and update our current
  // time so we get the right one next time.
  double dt = vrpn_TimevalDurationSeconds(info.msg_time, me->d_last_report_times[info.sensor]);
  if (dt <= 0) { dt = 1; }  // Avoid divide-by-zero in case of fluke.
  vrpn_float64 pos[3];
  vrpn_float64 quat[4];
  memcpy(pos, info.pos, sizeof(pos));
  memcpy(quat, info.quat, sizeof(quat));
  const vrpn_float64 *filtered = me->d_filters[info.sensor].filter(dt, pos);
  q_vec_copy(me->pos, filtered);
  const double *q_filtered = me->d_qfilters[info.sensor].filter(dt, quat);
  q_normalize(me->d_quat, q_filtered);
  me->timestamp = info.msg_time;
  me->d_sensor = info.sensor;
  me->d_last_report_times[info.sensor] = info.msg_time;

  // Send the filtered report.
  char msgbuf[512];
  int len = me->encode_to(msgbuf);
  if (me->d_connection->pack_message(len, me->timestamp, me->position_m_id,
          me->d_sender_id, msgbuf,vrpn_CONNECTION_LOW_LATENCY)) {
    fprintf(stderr, "vrpn_Tracker_FilterOneEuro: cannot write message: tossing\n");
  }
}

vrpn_Tracker_FilterOneEuro::vrpn_Tracker_FilterOneEuro(const char * name, vrpn_Connection * con,
                                                       const char *listen_tracker_name,
                                                       unsigned channels, vrpn_float64 vecMinCutoff,
                                                       vrpn_float64 vecBeta, vrpn_float64 vecDerivativeCutoff,
                                                       vrpn_float64 quatMinCutoff, vrpn_float64 quatBeta,
                                                       vrpn_float64 quatDerivativeCutoff)
  : vrpn_Tracker(name, con)
  , d_channels(channels)
{
  // Allocate space for the times.  Fill them in with now.
  d_last_report_times = new struct timeval[channels];

  vrpn_gettimeofday(&timestamp, NULL);

  // Allocate space for the filters.
  d_filters = new vrpn_OneEuroFilterVec[channels];
  d_qfilters = new vrpn_OneEuroFilterQuat[channels];

  // Fill in the parameters for each filter.
  for (int i = 0; i < static_cast<int>(channels); ++i) {
    d_filters[i].setMinCutoff(vecMinCutoff);
    d_filters[i].setBeta(vecBeta);
    d_filters[i].setDerivativeCutoff(vecDerivativeCutoff);

    d_qfilters[i].setMinCutoff(quatMinCutoff);
    d_qfilters[i].setBeta(quatBeta);
    d_qfilters[i].setDerivativeCutoff(quatDerivativeCutoff);
  }

  // Open and set up callback handler for the tracker we're listening to.
  // If the name starts with the '*' character, use the server
  // connection rather than making a new one.
  if (listen_tracker_name[0] == '*') {
    d_listen_tracker = new vrpn_Tracker_Remote(&(listen_tracker_name[1]),
      d_connection);
  } else {
    d_listen_tracker = new vrpn_Tracker_Remote(listen_tracker_name);
  }
  if (d_listen_tracker) d_listen_tracker->register_change_handler(this, handle_tracker_update);
}

vrpn_Tracker_FilterOneEuro::~vrpn_Tracker_FilterOneEuro()
{
  d_listen_tracker->unregister_change_handler(this, handle_tracker_update);
  try {
    delete d_listen_tracker;
  } catch (...) {
    fprintf(stderr, "vrpn_Tracker_FilterOneEuro::~vrpn_Tracker_FilterOneEuro(): delete failed\n");
    return;
  }
  try {
    if (d_qfilters) { delete[] d_qfilters; d_qfilters = NULL; }
    if (d_filters) { delete[] d_filters; d_filters = NULL; }
    if (d_last_report_times) { delete[] d_last_report_times; d_last_report_times = NULL; }
  } catch (...) {
    fprintf(stderr, "vrpn_Tracker_FilterOneEuro::~vrpn_Tracker_FilterOneEuro(): delete failed\n");
    return;
  }
}

void vrpn_Tracker_FilterOneEuro::mainloop()
{
  // See if we have anything new from our tracker.
  d_listen_tracker->mainloop();

  // Call the server_mainloop on our unique base class.
  server_mainloop();
}

vrpn_Tracker_DeadReckoning_Rotation::vrpn_Tracker_DeadReckoning_Rotation(
    std::string myName, vrpn_Connection *c, std::string origTrackerName,
    vrpn_int32 numSensors, vrpn_float64 predictionTime, bool estimateVelocity)
    : vrpn_Tracker_Server(myName.c_str(), c, numSensors)
    , d_estimateVelocity(estimateVelocity)
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
    try {
      if (origTrackerName[0] == '*') {
        d_origTracker = new vrpn_Tracker_Remote(&(origTrackerName.c_str()[1]), c);
      } else {
        d_origTracker = new vrpn_Tracker_Remote(origTrackerName.c_str());
      }
    } catch (...) {
      d_origTracker = NULL;
      return;
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
  try {
    delete d_origTracker;
  } catch (...) {
    fprintf(stderr, "vrpn_Tracker_DeadReckoning_Rotation::~vrpn_Tracker_DeadReckoning_Rotation(): delete failed\n");
    return;
  }
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
    // If we don't have permission to estimate velocity and haven't gotten it
    // either, then we just pass along the report.
    if (!state.d_receivedAngularVelocityReport && !d_estimateVelocity) {
        report_pose(sensor, state.d_lastReportTime, state.d_lastPosition,
                    state.d_lastOrientation);
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
    if (0 != report_pose(sensor, future_time, state.d_lastPosition, newOrientation)) {
      fprintf(stderr, "vrpn_Tracker_DeadReckoning_Rotation::sendNewPrediction(): Can't report pose\n");
    }
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

    if (!state.d_receivedAngularVelocityReport && me->d_estimateVelocity) {
        // If we have not received any velocity reports, then we estimate
        // the angular velocity using the last report (if any).  The new
        // combined rotation T3 = T2 * T1, where T2 is the difference in
        // rotation between the last time (T1) and now (T3).  We want to
        // solve for T2 (so we can keep applying it going forward).  We
        // find it by right-multiuplying both sides of the equation by
        // T1i (inverse of T1): T3 * T1i = T2.
        if (state.d_lastReportTime.tv_sec != 0) {
            q_type inverted;
            q_invert(inverted, state.d_lastOrientation);
            q_mult(state.d_rotationAmount, info.quat, inverted);
            state.d_rotationInterval = vrpn_TimevalDurationSeconds(
                info.msg_time, state.d_lastReportTime);
            // If we get a zero or negative rotation interval, we're
            // not going to be able to handle it, so we set things back
            // to no rotation over a unit-time interval.
            if (state.d_rotationInterval < 0) {
                state.d_rotationInterval = 1;
                q_make(state.d_rotationAmount, 0, 0, 0, 1);
            }
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

//===============================================================================
// Things related to the test() function go below here.

typedef struct {
    struct timeval  time;
    vrpn_int32  sensor;
    q_vec_type  pos;
    q_type      quat;
} POSE_INFO;
static POSE_INFO poseResponse;

typedef struct {
    struct timeval  time;
    vrpn_int32  sensor;
    q_vec_type  pos;
    q_type      quat;
    double      quat_dt;
} VEL_INFO;
static VEL_INFO velResponse;

// Checks whether two floating-point values are close enough
static bool isClose(double a, double b)
{
    return fabs(a - b) < 1e-6;
}

// Fills in the POSE_INFO structure that is passed in with the data it
// receives.
static void VRPN_CALLBACK handle_test_tracker_report(void *userdata,
    const vrpn_TRACKERCB info)
{
    POSE_INFO *pi = static_cast<POSE_INFO *>(userdata);
    pi->time = info.msg_time;
    pi->sensor = info.sensor;
    q_vec_copy(pi->pos, info.pos);
    q_copy(pi->quat, info.quat);
}

// Fills in the VEL_INFO structure that is passed in with the data it
// receives.
static void VRPN_CALLBACK handle_test_tracker_velocity_report(void *userdata,
    const vrpn_TRACKERVELCB info)
{
    VEL_INFO *vi = static_cast<VEL_INFO *>(userdata);
    vi->time = info.msg_time;
    vi->sensor = info.sensor;
    q_vec_copy(vi->pos, info.vel);
    q_copy(vi->quat, info.vel_quat);
    vi->quat_dt = info.vel_quat_dt;
}

int vrpn_Tracker_DeadReckoning_Rotation::test(void)
{
    // Construct a loopback connection to be used by all of our objects.
    vrpn_Connection *c = vrpn_create_server_connection("loopback:");

    // Create a tracker server to be the initator and a dead-reckoning
    // rotation tracker to use it as a base; have it predict 1 second
    // into the future.
    // Create a remote tracker to listen to t1 and set up its callbacks for
    // position and velocity reports.  They will fill in the static structures
    // listed above with whatever values they receive.
    vrpn_Tracker_Server *t0, *t1;
    vrpn_Tracker_Remote *tr;
    try {
      t0 = new vrpn_Tracker_Server("Tracker0", c, 2);
      t1 = new vrpn_Tracker_DeadReckoning_Rotation("Tracker1", c, "*Tracker0", 2, 1);
      tr = new vrpn_Tracker_Remote("Tracker1", c);
    } catch (...) {
      std::cerr << "vrpn_Tracker_DeadReckoning_Rotation::test: Out of memory" << std::endl;
      return 100;
    }

    tr->register_change_handler(&poseResponse, handle_test_tracker_report);
    tr->register_change_handler(&velResponse, handle_test_tracker_velocity_report);

    // Set up the values in the pose and velocity responses with incorrect values
    // so that things will fail if the class does not perform as expected.
    q_vec_set(poseResponse.pos, 1, 1, 1);
    q_make(poseResponse.quat, 0, 0, 1, 0);
    poseResponse.time.tv_sec = 0;
    poseResponse.time.tv_usec = 0;
    poseResponse.sensor = -1;

    // Send a pose report from sensors 0 and 1 on the original tracker that places
    // them at (1,1,1) and with the identity rotation.  We should get a response for
    // each of them so we should end up with the sensor-1 response matching what we
    // sent, with no change in position or orientation.
    q_vec_type pos = { 1, 1, 1 };
    q_type quat = { 0, 0, 0, 1 };
    struct timeval firstSent;
    vrpn_gettimeofday(&firstSent, NULL);
    t0->report_pose(0, firstSent, pos, quat);
    t0->report_pose(1, firstSent, pos, quat);

    t0->mainloop();
    t1->mainloop();
    c->mainloop();
    tr->mainloop();

    if ( (poseResponse.time.tv_sec != firstSent.tv_sec + 1)
        || (poseResponse.sensor != 1)
        || (q_vec_distance(poseResponse.pos, pos) > 1e-10)
        || (poseResponse.quat[Q_W] != 1)
       )
    {
        std::cerr << "vrpn_Tracker_DeadReckoning_Rotation::test(): Got unexpected"
            << " initial response: pos (" << poseResponse.pos[Q_X] << ", "
            << poseResponse.pos[Q_Y] << ", " << poseResponse.pos[Q_Z] << "), quat ("
            << poseResponse.quat[Q_X] << ", " << poseResponse.quat[Q_Y] << ", "
            << poseResponse.quat[Q_Z] << ", " << poseResponse.quat[Q_W] << ")"
            << " from sensor " << poseResponse.sensor
            << " at time " << poseResponse.time.tv_sec << ":" << poseResponse.time.tv_usec
            << std::endl;
        try {
          delete tr;
          delete t1;
          delete t0;
        } catch (...) {
          std::cerr << "vrpn_Tracker_DeadReckoning_Rotation::test(): delete failed" << std::endl;
          return 1;
        }
        c->removeReference();
        return 1;
    }
    
    // Send a second tracker report for sensor 0 coming 0.4 seconds later that has
    // translated to position (2,1,1) and rotated by 0.4 * 90 degrees around Z.
    // This should cause a prediction for one second later than this new pose
    // message that has rotated by very close to 1.4 * 90 degrees.
    q_vec_type pos2 = { 2, 1, 1 };
    q_type quat2;
    double angle2 = 0.4 * 90 * VRPN_PI / 180.0;
    q_from_axis_angle(quat2, 0, 0, 1, angle2);
    struct timeval p4Second = { 0, 400000 };
    struct timeval firstPlusP4 = vrpn_TimevalSum(firstSent, p4Second);
    t0->report_pose(0, firstPlusP4, pos2, quat2);

    t0->mainloop();
    t1->mainloop();
    c->mainloop();
    tr->mainloop();

    double x, y, z, angle;
    q_to_axis_angle(&x, &y, &z, &angle, poseResponse.quat);
    if ((poseResponse.time.tv_sec != firstPlusP4.tv_sec + 1)
        || (poseResponse.sensor != 0)
        || (q_vec_distance(poseResponse.pos, pos2) > 1e-10)
        || !isClose(x, 0) || !isClose(y, 0) || !isClose(z, 1)
        || !isClose(angle, 1.4 * 90 * VRPN_PI / 180.0)
       )
    {
        std::cerr << "vrpn_Tracker_DeadReckoning_Rotation::test(): Got unexpected"
            << " predicted pose response: pos (" << poseResponse.pos[Q_X] << ", "
            << poseResponse.pos[Q_Y] << ", " << poseResponse.pos[Q_Z] << "), quat ("
            << poseResponse.quat[Q_X] << ", " << poseResponse.quat[Q_Y] << ", "
            << poseResponse.quat[Q_Z] << ", " << poseResponse.quat[Q_W] << ")"
            << " from sensor " << poseResponse.sensor
            << std::endl;
        try {
          delete tr;
          delete t1;
          delete t0;
        } catch (...) {
          std::cerr << "vrpn_Tracker_DeadReckoning_Rotation::test(): delete failed" << std::endl;
          return 1;
        }
        c->removeReference();
        return 2;
    }

    // Send a velocity report for sensor 1 that has has translation by (1,0,0)
    // and rotating by 0.4 * 90 degrees per 0.4 of a second around Z.
    // This should cause a prediction for one second later than the first
    // report that has rotated by very close to 90 degrees.  The translation
    // should be ignored, so the position should be the original position.
    q_vec_type vel = { 0, 0, 0 };
    t0->report_pose_velocity(1, firstPlusP4, vel, quat2, 0.4);

    t0->mainloop();
    t1->mainloop();
    c->mainloop();
    tr->mainloop();

    q_to_axis_angle(&x, &y, &z, &angle, poseResponse.quat);
    if ((poseResponse.time.tv_sec != firstSent.tv_sec + 1)
        || (poseResponse.sensor != 1)
        || (q_vec_distance(poseResponse.pos, pos) > 1e-10)
        || !isClose(x, 0) || !isClose(y, 0) || !isClose(z, 1)
        || !isClose(angle, 90 * VRPN_PI / 180.0)
        )
    {
        std::cerr << "vrpn_Tracker_DeadReckoning_Rotation::test(): Got unexpected"
            << " predicted velocity response: pos (" << poseResponse.pos[Q_X] << ", "
            << poseResponse.pos[Q_Y] << ", " << poseResponse.pos[Q_Z] << "), quat ("
            << poseResponse.quat[Q_X] << ", " << poseResponse.quat[Q_Y] << ", "
            << poseResponse.quat[Q_Z] << ", " << poseResponse.quat[Q_W] << ")"
            << " from sensor " << poseResponse.sensor
            << std::endl;
        try {
          delete tr;
          delete t1;
          delete t0;
        } catch (...) {
          std::cerr << "vrpn_Tracker_DeadReckoning_Rotation::test(): delete failed" << std::endl;
          return 1;
        }
        c->removeReference();
        return 3;
    }

    // To test the behavior of the prediction code when we're moving around more
    // than one axis, and when we're starting from a non-identity orientation,
    // set sensor 1 to be rotated 180 degrees around X.  Then send a velocity
    // report that will produce a rotation of 180 degrees around Z.  The result
    // should match a prediction of 180 degrees around Y (plus or minus 180, plus
    // or minus Y axis are all equivalent).
    struct timeval oneSecond = { 1, 0 };
    struct timeval firstPlusOne = vrpn_TimevalSum(firstSent, oneSecond);
    q_type quat3;
    q_from_axis_angle(quat3, 1, 0, 0, VRPN_PI);
    t0->report_pose(1, firstPlusOne, pos, quat3);
    q_type quat4;
    q_from_axis_angle(quat4, 0, 0, 1, VRPN_PI);
    t0->report_pose_velocity(1, firstPlusOne, vel, quat4, 1.0);

    t0->mainloop();
    t1->mainloop();
    c->mainloop();
    tr->mainloop();

    q_to_axis_angle(&x, &y, &z, &angle, poseResponse.quat);
    if ((poseResponse.time.tv_sec != firstPlusOne.tv_sec + 1)
        || (poseResponse.sensor != 1)
        || (q_vec_distance(poseResponse.pos, pos) > 1e-10)
        || !isClose(x, 0) || !isClose(fabs(y), 1) || !isClose(z, 0)
        || !isClose(fabs(angle), VRPN_PI)
        )
    {
        std::cerr << "vrpn_Tracker_DeadReckoning_Rotation::test(): Got unexpected"
            << " predicted pose + velocity response: pos (" << poseResponse.pos[Q_X] << ", "
            << poseResponse.pos[Q_Y] << ", " << poseResponse.pos[Q_Z] << "), quat ("
            << poseResponse.quat[Q_X] << ", " << poseResponse.quat[Q_Y] << ", "
            << poseResponse.quat[Q_Z] << ", " << poseResponse.quat[Q_W] << ")"
            << " from sensor " << poseResponse.sensor
            << std::endl;
        try {
          delete tr;
          delete t1;
          delete t0;
        } catch (...) {
          std::cerr << "vrpn_Tracker_DeadReckoning_Rotation::test(): delete failed" << std::endl;
          return 1;
        }
        c->removeReference();
        return 4;
    }

    // To test the behavior of the prediction code when we're moving around more
    // than one axis, and when we're starting from a non-identity orientation,
    // set sensor 0 to start out at identity.  Then in one second it will be
    // rotated 180 degrees around X.  Then in another second it will be rotated
    // additionally by 90 degrees around Z; the prediction should be another
    // 90 degrees around Z, which should turn out to compose to +/-180 degrees
    // around +/- Y, as in the velocity case above.
    // To make this work, we send a sequence of three poses, one second apart,
    // starting at the original time.  We do this on sensor 0, which has never
    // had a velocity report, so that it will be using the pose-only prediction.
    struct timeval firstPlusTwo = vrpn_TimevalSum(firstPlusOne, oneSecond);
    q_from_axis_angle(quat3, 1, 0, 0, VRPN_PI);
    t0->report_pose(0, firstSent, pos, quat);
    t0->report_pose(0, firstPlusOne, pos, quat3);
    q_from_axis_angle(quat4, 0, 0, 1, VRPN_PI / 2);
    q_type quat5;
    q_mult(quat5, quat4, quat3);
    t0->report_pose(0, firstPlusTwo, pos, quat5);

    t0->mainloop();
    t1->mainloop();
    c->mainloop();
    tr->mainloop();

    q_to_axis_angle(&x, &y, &z, &angle, poseResponse.quat);
    if ((poseResponse.time.tv_sec != firstPlusTwo.tv_sec + 1)
        || (poseResponse.sensor != 0)
        || (q_vec_distance(poseResponse.pos, pos) > 1e-10)
        || !isClose(x, 0) || !isClose(fabs(y), 1.0) || !isClose(z, 0)
        || !isClose(fabs(angle), VRPN_PI)
        )
    {
        std::cerr << "vrpn_Tracker_DeadReckoning_Rotation::test(): Got unexpected"
            << " predicted pose + pose response: pos (" << poseResponse.pos[Q_X] << ", "
            << poseResponse.pos[Q_Y] << ", " << poseResponse.pos[Q_Z] << "), quat ("
            << poseResponse.quat[Q_X] << ", " << poseResponse.quat[Q_Y] << ", "
            << poseResponse.quat[Q_Z] << ", " << poseResponse.quat[Q_W] << ")"
            << " from sensor " << poseResponse.sensor
            << "; axis = (" << x << ", " << y << ", " << z << "), angle = "
            << angle
            << std::endl;
        try {
          delete tr;
          delete t1;
          delete t0;
        } catch (...) {
          std::cerr << "vrpn_Tracker_DeadReckoning_Rotation::test(): delete failed" << std::endl;
          return 1;
        }
        c->removeReference();
        return 5;
    }

    // Done; delete our objects and return 0 to indicate that
    // everything worked.
    try {
      delete tr;
      delete t1;
      delete t0;
    } catch (...) {
      std::cerr << "vrpn_Tracker_DeadReckoning_Rotation::test(): delete failed" << std::endl;
      return 1;
    }
    c->removeReference();
    return 0;
}

