#include <string.h>
#include <math.h>
#include <vector>
#include "vrpn_Tracker_WiimoteHead.h"

#undef	VERBOSE

#ifndef	M_PI
#define M_PI		3.14159265358979323846
#endif

static	double	duration(struct timeval t1, struct timeval t2) {
	return (t1.tv_usec - t2.tv_usec) / 1000000.0 +
	       (t1.tv_sec - t2.tv_sec);
}

vrpn_Tracker_WiimoteHead::vrpn_Tracker_WiimoteHead(const char* name, vrpn_Connection* trackercon, const char* wiimote, float update_rate) :
	vrpn_Tracker (name, trackercon),
	d_update_interval (update_rate ? (1 / update_rate) : 1.0),
	d_blobDistance (.145),
	d_hasBlob (false),
	d_gravDirty (true),
	d_name(wiimote) {

	int i;
	d_vGrav[0] = 0;
	d_vGrav[1] = -1;
	d_vGrav[2] = 0;

	// If the name is NULL, we're done.
	if (wiimote == NULL) {
		fprintf(stderr, "vrpn_Tracker_WiimoteHead: "
				"Can't start without a valid specified Wiimote device!\n");
		d_name = NULL;
		return;
	}
	//d_name = new char[128];
	//d_name[127] = 0;
	//strncpy(d_name, wiimote, 127);

	// Open the analog device and point the remote at it.
	// If the name starts with the '*' character, use the server
	// connection rather than making a new one.
	if (d_name[0] == '*') {
		d_ana = new vrpn_Analog_Remote( & (d_name[1]), d_connection);
#ifdef	VERBOSE
		printf("vrpn_Tracker_WiimoteHead: Adding local analog %s\n",
		       &(d_name[1]));
#endif
	} else {
		d_ana = new vrpn_Analog_Remote(d_name);
#ifdef	VERBOSE
		printf("vrpn_Tracker_WiimoteHead: Adding remote analog %s\n",
		       d_name);
#endif
	}
	if (d_ana == NULL) {
		fprintf(stderr, "vrpn_Tracker_WiimoteHead: "
				"Can't open Analog %s\n", d_name);
		return;
	}

	// register callback
	int ret = d_ana->register_change_handler(this, handle_analog_update);
	if (!ret) {
		fprintf(stderr, "vrpn_Tracker_WiimoteHead: "
				"Can't setup change handler on Analog %s\n", d_name);
		delete d_ana;
		d_ana = NULL;
		return;
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
	for (i = 0; i < 4; i++) {
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
	// in case we never hear from the Wiimote.
	vrpn_gettimeofday(&d_prevtime, NULL);
	vrpn_Tracker::timestamp = d_prevtime;
	q_matrix_copy(d_currentMatrix, d_initMatrix);
	convert_matrix_to_tracker();
}

vrpn_Tracker_WiimoteHead::~vrpn_Tracker_WiimoteHead (void) {
	if(d_name) {
		delete [] d_name;
		d_name = NULL;
	}

	// Tear down the analog update callback and remotes
	int	ret;

	// If the analog pointer is NULL, we're done.
	if (d_ana == NULL) { return; }

	// Turn off the callback handler
	ret = d_ana->unregister_change_handler(this,
						   handle_analog_update);

	// Delete the analog device.
	delete d_ana;
}

// This routine handles updates of the analog values. The value coming in is
// adjusted per the parameters in the full axis description, and then used to
// update the value there. The value is used by the matrix-generation code in
// mainloop() to update the transformations; that work is not done here.

void	vrpn_Tracker_WiimoteHead::handle_analog_update(void* userdata, const vrpn_ANALOGCB info) {
	vrpn_Tracker_WiimoteHead* wh = (vrpn_Tracker_WiimoteHead*)userdata;
	if (!wh) { return; }
	std::vector<double> x, y, size;

	int i, firstchan;
	// Grab all the blobs
	for (i = 0; i < 4; i++) {
		firstchan = i * 3 + 4;
		if (   info.channel[firstchan] != -1
			&& info.channel[firstchan + 1] != -1
			&& info.channel[firstchan + 2] != -1) {
				x.push_back(info.channel[firstchan]);
				y.push_back(info.channel[firstchan + 1]);
				size.push_back(info.channel[firstchan + 2]);
			} else {
				break;
			}
	}
	wh->d_vX = x;
	wh->d_vY = y;
	wh->d_vSize = size;
	wh->d_hasBlob = true;


	// Grab gravity
	for (i = 0; i < 3; i++) {
		if (info.channel[1 + i] != wh->d_vGrav[i]) {
			wh->d_vGrav[i] = info.channel[1 + i];
			wh->d_gravDirty = true;
		}
	}

	// Store the time of the report into the tracker's timestamp field.
	wh->vrpn_Tracker::timestamp = info.msg_time;
}

// This sets up the Analog Remote for one blob (3 channels), setting up
// the callback needed to adjust the value based on changes in the analog
// input.
// Returns 0 on success and -1 on failure.
/*
int	vrpn_Tracker_WiimoteHead::setup_blob(vrpn_TWH_blob* blob) {
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

int	vrpn_Tracker_WiimoteHead::teardown_blob(vrpn_TWH_blob* blob) {
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
*/
// static
int vrpn_Tracker_WiimoteHead::handle_newConnection(void* userdata, vrpn_HANDLERPARAM) {
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

void vrpn_Tracker_WiimoteHead::reset(void) {
	// Set the matrix back to the identity matrix
	q_matrix_copy(d_currentMatrix, d_initMatrix);
	vrpn_gettimeofday(&d_prevtime, NULL);

	// Convert the matrix into quaternion notation and copy into the
	// tracker pos and quat elements.
	convert_matrix_to_tracker();
}

void vrpn_Tracker_WiimoteHead::mainloop() {
	struct timeval        now;
	double        interval; // How long since the last report, in secs

	// Call generic server mainloop, since we are a server
	server_mainloop();

	// Mainloop() all of the analogs that are defined and the button
	// so that we will get all of the values fresh.
	//if (d_blobs[0].ana != NULL) { d_blobs[0].ana->mainloop(); }
	if (d_ana != NULL) { d_ana->mainloop(); }

	// See if it has been long enough since our last report.
	// If so, generate a new one.
	vrpn_gettimeofday(&now, NULL);
	interval = duration(now, d_prevtime);

	if (shouldReport(interval)) {
		// Figure out the new matrix based on the current values and
		// the length of the interval since the last report
		update_matrix_based_on_values(interval);

		// pack and deliver tracker report;
		if (d_connection) {
			char      msgbuf[1000];
			int       len = encode_to(msgbuf);
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

// This routine will update the current matrix based on the most recent values
// received from the Wiimote regarding IR blob location.

void	vrpn_Tracker_WiimoteHead::update_matrix_based_on_values(double time_interval) {
	double tx, ty, tz, rx, ry, rz; // Translation (m) and rotation (rad)
	q_matrix_type newM;    // New position matrix

	tx = ty = tz = 0;
	rx = ry = rz = 0;
	std::vector<double> x, y, size;
	int points = 0, i = 0;
	points = d_vX.size();
	/*
	while (i < 4 && d_blobs[i].x != -1 && d_blobs[i].y != -1 && d_blobs[i].size != -1) {
		x.push_back(d_blobs[i].x);
		y.push_back(d_blobs[i].y);
		size.push_back(d_blobs[i].size);
		points++;
		i++;
	}
	*/

	// If our gravity vector has changed and it's not 0,
	// we need to update our gravity correction matrix.
	if (d_gravDirty) {
		if (d_vGrav[0] != 0 ||
			d_vGrav[1] != 0 ||
			d_vGrav[2] != 0) {

			q_type regulargravity;
			regulargravity[1] = -1;
			q_from_two_vecs (d_qCorrectGravity, regulargravity, d_vGrav);
			q_to_col_matrix (d_mCorrectGravity, d_qCorrectGravity);
			d_gravDirty = false;
		}
	}

	if (points == 2) {
		// TODO right now only handling the 2-LED glasses at 14.5cm distance.
		// we simply stop updating if we lost LED's

		// Wiimote stats source: http://wiibrew.org/wiki/Wiimote#IR_Camera
		const double xResSensor = 1024.0, yResSensor = 768.0;
		const double fovX = 33.0, fovY = 23.0;
		double dx, dy;
		dx = d_vX[0] - d_vX[1];
		dy = d_vY[0] - d_vY[1];
		double dist = sqrt(dx * dx + dy * dy);
		// Note that this is an approximation, since we don't know the
		// distance/horizontal position.  (I think...)
		double radPerPx = (fovX / 180.0 * M_PI) / xResSensor;
		double angle = radPerPx * dist / 2.0;
		double headDist = (d_blobDistance / 2.0) / tan(angle);

		// Translate the distance along z axis, and tilt the head
		// TODO - the 3-led version will have a more complex rotation
		// calculation to perform, since this one assumes the user
		// does not rotate their head around x or y axes
		tz = headDist;
		rz = atan2(dy, dx);

		// Find the sensor pixel of the line of sight - directly between
		// the led's
		double avgX = (d_vX[0] + d_vX[1]) / 2.0;
		double avgY = (d_vY[0] + d_vY[1]) / 2.0;

		// b is the virtual depth in the sensor from a point to the full sensor
		// used for finding similar triangles to calculate x/y translation
		const double bHoriz = xResSensor / 2 / tan(fovX / 2);
		const double bVert = -1 * yResSensor / 2 / tan(fovY / 2);

		// World head displacement from a centered origin at the calculated
		// distance from the sensor
		double worldXdispl = headDist * (avgX - xResSensor / 2) / bHoriz;
		double worldYdispl = headDist * (avgY - yResSensor / 2) / bVert;

		double worldHalfWidth = headDist * tan(fovX / 2);
		double worldHalfHeight = headDist * tan(fovY / 2);

		// Finally, give us position from lower-left corner of the
		// sensor's view frustrum (?)
		//tx = worldXdispl + worldHalfWidth;
		//ty = worldYdispl + worldHalfHeight;

		tx = worldXdispl;
		ty = worldYdispl;

		// Build a rotation matrix, then add in the translation
		q_euler_to_col_matrix(newM, rz, ry, rx);
		newM[3][0] = tx; newM[3][1] = ty; newM[3][2] = tz;

		if (d_vGrav[0] != 0 ||
			d_vGrav[1] != 0 ||
			d_vGrav[2] != 0) {
			// we know gravity, so we are correcting for it.
			q_matrix_mult(newM, d_mCorrectGravity, newM);
		}

		// Apply the matrix.
		q_matrix_copy(d_currentMatrix, newM);

		// Finally, convert the matrix into a pos/quat
		// and copy it into the tracker position and quaternion structures.
		convert_matrix_to_tracker();
	}
}

void vrpn_Tracker_WiimoteHead::convert_matrix_to_tracker(void) {
	q_xyz_quat_type xq;
	int i;

	q_row_matrix_to_xyz_quat(&xq, d_currentMatrix);

	for (i = 0; i < 3; i++) {
		pos[i] = xq.xyz[i]; // position;
	}
	for (i = 0; i < 4; i++) {
		d_quat[i] = xq.quat[i]; // orientation.
	}
}

vrpn_bool vrpn_Tracker_WiimoteHead::shouldReport(double elapsedInterval) const {
	// If we haven't had enough time pass yet, don't report.
	if (elapsedInterval < d_update_interval) {
		return VRPN_FALSE;
	}

	// If we're sending a report every interval, regardless of
	// whether or not there are changes, then send one now.
	//if (!d_reportChanges) {
	//	return VRPN_TRUE;
	//}

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
	//return VRPN_FALSE;
}
