//XXX Send velocity and rotational velocity reports.

#include <math.h>                       // for pow, fabs

#include "quat.h"                       // for q_matrix_copy, etc
#include "vrpn_Connection.h"            // for vrpn_Connection, etc
#include "vrpn_Tracker_ButtonFly.h"

#undef	VERBOSE

vrpn_Tracker_ButtonFly::vrpn_Tracker_ButtonFly
         (const char * name, vrpn_Connection * trackercon,
          vrpn_Tracker_ButtonFlyParam * params, float update_rate,
	  bool reportChanges) :
	vrpn_Tracker (name, trackercon),
	d_update_interval (update_rate ? (1/update_rate) : 1.0),
        d_reportChanges (reportChanges),
	d_vel_scale(NULL),
	d_vel_scale_value(1.0),
	d_rot_scale(NULL),
        d_rot_scale_value(1.0)
{
  int i;

  //--------------------------------------------------------------------
  // Copy the parameter values and initialize the values and pointers
  // in the list of axes.  The setup_channel() call opens the button
  // remote and sets up a callback to handle the changes.
  d_num_axes = params->num_axes;
  for (i = 0; i < params->num_axes; i++) {
    d_axes[i].axis = params->axes[i];
    d_axes[i].active = false;
    d_axes[i].bf = this;
    setup_channel(&d_axes[i]);
  }

  //--------------------------------------------------------------------
  // Open the scale analogs if they have non-NULL, non-empty names.
  // If the name starts with the "*" character, use tracker
  //      connection rather than getting a new connection for it.
  // Set up a callback for each to set the scale factor.

  if (params->vel_scale_name[0] != '\0') {

    // Copy the parameters into our member variables
    d_vel_scale_channel = params->vel_scale_channel;
    d_vel_scale_offset = params->vel_scale_offset;
    d_vel_scale_scale = params->vel_scale_scale;
    d_vel_scale_power = params->vel_scale_power;

    // Open the analog device and point the remote at it.
    // If the name starts with the '*' character, use
    // the server connection rather than making a new one.
    if (params->vel_scale_name[0] == '*') {
      try {
        d_vel_scale = new vrpn_Analog_Remote
        (&(params->vel_scale_name[1]), d_connection);
      } catch (...) { d_vel_scale = NULL; }
    } else {
      try {
        d_vel_scale = new vrpn_Analog_Remote(params->vel_scale_name);
        } catch (...) { d_vel_scale = NULL; }
    }

    // Set up the callback handler
    if (d_vel_scale == NULL) {
      fprintf(stderr,"vrpn_Tracker_ButtonFly: "
           "Can't open Analog %s\n",params->vel_scale_name);
    } else {
      // Set up the callback handler for the channel
      d_vel_scale->register_change_handler(this, handle_velocity_update);
    }
  }

  if (params->rot_scale_name[0] != '\0') {

    // Copy the parameters into our member variables
    d_rot_scale_channel = params->rot_scale_channel;
    d_rot_scale_offset = params->rot_scale_offset;
    d_rot_scale_scale = params->rot_scale_scale;
    d_rot_scale_power = params->rot_scale_power;

    // Open the analog device and point the remote at it.
    // If the name starts with the '*' character, use
    // the server connection rather than making a new one.
    if (params->rot_scale_name[0] == '*') {
      try {
        d_rot_scale = new vrpn_Analog_Remote
        (&(params->rot_scale_name[1]), d_connection);
      } catch (...) { d_rot_scale = NULL; }
    } else {
      try {
        d_rot_scale = new vrpn_Analog_Remote(params->rot_scale_name);
      } catch (...) { d_rot_scale = NULL; }
    }

    // Set up the callback handler
    if (d_rot_scale == NULL) {
      fprintf(stderr,"vrpn_Tracker_ButtonFly: "
           "Can't open Analog %s\n",params->rot_scale_name);
    } else {
      // Set up the callback handler for the channel
      d_rot_scale->register_change_handler(this, handle_rotation_update);
    }
  }

  //--------------------------------------------------------------------
  // Whenever we get the first connection to this server, we also
  // want to reset the matrix to identity, so that you start at the
  // beginning. Set up a handler to do this.
  register_autodeleted_handler(d_connection->register_message_type
                                      (vrpn_got_first_connection),
		handle_newConnection, this);

  //--------------------------------------------------------------------
  // Set the initialization matrix to identity, then also set
  // the current matrix to identity.
  for ( i =0; i< 4; i++)
	  for (int j=0; j< 4; j++) 
		  d_initMatrix[i][j] = 0;

  d_initMatrix[0][0] = d_initMatrix[1][1] = d_initMatrix[2][2] =
                     d_initMatrix[3][3] = 1.0;
  reset();
}

