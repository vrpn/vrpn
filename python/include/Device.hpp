#ifndef __VRPN_PYTHON_DEVICE_HPP__
#define __VRPN_PYTHON_DEVICE_HPP__

#include "Base.hpp"
#include "Connection.hpp"
#include <string>
#include <vector>

namespace vrpn_python {
  class Device;

  class Device : public Base {

    std::string d_deviceName;
    Connection *d_connection;

    std::vector<void *> d_callbacks;

  protected:
    static PyObject *s_error;

    Device(PyObject *error, PyObject * args = NULL);
    ~Device();

    const std::string &getDeviceName() const { return d_deviceName ; }
    Connection *getConnection() { return d_connection ; }

  public:
    void addCallback(void *);
    void removeCallback(void *);

    static bool init_device_common_objects(PyObject* vrpn_module);

    static PyObject *getDateTimeFromTimeval(const struct timeval &time);
    static bool getTimevalFromDateTime(PyObject *py_time, struct timeval &tv_time);
  };
}

#endif // defined(__VRPN_PYTHON_DEVICE_H__)
