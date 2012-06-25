#include "include/Base.hpp"
#include "include/Connection.hpp"
#include <iostream>

namespace vrpn_python {
  BaseException::BaseException(const std::string &reason) : d_reason(reason) {
  }

  void BaseException::launch(const std::string &reason) {
    throw BaseException(reason);
  }

  Base::Base(PyObject *error) : d_error(error) {
  }
}
