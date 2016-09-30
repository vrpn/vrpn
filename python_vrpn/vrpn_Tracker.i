// file:	vrpn_Tracker.i
// author:	Thiebaut Mochel mochel@cecpv.u-strasbg.fr 2008-06-05
// copyright:	(C)2008  CECPV
// license:	Released to the Public Domain.
// depends:	python 2.4, swig 1.3.35, VRPN 07_15
// tested on:	Linux w/ gcc 4.1.2
// references:  http://python.org/ http://vrpn.org/
//              http://www.swig.org
//

%module vrpn_Tracker

%typemap(in) vrpn_float64 {
   $1 = PyFloat_AsDouble($input);
}
%typemap(in) vrpn_uint32 {
   $1 = PyInt_AsLong($input);
}

%{
#include "../vrpn_Types.h"
#include "../vrpn_BaseClass.h"
#include "../vrpn_Connection.h"
#include "../vrpn_Tracker.h"
#include "../vrpn_Tracker_3DMouse.h"
#include "../vrpn_Tracker_AnalogFly.h"
#include "../vrpn_Tracker_ButtonFly.h"
#include "../vrpn_Tracker_Crossbow.h"
#include "../vrpn_Tracker_DTrack.h"
#include "../vrpn_Tracker_Fastrak.h"
#include "../vrpn_Tracker_Liberty.h"
#include "../vrpn_Tracker_MotionNode.h"
#include "../vrpn_Tracker_isense.h"
%}

%ignore vrpn_Tracker_Remote::tracker_state;
%include "../vrpn_Configure.h"
%include "../vrpn_BaseClass.h"
%include "../vrpn_Connection.h"
%include "../vrpn_Tracker.h"

%{

static PyObject* convert_tracker_cb(vrpn_TRACKERCB* t) {
       return Py_BuildValue("(iddddddd)",t->sensor,t->pos[0],t->pos[1],t->pos[2],t->quat[0],t->quat[1],t->quat[2],t->quat[3]);
}

static PyObject* convert_trackervel_cb(vrpn_TRACKERVELCB* t) {
       return Py_BuildValue("(idddddddd)",t->sensor,t->vel[0],t->vel[1],t->vel[2],t->vel_quat[0],t->vel_quat[1],t->vel_quat[2],t->vel_quat[3],t->vel_quat_dt);
}

static PyObject* convert_trackeracc_cb(vrpn_TRACKERACCCB* t) {
       return Py_BuildValue("(idddddddd)",t->sensor,t->acc[0],t->acc[1],t->acc[2],t->acc_quat[0],t->acc_quat[1],t->acc_quat[2],t->acc_quat[3],t->acc_quat_dt);
}

static PyObject* convert_tracker2room_cb(vrpn_TRACKERTRACKER2ROOMCB* t) {
       return Py_BuildValue("(ddddddd)",t->tracker2room[0],t->tracker2room[1],t->tracker2room[2],t->tracker2room_quat[0],t->tracker2room_quat[1],t->tracker2room_quat[2],t->tracker2room_quat[3]);
}

static PyObject* convert_trackerunit2sensor_cb(vrpn_TRACKERUNIT2SENSORCB* t) {
       return Py_BuildValue("(iddddddd)",t->sensor,t->unit2sensor[0],t->unit2sensor[1],t->unit2sensor[2],t->unit2sensor_quat[0],t->unit2sensor_quat[1],t->unit2sensor_quat[2],t->unit2sensor_quat[3]);
}

static PyObject* convert_trackerworkspace_cb(vrpn_TRACKERWORKSPACECB* t) {
       return Py_BuildValue("(dddddd)",t->workspace_min[0],t->workspace_min[1],t->workspace_min[2],t->workspace_max[0],t->workspace_max[1],t->workspace_max[2]);
}
%}

%include python-callback-wrapper.i

PYTHON_CALLBACK_WRAPPER(tracker,vrpn_TRACKERCB,vrpn_TRACKERCHANGEHANDLER)
PYTHON_CALLBACK_WRAPPER(trackervel,vrpn_TRACKERVELCB,vrpn_TRACKERVELCHANGEHANDLER)
PYTHON_CALLBACK_WRAPPER(trackeracc,vrpn_TRACKERACCCB,vrpn_TRACKERACCCHANGEHANDLER)
PYTHON_CALLBACK_WRAPPER(tracker2room,vrpn_TRACKERTRACKER2ROOMCB,vrpn_TRACKERTRACKER2ROOMCHANGEHANDLER)
PYTHON_CALLBACK_WRAPPER(trackerunit2sensor,vrpn_TRACKERUNIT2SENSORCB,vrpn_TRACKERUNIT2SENSORCHANGEHANDLER)
PYTHON_CALLBACK_WRAPPER(trackerworkspace,vrpn_TRACKERWORKSPACECB,vrpn_TRACKERWORKSPACECHANGEHANDLER)

