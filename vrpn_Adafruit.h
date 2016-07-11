#pragma once

#include <vrpn_Configure.h>
#include <vrpn_Analog.h>
#include <string>

#ifdef VRPN_USE_I2CDEV

class vrpn_Adafruit_10DOF_Raspberry_pi : public vrpn_Analog_Server
{
public:
  vrpn_Adafruit_10DOF_Raspberry_pi(const std::string &name,
    vrpn_Connection *c,
    const std::string &device = "/dev/i2c-1",
    double read_interval_seconds = 5e-5);
  ~vrpn_Adafruit_10DOF_Raspberry_pi();

  virtual void mainloop();

protected:
  int d_i2c_dev;    //< File opened to read and write to device
  double d_read_interval_seconds;
};

#endif

