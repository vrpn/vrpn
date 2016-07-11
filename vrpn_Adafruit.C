#include "vrpn_Adafruit.h"
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>

#ifdef VRPN_USE_I2CDEV

static bool select_device(int file, int addr)
{
  if (ioctl(file, I2C_SLAVE, addr) < 0) {
    return false;
  }
  return true;
}

vrpn_Adafruit_10DOF_Raspberry_pi::vrpn_Adafruit_10DOF_Raspberry_pi(
    const std::string &name, vrpn_Connection *c,
    const std::string &device,
    double read_interval_seconds)
  : vrpn_Analog_Server(name.c_str(), c)
{
  // Set device parameters.
  d_read_interval_seconds = read_interval_seconds;
  num_channel = 11;
  for (size_t i = 0; i < num_channel; i++) {
    channel[i] = 0;
    last[i] = channel[i];
  }

  // Open the file we're going to use to talk with the device.
  d_i2c_dev = open(device.c_str(), O_RDWR);
  if (d_i2c_dev < 0) {
    fprintf(stderr,
      "vrpn_Adafruit_10DOF_Raspberry_pi::vrpn_Adafruit_10DOF_Raspberry_pi(): "
      "Cannot open %s", device.c_str());
    return;
  }

  // Configure the sensors on the device.

  // Record the time we opened the device.
  vrpn_gettimeofday(&timestamp, NULL);
}

vrpn_Adafruit_10DOF_Raspberry_pi::~vrpn_Adafruit_10DOF_Raspberry_pi()
{
  if (d_i2c_dev >= 0) {
    close(d_i2c_dev);
  }
}

void vrpn_Adafruit_10DOF_Raspberry_pi::mainloop()
{
  // Check and see if we have an open device.  If not, return.
  if (d_i2c_dev < 0) { return; }

  // Check to see if it has been long enough since our last report.
  // if not, return.  If so, reset the timestamp to now.
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  double duration = vrpn_TimevalDurationSeconds(now, timestamp);
  if (duration < d_read_interval_seconds) { return; }
  timestamp = now;

}

#endif

