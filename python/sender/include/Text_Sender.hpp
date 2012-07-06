#ifndef __VRPN_PYTHON_TEXT_SENDER_HPP__
#define __VRPN_PYTHON_TEXT_SENDER_HPP__

#include <vrpn_Text.h>
#include <include/Device.hpp>

namespace vrpn_python {
  template <class device_type> class definition;

  class Text_Sender : public Device {
    typedef vrpn_Text_Sender vrpn_type;
    friend class definition<Text_Sender>;
    vrpn_type *d_device;

    Text_Sender(PyObject *error, PyObject * args);

    static PyTypeObject &getType();
    static PyMethodDef* getMethods();
    static const std::string &getName();

  public:
    vrpn_type *getDevice() { return d_device ; }

    typedef definition<Text_Sender> _definition;

    static PyObject *send_message(PyObject *obj, PyObject *args);
  };
}

#endif // defined(__VRPN_PYTHON_TEXT_SENDER_H__)
