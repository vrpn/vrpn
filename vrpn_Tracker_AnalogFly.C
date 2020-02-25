#include <math.h>                       // for pow, fabs

#include "quat.h"                       // for q_xyz_quat_type, etc
#include "vrpn_Connection.h"            // for vrpn_Connection, etc
#include "vrpn_Tracker_AnalogFly.h"

#undef	VERBOSE

vrpn_Tracker_AnalogFly::vrpn_Tracker_AnalogFly
         (const char * name, vrpn_Connection * trackercon,
          vrpn_Tracker_AnalogFlyParam * params, float update_rate,
          bool absolute, bool reportChanges,
          bool worldFrame) :
	vrpn_Tracker (name, trackercon),
	d_update_interval (update_rate ? (1/update_rate) : 1.0),
	d_absolute (absolute),
	d_reportChanges (reportChanges),
	d_worldFrame (worldFrame),
	d_reset_button(NULL),
	d_which_button (params->reset_which),
	d_clutch_button(NULL),
	d_clutch_which (params->clutch_which),
	d_clutch_engaged(false),
	d_clutch_was_off(false)
{
	int i;

	d_x.axis = params->x; d_y.axis = params->y; d_z.axis = params->z;
	d_sx.axis = params->sx; d_sy.axis = params->sy; d_sz.axis = params->sz;

	d_x.ana = d_y.ana = d_z.ana = NULL;
	d_sx.ana = d_sy.ana = d_sz.ana = NULL;

	d_x.value = d_y.value = d_z.value = 0.0;
	d_sx.value = d_sy.value = d_sz.value = 0.0;

	d_x.af = this; d_y.af = this; d_z.af = this;
	d_sx.af = this; d_sy.af = this; d_sz.af = this;

	//--------------------------------------------------------------------
	// Open analog remotes for any channels that have non-NULL names.
	// If the name starts with the "*" character, use tracker
        //      connection rather than getting a new connection for it.
	// Set up callbacks to handle updates to the analog values
	setup_channel(&d_x);
	setup_channel(&d_y);
	setup_channel(&d_z);
	setup_channel(&d_sx);
	setup_channel(&d_sy);
	setup_channel(&d_sz);

	//--------------------------------------------------------------------
	// Open the reset button if is has a non-NULL name.
	// If the name starts with the "*" character, use tracker
        // connection rather than getting a new connection for it.
	// Set up callback for it to reset the matrix to identity.

	// If the name is NULL, don't do anything.
	if (params->reset_name != NULL) {

		// Open the button device and point the remote at it.
		// If the name starts with the '*' character, use
                // the server connection rather than making a new one.
		if (params->reset_name[0] == '*') {
                  try {
                    d_reset_button = new vrpn_Button_Remote
                    (&(params->reset_name[1]),
                      d_connection);
                  } catch (...) {
                    d_reset_button = NULL;
                  }
		} else {
                  try {
		    d_reset_button = new vrpn_Button_Remote
                      (params->reset_name);
                  } catch (...) {
                    d_reset_button = NULL;
                  }
		}
		if (d_reset_button == NULL) {
			fprintf(stderr,"vrpn_Tracker_AnalogFly: "
                             "Can't open Button %s\n",params->reset_name);
		} else {
			// Set up the callback handler for the channel
			d_reset_button->register_change_handler
                             (this, handle_reset_press);
		}
	}

	//--------------------------------------------------------------------
	// Open the clutch button if is has a non-NULL name.
	// If the name starts with the "*" character, use tracker
        // connection rather than getting a new connection for it.
	// Set up callback for it to control clutching.

	// If the name is NULL, don't do anything.
	if (params->clutch_name != NULL) {

		// Open the button device and point the remote at it.
		// If the name starts with the '*' character, use
                // the server connection rather than making a new one.
		if (params->clutch_name[0] == '*') {
                  try {
                    d_clutch_button = new vrpn_Button_Remote
                               (&(params->clutch_name[1]),
				d_connection);
                  } catch (...) {
                    d_clutch_button = NULL;
                  }
                } else {
                  try {
                    d_clutch_button = new vrpn_Button_Remote
                               (params->clutch_name);
                  } catch (...) {
                    d_clutch_button = NULL;
                  }
		}
		if (d_clutch_button == NULL) {
			fprintf(stderr,"vrpn_Tracker_AnalogFly: "
                             "Can't open Button %s\n",params->clutch_name);
		} else {
			// Set up the callback handler for the channel
			d_clutch_button->register_change_handler
                             (this, handle_clutch_press);
		}
	}

        // If the clutch button is NULL, then engage the clutch always.
        if (params->clutch_name == NULL) {
          d_clutch_engaged = true;
        }

	//--------------------------------------------------------------------
	// Whenever we get the first connection to this server, we also
        // want to reset the matrix to identity, so that you start at the
        // beginning. Set up a handler to do this.
	register_autodeleted_handler(d_connection->register_message_type(vrpn_got_first_connection),
		      handle_newConnection, this);

	//--------------------------------------------------------------------
	// Set the initialization matrix to identity, then also set
        // the current matrix to identity and the clutch matrix to
        // identity.
        for ( i =0; i< 4; i++) {
          for (int j=0; j< 4; j++)  {
	    d_initMatrix[i][j] = 0;
          }
        }

	d_initMatrix[0][0] = d_initMatrix[1][1] = d_initMatrix[2][2] =
                             d_initMatrix[3][3] = 1.0;
	reset();
        q_matrix_copy(d_clutchMatrix, d_initMatrix);
        q_matrix_copy(d_currentMatrix, d_initMatrix);

	//--------------------------------------------------------------------
	// Set the current timestamp to "now" and current matrix to identity
	// for absolute trackers.  This is done in case we never hear from the
	// analog devices.  Reset doesn't do this for absolute trackers.
	if (d_absolute) {
	    vrpn_gettimeofday(&d_prevtime, NULL);
	    vrpn_Tracker::timestamp = d_prevtime;
	    q_matrix_copy(d_currentMatrix, d_initMatrix);
	    convert_matrix_to_tracker();
	}
}