vrpn_Tracker_ButtonFly::~vrpn_Tracker_ButtonFly (void)
{
  int i;

  // Tear down the button axes (which will include the
  // button callbacks and remotes).
  for (i = 0; i < d_num_axes; i++) {
    teardown_channel(&d_axes[i]);
  }

  // Tear down the analog update callbacks and remotes (if they exist)
  if (d_vel_scale != NULL) {
    d_vel_scale->unregister_change_handler(this, handle_velocity_update);
    try {
      delete d_vel_scale;
    } catch (...) {
      fprintf(stderr, "vrpn_Tracker_ButtonFly::~vrpn_Tracker_ButtonFly(): delete failed\n");
      return;
    }
  }
  if (d_rot_scale != NULL) {
    d_rot_scale->unregister_change_handler(this, handle_rotation_update);
    try {
      delete d_rot_scale;
    } catch (...) {
      fprintf(stderr, "vrpn_Tracker_ButtonFly::~vrpn_Tracker_ButtonFly(): delete failed\n");
      return;
    }
  }
}

// This sets up the Button Remote for one channel, setting up the callback
// needed to adjust the value based on changes in the button.
// Returns 0 on success and -1 on failure.

int	vrpn_Tracker_ButtonFly::setup_channel(vrpn_TBF_fullaxis *full)
{
  // Open the button device and point the remote at it.
  // If the name starts with the '*' character, use the server
  // connection rather than making a new one.
  if (full->axis.name[0] == '*') {
    try {
      full->btn = new vrpn_Button_Remote(&(full->axis.name[1]),
          d_connection);
    } catch (...) { full->btn = NULL; }
#ifdef	VERBOSE
    printf("vrpn_Tracker_ButtonFly: Adding local button %s\n",
              &(full->axis.name[1]));
#endif
  } else {
    try {
      full->btn = new vrpn_Button_Remote(full->axis.name);
    } catch (...) { full->btn = NULL; }
#ifdef	VERBOSE
    printf("vrpn_Tracker_ButtonFly: Adding remote button %s\n",
              full->axis.name);
#endif
  }
  if (full->btn == NULL) {
    fprintf(stderr,"vrpn_Tracker_ButtonFly: "
            "Can't open Button %s\n",full->axis.name);
    return -1;
  }

  // Set up the callback handler for the channel
  return full->btn->register_change_handler(full, handle_button_update);
}

// This tears down the Button Remote for one channel, undoing everything that
// the setup did. Returns 0 on success and -1 on failure.

int	vrpn_Tracker_ButtonFly::teardown_channel(vrpn_TBF_fullaxis *full)
{
  int	ret;

  // If the button pointer is NULL, we're done.
  if (full->btn == NULL) { return 0; }

  // Turn off the callback handler for the channel
  ret = full->btn->unregister_change_handler((void*)full, handle_button_update);

  // Delete the analog device and point the remote at it.
  try {
    delete full->btn;
  } catch (...) {
    fprintf(stderr, "vrpn_Tracker_ButtonFly::teardown_channel(): delete failed\n");
    return -1;
  }

  return ret;
}
 
// This routine handles updates of the velocity-scale value. The value coming in is
// adjusted per the parameters in the member variables, and then used to
// update the value there. The value is used by the matrix-generation code in
// mainloop() to update the transformations; that work is not done here.

void	vrpn_Tracker_ButtonFly::handle_velocity_update
                     (void *userdata, const vrpn_ANALOGCB info)
{
  vrpn_Tracker_ButtonFly *me = (vrpn_Tracker_ButtonFly *)userdata;
  double value = info.channel[me->d_vel_scale_channel];
  double value_offset = value - me->d_vel_scale_offset;
  double value_scaled = value_offset * me->d_vel_scale_scale;
  double value_abs = fabs(value_scaled);
  double value_powered;

  // Scale and apply the power to the value (maintaining its sign)
  if (value_offset >=0) {
	  value_powered = pow(value_abs, (double) me->d_vel_scale_power);
  } else {
	  value_powered = -pow(value_abs, (double) me->d_vel_scale_power);
  }

  // Store the value for use by the matrix code
  me->d_vel_scale_value = (float)value_powered;
}

// This routine handles updates of the rotational-scale value. The value coming in is
// adjusted per the parameters in the member variables, and then used to
// update the value there. The value is used by the matrix-generation code in
// mainloop() to update the transformations; that work is not done here.

