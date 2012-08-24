#ifndef __VRPN_PYTHON_EXCEPTIONS_HPP__
#define __VRPN_PYTHON_EXCEPTIONS_HPP__

#include <string>

namespace vrpn_python {

  class BaseException {
    std::string d_reason;

  protected:
    BaseException(const std::string &reason);

  public:
    static void launch(const std::string &reason);

    const std::string &getReason() const { return d_reason ; }
  };

  class DeviceException : public BaseException {
  public:
    DeviceException(const std::string &reason);

  public:
    static void launch(const std::string &reason);
  };

  class CallbackException {
  };

}

#endif // defined(__VRPN_PYTHON_EXCEPTIONS_HPP__)
