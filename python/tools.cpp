#include "include/tools.hpp"
#include <iostream>

namespace vrpn_python {
  namespace tools {
    bool getStringFromPyObject(PyObject *py_str, std::string &std_str) {

      if (!PyUnicode_Check(py_str)) {
	return false;
      }

      PyObject *py_utf8 = PyUnicode_AsUTF8String(py_str);
      char *chr_str; Py_ssize_t len;
      if (PyBytes_AsStringAndSize(py_utf8, &chr_str, &len) < 0) {
	return false;
      }
      Py_DECREF(py_utf8);

      std_str = chr_str;

      return true;
    }

    bool getIntegerFromPyObject(PyObject *py_int, int &_int) {

      if (!PyNumber_Check(py_int)) {
	return false;
      }

      _int = PyNumber_AsSsize_t(py_int, PyExc_OverflowError);

      return true;
    }
  }
}
