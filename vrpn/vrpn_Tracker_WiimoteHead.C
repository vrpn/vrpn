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
	d_update_interval (update_rate ? (1 / update_rate) : 60.0),
	d_blobDistance (.145),
	d_hasBlob (false),
	d_updated (false),
	d_needWiimote (false),
	d_gravDirty (true),
	d_name(wiimote)
{

	d_vGrav[0] = 0;
	d_vGrav[1] = -1;
	d_vGrav[2] = 0;

	// If the name is NULL, we're done.
	if (wiimote == NULL) {
		d_name = NULL;
		fprintf(stderr, "vrpn_Tracker_WiimoteHead: "
				"Can't start without a valid specified Wiimote device!");
		return;
	}

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
	if (ret == -1) {
		fprintf(stderr, "vrpn_Tracker_WiimoteHead: "
				"Can't setup change handler on Analog %s\n", d_name);
		delete d_ana;
		d_ana = NULL;
		//return;
	}

	ret = register_custom_types();
	if (!ret) {
		fprintf(stderr, "vrpn_Tracker_WiimoteHead: "
				"Can't setup custom message and sender types\n");
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
	// Set the current matrix to identity, the current timestamp to "now",
	// the current matrix to identity in case we never hear from the Wiimote.
	// Also, set the updated flag to send a single report
	reset();
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
	if (!wh->d_hasBlob) {
		fprintf(stderr, "vrpn_Tracker_WiimoteHead: "
						"got first report from Wiimote!\n");
	}

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
	wh->d_updated = true;

	bool newgrav = false;

	// Grab gravity
	for (i = 0; i < 3; i++) {
		if (info.channel[1 + i] != wh->d_vGrav[i]) {
			newgrav = true;
			break;
		}
	}

	if (newgrav) {
		for (i = 0; i < 3; i++) {
			if (!wh->d_gravDirty) {
				// only slide back the previous gravity if we actually used it once.
				wh->d_vGravAntepenultimate[i] = wh->d_vGravPenultimate[i];
				wh->d_vGravPenultimate[i] = wh->d_vGrav[i];
			}
			wh->d_vGrav[i] = info.channel[1 + i];
		}
		wh->d_gravDirty = true;
	}

	// Store the time of the report into the tracker's timestamp field.
	wh->vrpn_Tracker::timestamp = info.msg_time;
}

// static
int vrpn_Tracker_WiimoteHead::handle_newConnection(void* userdata, vrpn_HANDLERPARAM) {
	vrpn_Tracker_WiimoteHead* wh = reinterpret_cast<vrpn_Tracker_WiimoteHead*>(userdata);
	// Indicate that we should grab the wiimote, if we haven't already
	// and that we should send a report with whatever we have.
	wh->d_needWiimote = true;
	wh->d_updated = true;

	// Always return 0 here, because nonzero return means that the input data
	// was garbage, not that there was an error. If we return nonzero from a
	// vrpn_Connection handler, it shuts down the connection.
	return 0;
}

// static
int vrpn_Tracker_WiimoteHead::handle_dropLastConnection(void* userdata, vrpn_HANDLERPARAM) {
	vrpn_Tracker_WiimoteHead* wh = reinterpret_cast<vrpn_Tracker_WiimoteHead*>(userdata);
	wh->d_needWiimote = false;

	// Always return 0 here, because nonzero return means that the input data
	// was garbage, not that there was an error. If we return nonzero from a
	// vrpn_Connection handler, it shuts down the connection.
	return 0;
}

/** Reset the current matrix to zero and store it into the tracker
    position/quaternion location, and set the updated flag.
*/

void vrpn_Tracker_WiimoteHead::reset(void) {
	// Set the matrix back to the identity matrix
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			d_currentMatrix[i][j] = 0;
		}
	}

	d_currentMatrix[0][0] = d_currentMatrix[1][1] = d_currentMatrix[2][2] =
							  d_currentMatrix[3][3] = 1.0;
	vrpn_gettimeofday(&d_prevtime, NULL);

	// Set the updated flag to send a report
	d_updated = true;

	// Convert the matrix into quaternion notation and copy into the
	// tracker pos and quat elements.
	convert_matrix_to_tracker();
}

bool vrpn_Tracker_WiimoteHead::register_custom_types()
{
    if (d_connection == NULL) {
		return false;
    }

    needwiimote_m_id = d_connection->register_message_type("vrpn_Tracker_WiimoteHead needWiimote");
	if (needwiimote_m_id == -1) {
		fprintf(stderr,"vrpn_Tracker_WiimoteHead: Can't register type IDs\n");
		d_connection = NULL;
		return false;
    }

    refreshwiimote_m_id = d_connection->register_message_type("vrpn_Tracker_WiimoteHead refreshWiimote");
	if (refreshwiimote_m_id == -1) {
		fprintf(stderr,"vrpn_Tracker_WiimoteHead: Can't register type IDs\n");
		d_connection = NULL;
		return false;
    }

    wmheadtrackserver_s_id = d_connection->register_sender("WMHeadTrackServer");
	if (wmheadtrackserver_s_id == -1) {
		fprintf(stderr,"vrpn_Tracker_WiimoteHead: Can't register sender IDs\n");
		d_connection = NULL;
		return false;
    }

    return true;
}

