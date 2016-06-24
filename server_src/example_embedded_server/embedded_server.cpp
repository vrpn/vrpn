//======================================================================
// Example of a free-standing server suitable for compiling into an
// embedded controller on a device that has the ability to communicate
// over the Internet.  There are stubs built in that show how to send
// messages of various types over the same connection.  Fill in the
// sections marked @todo to make this work with your particular device.
//
// Note: This is not the most efficient way to implement a multi-
// interface device, because it has a separate copy of the base
// class for each interface, but it is easy to code, not requiring the
// construction of a derived object that does all of the calls.  It
// uses the "has a" pattern, rather than the "is a" pattern.  An
// alternative is to define an object type that derives from Analog,
// Button, Tracker, and any other desired interface type and then
// calls the server base-class mainloop() in its mainloop() calls,
// opens the device in its constructor, and polls/sends in its
// mainloop().
// 
// @author russ@sensics.com
// @copyright 2016, Sensics, Inc.
// @license Boost Software License 1.0 (BSL1.0)

#include <vrpn_Connection.h>
#include <vrpn_Analog.h>
#include <vrpn_Button.h>
#include <vrpn_Tracker.h>

// @todo Remove if you don't need.  This is here for sin() and cos()
// in the simulated-motion code.
#include <cmath>

//======================================================================
// Configuration information for your device.  Set each of these to the
// appropriate value.  The deviceName is how all of the interfaces will
// appear to the client, to connect to a server running on the same host
// with the default name below you could use.
//  vrpn_print_devices MyDevice@localhost

static const char *deviceName = "MyDevice";   // @todo Set appropriately
static int numTrackerSensors = 3;             // @todo Set appropriately
static int numButtons = 1;                    // @todo Set appropriately
static int numAnalogChannels = 1;             // @todo Set appropriately

// @todo If you want the analog values to be reported regularly, even
// if they are not changing, set the period of repeat here to nonzero.
// With this being zero, they will only be reported when they change.
static double analogReportIntervalSeconds = 0;

// @todo This setting causes the server to delay a microsecond
// between updates on its mainloop, which avoids eating an entire CPU
// doing busy-waiting.  If you prefer, replace the pointer here with
// NULL and it will busy-wait.
static struct timeval microsecond = { 0, 1 };
static struct timeval *delay = &microsecond;