vrpn_Tracker_AnalogFly::~vrpn_Tracker_AnalogFly (void)
{
	// Tear down the analog update callbacks and remotes
	teardown_channel(&d_x);
	teardown_channel(&d_y);
	teardown_channel(&d_z);
	teardown_channel(&d_sx);
	teardown_channel(&d_sy);
	teardown_channel(&d_sz);

	// Tear down the reset button update callback and remote (if there is one)
	if (d_reset_button != NULL) {
		d_reset_button->unregister_change_handler(this,
                                        handle_reset_press);
                try {
                  delete d_reset_button;
                } catch (...) {
                  fprintf(stderr, "vrpn_Tracker_AnalogFly::~vrpn_Tracker_AnalogFly(): delete failed\n");
                  return;
                }
	}

	// Tear down the clutch button update callback and remote (if there is one)
	if (d_clutch_button != NULL) {
		d_clutch_button->unregister_change_handler(this,
                                        handle_clutch_press);
                try {
                  delete d_clutch_button;
                } catch (...) {
                  fprintf(stderr, "vrpn_Tracker_AnalogFly::~vrpn_Tracker_AnalogFly(): delete failed\n");
                  return;
                }
	}
}

// This routine handles updates of the analog values. The value coming in is
// adjusted per the parameters in the full axis description, and then used to
// update the value there. The value is used by the matrix-generation code in
// mainloop() to update the transformations; that work is not done here.

void	VRPN_CALLBACK vrpn_Tracker_AnalogFly::handle_analog_update
                     (void *userdata, const vrpn_ANALOGCB info)
{
	vrpn_TAF_fullaxis	*full = (vrpn_TAF_fullaxis *)userdata;
	double value = info.channel[full->axis.channel];
	double value_offset = value - full->axis.offset;
	double value_abs = fabs(value_offset);

	// If we're an absolute channel, store the time of the report
	// into the tracker's timestamp field.
	if (full->af->d_absolute) {
	    full->af->vrpn_Tracker::timestamp = info.msg_time;
	}

	// If we're not above threshold, store zero and we're done!
	if (value_abs <= full->axis.thresh) {
		full->value = 0.0;
		return;
	}

	// Scale and apply the power to the value (maintaining its sign)
	if (value_offset >=0) {
		full->value = pow(value_offset*full->axis.scale, (double) full->axis.power);
	} else {
		full->value = -pow(value_abs*full->axis.scale, (double) full->axis.power);
	}
}

