#include <string.h>
#include <math.h>
#include "vrpn_Tracker_AnalogFly.h"

#undef	VERBOSE

#ifndef	M_PI
const	double	M_PI = 3.14159;
#endif

static	double	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) / 1000000.0 +
	       (t1.tv_sec - t2.tv_sec);
}

vrpn_Tracker_AnalogFly::vrpn_Tracker_AnalogFly (const char *name, vrpn_Connection *trackercon,
			vrpn_Tracker_AnalogFlyParam *params, float update_rate) :
	vrpn_Tracker (name, trackercon),
	_connection(trackercon),
	_which_button(params->reset_which),
	_update_interval(update_rate ? (1/update_rate) : 1.0)
{
	int i;

	_x.axis = params->x; _y.axis = params->y; _z.axis = params->z;
	_sx.axis = params->sx; _sy.axis = params->sy; _sz.axis = params->sz;
	_x.ana = _y.ana = _z.ana = NULL;
	_sx.ana = _sy.ana = _sz.ana = NULL;
	_x.value = _y.value = _z.value = 0.0;
	_sx.value = _sy.value = _sz.value = 0.0;

	//--------------------------------------------------------------------------------
	// Open analog remotes for any channels that have non-NULL names.
	// If the name starts with the "*" character, use tracker connection rather than
	//	getting a new connection for it.
	// Set up callbacks to handle updates to the analog values
	setup_channel(&_x);
	setup_channel(&_y);
	setup_channel(&_z);
	setup_channel(&_sx);
	setup_channel(&_sy);
	setup_channel(&_sz);

	//--------------------------------------------------------------------------------
	// Open the reset button if is has a non-NULL name.
	// If the name starts with the "*" character, use tracker connection rather than
	//	getting a new connection for it.
	// Set up callback for it to reset the matrix to identity.

	// If the name is NULL, don't do anything.
	if (params->reset_name != NULL) {

		// Open the button device and point the remote at it.
		// If the name starts with the '*' character, use the server connection rather
		// than making a new one.
		if (params->reset_name[0] == '*') {
			reset_button = new vrpn_Button_Remote(&(params->reset_name[1]),
				_connection);
		} else {
			reset_button = new vrpn_Button_Remote(params->reset_name);
		}
		if (reset_button == NULL) {
			fprintf(stderr,"vrpn_Tracker_AnalogFly: Can't open Button %s\n",params->reset_name);
		} else {
			// Set up the callback handler for the channel
			reset_button->register_change_handler(this, handle_reset_press);
		}
	}

	//--------------------------------------------------------------------------------
	// Whenever we get the first connection to this server, we also want to reset
	// the matrix to identity, so that you start at the beginning. Set up a handler
	// to do this.
	_connection->register_handler(_connection->register_message_type(vrpn_got_first_connection),
		      handle_newConnection, this);

	//--------------------------------------------------------------------------------
	// Set the initialization matrix to identity, then also set the current matrix
	// to identity.
	for ( i =0; i< 4; i++)
		for (int j=0; j< 4; j++) 
			initMatrix[i][j] = 0;
	initMatrix[0][0] = initMatrix[1][1] = initMatrix[2][2] = initMatrix[3][3] = 1.0;
	reset();
}

vrpn_Tracker_AnalogFly::~vrpn_Tracker_AnalogFly()
{
	// Tear down the analog update callbacks and remotes
	teardown_channel(&_x);
	teardown_channel(&_y);
	teardown_channel(&_z);
	teardown_channel(&_sx);
	teardown_channel(&_sy);
	teardown_channel(&_sz);

	// Tear down the button update callback and remote (if there is one)
	if (reset_button != NULL) {
		reset_button->unregister_change_handler(this, handle_reset_press);
		delete reset_button;
	}

	// Tear down the "new connection" callback
	_connection->unregister_handler(_connection->register_message_type(vrpn_got_first_connection),
	      handle_newConnection, this);
}

// This routine handles updates of the analog values. The value coming in is
// adjusted per the parameters in the full axis description, and then used to
// update the value there. The value is used by the matrix-generation code in
// mainloop() to update the transformations, that work is not done here.

void	vrpn_Tracker_AnalogFly::handle_analog_update(void *userdata, const vrpn_ANALOGCB info)
{
	vrpn_TAF_fullaxis	*full = (vrpn_TAF_fullaxis *)userdata;
	double value = info.channel[full->axis.channel];
	double value_abs = fabs(value);

	// If we're not above threshold, store zero and we're done!
	if (value_abs <= full->axis.thresh) {
		full->value = 0.0;
		return;
	}

	// Scale and apply the power to the value (maintaining its sign)
	if (value >=0) {
		full->value = pow(value*full->axis.scale, full->axis.power);
	} else {
		full->value = -pow(value_abs*full->axis.scale, full->axis.power);
	}
}

// This routine will reset the matrix to identity when the reset button is
// pressed.

void vrpn_Tracker_AnalogFly::handle_reset_press(void *userdata, const vrpn_BUTTONCB info)
{
	vrpn_Tracker_AnalogFly	*me = (vrpn_Tracker_AnalogFly*)userdata;

	// If this is the right button, and it has just been pressed, then
	// reset the matrix.
	if ( (info.button == me->_which_button) && (info.state == 1) ) {
		me->reset();
	}
}

// This sets up the Analog Remote for one channel, setting up the callback needed
// to adjust the value based on changes in the analog input. Returns 0 on success
// and -1 on failure.

