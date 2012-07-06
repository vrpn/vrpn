#ifndef __VRPN_PYTHON_DIAL_HPP__
#define __VRPN_PYTHON_DIAL_HPP__

#include <vrpn_Dial.h>
#include <include/Device.hpp>

namespace vrpn_python {
  template <class device_type> class definition;

  class Dial : public Device {
    typedef vrpn_Dial_Remote vrpn_type;
    friend class definition<Dial>;

    vrpn_type *d_device;

    Dial(PyObject *error, PyObject * args);

    static PyTypeObject &getType();
    static PyMethodDef* getMethods();
    static const std::string &getName();

    static PyObject *work_on_change_handler(bool add, PyObject *obj, PyObject *args);

  public:
    vrpn_type *getDevice() { return d_device ; }

    typedef definition<Dial> _definition;
  };
}

#endif // defined(__VRPN_PYTHON_DIAL_REMOTE_H__)
