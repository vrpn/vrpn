#include "include/Tracker.hpp"
#include "include/definition.hpp"
#include "include/callback.hpp"
#include "include/handlers.hpp"
#include <iostream>

namespace vrpn_python {

  static PyTypeObject Tracker_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Tracker",            // tp_name
    sizeof(Tracker),      // tp_basicsize
  };

#define WRAPPER_REQUEST(NAME, DOC) {#NAME, (PyCFunction)Tracker::NAME, METH_NOARGS, DOC }

  static PyMethodDef Tracker_methods[] = {
    {"mainloop", (PyCFunction)Tracker::_definition::mainloop, METH_NOARGS, "Run the mainloop" },
    {"register_change_handler", (PyCFunction)Tracker::_definition::register_change_handler, METH_VARARGS, "Register a callback handler to handle a position change" },
    {"unregister_change_handler", (PyCFunction)Tracker::_definition::unregister_change_handler, METH_VARARGS, "Unregister a callback handler to handle a position change" },
    WRAPPER_REQUEST(request_t2r_xform, "request room from tracker xforms"),
    WRAPPER_REQUEST(request_u2s_xform, "request all available sensor from unit xforms"),
    WRAPPER_REQUEST(request_workspace, "request workspace bounding box"),
    WRAPPER_REQUEST(reset_origin, "reset origin to current tracker location (e.g. - to reinitialize a PHANToM in its reset position)"),
    {NULL}  /* Sentinel */
  };
#undef WRAPPER_REQUEST

  PyTypeObject &Tracker::getType() {
    return Tracker_Type;
  }

  PyMethodDef* Tracker::getMethods() {
    return (PyMethodDef*)Tracker_methods;
  }

  const std::string &Tracker::getName() {
    static const std::string name = "Tracker";
    return name;
  }

  Tracker::Tracker(PyObject *error, PyObject * args) : Device(error, args), d_device(NULL) {
  }

  PyObject * Tracker::work_on_change_handler(bool add, PyObject *obj, PyObject *args) {
    try {
      Tracker *self = _definition::get(obj);

      static std::string defaultCall("invalid call : register_change_handler(userdata, callback, type, sensor)");

      PyObject *callback;
      PyObject *userdata;
      char *_type;
      int sensor = vrpn_ALL_SENSORS;
      if ((!args) || (!PyArg_ParseTuple(args,"OOs|i",&userdata, &callback, &_type, &sensor))) {
	DeviceException::launch(defaultCall);
      }

      std::string thirdError("Third attribut must be 'position', 'velocity', 'acceleration', 'workspace', 'unit2sensor' or 'tracker2room' !");

      std::string type(_type);

      Callback cb(userdata, callback);

      if (type == "position") {
	handlers::register_handler<Tracker, vrpn_TRACKERCB>(self, add, cb, sensor, thirdError);
	Py_RETURN_TRUE;
      }

      if (type == "velocity") {
	handlers::register_handler<Tracker, vrpn_TRACKERVELCB>(self, add, cb, sensor, thirdError);
	Py_RETURN_TRUE;
      }

      if (type == "acceleration") {
	handlers::register_handler<Tracker, vrpn_TRACKERACCCB>(self, add, cb, sensor, thirdError);
	Py_RETURN_TRUE;
      }

      if (type == "unit2sensor") {
	handlers::register_handler<Tracker, vrpn_TRACKERUNIT2SENSORCB>(self, add, cb, sensor, thirdError);
	Py_RETURN_TRUE;
      }

      if (type == "workspace") {
	handlers::register_handler<Tracker, vrpn_TRACKERWORKSPACECB>(self, add, cb, thirdError);
	Py_RETURN_TRUE;
      }

      if (type == "tracker2room") {
	handlers::register_handler<Tracker, vrpn_TRACKERTRACKER2ROOMCB>(self, add, cb, thirdError);
	Py_RETURN_TRUE;
      }

      DeviceException::launch(thirdError);

    } catch (DeviceException &exception) {
      PyErr_SetString(Device::s_error, exception.getReason().c_str());
    }
    return NULL;
  }

