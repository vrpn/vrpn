#include "include/Device.hpp"
#include "include/Connection.hpp"
#include "include/tools.hpp"
#include "include/callback.hpp"
#include <datetime.h>
#include <iostream>
#include <algorithm>

namespace vrpn_python {

  PyObject *Device::s_error = NULL;

  DeviceException::DeviceException(const std::string &reason) : BaseException(reason) {
  }

  void DeviceException::launch(const std::string &reason) {
    throw DeviceException(reason);
  }

  Device::Device(PyObject *error, PyObject * args) : Base(error), d_connection(NULL) {
    if (args) {
      char *deviceName = NULL;
      PyObject *connection = NULL;
      if (!PyArg_ParseTuple(args,"s|O",&deviceName, &connection)) {
	std::string error = "Invalid call : ";
	error +=
#if PY_MAJOR_VERSION >= 3
	  ob_base.
#endif
	  ob_type->tp_name;
	error += "(name, connection = NULL) !";
	DeviceException::launch(error);
	return;
      }

      d_deviceName = deviceName;

      if (connection) {
	if (!Connection::check(connection)) {
	  std::string error = "Invalid call : ";
	  error += 
#if PY_MAJOR_VERSION >= 3
	    ob_base.
#endif
	    ob_type->tp_name;
	  error += "(name, connection = NULL): second argument must be a connexion !";
	  DeviceException::launch(error);
	  return;
	}
	/// @todo Why is this using/needing a c-style cast?
	//d_connection = static_cast<Connection *>(connection);
	d_connection = (Connection *)(connection);
      }
    }
  }

  Device::~Device() {
    while (d_callbacks.size() > 0) {
      Callback cb(d_callbacks.back());
      cb.decrement();
      d_callbacks.pop_back();
    }
  }

  void Device::addCallback(void *callback) {
    d_callbacks.push_back(callback);
  }

  void Device::removeCallback(void *callback) {
    std::vector<void *>::iterator it = std::find(d_callbacks.begin(), d_callbacks.end(), callback);
    if (it != d_callbacks.end()) {
      d_callbacks.erase(it);
    }
  }

  bool Device::init_device_common_objects(PyObject* vrpn_module) {
    s_error = PyErr_NewException(const_cast<char *>("vrpn.error"), NULL, NULL);
    if (!s_error) {
      return false;
    }
    Py_INCREF(s_error);
    PyModule_AddObject(vrpn_module, "error", s_error);

    PyDateTime_IMPORT;
    return true;
  }

  PyObject *Device::getDateTimeFromTimeval(const struct timeval &time) {
    const time_t seconds = time.tv_sec;
    struct tm* t = gmtime ( &seconds );
    if (t) {
      return PyDateTime_FromDateAndTime(t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, time.tv_usec);
    }
    return NULL;
  }

  bool Device::getTimevalFromDateTime(PyObject *py_time, struct timeval &tv_time) {
    if ((!py_time) || (!PyDateTime_Check(py_time))) {
      return false;
    }
    struct tm t;
    t.tm_year = PyDateTime_GET_YEAR(py_time) - 1900;
    t.tm_mon  = PyDateTime_GET_MONTH(py_time) - 1;
    t.tm_mday = PyDateTime_GET_DAY(py_time);
    t.tm_hour = PyDateTime_DATE_GET_HOUR(py_time);
    t.tm_min  = PyDateTime_DATE_GET_MINUTE(py_time);
    t.tm_sec  = PyDateTime_DATE_GET_SECOND(py_time);

    tv_time.tv_sec  = mktime(&t);
    tv_time.tv_usec = PyDateTime_DATE_GET_MICROSECOND(py_time);
    return true;
  }
}
