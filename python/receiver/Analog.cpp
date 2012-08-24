#include "include/Analog.hpp"
#include "include/callback.hpp"
#include "include/definition.hpp"
#include "include/handlers.hpp"
#include <iostream>

namespace vrpn_python {

  static PyTypeObject Analog_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Analog",   // tp_name
    sizeof(Analog),      // tp_basicsize
  };

  static PyMethodDef Analog_methods[] = {
    {"mainloop", (PyCFunction)Analog::_definition::mainloop, METH_NOARGS, "Run the mainloop" },
    {"register_change_handler", (PyCFunction)Analog::_definition::register_change_handler, METH_VARARGS, "Register a callback handler to handle a position change" },
    {"unregister_change_handler", (PyCFunction)Analog::_definition::unregister_change_handler, METH_VARARGS, "Unregister a callback handler to handle a position change" },
    {NULL}  /* Sentinel */
  };

  PyTypeObject &Analog::getType() {
    return Analog_Type;
  }

  PyMethodDef* Analog::getMethods() {
    return (PyMethodDef*)Analog_methods;
  }

  const std::string &Analog::getName() {
    static const std::string name = "Analog";
    return name;
  }


  Analog::Analog(PyObject *error, PyObject * args) : Device(error, args), d_device(NULL) {
  }

  PyObject * Analog::work_on_change_handler(bool add, PyObject *obj, PyObject *args) {
    try {
      Analog *self = _definition::get(obj);

      static std::string defaultCall("invalid call : register_change_handler(userdata, callback)");

      PyObject *callback;
      PyObject *userdata;
      if ((!args) || (!PyArg_ParseTuple(args,"OO",&userdata, &callback))) {
	DeviceException::launch(defaultCall);
      }

      Callback cb(userdata, callback);

      handlers::register_handler<Analog, vrpn_ANALOGCB>(self, add, cb, defaultCall);
      Py_RETURN_TRUE;

    } catch (DeviceException &exception) {
      PyErr_SetString(Device::s_error, exception.getReason().c_str());
    }
    return NULL;
  }

  namespace handlers {
    template<> PyObject *CALLBACK_CALL createPyObjectFromVRPN_Type(const vrpn_ANALOGCB &info) {
      PyObject *channel = PyTuple_New(info.num_channel);
      for (int i = 0 ; i < info.num_channel ; i++) {
	PyTuple_SetItem(channel, i, Py_BuildValue("f", info.channel[i]));
      }
      PyObject *result = Py_BuildValue("{sOsO}",
				       "time", Device::getDateTimeFromTimeval(info.msg_time),
				       "channel", channel);
      return result;
    }
  }
}
