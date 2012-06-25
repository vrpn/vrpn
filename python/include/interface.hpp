#ifndef __VRPN_PYTHON_INTERFACE_HPP__
#define __VRPN_PYTHON_INTERFACE_HPP__

#include <Python.h>

namespace vrpn_python {
  namespace receiver {
    bool init_types();
    void add_types(PyObject* module);
  }

  namespace sender {
    bool init_types();
    void add_types(PyObject* module);
  }

  namespace quaternion {
    bool init_types();
    void add_types(PyObject* module);
  }
}

#endif // defined(__VRPN_PYTHON_INTERFACE_H__)