// This routine will reset the matrix to identity when the reset button is
// pressed.

void VRPN_CALLBACK vrpn_Tracker_AnalogFly::handle_reset_press
                     (void *userdata, const vrpn_BUTTONCB info)
{
	vrpn_Tracker_AnalogFly	*me = (vrpn_Tracker_AnalogFly*)userdata;

	// If this is the correct button, and it has just been pressed, then
	// reset the matrix.
	if ( (info.button == me->d_which_button) && (info.state == 1) ) {
		me->reset();
	}
}

// This handle state changes associated with the clutch button.

void VRPN_CALLBACK vrpn_Tracker_AnalogFly::handle_clutch_press
                     (void *userdata, const vrpn_BUTTONCB info)
{
  vrpn_Tracker_AnalogFly	*me = (vrpn_Tracker_AnalogFly*)userdata;

  // If this is the correct button, set the clutch state according to
  // the value of the button.
  if (info.button == me->d_clutch_which) {
    if (info.state == 1) {
      me->d_clutch_engaged = true;
    } else {
      me->d_clutch_engaged = false;
    }
  }
}

// This sets up the Analog Remote for one channel, setting up the callback
// needed to adjust the value based on changes in the analog input.
// Returns 0 on success and -1 on failure.

int	vrpn_Tracker_AnalogFly::setup_channel(vrpn_TAF_fullaxis *full)
{
	// If the name is NULL, we're done.
	if (full->axis.name == NULL) { return 0; }

	// Open the analog device and point the remote at it.
	// If the name starts with the '*' character, use the server
        // connection rather than making a new one.
	if (full->axis.name[0] == '*') {
          try {
            full->ana = new vrpn_Analog_Remote(&(full->axis.name[1]),
              d_connection);
          } catch (...) { full->ana = NULL; }
#ifdef	VERBOSE
		printf("vrpn_Tracker_AnalogFly: Adding local analog %s\n",
                          &(full->axis.name[1]));
#endif
	} else {
          try {
            full->ana = new vrpn_Analog_Remote(full->axis.name);
          } catch (...) { full->ana = NULL; }

#ifdef	VERBOSE
		printf("vrpn_Tracker_AnalogFly: Adding remote analog %s\n",
                          full->axis.name);
#endif
	}
	if (full->ana == NULL) {
		fprintf(stderr,"vrpn_Tracker_AnalogFly: "
                        "Can't open Analog %s\n",full->axis.name);
		return -1;
	}

	// Set up the callback handler for the channel
	return full->ana->register_change_handler(full, handle_analog_update);
}

// This tears down the Analog Remote for one channel, undoing everything that
// the setup did. Returns 0 on success and -1 on failure.

int	vrpn_Tracker_AnalogFly::teardown_channel(vrpn_TAF_fullaxis *full)
{
	int	ret;

	// If the analog pointer is NULL, we're done.
	if (full->ana == NULL) { return 0; }

	// Turn off the callback handler for the channel
	ret = full->ana->unregister_change_handler((void*)full,
                          handle_analog_update);

	// Delete the analog device.
        try {
          delete full->ana;
        } catch (...) {
          fprintf(stderr, "vrpn_Tracker_AnalogFly::teardown_channel(): delete failed\n");
          return -1;
        }

	return ret;
}
 
// static
int VRPN_CALLBACK vrpn_Tracker_AnalogFly::handle_newConnection(void * userdata,
                                                 vrpn_HANDLERPARAM)
{
     
  printf("Get a new connection, reset virtual_Tracker\n");
  ((vrpn_Tracker_AnalogFly *) userdata)->reset();

  // Always return 0 here, because nonzero return means that the input data
  // was garbage, not that there was an error. If we return nonzero from a
  // vrpn_Connection handler, it shuts down the connection.
  return 0;
}

