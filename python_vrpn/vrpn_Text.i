// file:	vrpn_Text.i
// author:	Thiebaut Mochel mochel@cecpv.u-strasbg.fr 2008-06-05
// copyright:	(C)2008  CECPV
// license:	Released to the Public Domain.
// depends:	python 2.4, swig 1.3.35, VRPN 07_15
// tested on:	Linux w/ gcc 4.1.2
// references:  http://python.org/ http://vrpn.org/
//              http://www.swig.org
//

%module vrpn_Text

%typemap(in) vrpn_float64 {
   $1 = PyFloat_AsDouble($input);
}
%typemap(in) vrpn_uint32 {
   $1 = PyInt_AsLong($input);
}

%{
#include "../vrpn_Types.h"
#include "../vrpn_BaseClass.h"
#include "../vrpn_Text.h"
%}

%include "../vrpn_Configure.h"
%include "../vrpn_Connection.h"
%include "../vrpn_BaseClass.h"
%include "../vrpn_Text.h"

%{
static PyObject* convert_text_cb(vrpn_TEXTCB* t) {
       return Py_BuildValue("(s)",t->message);
}
%}

%include python-callback-wrapper.i

PYTHON_CALLBACK_WRAPPER(text,vrpn_TEXTCB,vrpn_TEXTHANDLER)