void vrpn_Tracker_WiimoteHead::mainloop() {
	struct timeval        now;
	double        interval; // How long since the last report, in secs

	// Call generic server mainloop, since we are a server
	server_mainloop();

	if (d_needWiimote && d_ana == NULL) {
		// TODO try to get the wiimote anew here
		// and register the handlers
	}

	// Mainloop() the wiimote to get fresh values
	if (d_ana != NULL) {
		d_needWiimote = false;
		d_ana->mainloop();
	}

	// See if it has been long enough since our last report.
	// If so, generate a new one.
	vrpn_gettimeofday(&now, NULL);
	interval = duration(now, d_prevtime);

	if (true /*shouldReport(interval)*/) {
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
		d_updated = false;
	}

	// We're not always sending reports, but we still want to
	// update the interval clock so that we don't integrate over
	// too long a timespan when we do finally report a change.
	// TODO RP is this still needed/useful?
	/*
	if (interval >= d_update_interval) {
		d_prevtime = now;
	}
	*/
}

/// Update the current matrix based on the most recent values
/// received from the Wiimote regarding IR blob location.
/// Time interval is passed for potential future Kalman filter, etc.
void	vrpn_Tracker_WiimoteHead::update_matrix_based_on_values(double time_interval) {
	double tx, ty, tz, rx, ry, rz; // Translation (m) and rotation (rad)
	q_matrix_type newM;    // New position matrix

	tx = ty = tz = 0;
	rx = ry = rz = 0;
	std::vector<double> x, y, size;
	int points = 0;
	points = d_vX.size();

	// If our gravity vector has changed and it's not 0,
	// we need to update our gravity correction matrix.
	if (d_gravDirty && haveGravity()) {
		// TODO perhaps set this quaternion as our tracker2room transform?
		q_vec_type movingAvg = Q_NULL_VECTOR;
		q_vec_copy (movingAvg, d_vGrav);
		q_vec_add (movingAvg, movingAvg, d_vGravPenultimate);
		q_vec_add (movingAvg, movingAvg, d_vGravAntepenultimate);
		q_vec_scale (movingAvg, 0.33333, movingAvg);

		q_vec_type regulargravity;

		regulargravity[1] = 1;
		q_from_two_vecs (d_qCorrectGravity, regulargravity, movingAvg);

		//q_euler_to_col_matrix(destMatrix, zRot, yRot, xRot)
		q_to_col_matrix (d_mCorrectGravity, d_qCorrectGravity);
		d_gravDirty = false;
	}

	if (points == 2) {
		// we simply stop updating our pos+orientation if we lost LED's

		// TODO right now only handling the 2-LED glasses
		// TODO at a fixed 14.5cm distance (set in the constructor)

		// Wiimote stats source: http://wiibrew.org/wiki/Wiimote#IR_Camera
		const double xResSensor = 1024.0, yResSensor = 768.0;
		const double fovX = 33.0, fovY = 23.0;
		double dx = d_vX[0] - d_vX[1];
		double dy = d_vY[0] - d_vY[1];
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
		rz = atan2(dx, dy);

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
		tx = headDist * (avgX - xResSensor / 2) / bHoriz;
		ty = headDist * (avgY - yResSensor / 2) / bVert;

		// Build a rotation matrix, then add in the translation
		q_euler_to_col_matrix(newM, rz, ry, rx);
		newM[3][0] = tx; newM[3][1] = ty; newM[3][2] = tz;

		if (haveGravity()) {
			// we know gravity, so we are correcting for it.
			// TODO: improve vrpn driver in juggler to handle tracker-to-room
			// transform and set that there, instead?

			//q_matrix_mult(newM, d_mCorrectGravity, newM);
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
	/*
	if (haveGravity()) {
		// we know gravity, so we are correcting for it.
		// TODO: improve vrpn driver in juggler to handle tracker-to-room
		// transform and set that there, instead?
		q_xform(xq.xyz, d_qCorrectGravity, xq.xyz);
		//q_matrix_mult(newM, newM, d_mCorrectGravity);
	}
	*/

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

	// If we've gotten new wiimote reports since our last report, return true.
	if (d_updated) {
		return VRPN_TRUE;
	}

	// If it's been more than a second, send an update anyway
	if (elapsedInterval >= 1/d_update_interval) {
		return VRPN_TRUE;
	}

	// Enough time has elapsed, but nothing has changed, so return false.
	return VRPN_FALSE;
}

bool vrpn_Tracker_WiimoteHead::haveGravity() const {
	return (d_vGrav[0] != 0 || d_vGrav[1] != 0 || d_vGrav[2] != 0);
}
