#include <string.h>
#include <math.h>
#include <algorithm>
#include "vrpn_Tracker_WiimoteHead.h"

#undef	VERBOSE

#ifndef	M_PI
#define M_PI		3.14159265358979323846
#endif

static	double	duration(struct timeval t1, struct timeval t2) {
	return (t1.tv_usec - t2.tv_usec) / 1000000.0 +
	       (t1.tv_sec - t2.tv_sec);
}

void make_identity_quat(q_type &dest) {
	dest[0] = dest[1] = dest[2] = 0;
	dest[3] = 1;
}

void make_null_vec(q_vec_type &dest) {
	dest[0] = dest[1] = dest[2] = 0;
}

vrpn_Tracker_WiimoteHead::vrpn_Tracker_WiimoteHead(const char* name, vrpn_Connection* trackercon, const char* wiimote, float update_rate) :
	vrpn_Tracker (name, trackercon),
	d_update_interval(update_rate ? (1 / update_rate) : 60.0),
	d_flipState(FLIP_UNKNOWN),
	d_blobDistance(.145),
	d_points(0),
	d_contact(false),
	d_lock(false),
	d_updated(false),
	d_needWiimote(false),
	d_ana(NULL),
	d_name(wiimote),
	d_gravDirty(true)
{

	// If the name is NULL, we're done.
	if (wiimote == NULL) {
		d_name = NULL;
		fprintf(stderr, "vrpn_Tracker_WiimoteHead: "
				"Can't start without a valid specified Wiimote device!");
		return;
	}

	setupWiimote();

	bool ret = register_custom_types();
	if (!ret) {
		fprintf(stderr, "vrpn_Tracker_WiimoteHead: "
				"Can't setup custom message and sender types\n");
		delete d_ana;
		d_ana = NULL;
		return;
	}

	//--------------------------------------------------------------------
	// Whenever we get a connection, set a flag so we try to get a Wiimote
	// if we haven't got one already. Set up a handler to do this.
	register_autodeleted_handler(d_connection->register_message_type(vrpn_got_connection),
				     handle_connection, this);

	//--------------------------------------------------------------------
	// Set the current matrix to identity, the current timestamp to "now",
	// the current matrix to identity in case we never hear from the Wiimote.
	// Also, set the updated flag to send a single report
	reset();

	// put a little z translation as a saner default
	d_currentPose.xyz[2] = 1;

	// Initialize all gravity vecs to a default
	make_null_vec(d_vGrav);
	make_null_vec(d_vGravPenultimate);
	make_null_vec(d_vGravAntepenultimate);
	d_vGravAntepenultimate[2] = d_vGravPenultimate[2] = d_vGrav[2] = 1;

	// Set up our initial "default" pose to make sure everything is
	// safely initialized before our first report.
	convert_pose_to_tracker();
}

void vrpn_Tracker_WiimoteHead::setupWiimote() {
	if (d_ana) {
		// Turn off the callback handler
		d_ana->unregister_change_handler(this, handle_analog_update);
		delete d_ana;
		d_ana = NULL;
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
		return;
	}

	// Alright, we got one!
	d_needWiimote = false;

	// will re-notify when we catch a report.
	d_contact = false;


}

vrpn_Tracker_WiimoteHead::~vrpn_Tracker_WiimoteHead (void) {
	if(d_name) {
		delete [] d_name;
		d_name = NULL;
	}

	// If the analog pointer is NULL, we're done.
	if (d_ana == NULL) { return; }

	// Turn off the callback handler
	int	ret;
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
	if (!wh->d_contact) {
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
				wh->d_vX[i] = info.channel[firstchan];
				wh->d_vY[i] = info.channel[firstchan + 1];
				wh->d_vSize[i] = info.channel[firstchan + 2];
				wh->d_points = i + 1;
			} else {
				break;
			}
	}

	wh->d_contact = true;
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
		if (!wh->d_gravDirty) {
			// only slide back the previous gravity if we actually used it once.
			q_vec_copy(wh->d_vGravAntepenultimate, wh->d_vGravPenultimate);
			q_vec_copy(wh->d_vGravPenultimate, wh->d_vGrav);
		}
		for (i = 0; i < 3; i++) {
			wh->d_vGrav[i] = info.channel[1 + i];
		}
		wh->d_gravDirty = true;
	}

	// Store the time of the report into the tracker's timestamp field.
	wh->vrpn_Tracker::timestamp = info.msg_time;
}