void	vrpn_Tracker_ButtonFly::handle_rotation_update
                     (void *userdata, const vrpn_ANALOGCB info)
{
  vrpn_Tracker_ButtonFly *me = (vrpn_Tracker_ButtonFly *)userdata;
  double value = info.channel[me->d_rot_scale_channel];
  double value_offset = value - me->d_rot_scale_offset;
  double value_scaled = value_offset * me->d_rot_scale_scale;
  double value_abs = fabs(value_scaled);
  double value_powered;

  // Scale and apply the power to the value (maintaining its sign)
  if (value_offset >=0) {
	  value_powered = pow(value_abs, (double) me->d_rot_scale_power);
  } else {
	  value_powered = -pow(value_abs, (double) me->d_rot_scale_power);
  }

  // Store the value for use by the matrix code
  me->d_rot_scale_value = (float)value_powered;
}

// This routine will handle a button being pressed.  For absolute
// buttons, it stores the value directly into the matrix when the
// button is pressed.  For differential channels, it adds marks
// the channel as active so that its difference will be included
// the the total difference computation.

void vrpn_Tracker_ButtonFly::handle_button_update
                     (void *userdata, const vrpn_BUTTONCB info)
{
  vrpn_TBF_fullaxis *axis = (vrpn_TBF_fullaxis*)userdata;

  // If this is not the correct button for this axis, return
  if (axis->axis.channel != info.button) {
    return;
  }

  // If this is an absolute axis, and the button has just been pressed,
  // then set the matrix to the one for this button.
  if (axis->axis.absolute) {
    if (info.state == 1) {
      double  tx,ty,tz, rx,ry,rz;	  //< Translation and rotation to set to
      q_matrix_type newMatrix;	  //< Matrix set to these values.

      // compute the translation and rotation
      tx = axis->axis.vec[0];
      ty = axis->axis.vec[1];
      tz = axis->axis.vec[2];

      rx = axis->axis.rot[0] * (2*VRPN_PI);
      ry = axis->axis.rot[1] * (2*VRPN_PI);
      rz = axis->axis.rot[2] * (2*VRPN_PI);

      // Build a rotation matrix, then add in the translation
      q_euler_to_col_matrix(newMatrix, rz, ry, rx);
      newMatrix[3][0] = tx; newMatrix[3][1] = ty; newMatrix[3][2] = tz;

      // Copy the new matrix to the current matrix
      // and then update the tracker based on it
      q_matrix_copy(axis->bf->d_currentMatrix, newMatrix);
      axis->bf->convert_matrix_to_tracker();

      // Mark the axis as active, so that a report will be generated
      // next time.  For absolute channels, this is marked inactive when
      // the report based on it is generated.
      axis->active = true;
    }

  // This is a differential axis, so mark it as active or not
  // depending on whether the button was pressed or not.
  } else {
    axis->active = (info.state != 0);

    // XXX To be strictly correct, we should record the time at which
    // the activity started so that it can take effect between update
    // intervals.
  }
}

// static
int vrpn_Tracker_ButtonFly::handle_newConnection(void * userdata,
                                                 vrpn_HANDLERPARAM)
{
     
  printf("Get a new connection, reset virtual_Tracker\n");
  ((vrpn_Tracker_ButtonFly *) userdata)->reset();

  // Always return 0 here, because nonzero return means that the input data
  // was garbage, not that there was an error. If we return nonzero from a
  // vrpn_Connection handler, it shuts down the connection.
  return 0;
}

/** Reset the current matrix to zero and store it into the tracker
    position/quaternion location.
*/

void vrpn_Tracker_ButtonFly::reset (void)
{
  // Set the matrix back to the identity matrix
  q_matrix_copy(d_currentMatrix, d_initMatrix);
  vrpn_gettimeofday(&d_prevtime, NULL);

  // Convert the matrix into quaternion notation and copy into the
  // tracker pos and quat elements.
  convert_matrix_to_tracker();
}

void vrpn_Tracker_ButtonFly::mainloop()
{
  int i;
  struct timeval  now;
  double	  interval;	// How long since the last report, in secs

  // Call generic server mainloop, since we are a server
  server_mainloop();

  // Mainloop() all of the buttons that are defined and the analog
  // scale values if they are defined so that we will get all of
  // the values fresh.
  for (i = 0; i < d_num_axes; i++) {
    d_axes[i].btn->mainloop();
  }
  if (d_vel_scale != NULL) { d_vel_scale->mainloop(); };
  if (d_rot_scale != NULL) { d_rot_scale->mainloop(); };

  // See if it has been long enough since our last report.
  // If so, generate a new one.
  vrpn_gettimeofday(&now, NULL);
  interval = vrpn_TimevalDurationSeconds(now, d_prevtime);

  if (shouldReport(interval)) {
    // Figure out the new matrix based on the current values and
    // the length of the interval since the last report
    update_matrix_based_on_values(interval);
    d_prevtime = now;

    // Set the time on the report to now.
    vrpn_Tracker::timestamp = now;

    // pack and deliver tracker report;
    if (d_connection) {
      char	msgbuf[1000];
      int	len = encode_to(msgbuf);
      if (d_connection->pack_message(len, vrpn_Tracker::timestamp,
	      position_m_id, d_sender_id, msgbuf,
	      vrpn_CONNECTION_LOW_LATENCY)) {
      fprintf(stderr,"Tracker ButtonFly: cannot write message: tossing\n");
      }
    } else {
      fprintf(stderr,"Tracker ButtonFly: No valid connection\n");
    }

  // We're not always sending reports, but we still want to
  // update the matrix so that we don't integrate over
  // too long a timespan when we do finally report a change.
  } else if (interval >= d_update_interval) {
    // Figure out the new matrix based on the current values and
    // the length of the interval since the last report
    update_matrix_based_on_values(interval);
    d_prevtime = now;
  }
}