#define WRAPPER_REQUEST(NAME, ERROR_MSG)				\
  PyObject *Tracker::NAME(PyObject *obj) {				\
    try {								\
      Tracker *self = _definition::get(obj);				\
      if (self->d_device->NAME() < 1)					\
	Py_RETURN_TRUE;							\
      DeviceException::launch(ERROR_MSG);				\
    } catch (DeviceException &exception) {				\
      PyErr_SetString(Device::s_error, exception.getReason().c_str());	\
    }									\
    return NULL;							\
  }

  WRAPPER_REQUEST(request_t2r_xform, "Tracker : cannot request t2r xform")
  WRAPPER_REQUEST(request_u2s_xform, "Tracker : cannot request u2s xform")
  WRAPPER_REQUEST(request_workspace, "Tracker : cannot request workspace")
  WRAPPER_REQUEST(reset_origin, "Tracker : cannot reset the origin")
#undef WRAPPER_REQUEST

  namespace handlers {
    template<> PyObject *CALLBACK_CALL createPyObjectFromVRPN_Type(const vrpn_TRACKERCB &info) {
      return Py_BuildValue("{sOsis(fff)s(ffff)}",
			   "time", Device::getDateTimeFromTimeval(info.msg_time),
			   "sensor", info.sensor,
			   "position", info.pos[0],info.pos[1],info.pos[2],
			   "quaternion", info.quat[0],info.quat[1],info.quat[2],info.quat[3]);
    }

    template<> PyObject *CALLBACK_CALL createPyObjectFromVRPN_Type(const vrpn_TRACKERVELCB &info) {
      return Py_BuildValue("{sOsis(fff)s(ffff)si}",
			   "time", Device::getDateTimeFromTimeval(info.msg_time),
			   "sensor", info.sensor,
			   "velocity", info.vel[0],info.vel[1],info.vel[2],
			   "future quaternion", info.vel_quat[0],info.vel_quat[1],info.vel_quat[2],info.vel_quat[3],
			   "future delta", info.vel_quat_dt);
    }

    template<> PyObject *CALLBACK_CALL createPyObjectFromVRPN_Type(const vrpn_TRACKERACCCB &info) {
      return Py_BuildValue("{sOsis(fff)s(ffff)si}",
			   "time", Device::getDateTimeFromTimeval(info.msg_time),
			   "sensor", info.sensor,
			   "acceleration", info.acc[0],info.acc[1],info.acc[2],
			   "future acceleration ?", info.acc_quat[0],info.acc_quat[1],info.acc_quat[2],info.acc_quat[3],
			   "future delta", info.acc_quat_dt);
    }

    template<> PyObject *CALLBACK_CALL createPyObjectFromVRPN_Type(const vrpn_TRACKERTRACKER2ROOMCB &info) {
      return Py_BuildValue("{sOs(fff)s(ffff)}",
			   "time", Device::getDateTimeFromTimeval(info.msg_time),
			   "position offset", info.tracker2room[0],info.tracker2room[1],info.tracker2room[2],
			   "quaternion offset", info.tracker2room_quat[0],info.tracker2room_quat[1],info.tracker2room_quat[2],info.tracker2room_quat[3]);
    }

    template<> PyObject *CALLBACK_CALL createPyObjectFromVRPN_Type(const vrpn_TRACKERUNIT2SENSORCB &info) {
      return Py_BuildValue("{sOsis(fff)s(ffff)}",
			   "time", Device::getDateTimeFromTimeval(info.msg_time),
			   "sensor", info.sensor,
			   "position offset", info.unit2sensor[0],info.unit2sensor[1],info.unit2sensor[2],
			   "quaternion offset", info.unit2sensor_quat[0],info.unit2sensor_quat[1],info.unit2sensor_quat[2],info.unit2sensor_quat[3]);
    }

    template<> PyObject *CALLBACK_CALL createPyObjectFromVRPN_Type(const vrpn_TRACKERWORKSPACECB &info) {
      return Py_BuildValue("{sOs(fff)s(fff)}",
			   "time", Device::getDateTimeFromTimeval(info.msg_time),
			   "minimum corner box", info.workspace_min[0],info.workspace_min[1],info.workspace_min[2],
			   "maximum corner box", info.workspace_max[0],info.workspace_max[1],info.workspace_max[2]);
    }

  }
}
