#pragma once

#include <vrpn_Configure.h>
#include <vrpn_Analog.h>
#include <string>

#ifdef VRPN_USE_I2CDEV

class vrpn_OzzMaker_BerryIMU: public vrpn_Analog_Server
{
public:
  vrpn_OzzMaker_BerryIMU(std::string const &name,
    vrpn_Connection *c,
    std::string const &device = "/dev/i2c-1",
    double read_interval_seconds = 10e-3);
  ~vrpn_OzzMaker_BerryIMU();

  virtual void mainloop();

protected:
  int d_i2c_dev;    //< File opened to read and write to device
  double d_read_interval_seconds;
};

#endif

