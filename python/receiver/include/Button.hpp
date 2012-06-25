#ifndef __VRPN_PYTHON_BUTTON_HPP__
#define __VRPN_PYTHON_BUTTON_HPP__

#include <vrpn_Button.h>
#include <include/Device.hpp>

namespace vrpn_python {
  template <class device_type> class definition;

  class Button : public Device {
    typedef vrpn_Button_Remote vrpn_type;
    friend class definition<Button>;

    vrpn_type *d_device;

    Button(PyObject *error, PyObject * args);

    static PyTypeObject &getType();
    static PyMethodDef* getMethods();
    static const std::string &getName();

    static PyObject *work_on_change_handler(bool add, PyObject *obj, PyObject *args);

  public:
    vrpn_type *getDevice() { return d_device ; }

    typedef definition<Button> _definition;
  };
}

#endif // defined(__VRPN_PYTHON_BUTTON_REMOTE_H__)
