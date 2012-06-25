#ifndef __VRPN_PYTHON_BASE_HPP__
#define __VRPN_PYTHON_BASE_HPP__

#include <Python.h>
#include <string>
#include "exceptions.hpp"

namespace vrpn_python {
  class Base {
  protected:
    PyObject_HEAD
    PyObject *d_error;
    Base(PyObject *error);

  };
}

#endif // defined(__VRPN_PYTHON_BASE_H__)
