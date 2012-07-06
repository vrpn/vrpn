#ifndef __VRPN_PYTHON_TOOLS_HPP__
#define __VRPN_PYTHON_TOOLS_HPP__

#include <Python.h>
#include <string>

namespace vrpn_python {
  namespace tools {
    bool getStringFromPyObject(PyObject *, std::string &);
    bool getIntegerFromPyObject(PyObject *py_int, int &_int);
  }
}

#endif // defined(__VRPN_PYTHON_BASE_H__)
