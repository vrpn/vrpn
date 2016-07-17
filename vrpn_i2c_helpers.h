// Helper functions that are not present in the i2c-dev.h file on
// all platforms.  They fill in the parameters and call the appropriate
// ioctl() and then package info for return.

// Ensure that we don't include i2c.h on platforms like Raspberry Pi
// where they pulled these definitions into i2c-dev.h rather than just
// doing #include i2c.h, and where they also did not define _LINUX_I2C_H
// to guard against its future inclusion.  Here, we pick one of the
// things that are defined in that file and check for it.

#ifndef I2C_M_TEN
#include <linux/i2c.h>
#endif

static inline vrpn_int32 vrpn_i2c_smbus_access(
        int file, char read_write, vrpn_uint8 command,
        int size, union i2c_smbus_data *data)
{ 
  struct i2c_smbus_ioctl_data args;
  
  args.read_write = read_write;
  args.command = command;
  args.size = size;
  args.data = data;
  return ioctl(file,I2C_SMBUS,&args);
}

static inline vrpn_int32 vrpn_i2c_smbus_write_byte_data(
        int file, vrpn_uint8 command, vrpn_uint8 value)
{
  union i2c_smbus_data data; 
  data.byte = value;
  return vrpn_i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
    I2C_SMBUS_BYTE_DATA, &data);
}

static inline vrpn_int32 vrpn_i2c_smbus_read_i2c_block_data(
        int file, vrpn_uint8 command, 
        vrpn_uint8 length, vrpn_uint8 *values)
{
  union i2c_smbus_data data;
  int i;

  if (length > 32) { length = 32; }
  data.block[0] = length;
  if (vrpn_i2c_smbus_access(file,I2C_SMBUS_READ,command,
      length == 32 ? I2C_SMBUS_I2C_BLOCK_BROKEN :
      I2C_SMBUS_I2C_BLOCK_DATA,&data)) {
    return -1;  
  } else {
    for (i = 0; i < data.block[0]; i++) {
      values[i] = data.block[i+1];
    }
    return data.block[0];
  }
}

