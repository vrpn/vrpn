#include "include/Connection.hpp"
#include <iostream>

namespace vrpn_python {

  bool Connection::check(const PyObject *other) {
    return (strcmp(other->ob_type->tp_name, "vrpn.Connection") == 0);
  }

  bool Connection_IP::check(const PyObject *other) {
    return (strcmp(other->ob_type->tp_name, "vrpn.Connection_IP") == 0);
  }
}
