/// @file vrpn_Tracker_IMU.C
/// @copyright 2015 ReliaSolve.com
/// @author ReliaSolve.com russ@reliasolve.com
/// @license Standard VRPN license.

#include "vrpn_Tracker_IMU.h"
#include <cmath>

#undef VERBOSE

vrpn_IMU_Magnetometer::vrpn_IMU_Magnetometer
          (std::string const &name, vrpn_Connection *output_con,
            vrpn_IMU_Axis_Params params,
           float update_rate, bool report_changes) :
	vrpn_Analog_Server(name.c_str(), output_con),
	d_update_interval (update_rate ? (1/update_rate) : 1.0),
	d_report_changes (report_changes)
{
  // We have 3 output channels.
  num_channel = 3;

	//--------------------------------------------------------------------
	// Set the current timestamp to "now" and current direction to along X.
	// This is done in case we never hear from the analog devices.
  vrpn_gettimeofday(&d_prevtime, NULL);
  vrpn_Analog::timestamp = d_prevtime;
  channel[0] = 1;
  channel[1] = 0;
  channel[2] = 0;

  // Set empty axis values.
	d_vector.params = params;
  d_vector.time = d_prevtime;

	//--------------------------------------------------------------------
	// Open analog remote to use to listen to.
	//   If the name starts with the "*" character, use our output
  // connection rather than getting a new connection for it.
	//   Set up callbacks to handle updates to the analog reports.
	setup_vector(&d_vector);

	//--------------------------------------------------------------------
  // Set the minimum and maximum values past each other and past what
  // we expect from any real device, so they will be inialized properly
  // the first time we get a report.
  for (size_t i = 0; i < 3; i++) {
    d_mins[i] = 1e100;
    d_maxes[i] = -1e100;
  }
}

vrpn_IMU_Magnetometer::~vrpn_IMU_Magnetometer()
{
	// Tear down the analog update callbacks and remotes
	teardown_vector(&d_vector);
}

// This routine handles updates of the analog values. The value coming in is
// adjusted per the parameters in the axis description, and then used to
// update the value there. The value is used by the device code in
// mainloop() to update its internal state; that work is not done here.

void	VRPN_CALLBACK vrpn_IMU_Magnetometer::handle_analog_update
                     (void *userdata, const vrpn_ANALOGCB info)
{
  vrpn_IMU_Vector	*vector = static_cast<vrpn_IMU_Vector *>(userdata);
  for (size_t i = 0; i < 3; i++) {
    vector->values[i] =
      (info.channel[vector->params.channels[i]] - vector->params.offsets[i])
      * vector->params.scales[i];
  }

  // Store the time we got the value.
  vector->time = info.msg_time;
}

// This sets up the Analog Remote for our vector, setting up the callback
// needed to adjust the value based on changes in the analog input.
// Returns 0 on success and -1 on failure.

int	vrpn_IMU_Magnetometer::setup_vector(vrpn_IMU_Vector *vector)
{
  // Set a default time of now.
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  vector->time = now;

  // If the name is empty, we're done.
  if (vector->params.name.size() == 0) { return 0; }

  // Open the analog device and point the remote at it.
  // If the name starts with the '*' character, use the server
  // connection rather than making a new one.
  try {
    if (vector->params.name[0] == '*') {
      vector->ana = new vrpn_Analog_Remote(vector->params.name.c_str()+1,
        d_connection);
#ifdef	VERBOSE
      printf("vrpn_IMU_Magnetometer: Adding local analog %s\n",
        vector->params.name.c_str()+1);
#endif
    } else {
      vector->ana = new vrpn_Analog_Remote(vector->params.name.c_str());
#ifdef	VERBOSE
      printf("vrpn_IMU_Magnetometer: Adding remote analog %s\n",
      vector->params.name.c_str());
#endif
    }
  } catch (...) { vector->ana = NULL; }
  if (vector->ana == NULL) {
	  fprintf(stderr,"vrpn_IMU_Magnetometer: "
            "Can't open Analog %s\n", vector->params.name.c_str());
	  return -1;
  }

  // Set up the callback handler for the channel
  return vector->ana->register_change_handler(vector, handle_analog_update);
}

// This tears down the Analog Remote for our vector, undoing everything that
// the setup did. Returns 0 on success and -1 on failure.

