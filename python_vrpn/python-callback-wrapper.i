// file:	python-callback-wrapper.i
// author:	Thiebaut Mochel mochel@cecpv.u-strasbg.fr 2008-06-05
// copyright:	(C)2008  CECPV
// license:	Released to the Public Domain.
// depends:	python 2.4, swig 1.3.35, VRPN 07_15
// tested on:	Linux w/ gcc 4.1.2
// references:  http://python.org/ http://vrpn.org/
//              http://www.swig.org
//

%{
#ifndef CONVERTVOID
static PyObject* convert_void(void* userdata) {
       return Py_BuildValue("s",userdata);
}
#define CONVERTVOID
#endif
%}

%define PYTHON_CALLBACK_WRAPPER(clname,clcallback,clhandler) 
%{

static PyObject* PyCallBack_ ## clname ## _change_handler=0;

void _cbwrap_ ## clname ## _change_handler(void* userdata,const clcallback t){
    PyObject *arglist,*result;
    PyObject *arg0, *arg1;
    
    if(!PyCallBack_ ## clname ## _change_handler) return;
    
    arglist=Py_BuildValue("(O&O&)",convert_void,userdata,convert_ ## clname ## _cb,&t); 
    
    result=PyEval_CallObject(PyCallBack_ ## clname ## _change_handler,arglist);
    Py_DECREF(arglist);
    { PyObject *args=Py_BuildValue("(O)",result);
      Py_XDECREF(result);
      
      Py_XDECREF(args);
    }
    return ;
}
%}

#ifdef SWIG<python>
%typemap(in) PyObject *{ $1=$input; }
#endif

%inline %{
void register_ ## clname ## _change_handler(PyObject *pyfunc){
    if (!PyCallable_Check(pyfunc)) {
      PyErr_SetString(PyExc_TypeError, "Need a callable object!");
      return;
    }
    if(PyCallBack_ ## clname ## _change_handler)Py_DECREF(PyCallBack_ ## clname ## _change_handler);
    PyCallBack_ ## clname ## _change_handler=pyfunc;
    Py_INCREF(pyfunc);
}
/* For SWIG Type Conversion */
/* void __null__ ## clname ## _change_handler( void* userdata,const clcallback t){} */
%}
%inline %{
 void (* clname ## _change_handler)(void* userdata,const clcallback t);
%}

%init %{
    clname ## _change_handler=&_cbwrap_ ## clname ## _change_handler;
%}
%inline %{
clhandler get_ ## clname ## _change_handler() {
      return reinterpret_cast< ## clhandler ## >(clname ## _change_handler);
}
%}

%enddef
