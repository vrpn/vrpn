//======================================================================
// Example of a free-standing server suitable for compiling into an
// embedded controller on a device that has Internet communications
// ability.  Fill in the locations marked @todo to customize for a
// given device.
//
// You can test the server, using its default configurations, if it
// is on a host named myhost.mydomain.com by running:
//   vrpn_print_devices MyDevice@myhost.mydomain.com
// 
// @author russ@sensics.com
// @copyright 2016, Sensics, Inc.
// @license Apache 2.0.

#include <vrpn_Connection.h>
#include <vrpn_Analog.h>
#include <vrpn_Button.h>
#include <vrpn_Tracker.h>
#include <string>

//======================================================================
// Configuration settings for the devices.

static std::string deviceName = "MyDevice";         // @todo Set to suit
static unsigned int numTrackerSensors = 3;          // @todo Set to suit
static unsigned int numAnalogs = 1;                // @todo Set to suit
static unsigned int numButtons = 2;                 // @todo Set to suit

//======================================================================
// Declared at global scope to make it easy for all of the device-handling
// functions to use directly.  Could also pass as parameters to the
// functions.

static vrpn_Analog_Server  *analog;
static vrpn_Button_Server  *button;
static vrpn_Tracker_Server *tracker;


int main (int argc, char ** argv)
{
  // Open a connection on the default VRPN port (3883)
  static vrpn_Connection *con = vrpn_create_server_connection();

  // Open all of the devices, giving them the same name, and setting
  // how many elements each has.
  analog = vrpn_Analog_Server(deviceName.c_str(), con, numAnalogs);

  while (1) {
    do_audio_throughput_magic(ats->channels());
    ats->report_changes();
    do_video_throughput_magic(vts->channels());
    vts->report_changes();
    c->mainloop(&delay);
fprintf(stderr, "while():  a = %.2g, v = %.2g.\n", ats->channels()[0],
vts->channels()[0]);
  }
}