int	vrpn_IMU_Magnetometer::teardown_vector(vrpn_IMU_Vector *vector)
{
	int	ret;

	// If the analog pointer is empty, we're done.
	if (vector->ana == NULL) { return 0; }

	// Turn off the callback handler for the channel
	ret = vector->ana->unregister_change_handler((void*)vector,
                          handle_analog_update);

	// Delete the analog device.
        try {
          delete vector->ana;
        } catch (...) {
          fprintf(stderr, "vrpn_IMU_Magnetometer::teardown_vector(): delete failed\n");
          return -1;
        }

	return ret;
}
 
void vrpn_IMU_Magnetometer::mainloop()
{
  struct timeval	now;
  double	interval;	// How long since the last report, in secs

  // Call generic server mainloop, since we are a server
  server_mainloop();

  // Mainloop() all of the analogs that are defined
  // so that we will get all of the values fresh.
  if (d_vector.ana != NULL) { d_vector.ana->mainloop(); };

  // Keep track of the minimum and maximum values of
  // our reports.
  for (size_t i = 0; i < 3; i++) {
    if (d_vector.values[i] < d_mins[i]) {
      d_mins[i] = d_vector.values[i];
    }
    if (d_vector.values[i] > d_maxes[i]) {
      d_maxes[i] = d_vector.values[i];
    }
  }

  // See if it has been long enough since our last report.
  // If so, generate a new one.
  vrpn_gettimeofday(&now, NULL);
  interval = vrpn_TimevalDurationSeconds(now, d_prevtime);

  if (interval >= d_update_interval) {

    // Set the time on the report
    timestamp = d_vector.time;

    // Compute the unit vector by finding the normalized
    // distance between the min and max for each axis and
    // converting it to the range (-1,1) and then normalizing
    // the resulting vector.  Watch out for min and max
    // being the same.
    for (size_t i = 0; i < 3; i++) {
      double diff = d_vector.values[i] - d_mins[i];
      if (diff == 0) {
        channel[i] = 0;
      }
      else {
        channel[i] = -1 + 2 * diff / (d_maxes[i] - d_mins[i]);
      }
    }
    double len = sqrt( channel[0] * channel[0] +
      channel[1] * channel[1] + channel[2] * channel[2]);
    if (len == 0) {
      channel[0] = 1;
      channel[1] = 0;
      channel[2] = 0;
    } else {
      channel[0] /= len;
      channel[1] /= len;
      channel[2] /= len;
    }

    // pack and deliver analog unit vector report;
    if (d_connection) {
      if (d_report_changes) {
        vrpn_Analog_Server::report_changes();
      } else {
        vrpn_Analog_Server::report();
      }
    } else {
      fprintf(stderr,"vrpn_IMU_Magnetometer: "
              "No valid connection\n");
    }

    // We just sent a report, so reset the time
    d_prevtime = now;
  }

  // We're not always sending reports, but we still want to
  // update the interval clock so that we don't integrate over
  // too long a timespan when we do finally report a change.
  if (interval >= d_update_interval) {
    d_prevtime = now;
  }
}

vrpn_IMU_SimpleCombiner::vrpn_IMU_SimpleCombiner
    (const char * name, vrpn_Connection * trackercon,
      vrpn_Tracker_IMU_Params *params,
      float update_rate, bool report_changes)
  : vrpn_Tracker(name, trackercon),
  d_update_interval(update_rate ? (1 / update_rate) : 1.0),
  d_report_changes(report_changes)
{
  // Set the restoration rates for gravity and north.
  // These are the fraction of the way to full restoration that should
  // occur over one second.
  d_gravity_restore_rate = 0.9;
  d_north_restore_rate = 0.9;

  // Hook up the parameters for acceleration and rotational velocity
  d_acceleration.params = params->d_acceleration;
  d_rotational_vel.params = params->d_rotational_vel;

  // Magetometers by definition produce unit vectors and
  // have their axes as (x,y,z) so we just have to specify
  // the name here and the others are all set to pass the
  // values through.
  d_magnetometer.params.name = params->d_magnetometer_name;
  for (int i = 0; i < 3; i++) {
    d_magnetometer.params.channels[i] = i;
    d_magnetometer.params.offsets[i] = 0;
    d_magnetometer.params.scales[i] = 1;
  }

  //--------------------------------------------------------------------
  // Open analog remotes for any channels that have non-NULL names.
  // If the name starts with the "*" character, use tracker
  // connection rather than getting a new connection for it.
  // Set up callbacks to handle updates to the analog values.
  setup_vector(&d_acceleration, handle_analog_update);
  setup_vector(&d_rotational_vel, handle_analog_update);
  setup_vector(&d_magnetometer, handle_analog_update);

  //--------------------------------------------------------------------
  // Set the current timestamp to "now", the current orientation to identity
  // and the angular rotation velocity to zero.
  vrpn_gettimeofday(&d_prevtime, NULL);
  d_prev_update_time = d_prevtime;
  vrpn_Tracker::timestamp = d_prevtime;
  q_vec_set(pos, 0, 0, 0);
  q_from_axis_angle(d_quat, 0, 0, 1, 0);
  q_vec_set(vel, 0, 0, 0);
  q_from_axis_angle(vel_quat, 0, 0, 1, 0);
  vel_quat_dt = 1;
}

