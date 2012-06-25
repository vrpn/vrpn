#include "include/Text_Receiver.hpp"
#include "include/callback.hpp"
#include "include/definition.hpp"
#include "include/handlers.hpp"
#include <iostream>

namespace vrpn_python {

  static PyTypeObject Text_Receiver_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Text",            // tp_name
    sizeof(Text_Receiver),      // tp_basicsize
  };

  static PyMethodDef Text_Receiver_methods[] = {
    {"mainloop", (PyCFunction)Text_Receiver::_definition::mainloop, METH_NOARGS, "Run the mainloop" },
    {"register_change_handler", (PyCFunction)Text_Receiver::_definition::register_change_handler, METH_VARARGS, "Register a callback handler to handle a position change" },
    {"unregister_change_handler", (PyCFunction)Text_Receiver::_definition::unregister_change_handler, METH_VARARGS, "Unregister a callback handler to handle a position change" },
    {NULL}  /* Sentinel */
  };

  PyTypeObject &Text_Receiver::getType() {
    return Text_Receiver_Type;
  }

  PyMethodDef* Text_Receiver::getMethods() {
    return (PyMethodDef*)Text_Receiver_methods;
  }

  const std::string &Text_Receiver::getName() {
    static const std::string name = "Text";
    return name;
  }


  Text_Receiver::Text_Receiver(PyObject *error, PyObject * args) : Device(error, args), d_device(NULL) {
  }

  PyObject * Text_Receiver::work_on_change_handler(bool add, PyObject *obj, PyObject *args) {
    try {
      Text_Receiver *self = _definition::get(obj);

      static std::string defaultCall("invalid call : register_change_handler(userdata, callback)");

      PyObject *callback;
      PyObject *userdata;
      if ((!args) || (!PyArg_ParseTuple(args,"OO",&userdata, &callback))) {
	DeviceException::launch(defaultCall);
      }

      Callback cb(userdata, callback);

      if (add) {
	if (self->getDevice()->register_message_handler(cb.getData(), handlers::change_handler<vrpn_TEXTCB>) >= 0) {
	  cb.increment();
	  Py_RETURN_TRUE;
	}
      } else {
	if (self->getDevice()->unregister_message_handler(cb.getData(), handlers::change_handler<vrpn_TEXTCB>) >= 0) {
	  cb.decrement();
	  Py_RETURN_TRUE;
	}
      }
      DeviceException::launch(defaultCall);

    } catch (DeviceException &exception) {
      PyErr_SetString(Device::s_error, exception.getReason().c_str());
    }
    return NULL;
  }

  namespace handlers {
    template<> PyObject *CALLBACK_CALL createPyObjectFromVRPN_Type(const vrpn_TEXTCB &info) {
      const char *severity;
      switch (info.type) {
      case vrpn_TEXT_NORMAL:
	severity = "normal";
	break;
      case vrpn_TEXT_WARNING:
	severity = "warning";
	break;
      case vrpn_TEXT_ERROR:
	severity = "error";
	break;
      default:
	DeviceException::launch("Invalid severity : should be normal, warning or error");
	return NULL;
      };
      return Py_BuildValue("{sOsssssi}",
			   "time", Device::getDateTimeFromTimeval(info.msg_time),
			   "message", info.message,
			   "severity", severity,
			   "level", info.level);
    }
  }
}