/** Reset the current matrix to zero and store it into the tracker
    position/quaternion location.  This is is not done for an absolute
    tracker, whose position and orientation are locked to the reports from
    the analog device.
*/

void vrpn_Tracker_AnalogFly::reset (void)
{
  // Set the clutch matrix to the identity.
  q_matrix_copy(d_clutchMatrix, d_initMatrix);

  // Set the matrix back to the identity matrix
  q_matrix_copy(d_currentMatrix, d_initMatrix);
  vrpn_gettimeofday(&d_prevtime, NULL);

  // Convert the matrix into quaternion notation and copy into the
  // tracker pos and quat elements.
  convert_matrix_to_tracker();
}

void vrpn_Tracker_AnalogFly::mainloop()
{
  struct timeval	now;
  double	interval;	// How long since the last report, in secs

  // Call generic server mainloop, since we are a server
  server_mainloop();

  // Mainloop() all of the analogs that are defined and the button
  // so that we will get all of the values fresh.
  if (d_x.ana != NULL) { d_x.ana->mainloop(); };
  if (d_y.ana != NULL) { d_y.ana->mainloop(); };
  if (d_z.ana != NULL) { d_z.ana->mainloop(); };
  if (d_sx.ana != NULL) { d_sx.ana->mainloop(); };
  if (d_sy.ana != NULL) { d_sy.ana->mainloop(); };
  if (d_sz.ana != NULL) { d_sz.ana->mainloop(); };
  if (d_reset_button != NULL) { d_reset_button->mainloop(); };
  if (d_clutch_button != NULL) { d_clutch_button->mainloop(); };

  // See if it has been long enough since our last report.
  // If so, generate a new one.
  vrpn_gettimeofday(&now, NULL);
  interval = vrpn_TimevalDurationSeconds(now, d_prevtime);

  if (shouldReport(interval)) {

    // Set the time on the report to now, if not an absolute
    // tracker.  Absolute trackers have their time values set
    // to match the time at which their analog devices gave the
    // last report.
    if (!d_absolute) {
      vrpn_Tracker::timestamp = now;
    }

    // Figure out the new matrix based on the current values and
    // the length of the interval since the last report
    update_matrix_based_on_values(interval);

    // pack and deliver tracker report;
    if (d_connection) {
      char	msgbuf[1000];
      int	len = encode_to(msgbuf);
      if (d_connection->pack_message(len, vrpn_Tracker::timestamp,
	      position_m_id, d_sender_id, msgbuf,
	      vrpn_CONNECTION_LOW_LATENCY)) {
      fprintf(stderr,"Tracker AnalogFly: "
              "cannot write message: tossing\n");
      }
    } else {
      fprintf(stderr,"Tracker AnalogFly: "
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

// This routine will update the current matrix based on the current values
// in the offsets list for each axis, and the length of time over which the
// action is taking place (time_interval).
// Handling of non-absolute trackers: It treats the values as either
// meters/second or else rotations/second to be integrated over the interval
// to adjust the current matrix.
// Handling of absolute trackers: It always assumes that the starting matrix
// is the identity, so that the values are absolute offsets/rotations.
// XXX Later, it would be cool to have non-absolute trackers send velocity
// information as well, since it knows what this is.

void	vrpn_Tracker_AnalogFly::update_matrix_based_on_values
                  (double time_interval)
{
  double tx,ty,tz, rx,ry,rz;	// Translation (m/s) and rotation (rad/sec)
  q_matrix_type diffM;		// Difference (delta) matrix

  // For absolute trackers, the interval is treated as "1", so that the
  // translations and rotations are unscaled;
  if (d_absolute) { time_interval = 1.0; };

  // compute the translation and rotation
  tx = d_x.value * time_interval;
  ty = d_y.value * time_interval;
  tz = d_z.value * time_interval;
  
  rx = d_sx.value * time_interval * (2*VRPN_PI);
  ry = d_sy.value * time_interval * (2*VRPN_PI);
  rz = d_sz.value * time_interval * (2*VRPN_PI);

  // Build a rotation matrix, then add in the translation
  q_euler_to_col_matrix(diffM, rz, ry, rx);
  diffM[3][0] = tx; diffM[3][1] = ty; diffM[3][2] = tz;

  // While the clutch is not engaged, we don't move.  Record that
  // the clutch was off so that we know later when it is re-engaged.
  if (!d_clutch_engaged) {
    d_clutch_was_off = true;
    return;
  }
  
  // When the clutch becomes re-engaged, we store the current matrix
  // multiplied by the inverse of the present differential matrix so that
  // the first frame of the mouse-hold leaves us in the same location.
  // For the absolute matrix, this re-engages new motion at the previous
  // location.
  if (d_clutch_engaged && d_clutch_was_off) {
    d_clutch_was_off = false;
    q_type  diff_orient;
    // This is backwards, because Euler angles have rotation about Z first...
    q_from_euler(diff_orient, rz, ry, rx);
    q_xyz_quat_type diff;
    q_vec_set(diff.xyz, tx, ty, tz);
    q_copy(diff.quat, diff_orient);
    q_xyz_quat_type  diff_inverse;
    q_xyz_quat_invert(&diff_inverse, &diff);
    q_matrix_type di_matrix;
    q_to_col_matrix(di_matrix, diff_inverse.quat);
    di_matrix[3][0] = diff_inverse.xyz[0];
    di_matrix[3][1] = diff_inverse.xyz[1];
    di_matrix[3][2] = diff_inverse.xyz[2];
    q_matrix_mult(d_clutchMatrix, di_matrix, d_currentMatrix);
  }

  // Apply the matrix.
  if (d_absolute) {
      // The difference matrix IS the current matrix.  Catenate it
      // onto the clutch matrix.  If there is no clutching happening,
      // this matrix will always be the identity so this will just
      // copy the difference matrix.
      q_matrix_mult(d_currentMatrix, diffM, d_clutchMatrix);
  } else {
      // Multiply the current matrix by the difference matrix to update
      // it to the current time. 
      if (d_worldFrame) {
        // If using world frame:
        // 1. Separate out the translation and add to the differential translation
        tx += d_currentMatrix[3][0];
        ty += d_currentMatrix[3][1];
        tz += d_currentMatrix[3][2];
        diffM[3][0] = 0; diffM[3][1] = 0; diffM[3][2] = 0;
        d_currentMatrix[3][0] = 0; d_currentMatrix[3][1] = 0; d_currentMatrix[3][2] = 0;

        // 2. Compose the rotations.
        q_matrix_mult(d_currentMatrix, d_currentMatrix, diffM);

        // 3. Put the new translation back in the matrix.
        d_currentMatrix[3][0] = tx; d_currentMatrix[3][1] = ty; d_currentMatrix[3][2] = tz;

      } else {
        q_matrix_mult(d_currentMatrix, diffM, d_currentMatrix);
      }
  }

  // Finally, convert the matrix into a pos/quat
  // and copy it into the tracker position and quaternion structures.
  convert_matrix_to_tracker();
}

void vrpn_Tracker_AnalogFly::convert_matrix_to_tracker (void)
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

bool vrpn_Tracker_AnalogFly::shouldReport
                  (double elapsedInterval) const {
  
  // If we haven't had enough time pass yet, don't report.
  if (elapsedInterval < d_update_interval) {
    return VRPN_FALSE;
  }

  // If we're sending a report every interval, regardless of
  // whether or not there are changes, then send one now.
  if (!d_reportChanges) {
    return VRPN_TRUE;
  }

  // If anything's nonzero, send the report.
  // HACK:  This values may be unstable, depending on device characteristics;
  // we may need to designate a small dead zone around zero and only report
  // if the value is outside the dead zone.
  if (d_x.value || d_y.value || d_z.value ||
      d_sx.value || d_sy.value || d_sz.value) {
    return VRPN_TRUE;
  }

  // Enough time has elapsed, but nothing has changed, so return false.
  return VRPN_FALSE;
}