vrpn_IMU_SimpleCombiner::~vrpn_IMU_SimpleCombiner(void)
{
  // Tear down the analog update callbacks and remotes
  teardown_vector(&d_acceleration, handle_analog_update);
  teardown_vector(&d_rotational_vel, handle_analog_update);
  teardown_vector(&d_magnetometer, handle_analog_update);
}

// This routine handles updates of the analog values. The value coming in is
// adjusted per the parameters in the axis description, and then used to
// update the value there. The value is used by the device code in
// mainloop() to update its internal state; that work is not done here.

void	VRPN_CALLBACK vrpn_IMU_SimpleCombiner::handle_analog_update
(void *userdata, const vrpn_ANALOGCB info)
{
  vrpn_IMU_Vector	*vector = static_cast<vrpn_IMU_Vector *>(userdata);
  for (size_t i = 0; i < 3; i++) {
    vector->values[i] =
      (info.channel[vector->params.channels[i]] - vector->params.offsets[i])
      * vector->params.scales[i];
  }

  // Store the time we got the value.
  vector->time = info.msg_time;
}

// This sets up the Analog Remote for one channel, setting up the callback
// needed to adjust the value based on changes in the analog input.
// Returns 0 on success and -1 on failure.

int	vrpn_IMU_SimpleCombiner::setup_vector(vrpn_IMU_Vector *vector,
  vrpn_ANALOGCHANGEHANDLER f)
{
  // If the name is empty, we're done.
  if (vector->params.name.size() == 0) { return 0; }

  // Open the analog device and point the remote at it.
  // If the name starts with the '*' character, use the server
  // connection rather than making a new one.
  try {
    if (vector->params.name[0] == '*') {
      vector->ana = new vrpn_Analog_Remote(vector->params.name.c_str() + 1,
        d_connection);
#ifdef	VERBOSE
      printf("vrpn_Tracker_AnalogFly: Adding local analog %s\n",
        vector->params.name.c_str() + 1);
#endif
    } else {
      vector->ana = new vrpn_Analog_Remote(vector->params.name.c_str());
#ifdef	VERBOSE
      printf("vrpn_Tracker_AnalogFly: Adding remote analog %s\n",
        vector->params.name.c_str());
#endif
    }
  } catch (...) { vector->ana = NULL; }
  if (vector->ana == NULL) {
    fprintf(stderr, "vrpn_Tracker_AnalogFly: "
      "Can't open Analog %s\n", vector->params.name.c_str());
    return -1;
  }

  // Set up the callback handler for the channel
  return vector->ana->register_change_handler(vector, f);
}

// This tears down the Analog Remote for one channel, undoing everything that
// the setup did. Returns 0 on success and -1 on failure.

int	vrpn_IMU_SimpleCombiner::teardown_vector(vrpn_IMU_Vector *vector,
  vrpn_ANALOGCHANGEHANDLER f)
{
  int	ret;

  // If the analog pointer is NULL, we're done.
  if (vector->ana == NULL) { return 0; }

  // Turn off the callback handler for the channel
  ret = vector->ana->unregister_change_handler((void*)vector, f);

  // Delete the analog device.
  try {
    delete vector->ana;
  } catch (...) {
    fprintf(stderr, "vrpn_IMU_SimpleCombiner::teardown_vector(): delete failed\n");
    return -1;
  }

  return ret;
}

