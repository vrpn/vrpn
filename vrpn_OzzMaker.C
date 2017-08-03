#include "vrpn_OzzMaker.h"

#ifdef VRPN_USE_I2CDEV
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <cmath>

// Developed using information from
// http://ozzmaker.com/berryimu/
 
// Constants that describe the device registers
#define LSM9DS0_CTRL_REG1_XM (0x20)
#define LSM9DS0_CTRL_REG2_XM (0x21)
#define LSM9DS0_CTRL_REG5_XM (0x24)
#define LSM9DS0_CTRL_REG1_G (0x20)
#define LSM9DS0_CTRL_REG2_G (0x21)
#define LSM9DS0_CTRL_REG4_G (0x23)
#define LSM9DS0_CTRL_REG6_G (0x25)
#define LSM9DS0_CTRL_REG7_G (0x26)

#define LSM9DS0_OUT_X_L_A (0x28)
#define LSM9DS0_OUT_X_L_G (0x28)
#define LSM9DS0_OUT_X_L_M (0x08)

// Constants that define the I2C bus addresses
#define GYRO_ADDRESS (0x6a)
#define ACC_ADDRESS (0x1e)
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

static bool write_mag_register(int file, vrpn_uint8 reg, vrpn_uint8 value)
{
  if (!select_device(file, MAG_ADDRESS)) {
    fprintf(stderr,"write_mag_register(): Cannot select device\n");
    return false;
  }
  return vrpn_i2c_smbus_write_byte_data(file, reg, value) >= 0;
}

