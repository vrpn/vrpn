#ifndef __VRPN_PYTHON_POSER_HPP__
#define __VRPN_PYTHON_POSER_HPP__

#include <vrpn_Poser.h>
#include <include/Device.hpp>

namespace vrpn_python {
  template <class device_type> class definition;

  class Poser : public Device {
    typedef vrpn_Poser_Remote vrpn_type;
    friend class definition<Poser>;
    vrpn_type *d_device;

    Poser(PyObject *error, PyObject * args);

    static PyTypeObject &getType();
    static PyMethodDef* getMethods();
    static const std::string &getName();

  public:
    vrpn_type *getDevice() { return d_device ; }

    typedef definition<Poser> _definition;

    static PyObject *request_pose(PyObject *obj, PyObject *args);
    static PyObject *request_pose_relative(PyObject *obj, PyObject *args);
    static PyObject *request_pose_velocity(PyObject *obj, PyObject *args);
    static PyObject *request_pose_velocity_relative(PyObject *obj, PyObject *args);
  };
}

#endif // defined(__VRPN_PYTHON_POSER_H__)
