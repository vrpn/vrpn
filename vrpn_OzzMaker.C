#include "vrpn_OzzMaker.h"
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/ioctl.h>

#ifdef VRPN_USE_I2CDEV

// Developed using information from
// http://ozzmaker.com/berryimu/
 
// Constants that describe the device
#define LSM9DS0_CTRL_REG1_XM (0x20)
#define LSM9DS0_CTRL_REG2_XM (0x21)
#define LSM9DS0_CTRL_REG1_G (0x20)
#define LSM9DS0_CTRL_REG2_G (0x21)

#define GYRO_ADDRESS (0x6a)
#define ACC_ADDRESS (0x1e)
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

bool write_meg_register(int file, vrpn_uint8 reg, vrpn_uint8 value)
{
  if (!select_device(file, MAG_ADDRESS)) {
    fprintf(stderr,"write_mag_register(): Cannot select device\n");
    return false;
  }
  return i2c_smbus_write_byte_data(file, reg, value) >= 0;
}

vrpn_OzzMaker_BerryIMU::vrpn_OzzMaker_BerryIMU(
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
      "vrpn_OzzMaker_BerryIMU::vrpn_OzzMaker_BerryIMU(): "
      "Cannot open %s\n", device.c_str());
    return;
  }

  //--------------------------------------------------------
  // Configure the sensors on the device.

  // Enable the accelerometer at a rate consistent with our read rate.
  // Enable all 3 axes for continuous update.
  // @todo For now, 100 Hz data rate
  if (!write_acc_register(d_i2c_dev, LSMDS0_CTRL_REG1_XM, 0b01100111)) {
    fprintf(stderr,
      "vrpn_OzzMaker_BerryIMU::vrpn_OzzMaker_BerryIMU(): "
      "Cannot configure accelerometer on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
    return;
  }
  // +/- 16G full scale
  if (!write_acc_register(d_i2c_dev, LSMDS0_CTRL_REG2_XM, 0b00100000)) {
    fprintf(stderr,
      "vrpn_OzzMaker_BerryIMU::vrpn_OzzMaker_BerryIMU(): "
      "Cannot configure accelerometer range on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
    return;
  }

  // Enable the gyro for all axes.
  if (!write_gyro_register(d_i2c_dev, LSM9DS0_CTRL_REG1_G, 0b00001111)) {
    fprintf(stderr,
      "vrpn_OzzMaker_BerryIMU::vrpn_OzzMaker_BerryIMU(): "
      "Cannot configure gyro on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
    return;
  }
  // Continuous update, 2000 dps full scale
  if (!write_gyro_register(d_i2c_dev, LSM9DS0_CTRL_REG1_G, 0b00110000)) {
    fprintf(stderr,
      "vrpn_OzzMaker_BerryIMU::vrpn_OzzMaker_BerryIMU(): "
      "Cannot configure gyro range on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
    return;
  }

  // @todo Enable the magnetometer

  //--------------------------------------------------------
  // Record the time we opened the device.
  vrpn_gettimeofday(&timestamp, NULL);
}

vrpn_OzzMaker_BerryIMU::~vrpn_OzzMaker_BerryIMU()
{
  if (d_i2c_dev >= 0) {
    close(d_i2c_dev);
  }
}

void vrpn_OzzMaker_BerryIMU::mainloop()
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
    fprintf(stderr,"vrpn_OzzMaker_BerryIMU::mainloop: Cannot select accelerometer\n");
    return;
  }

  // Read and parse the raw values from the accelerometer
  vrpn_uint8 block[6];
  int result = i2c_smbus_read_i2c_block_data(d_i2c_dev,
     0x80 | LSM303_OUT_X_L_A, sizeof(block), block);
  if (result != sizeof(block)) {
    printf("vrpn_OzzMaker_BerryIMU::mainloop: Failed to read from accelerometer.");
    return;
  }
  channel[0] = (block[0] | static_cast<vrpn_int16>(block[1]) << 8) >> 4;
  channel[1] = (block[2] | static_cast<vrpn_int16>(block[3]) << 8) >> 4;
  channel[2] = (block[4] | static_cast<vrpn_int16>(block[5]) << 8) >> 4;

  // Convert to meters/second/second
  // @todo

  // Select the Gyroscope device
  if (!select_device(d_i2c_dev, GYRO_ADDRESS)) {
    fprintf(stderr,"vrpn_OzzMaker_BerryIMU::mainloop: Cannot select gyroscope\n");
    return;
  }

  // Read and parse the raw values from the accelerometer
  result = i2c_smbus_read_i2c_block_data(d_i2c_dev,
    0x80 | L3G_OUT_X_L, sizeof(block), block);
  if (result != sizeof(block)) {
    printf("vrpn_OzzMaker_BerryIMU::mainloop: Failed to read from gyro.");
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

