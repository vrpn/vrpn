#ifndef __VRPN_PYTHON_CALLBACK_HPP__
#define __VRPN_PYTHON_CALLBACK_HPP__

#include <Python.h>
#include <iostream>

namespace vrpn_python {

  class callbackEntry;

  class Callback {
    callbackEntry *d_entry;
    PyObject *d_userdata;
    PyObject *d_callback;
  public:
    Callback(PyObject *userdata, PyObject *callback);
    Callback(void *data);
    ~Callback();

    void *getData() { return (void*)d_entry ; }

    void increment();
    void decrement();

    static void get(void *, PyObject *&userdata, PyObject *&callback);
  };
}

#endif // defined(__VRPN_PYTHON_CALLBACK_H__)
