/// @file vrpn_Tracker_IMU.C
/// @author Russ Taylor <russ@reliasolve.com>
/// @license Standard VRPN license.

#include "vrpn_Tracker_IMU.h"
#include <cmath>

#undef VERBOSE

vrpn_IMU_Magnetometer::vrpn_IMU_Magnetometer
          (std::string name, vrpn_Connection *output_con,
           vrpn_IMU_Axis_Params x, vrpn_IMU_Axis_Params y, vrpn_IMU_Axis_Params z,
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
  d_ox = 1; d_oy = 0; d_oz = 0;
  channel[0] = d_ox;
  channel[1] = d_oy;
  channel[2] = d_oz;

  // Set empty axis values.
	d_x.params = x; d_y.params = y; d_z.params = z;
	d_x.ana = d_y.ana = d_z.ana = NULL;
	d_x.value = d_y.value = d_z.value = 0.0;
  d_x.time = d_prevtime; d_y.time = d_prevtime; d_z.time = d_prevtime;

	//--------------------------------------------------------------------
	// Open analog remotes for any channels that have non-empty names.
	//   If the name starts with the "*" character, use our output
  // connection rather than getting a new connection for it.
	//   Set up callbacks to handle updates to the analog values.
	setup_axis(&d_x);
	setup_axis(&d_y);
	setup_axis(&d_z);

	//--------------------------------------------------------------------
  // Set the minimum and maximum values past each other and past what
  // we expect from any real device, so they will be inialized properly
  // the first time we get a report.
  d_minx = d_miny = d_minz = 1e100;
  d_maxx = d_maxy = d_maxz = -1e100;
}

vrpn_IMU_Magnetometer::~vrpn_IMU_Magnetometer()
{
	// Tear down the analog update callbacks and remotes
	teardown_axis(&d_x);
	teardown_axis(&d_y);
	teardown_axis(&d_z);
}

// This routine handles updates of the analog values. The value coming in is
// adjusted per the parameters in the axis description, and then used to
// update the value there. The value is used by the device code in
// mainloop() to update its internal state; that work is not done here.

void	VRPN_CALLBACK vrpn_IMU_Magnetometer::handle_analog_update
                     (void *userdata, const vrpn_ANALOGCB info)
{
	vrpn_IMU_Axis	*axis = (vrpn_IMU_Axis *)userdata;
	double value = info.channel[axis->params.channel];

	// Scale the value
  axis->value = value * axis->params.scale;

  // Store the time we got the value.
  axis->time = info.msg_time;
}

// This sets up the Analog Remote for one axis, setting up the callback
// needed to adjust the value based on changes in the analog input.
// Returns 0 on success and -1 on failure.

int	vrpn_IMU_Magnetometer::setup_axis(vrpn_IMU_Axis *axis)
{
  // Set a default value of 0 and a time of now.
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  axis->value = 0;
  axis->time = now;

	// If the name is empty, we're done.
	if (axis->params.name.size() == 0) { return 0; }

	// Open the analog device and point the remote at it.
	// If the name starts with the '*' character, use the server
  // connection rather than making a new one.
	if (axis->params.name[0] == '*') {
		axis->ana = new vrpn_Analog_Remote(axis->params.name.c_str()+1,
                      d_connection);
#ifdef	VERBOSE
		printf("vrpn_Tracker_AnalogFly: Adding local analog %s\n",
                          axis->params.name.c_str()+1);
#endif
	} else {
		axis->ana = new vrpn_Analog_Remote(axis->params.name.c_str());
#ifdef	VERBOSE
		printf("vrpn_Tracker_AnalogFly: Adding remote analog %s\n",
                          axis->params.name.c_str());
#endif
	}
	if (axis->ana == NULL) {
		fprintf(stderr,"vrpn_Tracker_AnalogFly: "
                        "Can't open Analog %s\n",axis->params.name.c_str());
		return -1;
	}

	// Set up the callback handler for the channel
	return axis->ana->register_change_handler(axis, handle_analog_update);
}