void vrpn_IMU_SimpleCombiner::mainloop()
{
  // Call generic server mainloop, since we are a server
  server_mainloop();

  // Mainloop() all of the analogs that are defined and the button
  // so that we will get all of the values fresh.
  if (d_acceleration.ana != NULL) { d_acceleration.ana->mainloop(); }
  if (d_rotational_vel.ana != NULL) { d_rotational_vel.ana->mainloop(); }
  if (d_magnetometer.ana != NULL) { d_magnetometer.ana->mainloop(); }

  // Update the matrix based on the change in time since the last
  // update and the current values.
  struct timeval	now;
  vrpn_gettimeofday(&now, NULL);
  double delta_t = vrpn_TimevalDurationSeconds(now, d_prev_update_time);
  update_matrix_based_on_values(delta_t);
  d_prev_update_time = now;

  // See if it has been long enough since our last report.
  // If so, generate a new one.
  double interval = vrpn_TimevalDurationSeconds(now, d_prevtime);
  if (interval >= d_update_interval) {

    // pack and deliver tracker report with info from the current matrix;
    if (d_connection) {
      char	msgbuf[1000];
      int	len = encode_to(msgbuf);
      if (d_connection->pack_message(len, now,
        position_m_id, d_sender_id, msgbuf,
        vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr, "vrpn_IMU_SimpleCombiner: "
          "cannot write pose message: tossing\n");
      }
      len = encode_vel_to(msgbuf);
      if (d_connection->pack_message(len, now,
        velocity_m_id, d_sender_id, msgbuf,
        vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr, "vrpn_IMU_SimpleCombiner: "
          "cannot write velocity message: tossing\n");
      }
    } else {
      fprintf(stderr, "vrpn_IMU_SimpleCombiner: "
        "No valid connection\n");
    }

    // We just sent a report, so reset the time
    d_prevtime = now;
  }
}

// This routine will update the current matrix based on the current values
// in the offsets list for each axis, and the length of time over which the
// action is taking place (time_interval).

