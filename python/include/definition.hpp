#ifndef __VRPN_PYTHON_DEFINITION_HPP__
#define __VRPN_PYTHON_DEFINITION_HPP__

#include "Connection.hpp"
#include "Device.hpp"
#include <iostream>

namespace vrpn_python {

  template <class device_type> class definition {

    typedef typename device_type::vrpn_type vrpn_type;

  public:
    static bool check(const PyObject *obj) {
      if (!obj) {
	return false;
      }
      if (PyType_IsSubtype(obj->ob_type, &device_type::getType())) {
	return true;
      }
      return (device_type::getName() == device_type::getName());
    }

    static device_type *get(PyObject *obj) {
      if (!obj) {
	std::string error = "Invalid object mapping from 'NULL' to '";
	error += device_type::getName();
	error += "' !";
	DeviceException::launch(error);
      }
      if (!check(obj)) {
	std::string error = "Invalid object mapping from '";
	error += obj->ob_type->tp_name;
	error += "' to '";
	error += device_type::getName();
	error += "' !";
	DeviceException::launch(error);
      }
      return (device_type*)obj;
    }

    static int init(PyObject *obj, PyObject *args, PyObject * /*kwds*/) {
      try {
	get(obj);
	device_type *self = new(obj) device_type(device_type::s_error, args);
	Connection *Py_Connection = self->getConnection();
	vrpn_Connection *connection = NULL;
	if (Py_Connection != NULL) {
	  connection = Py_Connection->getConnection();
	}
	self->d_device = new vrpn_type(self->getDeviceName().c_str(), connection);
	if (self->d_device) {
	  return 0;
	}
      } catch (DeviceException &exception) {
	PyErr_SetString(device_type::s_error, exception.getReason().c_str());
      }
      return -1;
    }

    static void dealloc(PyObject* obj) {
      try {
	device_type *self = get(obj);
	self->~device_type();
	if (self->d_device) {
	  delete self->d_device;
	  self->d_device = NULL;
	}
      } catch (DeviceException &exception) {
	PyErr_SetString(device_type::s_error, exception.getReason().c_str());
      }
      Py_TYPE(obj)->tp_free(obj);
    }

    static PyObject *mainloop(PyObject *obj) {
      try {
	device_type *self = get(obj);
	self->d_device->mainloop();
	Py_RETURN_TRUE;
      } catch (DeviceException &exception) {
	PyErr_SetString(device_type::s_error, exception.getReason().c_str());
	Py_RETURN_FALSE;
      } catch (CallbackException) {
	return NULL;
      }
    }

    static bool init_type() {
      PyTypeObject &device_object_type = device_type::getType();
      device_object_type.tp_new        = PyType_GenericNew; 
      device_object_type.tp_dealloc    = dealloc;
      device_object_type.tp_init       = init;
      device_object_type.tp_flags      = Py_TPFLAGS_DEFAULT;
      std::string doc = device_type::getName() + " VRPN objects";
      device_object_type.tp_doc        = doc.c_str();
      device_object_type.tp_methods    = device_type::getMethods();

      if (PyType_Ready(&device_object_type) < 0)
	return false;
      return true;
    }

    static void add_type(PyObject *module) {
      PyTypeObject &device_object_type = device_type::getType();
      Py_INCREF(&device_object_type);
      PyModule_AddObject(module, device_type::getName().c_str(), (PyObject *)&device_object_type);

      std::string error_name = device_type::getName() + ".error";
      char *exception_name = new char [strlen(error_name.c_str()) + 1];
      strcpy(exception_name, error_name.c_str());
      device_type::s_error = PyErr_NewException(exception_name, NULL, NULL);
      delete [] exception_name;
      Py_INCREF(device_type::s_error);
      PyModule_AddObject(module, error_name.c_str(), device_type::s_error);
    }

    static PyObject *register_change_handler(PyObject *obj, PyObject *args) {
      return device_type::work_on_change_handler(true, obj, args) ;
    }

    static PyObject *unregister_change_handler(PyObject *obj, PyObject *args) {
      return device_type::work_on_change_handler(false, obj, args) ;
    }

  };
}

#endif // defined(__VRPN_PYTHON_DEFINITION_H__)
