#ifndef __VRPN_PYTHON_TRACKER_HPP__
#define __VRPN_PYTHON_TRACKER_HPP__

#include <vrpn_Tracker.h>
#include <include/Device.hpp>

namespace vrpn_python {
  template <class device_type> class definition;

  class Tracker : public Device {
    typedef vrpn_Tracker_Remote vrpn_type;
    friend class definition<Tracker>;
    vrpn_type *d_device;

    Tracker(PyObject *error, PyObject * args);

    static PyTypeObject &getType();
    static PyMethodDef* getMethods();
    static const std::string &getName();

    static PyObject *work_on_change_handler(bool add, PyObject *obj, PyObject *args);

  public:
    vrpn_type *getDevice() { return d_device ; }

    static PyObject *request_t2r_xform(PyObject *obj);
    static PyObject *request_u2s_xform(PyObject *obj);
    static PyObject *request_workspace(PyObject *obj);
    static PyObject *reset_origin(PyObject *obj);

    typedef definition<Tracker> _definition;
  };
}

#endif // defined(__VRPN_PYTHON_TRACKER_H__)
