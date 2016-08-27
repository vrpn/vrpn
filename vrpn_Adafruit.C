#include "vrpn_Adafruit.h"

#ifdef VRPN_USE_I2CDEV
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/ioctl.h>

// Developed using information from
// http://ozzmaker.com/guide-to-interfacing-a-gyro-and-accelerometer-with-a-raspberry-pi
// @todo This implementation is not yet complete, nor completely tested.
// It does open the device and read raw values from the acceleromater. The gyro
// read may be working as well.

// Constants that describe the device
#define LSM303_CTRL_REG1_A (0x20)
#define LSM303_CTRL_REG4_A (0x23)
#define LSM303_OUT_X_L_A (0x28)
#define L3G_CTRL_REG1 (0x20)
#define L3G_CTRL_REG4 (0x23)
#define L3G_OUT_X_L (0x28)

#define GYRO_ADDRESS (0x6b)
#define ACC_ADDRESS (0x19)
#define MAG_ADDRESS (0x1e)

#include "vrpn_i2c_helpers.h"

// Helper functions
static bool select_device(int file, int addr)
{
  if (ioctl(file, I2C_SLAVE, addr) < 0) {
    return false;
  }
  return true;
}

static bool write_acc_register(int file, vrpn_uint8 reg, vrpn_uint8 value)
{
  if (!select_device(file, ACC_ADDRESS)) {
    fprintf(stderr,"write_acc_register(): Cannot select device\n");
    return false;
  }
  return vrpn_i2c_smbus_write_byte_data(file, reg, value) >= 0;
}

static bool write_gyro_register(int file, vrpn_uint8 reg, vrpn_uint8 value)
{
  if (!select_device(file, GYRO_ADDRESS)) {
    fprintf(stderr,"write_gyro_register(): Cannot select device\n");
    return false;
  }
  return vrpn_i2c_smbus_write_byte_data(file, reg, value) >= 0;
}

vrpn_Adafruit_10DOF::vrpn_Adafruit_10DOF(
    std::string const &name, vrpn_Connection *c,
    std::string const &device,
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
    return;
  }
  // +/- 8G full scale, FS = 10 on HLHC, high resolution output mode
  if (!write_acc_register(d_i2c_dev, LSM303_CTRL_REG4_A, 0b00101000)) {
    fprintf(stderr,
      "vrpn_Adafruit_10DOF::vrpn_Adafruit_10DOF(): "
      "Cannot configure accelerometer range on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
    return;
  }

  // Enable the gyro.
  if (!write_gyro_register(d_i2c_dev, L3G_CTRL_REG1, 0b01010111)) {
    fprintf(stderr,
      "vrpn_Adafruit_10DOF::vrpn_Adafruit_10DOF(): "
      "Cannot configure gyro on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
    return;
  }
  // Continuous update, 2000 dps full scale
  if (!write_gyro_register(d_i2c_dev, L3G_CTRL_REG4, 0b00110000)) {
    fprintf(stderr,
      "vrpn_Adafruit_10DOF::vrpn_Adafruit_10DOF(): "
      "Cannot configure gyro range on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
    return;
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
  server_mainloop();

  // Check and see if we have an open device.  If not, return.
  if (d_i2c_dev < 0) { return; }

  // Check to see if it has been long enough since our last report.
  // if not, return.  If so, reset the timestamp to now.
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  double duration = vrpn_TimevalDurationSeconds(now, timestamp);
  if (duration < d_read_interval_seconds) { return; }
  timestamp = now;

  // Select the Accelerometer device
  if (!select_device(d_i2c_dev, ACC_ADDRESS)) {
    fprintf(stderr,"vrpn_Adafruit_10DOF::mainloop: Cannot select accelerometer\n");
    return;
  }

  // Read and parse the raw values from the accelerometer
  vrpn_uint8 block[6];
  int result = vrpn_i2c_smbus_read_i2c_block_data(d_i2c_dev,
     0x80 | LSM303_OUT_X_L_A, sizeof(block), block);
  if (result != sizeof(block)) {
    printf("vrpn_Adafruit_10DOF::mainloop: Failed to read from accelerometer.");
    return;
  }
  channel[0] = (block[0] | static_cast<vrpn_int16>(block[1]) << 8) >> 4;
  channel[1] = (block[2] | static_cast<vrpn_int16>(block[3]) << 8) >> 4;
  channel[2] = (block[4] | static_cast<vrpn_int16>(block[5]) << 8) >> 4;

  // Convert to meters/second/second
  // @todo

  // Select the Gyroscope device
  if (!select_device(d_i2c_dev, GYRO_ADDRESS)) {
    fprintf(stderr,"vrpn_Adafruit_10DOF::mainloop: Cannot select gyroscope\n");
    return;
  }

  // Read and parse the raw values from the accelerometer
  result = vrpn_i2c_smbus_read_i2c_block_data(d_i2c_dev,
    0x80 | L3G_OUT_X_L, sizeof(block), block);
  if (result != sizeof(block)) {
    printf("vrpn_Adafruit_10DOF::mainloop: Failed to read from gyro.");
    return;
  }
  channel[4] = (block[0] | static_cast<vrpn_int16>(block[1]) << 8);
  channel[5] = (block[2] | static_cast<vrpn_int16>(block[3]) << 8);
  channel[6] = (block[4] | static_cast<vrpn_int16>(block[5]) << 8);

  // Convert to radians/second
  // @todo

  report_changes(); 
}

#endif

