#include "include/Poser.hpp"
#include "include/callback.hpp"
#include "include/definition.hpp"
#include <iostream>

namespace vrpn_python {

  static PyTypeObject Poser_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Poser",     // tp_name
    sizeof(Poser),      // tp_basicsize
  };

  static PyMethodDef Poser_methods[] = {
    {"mainloop", (PyCFunction)Poser::_definition::mainloop, METH_NOARGS, "Run the mainloop" },
    {"request_pose", (PyCFunction)Poser::request_pose, METH_VARARGS, "Set the pose of the poser" },
    {"request_pose_relative", (PyCFunction)Poser::request_pose_relative, METH_VARARGS, "Set the relative pose of the poser" },
    {"request_pose_velocity", (PyCFunction)Poser::request_pose_velocity, METH_VARARGS, "Set the velocity of the poser" },
    {"request_pose_velocity", (PyCFunction)Poser::request_pose_velocity, METH_VARARGS, "Set the velocity of the poser" },
    {NULL}  /* Sentinel */
  };

  PyTypeObject &Poser::getType() {
    return Poser_Type;
  }

  PyMethodDef* Poser::getMethods() {
    return (PyMethodDef*)Poser_methods;
  }

  const std::string &Poser::getName() {
    static const std::string name = "Poser";
    return name;
  }

  Poser::Poser(PyObject *error, PyObject * args) : Device(error, args), d_device(NULL) {
  }

  PyObject *Poser::request_pose(PyObject *obj, PyObject *args) {
    try {
      Poser *self = _definition::get(obj);

      static std::string defaultCall("invalid call : request_pose(datetime, double position[3], double quaternion[4])");

      PyObject *py_time = NULL;
      double position[3];
      double quaternion[4];

      if ((!args) || (!PyArg_ParseTuple(args,"O(ddd)(dddd)", &py_time,
					&position[0], &position[1], &position[2],
					&quaternion[0], &quaternion[1], &quaternion[2], &quaternion[3]))) {
	DeviceException::launch(defaultCall);
      }

      timeval time;
      if (!Device::getTimevalFromDateTime(py_time, time)) {
	DeviceException::launch("First argument must be a datetime object !");
      }

      if (!self->d_device->request_pose(time, position, quaternion)) {
	DeviceException::launch("vrpn.Poser : request_pose failed");
      }
      Py_RETURN_TRUE;

    } catch (DeviceException &exception) {
      PyErr_SetString(Device::s_error, exception.getReason().c_str());
    }
    return NULL;
  }

  PyObject *Poser::request_pose_relative(PyObject *obj, PyObject *args) {
    try {
      Poser *self = _definition::get(obj);

      static std::string defaultCall("invalid call : request_pose_relative(int time[2](second and microsecond), double position_delta[3], double quaternion[4])");

      PyObject *py_time;
      double position[3];
      double quaternion[4];

      if (!PyArg_ParseTuple(args,"O(ddd)(dddd)", &py_time,
			    &position[0], &position[1], &position[2],
			    &quaternion[0], &quaternion[1], &quaternion[2], &quaternion[3])) {
	DeviceException::launch(defaultCall);
      }

      timeval time;
      if (!Device::getTimevalFromDateTime(py_time, time)) {
	DeviceException::launch("First argument must be a datetime object !");
      }

      if (!self->d_device->request_pose_relative(time, position, quaternion)) {
	DeviceException::launch("vrpn.Poser : request_pose_relative failed");
      }

      Py_RETURN_TRUE;

    } catch (DeviceException &exception) {
      PyErr_SetString(Device::s_error, exception.getReason().c_str());
    }
    return NULL;
  }

  PyObject *Poser::request_pose_velocity(PyObject *obj, PyObject *args) {
    try {
      Poser *self = _definition::get(obj);

      static std::string defaultCall("invalid call : request_pose_velocity(int time[2](second and microsecond), double velocity[3], double quaternion[4], double interval)");

      PyObject *py_time;
      double position[3];
      double quaternion[4];
      double interval;

      if (!PyArg_ParseTuple(args,"O(ddd)(dddd)d", &py_time,
			    &position[0], &position[1], &position[2],
			    &quaternion[0], &quaternion[1], &quaternion[2], &quaternion[3],
			    &interval)) {
	DeviceException::launch(defaultCall);
      }

      timeval time;
      if (!Device::getTimevalFromDateTime(py_time, time)) {
	DeviceException::launch("First argument must be a datetime object !");
      }

      if (!self->d_device->request_pose_velocity(time, position, quaternion, interval)) {
	DeviceException::launch("vrpn.Poser : request_pose_velocity failed");
      }

      Py_RETURN_TRUE;

    } catch (DeviceException &exception) {
      PyErr_SetString(Device::s_error, exception.getReason().c_str());
    }
    return NULL;
  }

  PyObject *Poser::request_pose_velocity_relative(PyObject *obj, PyObject *args) {
    try {
      Poser *self = _definition::get(obj);

      static std::string defaultCall("invalid call : request_pose_velocity_relative(int time[2](second and microsecond), double velocity_delta[3], double quaternion[4], double interval)");

      PyObject *py_time;
      double position[3];
      double quaternion[4];
      double interval;

      if (!PyArg_ParseTuple(args,"O(ddd)(dddd)d", &py_time,
			    &position[0], &position[1], &position[2],
			    &quaternion[0], &quaternion[1], &quaternion[2], &quaternion[3],
			    &interval)) {
	DeviceException::launch(defaultCall);
      }

      timeval time;
      if (!Device::getTimevalFromDateTime(py_time, time)) {
	DeviceException::launch("First argument must be a datetime object !");
      }

      if (!self->d_device->request_pose_velocity_relative(time, position, quaternion, interval)) {
	DeviceException::launch("vrpn.Poser : request_pose_velocity_relative failed");
      }

      Py_RETURN_TRUE;

    } catch (DeviceException &exception) {
      PyErr_SetString(Device::s_error, exception.getReason().c_str());
    }
    return NULL;
  }
}
