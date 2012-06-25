#ifndef __VRPN_PYTHON_HANDLERS_HPP__
#define __VRPN_PYTHON_HANDLERS_HPP__

#include "callback.hpp"
#include "exceptions.hpp"
#include <iostream>

namespace vrpn_python {
  class Device;

  namespace handlers {
    template <typename vrpn_info_type> PyObject *CALLBACK_CALL createPyObjectFromVRPN_Type(const vrpn_info_type &info);

    template <typename vrpn_info_type> void CALLBACK_CALL change_handler(void *data, const vrpn_info_type info) {
      PyObject *userdata;
      PyObject *callback;

      Callback::get(data, userdata, callback);

      PyObject *value = createPyObjectFromVRPN_Type<vrpn_info_type>(info);
      PyObject *arglist = Py_BuildValue("OO", userdata, value);
      Py_DECREF(value); // Destroy entity created by createPyObjectFromVRPN_Type<>(info)

      PyObject *result = PyEval_CallObject(callback,arglist);
      Py_DECREF(arglist); // Destroy entities created by Py_BuildValue("OO", userdata, value)

      if (result != NULL) {
	Py_DECREF(result); // Destroy the evaluation !
      } else {
	throw CallbackException();
      }
    }

    template <class device_type, typename vrpn_info_type> void register_handler(device_type *self, bool add, Callback &cb, int sensor, const std::string &error) {
      if (add) {
	if (self->getDevice()->register_change_handler(cb.getData(), change_handler<vrpn_info_type>, sensor) >= 0) {
	  cb.increment();
	  return;
	}
      } else {
	if (self->getDevice()->unregister_change_handler(cb.getData(), change_handler<vrpn_info_type>, sensor) >= 0) {
	  cb.decrement();
	  return;
	}
      }
      DeviceException::launch(error);
    }

    template <class device_type, typename vrpn_info_type> void register_handler(device_type *self, bool add, Callback &cb, const std::string &error) {
      if (add) {
	if (self->getDevice()->register_change_handler(cb.getData(), change_handler<vrpn_info_type>) >= 0) {
	  cb.increment();
	  return;
	}
      } else {
	if (self->getDevice()->unregister_change_handler(cb.getData(), change_handler<vrpn_info_type>) >= 0) {
	  cb.decrement();
	  return;
	}
      }
      DeviceException::launch(error);
    }
  }
}

#endif // defined(__VRPN_PYTHON_HANDLERS_H__)
