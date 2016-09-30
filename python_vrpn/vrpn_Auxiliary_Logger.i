// file:	vrpn_Auxiliary_Logger.i
// author:	Thiebaut Mochel mochel@cecpv.u-strasbg.fr 2008-06-05
// copyright:	(C)2008  CECPV
// license:	Released to the Public Domain.
// depends:	python 2.4, swig 1.3.35, VRPN 07_15
// tested on:	Linux w/ gcc 4.1.2
// references:  http://python.org/ http://vrpn.org/
//              http://www.swig.org
//

%module vrpn_Auxiliary_Logger

%typemap(in) vrpn_float64 {
   $1 = PyFloat_AsDouble($input);
}
%typemap(in) vrpn_uint32 {
   $1 = PyInt_AsLong($input);
}

%{
#include "../vrpn_Types.h"
#include "../vrpn_BaseClass.h"
#include "../vrpn_Auxiliary_Logger.h"
%}

%include "../vrpn_Configure.h"
#include "../vrpn_Connection.h"
%include "../vrpn_BaseClass.h"
%include "../vrpn_Auxiliary_Logger.h"

%{

static PyObject* convert_auxlogger_cb(vrpn_AUXLOGGERCB* t) {
       return Py_BuildValue("(ssss)",t->local_in_logfile_name,t->local_out_logfile_name,t->remote_in_logfile_name,t->remote_out_logfile_name);
}

%}

%include python-callback-wrapper.i

PYTHON_CALLBACK_WRAPPER(auxlogger,vrpn_AUXLOGGERCB,vrpn_AUXLOGGERREPORTHANDLER)
