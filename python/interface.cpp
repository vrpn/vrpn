#include "include/interface.hpp"
#include "include/definition.hpp"
#include "include/Device.hpp"

#ifdef DEFINE_REFCOUNT
static PyObject *refCount(PyObject *self, PyObject *args) {
  PyObject *real_object;

  if (!PyArg_ParseTuple(args, "O", &real_object))
    return NULL;

  return PyLong_FromLong(real_object->ob_refcnt);
}

static PyMethodDef vrpnMethods[] = {
  {"refCount",  refCount, METH_VARARGS, "Execute a shell command."},
  {NULL, NULL, 0, NULL}
};
#endif

#if PY_MAJOR_VERSION >= 3
static PyModuleDef module_definition = {
  PyModuleDef_HEAD_INIT,
  "vrpn",
  "VRPN wrapper classes.",
  -1,
#   ifdef DEFINE_REFCOUNT
  vrpnMethods
#   else
  NULL
#   endif
};

#define INIT_FUNCTION_RETURN_VALUE(VALUE) return VALUE
PyMODINIT_FUNC PyInit_vrpn(void)  {

#else // if PY_MAJOR_VERSION < 3
#define INIT_FUNCTION_RETURN_VALUE(VALUE) return
PyMODINIT_FUNC initvrpn(void)  {

#endif

  if (!vrpn_python::receiver::init_types()) INIT_FUNCTION_RETURN_VALUE (NULL);
  if (!vrpn_python::sender::init_types()) INIT_FUNCTION_RETURN_VALUE (NULL);
  if (!vrpn_python::quaternion::init_types()) INIT_FUNCTION_RETURN_VALUE (NULL);

  PyObject* vrpn_module = 
#if (PY_MAJOR_VERSION >= 3) && (PY_MINOR_VERSION >= 1)
    PyModule_Create(&module_definition);
#else
#   ifdef DEFINE_REFCOUNT
    Py_InitModule("vrpn", vrpnMethods);
#   else
    Py_InitModule("vrpn", NULL);
#   endif
#endif

  if (vrpn_module == NULL) INIT_FUNCTION_RETURN_VALUE (NULL);

  if (!vrpn_python::Device::init_device_common_objects(vrpn_module)) {
    INIT_FUNCTION_RETURN_VALUE (NULL);
  }

  vrpn_python::receiver::add_types(vrpn_module);
  vrpn_python::sender::add_types(vrpn_module);
  vrpn_python::quaternion::add_types(vrpn_module);

  INIT_FUNCTION_RETURN_VALUE (vrpn_module);
}
