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

%typemap(in) vrpn_float64 {
   $1 = PyFloat_AsDouble($input);
}
%typemap(in) vrpn_uint32 {
   $1 = PyInt_AsLong($input);
}

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

// We seem to have a problem with the automatically generated code for the
// request_change_channel_value() function and have chased it down to having
// an issue with the default-value last parameter.  Even though Swig generates
// the code to handle both cases, when the switching function that is generated
// is called it is always called with 4 arguments even when there are in fact 3, so
// it fails to find the right function.
//  To work around this, we here construct a python-callable method that does
// not have a variable number of arguments and have it call the original function.
%extend vrpn_Analog_Output_Remote {
    virtual bool request_change_channel_value_python(
        unsigned int chan, vrpn_float64 val)
    {
          return $self->request_change_channel_value( chan, val, vrpn_CONNECTION_RELIABLE);
    }
};

%include python-callback-wrapper.i

PYTHON_CALLBACK_WRAPPER(analogoutput,vrpn_ANALOGOUTPUTCB,vrpn_ANALOGOUTPUTCHANGEHANDLER)
