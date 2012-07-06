#ifndef __VRPN_PYTHON_TEXT_RECEIVER_HPP__
#define __VRPN_PYTHON_TEXT_RECEIVER_HPP__

#include <vrpn_Text.h>
#include <include/Device.hpp>

namespace vrpn_python {
  template <class device_type> class definition;

  class Text_Receiver : public Device {
    typedef vrpn_Text_Receiver vrpn_type;
    friend class definition<Text_Receiver>;

    vrpn_type *d_device;

    Text_Receiver(PyObject *error, PyObject * args);

    static PyTypeObject &getType();
    static PyMethodDef* getMethods();
    static const std::string &getName();

    static PyObject *work_on_change_handler(bool add, PyObject *obj, PyObject *args);

  public:
    vrpn_type *getDevice() { return d_device ; }

    typedef definition<Text_Receiver> _definition;
  };
}

#endif // defined(__VRPN_PYTHON_TEXT_RECEIVER_H__)