void	vrpn_IMU_SimpleCombiner::update_matrix_based_on_values(double time_interval)
{
  //==================================================================
  // Adjust the orientation based on the rotational velocity, which is
  // measured in the rotated coordinate system.  We need to rotate the
  // difference vector back to the canonical orientation, apply the
  // orientation change there, and then rotate back.
  // Be sure to scale by the time value.
  q_type forward, inverse;
  q_copy(forward, d_quat);
  q_invert(inverse, forward);
  // Remember that Euler angles in Quatlib have rotation around Z in
  // the first term.  Compute the time-scaled delta transform in
  // canonical space.
  q_type delta;
  q_from_euler(delta,
    time_interval * d_rotational_vel.values[Q_Z],
    time_interval * d_rotational_vel.values[Q_Y],
    time_interval * d_rotational_vel.values[Q_X]);
  // Bring the delta back into canonical space
  q_type canonical;
  q_mult(canonical, delta, inverse);
  q_mult(canonical, forward, canonical);
  q_mult(d_quat, canonical, d_quat);

  //==================================================================
  // To the extent that the acceleration vector's magnitude is equal
  // to the expected gravity, rotate the orientation so that the vector
  // points downward.  This is measured in the rotated coordinate system,
  // so we need to rotate back to canonical orientation and apply
  // the difference there and then rotate back.  The rate of rotation
  // should be as specified in the gravity-rotation-rate parameter so
  // we don't swing the head around too quickly but only slowly re-adjust.

  double accel = sqrt(
      d_acceleration.values[0] * d_acceleration.values[0] +
      d_acceleration.values[1] * d_acceleration.values[1] +
      d_acceleration.values[2] * d_acceleration.values[2] );
  double diff = fabs(accel - 9.80665);

  // Only adjust if we're close enough to the expected gravity
  // @todo In a more complicated IMU tracker, base this on state and
  // error estimates from a Kalman or other filter.
  double scale = 1.0 - diff;
  if (scale > 0) {
    // Get a new forward and inverse matrix from the current, just-
    // rotated matrix.
    q_copy(forward, d_quat);

    // Change how fast we adjust based on how close we are to the
    // expected value of gravity.  Then further scale this by the
	// amount of time since the last estimate.
    double gravity_scale = scale * d_gravity_restore_rate * time_interval;

	// Rotate the gravity vector by the estimated transform.
	// We expect this direction to match the global down (-Y) vector.
	q_vec_type gravity_global;
	q_vec_set(gravity_global, d_acceleration.values[0],
		d_acceleration.values[1], d_acceleration.values[2]);
	q_vec_normalize(gravity_global, gravity_global);
	q_xform(gravity_global, forward, gravity_global);
	//printf("  XXX Gravity: %lg, %lg, %lg\n", gravity_global[0], gravity_global[1], gravity_global[2]);

	// Determine the rotation needed to take gravity and rotate
	// it into the direction of -Y.
	q_vec_type neg_y;
	q_vec_set(neg_y, 0, -1, 0);
	q_type rot;
	q_from_two_vecs(rot, gravity_global, neg_y);

	// Scale the rotation by the fraction of the orientation we
	// should remove based on the time that has passed, how well our
	// gravity vector matches expected, and the specified rate of
	// correction.
	static q_type identity = { 0, 0, 0, 1 };
	q_type scaled_rot;
	q_slerp(scaled_rot, identity, rot, gravity_scale);
	//printf("XXX Scaling gravity vector by %lg\n", gravity_scale);

    // Rotate by this angle.
    q_mult(d_quat, scaled_rot, d_quat);

    //==================================================================
    // If we are getting compass data, and to the extent that the
    // acceleration vector's magnitude is equal to the expected gravity,
    // compute the cross product of the cross product to find the
    // direction of north perpendicular to down.  This is measured in
    // the rotated coordinate system, so we need to rotate back to the
    // canonical orientation and do the comparison there.
	//  The fraction of rotation should be as specified in the
    // magnetometer-rotation-rate parameter so we don't swing the head
    // around too quickly but only slowly re-adjust.

    if (d_magnetometer.ana != NULL) {
      // Get a new forward and inverse matrix from the current, just-
      // rotated matrix.
      q_copy(forward, d_quat);

      // Find the North vector that is perpendicular to gravity by
      // clearing its Y axis to zero and renormalizing.
	  q_vec_type magnetometer;
      q_vec_set(magnetometer, d_magnetometer.values[0],
        d_magnetometer.values[1], d_magnetometer.values[2]);
	  q_vec_type magnetometer_global;
	  q_xform(magnetometer_global, forward, magnetometer);
	  magnetometer_global[Q_Y] = 0;
	  q_vec_type north_global;
	  q_vec_normalize(north_global, magnetometer_global);
	  //printf("  XXX north_global: %lg, %lg, %lg\n", north_global[0], north_global[1], north_global[2]);

      // Determine the rotation needed to take north and rotate it
	  // into the direction of negative Z.
	  q_vec_type neg_z;
	  q_vec_set(neg_z, 0, 0, -1);
	  q_from_two_vecs(rot, north_global, neg_z);

	  // Change how fast we adjust based on how close we are to the
	  // expected value of gravity.  Then further scale this by the
	  // amount of time since the last estimate.
	  double north_rate = scale * d_north_restore_rate * time_interval;

	  // Scale the rotation by the fraction of the orientation we
	  // should remove based on the time that has passed, how well our
	  // gravity vector matches expected, and the specified rate of
	  // correction.
	  static q_type identity = { 0, 0, 0, 1 };
	  q_slerp(scaled_rot, identity, rot, north_rate);

      // Rotate by this angle.
      q_mult(d_quat, scaled_rot, d_quat);
    }
  }

  //==================================================================
  // Compute and store the velocity information
  // This will be in the rotated coordinate system, so we need to
  // rotate back to the identity orientation before storing.
  // Convert from radians/second into a quaternion rotation as
  // rotated in a hundredth of a second and set the rotational
  // velocity dt to a hundredth of a second so that we don't
  // risk wrapping.
  // Remember that Euler angles in Quatlib have rotation around Z in
  // the first term.  Compute the time-scaled delta transform in
  // canonical space.
  q_from_euler(delta,
    1e-2 * d_rotational_vel.values[Q_Z],
    1e-2 * d_rotational_vel.values[Q_Y],
    1e-2 * d_rotational_vel.values[Q_X]);
  // Bring the delta back into canonical space
  q_mult(canonical, delta, inverse);
  q_mult(vel_quat, forward, canonical);
  vel_quat_dt = 1e-2;
}

