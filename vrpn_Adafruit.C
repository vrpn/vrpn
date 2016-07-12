#include "vrpn_Adafruit.h"
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/ioctl.h>

#ifdef VRPN_USE_I2CDEV

// Developed using information from
// http://ozzmaker.com/guide-to-interfacing-a-gyro-and-accelerometer-with-a-raspberry-pi

// Constants that describe the device
#define LSM303_CTRL_REG1_A (0x20)
#define LSM303_CTRL_REG4_A (0x23)
#define L3G_CTRL_REG1 (0x20)
#define L3G_CTRL_REG4 (0x23)

#define GYRO_ADDRESS (0x6b)
#define ACC_ADDRESS (0x19)
#define MAG_ADDRESS (0x1e)

// Helper functions
static bool select_device(int file, int addr)
{
  if (ioctl(file, I2C_SLAVE, addr) < 0) {
    return false;
  }
  return true;
}

bool write_acc_register(int file, vrpn_uint8 reg, vrpn_uint8 value)
{
  if (!select_device(file, ACC_ADDRESS)) {
    fprintf(stderr,"write_acc_register(): Cannot select device\n");
    return false;
  }
  return i2c_smbus_write_byte_data(file, reg, value) >= 0;
}

bool write_gyro_register(int file, vrpn_uint8 reg, vrpn_uint8 value)
{
  if (!select_device(file, GYRO_ADDRESS)) {
    fprintf(stderr,"write_gyro_register(): Cannot select device\n");
    return false;
  }
  return i2c_smbus_write_byte_data(file, reg, value) >= 0;
}

vrpn_Adafruit_10DOF::vrpn_Adafruit_10DOF(
    const std::string &name, vrpn_Connection *c,
    const std::string &device,
    double read_interval_seconds)
  : vrpn_Analog_Server(name.c_str(), c)
{
  // Set device parameters.
  d_read_interval_seconds = read_interval_seconds;
  num_channel = 11;
  for (vrpn_int32 i = 0; i < num_channel; i++) {
    channel[i] = 0;
    last[i] = channel[i];
  }

  //--------------------------------------------------------
  // Open the file we're going to use to talk with the device.
  d_i2c_dev = open(device.c_str(), O_RDWR);
  if (d_i2c_dev < 0) {
    fprintf(stderr,
      "vrpn_Adafruit_10DOF::vrpn_Adafruit_10DOF(): "
      "Cannot open %s\n", device.c_str());
    return;
  }

  //--------------------------------------------------------
  // Configure the sensors on the device.

  // Enable the accelerometer at a rate consistent with our
  // read rate.
  // @todo For now, 100 Hz data rate
  if (!write_acc_register(d_i2c_dev, LSM303_CTRL_REG1_A, 0b01010111)) {
    fprintf(stderr,
      "vrpn_Adafruit_10DOF::vrpn_Adafruit_10DOF(): "
      "Cannot configure accelerometer on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
//    return;
  }
  // +/- 8G full scale, FS = 10 on HLHC, high resolution output mode
  if (!write_acc_register(d_i2c_dev, LSM303_CTRL_REG4_A, 0b00101000)) {
    fprintf(stderr,
      "vrpn_Adafruit_10DOF::vrpn_Adafruit_10DOF(): "
      "Cannot configure accelerometer range on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
//    return;
  }

  // Enable the gyro.
  if (!write_gyro_register(d_i2c_dev, L3G_CTRL_REG1, 0b01010111)) {
    fprintf(stderr,
      "vrpn_Adafruit_10DOF::vrpn_Adafruit_10DOF(): "
      "Cannot configure gyro on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
//    return;
  }
  // Continuous update, 2000 dps full scale
  if (!write_acc_register(d_i2c_dev, L3G_CTRL_REG4, 0b00110000)) {
    fprintf(stderr,
      "vrpn_Adafruit_10DOF::vrpn_Adafruit_10DOF(): "
      "Cannot configure gyro range on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
//    return;
  }

  //--------------------------------------------------------
  // Record the time we opened the device.
  vrpn_gettimeofday(&timestamp, NULL);
}

vrpn_Adafruit_10DOF::~vrpn_Adafruit_10DOF()
{
  if (d_i2c_dev >= 0) {
    close(d_i2c_dev);
  }
}

void vrpn_Adafruit_10DOF::mainloop()
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

