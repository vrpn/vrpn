// file:	vrpn_Dial.i
// author:	Thiebaut Mochel mochel@cecpv.u-strasbg.fr 2008-06-05
// copyright:	(C)2008  CECPV
// license:	Released to the Public Domain.
// depends:	python 2.4, swig 1.3.35, VRPN 07_15
// tested on:	Linux w/ gcc 4.1.2
// references:  http://python.org/ http://vrpn.org/
//              http://www.swig.org
//

%module vrpn_Dial

%typemap(in) vrpn_float64 {
   $1 = PyFloat_AsDouble($input);
}
%typemap(in) vrpn_uint32 {
   $1 = PyInt_AsLong($input);
}

%{
#include "../vrpn_Types.h"
#include "../vrpn_Dial.h"
%}


%include "../vrpn_Configure.h"
%include "../vrpn_Connection.h"
%include "../vrpn_BaseClass.h"
%include "../vrpn_Dial.h"

%{
static PyObject* convert_dial_cb(vrpn_DIALCB* t) {
       return Py_BuildValue("(id)",t->dial,t->change);
}
%}

%include python-callback-wrapper.i

PYTHON_CALLBACK_WRAPPER(dial,vrpn_DIALCB,vrpn_DIALCHANGEHANDLER)
