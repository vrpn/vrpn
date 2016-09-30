// file:	vrpn_Poser.i
// author:	Thiebaut Mochel mochel@cecpv.u-strasbg.fr 2008-06-05
// copyright:	(C)2008  CECPV
// license:	Released to the Public Domain.
// depends:	python 2.4, swig 1.3.35, VRPN 07_15
// tested on:	Linux w/ gcc 4.1.2
// references:  http://python.org/ http://vrpn.org/
//              http://www.swig.org
//

%module vrpn_Poser

%typemap(in) vrpn_float64 {
   $1 = PyFloat_AsDouble($input);
}
%typemap(in) vrpn_uint32 {
   $1 = PyInt_AsLong($input);
}

%{
#include "../vrpn_Types.h"
#include "../vrpn_BaseClass.h"
#include "../vrpn_Poser.h"
#include "../vrpn_Poser_Analog.h"
#include "../vrpn_Poser_Tek4662.h"
%}

%include "../vrpn_Configure.h"
%include "../vrpn_BaseClass.h"
%include "../vrpn_Poser.h"

%{
static PyObject* convert_poser_cb(vrpn_POSERCB* t) {
       return Py_BuildValue("(ddddddd)",t->pos[0],t->pos[1],t->pos[2],t->quat[0],t->quat[1],t->quat[2],t->quat[3]);
}
%}

%include python-callback-wrapper.i

PYTHON_CALLBACK_WRAPPER(poser,vrpn_POSERCB,vrpn_POSERHANDLER)
