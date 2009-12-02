#include <string.h>
#include <math.h>
#include <vector>
#include "vrpn_Tracker_WiimoteHead.h"

#undef	VERBOSE

#ifndef	M_PI
#define M_PI		3.14159265358979323846
#endif

static	double	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) / 1000000.0 +
	       (t1.tv_sec - t2.tv_sec);
}

vrpn_Tracker_WiimoteHead::vrpn_Tracker_WiimoteHead(const char* name, vrpn_Connection* trackercon, const char* wiimote, float update_rate,	vrpn_bool absolute, vrpn_bool reportChanges) :
	vrpn_Tracker (name, trackercon),
	d_update_interval (update_rate ? (1 / update_rate) : 1.0),
	d_absolute (absolute),
	d_reportChanges (reportChanges),
	d_blobDistance (.15)
{
	int i;
	for (i = 0; i < 4; i++) {
		d_blobs[i].name = wiimote;
		d_blobs[i].first_channel = i * 3 + 4;
		d_blobs[i].ana = NULL;
		d_blobs[i].x = d_blobs[i].y = d_blobs[i].size = -1;
		d_blobs[i].wh = this;
		setup_blob(&d_blobs[i]);
	}

	//--------------------------------------------------------------------
	// Whenever we get the first connection to this server, we also
	// want to reset the matrix to identity, so that you start at the
	// beginning. Set up a handler to do this.
	register_autodeleted_handler(d_connection->register_message_type(vrpn_got_first_connection),
				     handle_newConnection, this);

	//--------------------------------------------------------------------
	// Set the initialization matrix to identity, then also set
	// the current matrix to identity
	for ( i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			d_initMatrix[i][j] = 0;
		}
	}

	d_initMatrix[0][0] = d_initMatrix[1][1] = d_initMatrix[2][2] =
							  d_initMatrix[3][3] = 1.0;
	reset();

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

vrpn_Tracker_WiimoteHead::~vrpn_Tracker_WiimoteHead (void)
{
	// Tear down the analog update callbacks and remotes
	teardown_blob(&d_blobs[0]);
	teardown_blob(&d_blobs[1]);
	teardown_blob(&d_blobs[2]);
	teardown_blob(&d_blobs[3]);
}

// This routine handles updates of the analog values. The value coming in is
// adjusted per the parameters in the full axis description, and then used to
// update the value there. The value is used by the matrix-generation code in
// mainloop() to update the transformations; that work is not done here.

void	vrpn_Tracker_WiimoteHead::handle_analog_update(void* userdata, const vrpn_ANALOGCB info)
{
	vrpn_TWH_blob* blob = (vrpn_TWH_blob*)userdata;
	blob->x = info.channel[blob->first_channel];
	blob->y = info.channel[blob->first_channel+1];
	blob->size = info.channel[blob->first_channel+2];

	// If we're an absolute channel, store the time of the report
	// into the tracker's timestamp field.
	if (blob->wh->d_absolute) {
		blob->wh->vrpn_Tracker::timestamp = info.msg_time;
	}
}

// This sets up the Analog Remote for one blob (3 channels), setting up
// the callback needed to adjust the value based on changes in the analog
// input.
// Returns 0 on success and -1 on failure.

int	vrpn_Tracker_WiimoteHead::setup_blob(vrpn_TWH_blob* blob)
{
	if (!blob) { return 0; }

	// If the name is NULL, we're done.
	if (blob->name == NULL) { return 0; }

	// Open the analog device and point the remote at it.
	// If the name starts with the '*' character, use the server
	// connection rather than making a new one.
	if (blob->name[0] == '*') {
		blob->ana = new vrpn_Analog_Remote( & (blob->name[1]), d_connection);
#ifdef	VERBOSE
		printf("vrpn_Tracker_WiimoteHead: Adding local analog %s\n",
		       &(full->axis.name[1]));
#endif
	} else {
		blob->ana = new vrpn_Analog_Remote(blob->name);
#ifdef	VERBOSE
		printf("vrpn_Tracker_WiimoteHead: Adding remote analog %s\n",
		       blob->name);
#endif
	}
	if (blob->ana == NULL) {
		fprintf(stderr, "vrpn_Tracker_WiimoteHead: "
				"Can't open Analog %s\n", blob->name);
		return -1;
	}

	// Set up the callback handler for the channel
	return blob->ana->register_change_handler(blob, handle_analog_update);
}

// This tears down the Analog Remote for one channel, undoing everything that
// the setup did. Returns 0 on success and -1 on failure.

int	vrpn_Tracker_WiimoteHead::teardown_blob(vrpn_TWH_blob* blob)
{
	int	ret;

	// If the analog pointer is NULL, we're done.
	if (blob->ana == NULL) { return 0; }

	// Turn off the callback handler for the channel
	ret = blob->ana->unregister_change_handler(blob,
						   handle_analog_update);

	// Delete the analog device.
	delete blob->ana;

	return ret;
}