// This routine will update the current matrix based on the current values
// in the offsets list for each axis, and the length of time over which the
// action is taking place (time_interval).
// Handling of non-absolute trackers: It treats the values as either
// meters/second or else rotations/second to be integrated over the interval
// to adjust the current matrix.  All active differential axis are added
// together, and then the results are scaled by the velocity and rotational
// analog scale factors.
// Handling of absolute trackers: Their effects are handled in the button
// callback and in the shouldReport methods.
// XXX Later, it would be cool to have non-absolute trackers send velocity
// information as well, since it knows what this is.

void	vrpn_Tracker_ButtonFly::update_matrix_based_on_values
                  (double time_interval)
{
  double tx,ty,tz, rx,ry,rz;	// Translation (m/s) and rotation (rad/sec)
  q_matrix_type diffM;		// Difference (delta) matrix
  int i;

  // Set the translation and rotation to zero, then go through all of the
  // axis and sum in the contributions from any that are differential and
  // active.
  tx = ty = tz = rx = ry = rz = 0.0;

  for (i = 0; i < d_num_axes; i++) {
    if (d_axes[i].active && !d_axes[i].axis.absolute) {
      tx += d_axes[i].axis.vec[0] * time_interval * d_vel_scale_value;
      ty += d_axes[i].axis.vec[1] * time_interval * d_vel_scale_value;
      tz += d_axes[i].axis.vec[2] * time_interval * d_vel_scale_value;
  
      rx = d_axes[i].axis.rot[0] * time_interval * (2*VRPN_PI) * d_rot_scale_value;
      ry = d_axes[i].axis.rot[1] * time_interval * (2*VRPN_PI) * d_rot_scale_value;
      rz = d_axes[i].axis.rot[2] * time_interval * (2*VRPN_PI) * d_rot_scale_value;
    }
  }

  // Build a rotation matrix, then add in the translation
  q_euler_to_col_matrix(diffM, rz, ry, rx);
  diffM[3][0] = tx; diffM[3][1] = ty; diffM[3][2] = tz;

  // Multiply the current matrix by the difference matrix to update
  // it to the current time. Then convert the matrix into a pos/quat
  // and copy it into the tracker position and quaternion structures.
  q_matrix_type final;
  q_matrix_mult(final, diffM, d_currentMatrix);
  q_matrix_copy(d_currentMatrix, final);
  convert_matrix_to_tracker();
}

void vrpn_Tracker_ButtonFly::convert_matrix_to_tracker (void)
{
  q_xyz_quat_type xq;
  int i;

  q_row_matrix_to_xyz_quat( & xq, d_currentMatrix);

  for (i=0; i< 3; i++) {
    pos[i] = xq.xyz[i]; // position;
  }
  for (i=0; i< 4; i++) {
    d_quat[i] = xq.quat[i]; // orientation. 
  }
}

bool vrpn_Tracker_ButtonFly::shouldReport
                  (double elapsedInterval) {
  int i;
  bool	found_any;

  // If we come across an absolute channel that is active,
  // we'll want to report.  We also want to reset that channel's
  // activity to false.  Go through the whole list to make sure
  // we handle all of them at once.
  found_any = false;
  for (i = 0; i < d_num_axes; i++) {
    if (d_axes[i].active && d_axes[i].axis.absolute) {
      found_any = true;
      d_axes[i].active = false;
    }
  }
  if (found_any) {
    return VRPN_TRUE;
  }

  // If we haven't had enough time pass yet, don't report.
  if (elapsedInterval < d_update_interval) {
    return VRPN_FALSE;
  }

  // If we're sending a report every interval, regardless of
  // whether or not there are changes, then send one now.
  if (!d_reportChanges) {
    return VRPN_TRUE;
  }

  // If any differential channels are active, send the report.
  found_any = false;
  for (i = 0; i < d_num_axes; i++) {
    if (d_axes[i].active && !d_axes[i].axis.absolute) {
      found_any = true;
    }
  }
  if (found_any) {
    return VRPN_TRUE;
  }

  // Enough time has elapsed, but nothing has changed, so return false.
  return VRPN_FALSE;
}

