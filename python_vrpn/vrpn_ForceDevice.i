// file:	vrpn_ForceDevice.i
// author:	Thiebaut Mochel mochel@cecpv.u-strasbg.fr 2008-06-05
// copyright:	(C)2008  CECPV
// license:	Released to the Public Domain.
// depends:	python 2.4, swig 1.3.35, VRPN 07_15
// tested on:	Linux w/ gcc 4.1.2
// references:  http://python.org/ http://vrpn.org/
//              http://www.swig.org
//

%module vrpn_ForceDevice

%typemap(in) vrpn_float64 {
   $1 = PyFloat_AsDouble($input);
}
%typemap(in) vrpn_uint32 {
   $1 = PyInt_AsLong($input);
}

%{
#include "../vrpn_Types.h"
#include "../vrpn_BaseClass.h"
#include "../vrpn_ForceDevice.h"
#include "../vrpn_ForceDeviceServer.h"
%}

%include "../vrpn_Configure.h"
%include "../vrpn_Connection.h"
%include "../vrpn_BaseClass.h"
%include "../vrpn_ForceDevice.h"

%{
static PyObject* convert_force_cb(vrpn_FORCECB* t) {
       return Py_BuildValue("(ddd)",t->force[0],t->force[1],t->force[2]);
}

static PyObject* convert_forcescp_cb(vrpn_FORCESCPCB* t) {
       return Py_BuildValue("(ddddddd)",t->pos[0],t->pos[1],t->pos[2],t->quat[0],t->quat[1],t->quat[2],t->quat[3]);
}

static PyObject* convert_forceerror_cb(vrpn_FORCEERRORCB* t) {
       return Py_BuildValue("(i)",t->error_code);
}
%}


%include python-callback-wrapper.i

PYTHON_CALLBACK_WRAPPER(force,vrpn_FORCECB,vrpn_FORCECHANGEHANDLER)
PYTHON_CALLBACK_WRAPPER(forceerror,vrpn_FORCEERRORCB,vrpn_FORCEERRORHANDLER)
PYTHON_CALLBACK_WRAPPER(forcescp,vrpn_FORCESCPCB,vrpn_FORCESCPHANDLER)