// static
int vrpn_Tracker_WiimoteHead::handle_newConnection(void* userdata, vrpn_HANDLERPARAM)
{
	printf("Get a new connection, reset virtual_Tracker\n");
	((vrpn_Tracker_WiimoteHead*) userdata)->reset();

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

void vrpn_Tracker_WiimoteHead::reset(void)
{
	// Set the matrix back to the identity matrix
	q_matrix_copy(d_currentMatrix, d_initMatrix);
	vrpn_gettimeofday(&d_prevtime, NULL);

	// Convert the matrix into quaternion notation and copy into the
	// tracker pos and quat elements.
	convert_matrix_to_tracker();
}

void vrpn_Tracker_WiimoteHead::mainloop()
{
	struct timeval	      now;
	double	      interval; // How long since the last report, in secs

	// Call generic server mainloop, since we are a server
	server_mainloop();

	// Mainloop() all of the analogs that are defined and the button
	// so that we will get all of the values fresh.
	if (d_blobs[0].ana != NULL) { d_blobs[0].ana->mainloop(); }

	// See if it has been long enough since our last report.
	// If so, generate a new one.
	vrpn_gettimeofday(&now, NULL);
	interval = duration(now, d_prevtime);

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
			char	  msgbuf[1000];
			int	  len = encode_to(msgbuf);
			if (d_connection->pack_message(len, vrpn_Tracker::timestamp,
						       position_m_id, d_sender_id, msgbuf,
						       vrpn_CONNECTION_LOW_LATENCY)) {
				fprintf(stderr, "vrpn_Tracker_WiimoteHead: "
						"cannot write message: tossing\n");
			}
		} else {
			fprintf(stderr, "vrpn_Tracker_WiimoteHead: "
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

void	vrpn_Tracker_WiimoteHead::update_matrix_based_on_values(double time_interval)
{
	double tx, ty, tz, rx, ry, rz; // Translation (m/s) and rotation (rad/sec)
	q_matrix_type newM;    // Difference (delta) matrix

	// For absolute trackers, the interval is treated as "1", so that the
	// translations and rotations are unscaled;
	if (d_absolute) { time_interval = 1.0; }


	// TODO RP Implement the math here!
	tx = ty = tz = 0;
 	std::vector<double> x, y, size;
 	int points = 0, i;
 	for (i = 0; i < 4; i++) {
 		if (d_blobs[i].x != -1 && d_blobs[i].y != -1 && d_blobs[i].size != -1) {
 			x[points] = d_blobs[i].x;
 			y[points] = d_blobs[i].y;
 			size[points] = d_blobs[i].size;
 			points++;
 		}
 	}

 	if (points == 2) {
 		// TODO right now only handling the 2-LED glasses at 15cm distance.
 		double dx, dy;
 		dx = x[0] - x[1];
 		dy = y[0] - y[1];
 		double dist = sqrt(dx * dx + dy * dy);
 		// ~33 degree horizontal FOV - source http://wiibrew.org/wiki/Wiimote#IR_Camera
 		double radPerPx = (33 / 180 * M_PI) / 1024;
 		double angle = radPerPx * dist / 2;
 		double headDist = (d_blobDistance / 2) / tan(angle);

 		float avgX = (x[0] + x[1]) / 2;
 		float avgY = (y[0] + y[1]) / 2;

 		tz = headDist;


 	}

	// compute the translation and rotation
	/*
	tx = d_x.value * time_interval;
	ty = d_y.value * time_interval;
	tz = d_z.value * time_interval;

	rx = d_sx.value * time_interval * (2 * M_PI);
	ry = d_sy.value * time_interval * (2 * M_PI);
	rz = d_sz.value * time_interval * (2 * M_PI);
	*/
	// Build a rotation matrix, then add in the translation
	rx = ry = rz = 0;
	q_euler_to_col_matrix(newM, rz, ry, rx);
	newM[3][0] = tx; newM[3][1] = ty; newM[3][2] = tz;

	// Apply the matrix.
	if (d_absolute) {
		// The difference matrix IS the current matrix.
		q_matrix_copy(d_currentMatrix, newM);
	} else {
		// Multiply the current matrix by the difference matrix to update
		// it to the current time.
		q_matrix_mult(d_currentMatrix, newM, d_currentMatrix);
	}

	// Finally, convert the matrix into a pos/quat
	// and copy it into the tracker position and quaternion structures.
	convert_matrix_to_tracker();
}

void vrpn_Tracker_WiimoteHead::convert_matrix_to_tracker(void)
{
	q_xyz_quat_type xq;
	int i;

	q_row_matrix_to_xyz_quat( &xq, d_currentMatrix);

	for (i = 0; i < 3; i++) {
		pos[i] = xq.xyz[i]; // position;
	}
	for (i = 0; i < 4; i++) {
		d_quat[i] = xq.quat[i]; // orientation.
	}
}

vrpn_bool vrpn_Tracker_WiimoteHead::shouldReport(double elapsedInterval) const
{
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
	// TODO RP : replace logic here
	//if (d_x.value || d_y.value || d_z.value ||
	//    d_sx.value || d_sy.value || d_sz.value) {
		return VRPN_TRUE;
	//}

	// Enough time has elapsed, but nothing has changed, so return false.
	return VRPN_FALSE;
}
