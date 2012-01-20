// file:	vrpn_Analog_Output.i
// author:	Thiebaut Mochel mochel@cecpv.u-strasbg.fr 2008-06-05
// copyright:	(C)2008  CECPV
// license:	Released to the Public Domain.
// depends:	python 2.4, swig 1.3.35, VRPN 07_15
// tested on:	Linux w/ gcc 4.1.2
// references:  http://python.org/ http://vrpn.org/
//              http://www.swig.org
//

%module vrpn_Analog_Output
%{
#include "../vrpn_Types.h"
#include "../vrpn_BaseClass.h"
#include "../vrpn_Analog_Output.h"
%}

%include "../vrpn_Configure.h"
%include "../vrpn_Connection.h"
%include "../vrpn_BaseClass.h"
#include "../vrpn_Analog.h"    // just for some #define's and constants
%include "../vrpn_Analog_Output.h"

%{
static PyObject* convert_analogoutput_cb(vrpn_ANALOGOUTPUTCB* t) {
       return Py_BuildValue("(i|ddddd)",t->num_channel,t->channel[0],t->channel[1],t->channel[2],t->channel[3],t->channel[4]);
}
%}

%include python-callback-wrapper.i

PYTHON_CALLBACK_WRAPPER(analogoutput,vrpn_ANALOGOUTPUTCB,vrpn_ANALOGOUTPUTCHANGEHANDLER)