vrpn_OzzMaker_BerryIMU::vrpn_OzzMaker_BerryIMU(
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
      "vrpn_OzzMaker_BerryIMU::vrpn_OzzMaker_BerryIMU(): "
      "Cannot open %s\n", device.c_str());
    return;
  }

  //--------------------------------------------------------
  // Configure the sensors on the device.

  // Enable the accelerometer at a rate consistent with our read rate.
  // Enable all 3 axes for continuous update.
  // @todo For now, 100 Hz data rate
  if (!write_acc_register(d_i2c_dev, LSM9DS0_CTRL_REG1_XM, 0b01100111)) {
    fprintf(stderr,
      "vrpn_OzzMaker_BerryIMU::vrpn_OzzMaker_BerryIMU(): "
      "Cannot configure accelerometer on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
    return;
  }
  // +/- 16G full scale, 773 Hz (max) anti-alias filter bandwidth
  if (!write_acc_register(d_i2c_dev, LSM9DS0_CTRL_REG2_XM, 0b00100000)) {
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
  if (!write_gyro_register(d_i2c_dev, LSM9DS0_CTRL_REG4_G, 0b00110000)) {
    fprintf(stderr,
      "vrpn_OzzMaker_BerryIMU::vrpn_OzzMaker_BerryIMU(): "
      "Cannot configure gyro range on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
    return;
  }

  // Enable the magnetometer
  // Temperature enable, M data rate = 50Hz
  // @todo Change data rate to match what we are planning to use
  if (!write_mag_register(d_i2c_dev, LSM9DS0_CTRL_REG5_XM, 0b11110000)) {
    fprintf(stderr,
      "vrpn_OzzMaker_BerryIMU::vrpn_OzzMaker_BerryIMU(): "
      "Cannot configure magnetometer on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
    return;
  }
  // +/- 12 Gauss
  if (!write_mag_register(d_i2c_dev, LSM9DS0_CTRL_REG6_G, 0b01100000)) {
    fprintf(stderr,
      "vrpn_OzzMaker_BerryIMU::vrpn_OzzMaker_BerryIMU(): "
      "Cannot configure magnetometer range on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
    return;
  }
  // Continuous update
  if (!write_mag_register(d_i2c_dev, LSM9DS0_CTRL_REG7_G, 0b00000000)) {
    fprintf(stderr,
      "vrpn_OzzMaker_BerryIMU::vrpn_OzzMaker_BerryIMU(): "
      "Cannot configure magnetometer mode on %s\n", device.c_str());
    close(d_i2c_dev);
    d_i2c_dev = -1;
    return;
  }

  // @todo Enable the temperature and pressure unit using info from
  // http://ozzmaker.com/wp-content/uploads/2015/01/BMP180-DS000-09.pdf
  // The sensor is located at address 0x77.

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
  int result = vrpn_i2c_smbus_read_i2c_block_data(d_i2c_dev,
     0x80 | LSM9DS0_OUT_X_L_A, sizeof(block), block);
  if (result != sizeof(block)) {
    printf("vrpn_OzzMaker_BerryIMU::mainloop: Failed to read from accelerometer.");
    return;
  }
  channel[0] = static_cast<vrpn_int16>(block[0] | (block[1] << 8));
  channel[1] = static_cast<vrpn_int16>(block[2] | (block[3] << 8));
  channel[2] = static_cast<vrpn_int16>(block[4] | (block[5] << 8));

  // Convert to meters/second/second.
  // For range of +/- 16g, it report 0.732 mg/count.
  const double acc_gain = 9.80665 * 0.732e-3;
  channel[0] *= acc_gain;
  channel[1] *= acc_gain;
  channel[2] *= acc_gain;

  // Select the Gyroscope device
  if (!select_device(d_i2c_dev, GYRO_ADDRESS)) {
    fprintf(stderr,"vrpn_OzzMaker_BerryIMU::mainloop: Cannot select gyroscope\n");
    return;
  }

  // Read and parse the raw values from the gyroscope
  result = vrpn_i2c_smbus_read_i2c_block_data(d_i2c_dev,
    0x80 | LSM9DS0_OUT_X_L_G, sizeof(block), block);
  if (result != sizeof(block)) {
    printf("vrpn_OzzMaker_BerryIMU::mainloop: Failed to read from gyro.");
    return;
  }
  channel[3] = static_cast<vrpn_int16>(block[0] | (block[1] << 8));
  channel[4] = static_cast<vrpn_int16>(block[2] | (block[3] << 8));
  channel[5] = static_cast<vrpn_int16>(block[4] | (block[5] << 8));

  // Convert to radians/second.
  // For 2000 degree/second full range, it reports in 70 millidegrees
  // per second for each count.
  const double gyro_gain = (VRPN_PI/180.0) * 70e-3;
  channel[3] *= gyro_gain;
  channel[4] *= gyro_gain;
  channel[5] *= gyro_gain;

  // Select the Magnetometer device
  if (!select_device(d_i2c_dev, MAG_ADDRESS)) {
    fprintf(stderr,"vrpn_OzzMaker_BerryIMU::mainloop: Cannot select magnetometer\n");
    return;
  }

  // Read and parse the raw values from the magnetometer
  result = vrpn_i2c_smbus_read_i2c_block_data(d_i2c_dev,
    0x80 | LSM9DS0_OUT_X_L_M, sizeof(block), block);
  if (result != sizeof(block)) {
    printf("vrpn_OzzMaker_BerryIMU::mainloop: Failed to read from magnetometer.");
    return;
  }
  channel[6] = static_cast<vrpn_int16>(block[0] | (block[1] << 8));
  channel[7] = static_cast<vrpn_int16>(block[2] | (block[3] << 8));
  channel[8] = static_cast<vrpn_int16>(block[4] | (block[5] << 8));

  // Convert to Gauss.
  // For 12 Gauss full range, it reports in 0.48 milliGauss
  // per second for each count.
  const double mag_gain = 0.48e-3;
  channel[6] *= mag_gain;
  channel[7] *= mag_gain;
  channel[8] *= mag_gain;

  // @todo Read and convert temperature and pressure using info from
  // http://ozzmaker.com/wp-content/uploads/2015/01/BMP180-DS000-09.pdf

  report_changes(); 
}

#endif