int	vrpn_Tracker_AnalogFly::setup_channel(vrpn_TAF_fullaxis *full)
{
	// If the name is NULL, we're done.
	if (full->axis.name == NULL) { return 0; }

	// Open the analog device and point the remote at it.
	// If the name starts with the '*' character, use the server connection rather
	// than making a new one.
	if (full->axis.name[0] == '*') {
		full->ana = new vrpn_Analog_Remote(&(full->axis.name[1]), _connection);
#ifdef	VERBOSE
		printf("vrpn_Tracker_AnalogFly: Adding local analog %s\n",&(full->axis.name[1]));
#endif
	} else {
		full->ana = new vrpn_Analog_Remote(full->axis.name);
#ifdef	VERBOSE
		printf("vrpn_Tracker_AnalogFly: Adding remote analog %s\n",full->axis.name[1]);
#endif
	}
	if (full->ana == NULL) {
		fprintf(stderr,"vrpn_Tracker_AnalogFly: Can't open Analog %s\n",full->axis.name);
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
	ret = full->ana->unregister_change_handler((void*)full, handle_analog_update);

	// Delete the analog device and point the remote at it.
	delete full->ana;

	return ret;
}
 
// static
int vrpn_Tracker_AnalogFly::handle_newConnection(void * userdata, vrpn_HANDLERPARAM)
{
     
  printf("Get a new connection, reset virtual_Tracker\n");
  ((vrpn_Tracker_AnalogFly *) userdata)->reset();

  // Always return 0 here, because nonzero return means that the input data
  // was garbage, not that there was an error. If we return nonzero from a
  // vrpn_Connection handler, it shuts down the connection.
  return 0;
}

void vrpn_Tracker_AnalogFly::reset()
{
	// Set the matrix back to the identity matrix
	q_matrix_copy(currentMatrix, initMatrix);
	prevtime.tv_sec = -1;
	prevtime.tv_usec = -1;

	// Convert the matrix into quaternion notation and copy into the
	// tracker pos and quat elements.
	convert_matrix_to_tracker();
}

void vrpn_Tracker_AnalogFly::mainloop(const struct timeval * timeout)
{
	struct timeval	now;
	double	interval;	// How long since the last report, in secs

	// Mainloop() all of the analogs that are defined and the button
	// so that we will get all of the values fresh.
	if (_x.ana != NULL) { _x.ana->mainloop(); };
	if (_y.ana != NULL) { _y.ana->mainloop(); };
	if (_z.ana != NULL) { _z.ana->mainloop(); };
	if (_sx.ana != NULL) { _sx.ana->mainloop(); };
	if (_sy.ana != NULL) { _sy.ana->mainloop(); };
	if (_sz.ana != NULL) { _sz.ana->mainloop(); };
	if (reset_button != NULL) { reset_button->mainloop(); };

	// See if it has been long enough since our last report. If so, generate
	// a new one.
	gettimeofday(&now, NULL);
	interval = duration(now, prevtime);
	if (interval > _update_interval) {

		// Set the time on the report to now
		vrpn_Tracker::timestamp = now;

		// Figure out the new matrix based on the current values and the
		// length of the interval since the last report
		update_matrix_based_on_values(interval);

		// pack and deliver tracker report;
		if (_connection) {
			char	msgbuf[1000];
			int	len = encode_to(msgbuf);
			if (_connection->pack_message(len, timestamp,
				position_m_id, my_id, msgbuf,
				vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr,"Tracker AnalogFly: cannot write message: tossing\n");
			}
		} else {
			fprintf(stderr,"Tracker AnalogFly: No valid connection\n");
		}

		// We just sent a report, so reset the time
		prevtime = now;
	}
}

// This routine will update the current matrix based on the current values
// in the offsets list for each axis, and the length of time over which the
// action is taking place (time_interval). It treats the values as either
// meters/second or else rotations/second to be integrated over the interval
// to adjust the current matrix.
// XXX Later, it would be cool to have it send velocity information as well,
// since it knows what this is.

void	vrpn_Tracker_AnalogFly::update_matrix_based_on_values(double time_interval)
{
  double tx,ty,tz, rx,ry,rz;	// Translation (m/s) and rotation (rad/sec)
  q_matrix_type diffM;		// Difference (delta) matrix

  // compute the translation and rotation
  tx = _x.value * time_interval;
  ty = _y.value * time_interval;
  tz = _z.value * time_interval;
  
  rx = _sx.value * time_interval * (2*M_PI);
  ry = _sy.value * time_interval * (2*M_PI);
  rz = _sz.value * time_interval * (2*M_PI);

  // Build a rotation matrix, then add in the translation
  q_euler_to_col_matrix(diffM, rz, ry, rx);
  diffM[3][0] = tx; diffM[3][1] = ty; diffM[3][2] = tz;

  // Multiply the current matrix by the difference matrix to update
  // it to the current time. Then convert the matrix into a pos/quat
  // and copy it into the tracker position and quaternion structures.

  q_matrix_type final;

  q_matrix_mult(final, diffM, currentMatrix);
  q_matrix_copy(currentMatrix, final);

  convert_matrix_to_tracker();
}

void vrpn_Tracker_AnalogFly::convert_matrix_to_tracker(void)
{
  q_xyz_quat_type xq;
  int i;

  q_row_matrix_to_xyz_quat( & xq, currentMatrix);

  for (i=0; i< 3; i++) {
    pos[i] = xq.xyz[i]; // position;
  }
  for (i=0; i< 4; i++) {
    quat[i] = xq.quat[i]; // orientation. 
  }
}
