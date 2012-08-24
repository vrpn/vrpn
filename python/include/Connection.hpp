#ifndef __VRPN_PYTHON_CONNECTION_HPP__
#define __VRPN_PYTHON_CONNECTION_HPP__

#include <vrpn_Tracker.h>
#include "Base.hpp"

namespace vrpn_python {
  class Connection : public Base {
    vrpn_Connection *d_connection;

  public:
    static bool check(const PyObject *other);

    vrpn_Connection *getConnection() { return d_connection ; }
  };

  class Connection_IP : public Connection {

  public:
    static bool check(const PyObject *other);
  };


}

#endif // defined(__VRPN_PYTHON_CONNECTION_REMOTE_H__)