// This function services the device.  It should change any analog
// values, send tracker reports, send button reports, and set quit
// when it is time for the program to exit.
static void serviceDevice(
  vrpn_Analog_Server  *a,
  vrpn_Button_Server  *b,
  vrpn_Tracker_Server *t,
  bool *quit)
{
  // @todo Remove this block of code.
  // Static timer used to simulate regular reports coming from the
  // device.  It assumes that all reports arrive at once, but that
  // does not have to be the case, you don't have to set values for
  // all of the interfaces at the same time.
  static struct timeval lastSend = { 0, 0 };
  struct timeval now;
  vrpn_gettimeofday(&now);
  bool timeForReport = false;
  if (vrpn_TimevalDurationSeconds(now, lastSend) > 0.5) {
    timeForReport = true;
    lastSend = now;
  }

  // @todo Remove this block of code.
  // Artificial state for the devices, maintained here so that we can
  // see changes on the client side when running this program to make
  // it more interesting.
  static struct timeval start = { 0, 0 };
  if (start.tv_sec == 0) {
    vrpn_gettimeofday(&start);
  }
  double duration = vrpn_TimevalDurationSeconds(now, start);
  double analogValue = duration;
  int buttonValue = static_cast<int>(duration) % 2;

  // @todo Service your device, reading any new state.

  // @todo Replace the values sent with those read
  // from your device.
  if (timeForReport) {
    // Report the analog values.
    for (size_t i = 0; i < numAnalogChannels; i++) {
      a->channels()[i] = analogValue;
    }

    // Report any button changes
    for (int i = 0; i < numButtons; i++) {
      b->set_button(i, buttonValue);
    }

    // Report tracker positions.  Here we have each tracker be one
    // decimeter higher than the previous in Z.
    for (int i = 0; i < numTrackerSensors; i++) {
      // @todo replace with actual values, in meters.
      vrpn_float64 pos[3];
      pos[0] = sin(duration);
      pos[1] = cos(duration);
      pos[2] = i * 0.1;

      // @todo Replace with unit Quaternion telling orientation.
      // Note that the Quaternion used by VRPN matches that from
      // Quatlib, which is stored in the order X, Y, Z, W; this is
      // not usually how they are stored, so remember to re-order
      // them.
      // Note that the time should be the actual time that the
      // measurement was made, so if you have a communication delay
      // with your tracker, you should push the time slightly into
      // the past to account for that.
      // Note that all of the reports should be in the same space,
      // the "base to sensor" space for the tracker.  If you have
      // reports that are relative to other reports, transform them
      // (perhaps using Quatlib or Eigen) so that they are all in
      // the same space.  Be especially careful about velocity and
      // acceleration transforms, which require a change of basis
      // rather than just a catenation of transforms.
      vrpn_float64 quat[4];
      quat[0] = quat[1] = quat[2] = 0;
      quat[3] = 1;

      t->report_pose(i, now, pos, quat);

      // @todo Either remove the report of velocity and/or acceleration
      // or else replace their values with correct ones.  Trackers do
      // not have to report either or both of these, and not all client
      // programs use this information, but if it is present then they can
      // use it for predictive tracking.
      // The interval is the period over which the specified position
      // change (for velocity) or velocity change (for acceleration)
      // occurs.  We need to specify this because Quaternions can wrap
      // around if they get above half a rotation.  Make the interval
      // small enough so that this will not happen and scale the velocity
      // and acceleration to match.
      vrpn_float64 zero[3];
      vrpn_float64 intervalSecs = 0.01;
      t->report_pose_velocity(i, now, zero, quat, intervalSecs);
      t->report_pose_acceleration(i, now, zero, quat, intervalSecs);
    }
  }
}

int main (int argc, char ** argv)
{
  // Create the server connection that will listen on port 3883, which
  // is the default VRPN listening port.  We'll pass this to the
  // constructor for the other servers so that they will all use it
  // to communicate to any clients.
  vrpn_Connection *con = vrpn_create_server_connection();

  vrpn_Analog_Server  *analog;
  vrpn_Button_Server  *button;
  vrpn_Tracker_Server *tracker;

  // Construct each of the servers that will represent interfaces on the
  // devices.
  tracker = new vrpn_Tracker_Server(deviceName, con, numTrackerSensors);
  button = new vrpn_Button_Server(deviceName, con, numButtons);
  analog = new vrpn_Analog_Server(deviceName, con, numAnalogChannels);

  // @todo Open and configure your hardware device.

  // Keep looping until we find out that it is time to quit.
  bool quit = false;
  while (!quit) {
    // Service the hardware device, updating any analogs and also
    // sending any tracker or button reports that it detects.
    serviceDevice(analog, button, tracker, &quit);

    // The tracker and button devices will have done their reporting
    // when their values changed.
    // Have the analog device report any changes that happened.
    // If we have a non-zero report interval for guaranteed reporting,
    // go ahead and send if it has been long enough.
    analog->report_changes();
    if (analogReportIntervalSeconds != 0) {
      static struct timeval lastReport = { 0, 0 };
      struct timeval now;
      vrpn_gettimeofday(&now);
      if (vrpn_TimevalDurationSeconds(now, lastReport) >= analogReportIntervalSeconds) {
        analog->report();
        lastReport = now;
      }
    }

    // Service the devices and the connection.  We need to service the
    // devices so that they will answer ping requests from the client.
    analog->mainloop();
    button->mainloop();
    tracker->mainloop();
    con->mainloop(delay);
  }

  // @todo Shut down and close your hardware device.

  // Clean up the things we constructed, deleting the devices before the connection.
  delete tracker;
  delete button;
  delete analog;
  delete con;
}

