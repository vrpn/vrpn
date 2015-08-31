#include "include/Text_Sender.hpp"
#include "include/callback.hpp"
#include "include/definition.hpp"
#include <iostream>

namespace vrpn_python {

  static PyTypeObject Text_Sender_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Text",     // tp_name
    sizeof(Text_Sender),      // tp_basicsize
  };

  static PyMethodDef Text_Sender_methods[] = {
    {"mainloop", (PyCFunction)Text_Sender::_definition::mainloop, METH_NOARGS, "Run the mainloop" },
    {"send_message", (PyCFunction)Text_Sender::send_message, METH_VARARGS, "Send a text message" },
    {NULL}  /* Sentinel */
  };

  PyTypeObject &Text_Sender::getType() {
    return Text_Sender_Type;
  }

  PyMethodDef* Text_Sender::getMethods() {
    return (PyMethodDef*)Text_Sender_methods;
  }

  const std::string &Text_Sender::getName() {
    static const std::string name = "Text";
    return name;
  }

  Text_Sender::Text_Sender(PyObject *error, PyObject * args) : Device(error, args), d_device(NULL) {
  }

  PyObject *Text_Sender::send_message(PyObject *obj, PyObject *args) {
    try {
      Text_Sender *self = _definition::get(obj);

      static std::string defaultCall("invalid call : send_message(message, severity = normal, level = 0, datetime = NOW");

      char *msg = NULL;
      const char *severity = "normal";
      int level = 0;
      PyObject *py_time = NULL;

      if ((!args) || (!PyArg_ParseTuple(args,"s|siO", &msg, &severity, &level, &py_time))) {
	DeviceException::launch(defaultCall);
      }

      timeval time = vrpn_TEXT_NOW;

      if (py_time) {
	if (!Device::getTimevalFromDateTime(py_time, time)) {
	  DeviceException::launch("Last argument must be a datetime object !");
	}
      }

      vrpn_TEXT_SEVERITY type = vrpn_TEXT_NORMAL;
      if (strcmp(severity,"normal") == 0 ) {
	type = vrpn_TEXT_NORMAL;
      } else if (strcmp(severity,"warning") == 0 ) {
	type = vrpn_TEXT_WARNING;
      } else if (strcmp(severity,"error") == 0 ) {
	type = vrpn_TEXT_ERROR;
      } else {
	DeviceException::launch("Severity must be normal, warning or error");
      }

      if (self->d_device->send_message(msg, type, level, time) != 0) {
	DeviceException::launch("vrpn.sender.Text : send_message failed");
      }

      Py_RETURN_TRUE;

    } catch (DeviceException &exception) {
      PyErr_SetString(Device::s_error, exception.getReason().c_str());
    }
    return NULL;
  }
}