// static
int vrpn_Tracker_WiimoteHead::handle_connection(void* userdata, vrpn_HANDLERPARAM) {
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

// static
void handle_refresh_wiimote(void* userdata, vrpn_HANDLERPARAM) {
	vrpn_Tracker_WiimoteHead* wh = reinterpret_cast<vrpn_Tracker_WiimoteHead*>(userdata);
	if (wh) {
		wh->setupWiimote();
	}
}

/** Pack and send message with latest state information.
*/

void vrpn_Tracker_WiimoteHead::report() {
	struct timeval now;
	double interval;
	// See if it has been long enough since our last report.
	// If so, generate a new one.
	vrpn_gettimeofday(&now, NULL);
	interval = duration(now, d_prevtime);


	// Figure out the new matrix based on the current values and
	// the length of the interval since the last report
	update_pose(interval);

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

/** Reset the current pose to identity and store it into the tracker
    position/quaternion location, and set the updated flag.
*/

void vrpn_Tracker_WiimoteHead::reset(void) {
	// Reset to the identity pose
	make_null_vec(d_currentPose.xyz);
	make_identity_quat(d_currentPose.quat);

	make_null_vec(d_gravityXform.xyz);
	make_identity_quat(d_gravityXform.quat);

	vrpn_gettimeofday(&d_prevtime, NULL);

	// Set the updated flag to send a report
	d_updated = true;
	d_flipState = FLIP_UNKNOWN;
	d_lock = false;

	make_null_vec(d_vGrav);
	make_null_vec(d_vSensorZAxis);

	make_identity_quat(d_flip);

	d_vX[0] = d_vX[1] = d_vX[2] = d_vX[3] = -1;
	d_vY[0] = d_vY[1] = d_vY[2] = d_vY[3] = -1;
	d_vSize[0] = d_vSize[1] = d_vSize[2] = d_vSize[3] = -1;

	// Convert the matrix into quaternion notation and copy into the
	// tracker pos and quat elements.
	convert_pose_to_tracker();
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

	// See if we have new data, or if it has been too long since our last
	// report.  Send a new report in either case.
	vrpn_gettimeofday(&now, NULL);
	interval = duration(now, d_prevtime);

	if (shouldReport(interval)) {
		report();
	}
}

/// Update the current matrix based on the most recent values
/// received from the Wiimote regarding IR blob location.
/// Time interval is passed for potential future Kalman filter, etc.
void	vrpn_Tracker_WiimoteHead::update_pose(double time_interval) {

	q_xyz_quat_type newPose;

	// Start at the identity pose
	make_null_vec(newPose.xyz);
	make_identity_quat(newPose.quat);

	// If our gravity vector has changed and it's not 0,
	// we need to update our gravity correction transform.
	if (d_gravDirty && haveGravity()) {
		// TODO perhaps set this quaternion as our tracker2room transform?

		// Moving average
		q_vec_type movingAvg = Q_NULL_VECTOR;
		q_vec_copy (movingAvg, d_vGrav);
		q_vec_add (movingAvg, movingAvg, d_vGravPenultimate);
		q_vec_add (movingAvg, movingAvg, d_vGravAntepenultimate);
		q_vec_scale (movingAvg, 0.33333, movingAvg);

		// reset gravity transform
		make_identity_quat(d_gravityXform.quat);
		make_null_vec(d_gravityXform.xyz);

		q_vec_type regulargravity = Q_NULL_VECTOR;
		regulargravity[1] = 1;

		q_from_two_vecs (d_gravityXform.quat, movingAvg, regulargravity);

		// Set up a 180-degree rotation around sensor Z for future use
		q_vec_type zAxis = {0,1,0};
		q_xyz_quat_xform(zAxis, &d_gravityXform, zAxis);
		q_from_axis_angle(d_flip, zAxis[0], zAxis[1], zAxis[2], Q_PI);

		d_gravDirty = false;
	}

	if (d_points == 2) {
		d_lock = true;
		// we simply stop updating our pos+orientation if we lost LED's

		// TODO right now only handling the 2-LED glasses
		// TODO at a fixed 14.5cm distance (set in the constructor)

		double rx, ry, rz; // Rotation (rad)
		rx = ry = rz = 0;

		// Wiimote stats source: http://wiibrew.org/wiki/Wiimote#IR_Camera
		// TODO: verify this with spec sheet or experimental data
		const double xResSensor = 1024.0, yResSensor = 768.0;
		const double fovX = 33.0, fovY = 23.0;
		const double radPerPx = (fovX / 180.0 * M_PI) / xResSensor;
		double X0, X1, Y0, Y1;

		X0 = d_vX[0];
		X1 = d_vX[1];
		Y0 = d_vY[0];
		Y1 = d_vY[1];

		if (d_flipState == FLIP_180) {
			std::swap(X0, X1);
			std::swap(Y0, Y1);

		}

		double dx = X0 - X1;
		double dy = Y0 - Y1;
		double dist = sqrt(dx * dx + dy * dy);

		// Note that this is an approximation, since we don't know the
		// distance/horizontal position.  (I think...)
		double angle = radPerPx * dist / 2.0;
		double headDist = (d_blobDistance / 2.0) / tan(angle);

		// Translate the distance along z axis, and tilt the head
		// TODO - the 3-led version will have a more complex rotation
		// calculation to perform, since this one assumes the user
		// does not rotate their head around x or y axes

		newPose.xyz[2] = headDist; // translate along Z
		// double atan2 (double y, double x);
		rz = atan2(dy, dx); // rotate around Z

		// Find the sensor pixel of the line of sight - directly between
		// the led's
		double avgX = (X0 + X1) / 2.0;
		double avgY = (Y0 + Y1) / 2.0;

		// b is the virtual depth in the sensor from a point to the full sensor
		// used for finding similar triangles to calculate x/y translation
		const double bHoriz = xResSensor / 2 / tan(fovX / 2);
		const double bVert = -1 * yResSensor / 2 / tan(fovY / 2);

		// World head displacement (X and Y) from a centered origin at
		// the calculated distance from the sensor
		newPose.xyz[0] = headDist * (avgX - xResSensor / 2) / bHoriz;
		newPose.xyz[1] = headDist * (avgY - yResSensor / 2) / bVert;

		// set the quat. part of our pose with rotation angles
		q_from_euler(newPose.quat, rz, ry, rx);

		// Apply the new pose
		q_vec_copy(d_currentPose.xyz, newPose.xyz);
		q_copy(d_currentPose.quat, newPose.quat);

		// Finally, apply gravity to our pose and
		// and copy it into the tracker position and quaternion structures.
		convert_pose_to_tracker();
	} else {
		// TODO: right now if we don't have exactly 2 points we lose the lock
		d_lock = false;
		d_flipState = FLIP_UNKNOWN;
	}
}

void vrpn_Tracker_WiimoteHead::convert_pose_to_tracker() {

	if (haveGravity()) {
		// we know gravity, so we are correcting for it.
		// TODO (maybe): improve vrpn driver in juggler to handle tracker-to-room
		// transform and set that there, instead?
		q_xyz_quat_compose(&d_currentPose, &d_currentPose, &d_gravityXform);

	}

	if (d_flipState == FLIP_UNKNOWN) {
		q_vec_type upVec = {0, 1, 0};
		q_xform(upVec, d_currentPose.quat, upVec);
		if (upVec[1] < 0) {
			// We are upside down - we will need to rotate 180 about the sensor Z
			d_flipState = FLIP_180;
			fprintf(stderr,"vrpn_Tracker_WiimoteHead: d_flipState = FLIP_180\n");
			return;
		} else {
			// OK, we are fine - there is a positive Y component to our up vector
			d_flipState = FLIP_NORMAL;
			fprintf(stderr,"vrpn_Tracker_WiimoteHead: d_flipState = FLIP_NORMAL\n");
		}
	}

	if (d_flipState == FLIP_180) {
		//q_mult(d_currentPose.quat, d_flip, d_currentPose.quat);
	}

	q_vec_copy(pos, d_currentPose.xyz); // set position;
	q_copy(d_quat, d_currentPose.quat); // set orientation
}

vrpn_bool vrpn_Tracker_WiimoteHead::shouldReport(double elapsedInterval) const {
	// If we've gotten new wiimote reports since our last report, return true.
	if (d_updated) {
		return VRPN_TRUE;
	}

	// If it's been more than our max interval, send an update anyway
	if (elapsedInterval >= d_update_interval) {
		return VRPN_TRUE;
	}

	// Not time has elapsed, and nothing has changed, so return false.
	return VRPN_FALSE;
}

bool vrpn_Tracker_WiimoteHead::haveGravity() const {
	return (d_vGrav[0] != 0 || d_vGrav[1] != 0 || d_vGrav[2] != 0);
}
