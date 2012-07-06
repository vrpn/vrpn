#ifndef __VRPN_PYTHON_ANALOG_HPP__
#define __VRPN_PYTHON_ANALOG_HPP__

#include <vrpn_Analog.h>
#include <include/Device.hpp>

namespace vrpn_python {
  template <class device_type> class definition;

  class Analog : public Device {
    typedef vrpn_Analog_Remote vrpn_type;
    friend class definition<Analog>;

    vrpn_type *d_device;

    Analog(PyObject *error, PyObject * args);

    static PyTypeObject &getType();
    static PyMethodDef* getMethods();
    static const std::string &getName();

    static PyObject *work_on_change_handler(bool add, PyObject *obj, PyObject *args);

  public:
    vrpn_type *getDevice() { return d_device ; }

    typedef definition<Analog> _definition;
  };
}

#endif // defined(__VRPN_PYTHON_ANALOG_REMOTE_H__)