// This tears down the Analog Remote for one channel, undoing everything that
// the setup did. Returns 0 on success and -1 on failure.

int	vrpn_IMU_Magnetometer::teardown_axis(vrpn_IMU_Axis *axis)
{
	int	ret;

	// If the analog pointer is empty, we're done.
	if (axis->ana == NULL) { return 0; }

	// Turn off the callback handler for the channel
	ret = axis->ana->unregister_change_handler((void*)axis,
                          handle_analog_update);

	// Delete the analog device.
	delete axis->ana;

	return ret;
}
 
void vrpn_IMU_Magnetometer::mainloop()
{
  struct timeval	now;
  double	interval;	// How long since the last report, in secs

  // Call generic server mainloop, since we are a server
  server_mainloop();

  // Keep track of the minimum and maximum values of
  // our reports.
  if (d_x.value < d_minx) { d_minx = d_x.value; }
  if (d_y.value < d_miny) { d_miny = d_y.value; }
  if (d_z.value < d_minz) { d_minz = d_z.value; }

  if (d_x.value > d_maxx) { d_maxx = d_x.value; }
  if (d_y.value > d_maxy) { d_maxy = d_y.value; }
  if (d_z.value > d_maxz) { d_maxz = d_z.value; }

  // Mainloop() all of the analogs that are defined
  // so that we will get all of the values fresh.
  if (d_x.ana != NULL) { d_x.ana->mainloop(); };
  if (d_y.ana != NULL) { d_y.ana->mainloop(); };
  if (d_z.ana != NULL) { d_z.ana->mainloop(); };

  // See if it has been long enough since our last report.
  // If so, generate a new one.
  vrpn_gettimeofday(&now, NULL);
  interval = vrpn_TimevalDurationSeconds(now, d_prevtime);

  if (should_report(interval)) {

    // Set the time on the report to the largest time of any
    // of our axes.
    struct timeval max_time = d_x.time;
    if (vrpn_TimevalGreater(d_y.time, max_time)) {
      max_time = d_y.time;
    }
    if (vrpn_TimevalGreater(d_z.time, max_time)) {
      max_time = d_z.time;
    }

    // Compute the unit vector by finding the normalized
    // distance between the min and max for each axis and
    // converting it to the range (-1,1) and then normalizing
    // the resulting vector.  Watch out for min and max
    // being the same.
    double dx = d_x.value - d_minx;
    if (dx == 0) {
      d_ox = 0;
    } else {
      d_ox = -1 + 2 * dx / (d_maxx - d_minx);
    }
    double dy = d_y.value - d_miny;
    if (dy == 0) {
      d_oy = 0;
    } else {
      d_oy = -1 + 2 * dy / (d_maxy - d_miny);
    }
    double dz = d_z.value - d_minz;
    if (dz == 0) {
      d_oz = 0;
    } else {
      d_oz = -1 + 2 * dz / (d_maxz - d_minz);
    }
    double len = sqrt( d_ox*d_ox + d_oy*d_oy + d_oz*d_oz );
    if (len == 0) {
      d_ox = 1;
      d_oy = 0;
      d_oz = 0;
    } else {
      d_ox /= len;
      d_oy /= len;
      d_oz /= len;
    }

    // pack and deliver analog unit vector report;
    if (d_connection) {
      channel[0] = d_ox;
      channel[1] = d_oy;
      channel[2] = d_oz;
      vrpn_Analog_Server::report();
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

bool vrpn_IMU_Magnetometer::should_report(double elapsedInterval) const
{
  // If we haven't had enough time pass yet, don't report.
  if (elapsedInterval < d_update_interval) {
    return VRPN_FALSE;
  }

  // If we're sending a report every interval, regardless of
  // whether or not there are changes, then send one now.
  if (!d_report_changes) {
    return VRPN_TRUE;
  }

  // Enough time has elapsed, but nothing has changed, so return false.
  return VRPN_FALSE;
}

