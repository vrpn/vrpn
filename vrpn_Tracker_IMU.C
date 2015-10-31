/// @file vrpn_Tracker_IMU.C
/// @author Russ Taylor <russ@reliasolve.com>
/// @license Standard VRPN license.

#include "vrpn_Tracker_IMU.h"
#include <cmath>

#undef VERBOSE

vrpn_IMU_Magnetometer::vrpn_IMU_Magnetometer
          (std::string name, vrpn_Connection *output_con,
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
  vrpn_IMU_Vector	*vector = (vrpn_IMU_Vector *)userdata;
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
	delete vector->ana;

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
