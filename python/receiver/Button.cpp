#include "include/Button.hpp"
#include "include/callback.hpp"
#include "include/definition.hpp"
#include "include/handlers.hpp"
#include <iostream>

namespace vrpn_python {

  static PyTypeObject Button_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Button",   // tp_name
    sizeof(Button),      // tp_basicsize
  };

  static PyMethodDef Button_methods[] = {
    {"mainloop", (PyCFunction)Button::_definition::mainloop, METH_NOARGS, "Run the mainloop" },
    {"register_change_handler", (PyCFunction)Button::_definition::register_change_handler, METH_VARARGS, "Register a callback handler to handle a position change" },
    {"unregister_change_handler", (PyCFunction)Button::_definition::unregister_change_handler, METH_VARARGS, "Unregister a callback handler to handle a position change" },
    {NULL}  /* Sentinel */
  };

  PyTypeObject &Button::getType() {
    return Button_Type;
  }

  PyMethodDef* Button::getMethods() {
    return (PyMethodDef*)Button_methods;
  }

  const std::string &Button::getName() {
    static const std::string name = "Button";
    return name;
  }


  Button::Button(PyObject *error, PyObject * args) : Device(error, args), d_device(NULL) {
  }

  PyObject * Button::work_on_change_handler(bool add, PyObject *obj, PyObject *args) {
    try {
      Button *self = _definition::get(obj);

      static std::string defaultCall("invalid call : register_change_handler(userdata, callback)");

      PyObject *callback;
      PyObject *userdata;
      if ((!args) || (!PyArg_ParseTuple(args,"OO",&userdata, &callback))) {
	DeviceException::launch(defaultCall);
      }

      Callback cb(userdata, callback);

      handlers::register_handler<Button, vrpn_BUTTONCB>(self, add, cb, defaultCall);
      Py_RETURN_TRUE;

    } catch (DeviceException &exception) {
      PyErr_SetString(Button::s_error, exception.getReason().c_str());
    }
    return NULL;
  }

  namespace handlers {
    template<> PyObject *CALLBACK_CALL createPyObjectFromVRPN_Type(const vrpn_BUTTONCB &info) {
      return Py_BuildValue("{sOsisi}",
			   "time", Device::getDateTimeFromTimeval(info.msg_time),
			   "button", info.button,
			   "state", info.state);
    }
  }
}
