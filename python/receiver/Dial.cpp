#include "include/Dial.hpp"
#include "include/callback.hpp"
#include "include/definition.hpp"
#include "include/handlers.hpp"
#include <iostream>

namespace vrpn_python {

  static PyTypeObject Dial_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Dial",            // tp_name
    sizeof(Dial),      // tp_basicsize
  };

  static PyMethodDef Dial_methods[] = {
    {"mainloop", (PyCFunction)Dial::_definition::mainloop, METH_NOARGS, "Run the mainloop" },
    {"register_change_handler", (PyCFunction)Dial::_definition::register_change_handler, METH_VARARGS, "Register a callback handler to handle a position change" },
    {"unregister_change_handler", (PyCFunction)Dial::_definition::unregister_change_handler, METH_VARARGS, "Unregister a callback handler to handle a position change" },
    {NULL}  /* Sentinel */
  };

  PyTypeObject &Dial::getType() {
    return Dial_Type;
  }

  PyMethodDef* Dial::getMethods() {
    return (PyMethodDef*)Dial_methods;
  }

  const std::string &Dial::getName() {
    static const std::string name = "Dial";
    return name;
  }


  Dial::Dial(PyObject *error, PyObject * args) : Device(error, args), d_device(NULL) {
  }

  PyObject * Dial::work_on_change_handler(bool add, PyObject *obj, PyObject *args) {
    try {
      Dial *self = _definition::get(obj);

      static std::string defaultCall("invalid call : register_change_handler(userdata, callback)");

      PyObject *callback;
      PyObject *userdata;
      if ((!args) || (!PyArg_ParseTuple(args,"OO",&userdata, &callback))) {
	DeviceException::launch(defaultCall);
      }

      Callback cb(userdata, callback);

      handlers::register_handler<Dial, vrpn_DIALCB>(self, add, cb, defaultCall);
      Py_RETURN_TRUE;

    } catch (DeviceException &exception) {
      PyErr_SetString(Device::s_error, exception.getReason().c_str());
    }
    return NULL;
  }

  namespace handlers {
    template<> PyObject *CALLBACK_CALL createPyObjectFromVRPN_Type(const vrpn_DIALCB &info) {
      return Py_BuildValue("{sOsisd}",
			   "time", Device::getDateTimeFromTimeval(info.msg_time),
			   "dial", info.dial,
			   "change", info.change);
    }
  }
}
