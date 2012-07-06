#include <include/interface.hpp>
#include <quat/quat.h>
#include <iostream>

namespace vrpn_python {
  namespace quaternion {

    static PyObject *to_col_matrix(PyObject *self, PyObject *args) {
      q_type quaternion; 

      if (!PyArg_ParseTuple(args, "(dddd)", &quaternion[0], &quaternion[1], &quaternion[2], &quaternion[3]))
	return NULL;

      q_matrix_type matrix;

      q_to_col_matrix(matrix, quaternion);

      return Py_BuildValue("((ddd)(ddd)(ddd))",
			   matrix[0][0], matrix[0][1], matrix[0][2],
			   matrix[1][0], matrix[1][1], matrix[1][2],
			   matrix[2][0], matrix[2][1], matrix[2][2]);
    }

    static PyObject *to_row_matrix(PyObject *self, PyObject *args) {
      q_type quaternion; 

      if (!PyArg_ParseTuple(args, "(dddd)", &quaternion[0], &quaternion[1], &quaternion[2], &quaternion[3]))
	return NULL;

      q_matrix_type matrix;

      q_to_row_matrix(matrix, quaternion);

      return Py_BuildValue("((ddd)(ddd)(ddd))",
			   matrix[0][0], matrix[0][1], matrix[0][2],
			   matrix[1][0], matrix[1][1], matrix[1][2],
			   matrix[2][0], matrix[2][1], matrix[2][2]);
    }

    static PyMethodDef vrpnMethods[] = {
      {"to_col_matrix",  to_col_matrix, METH_VARARGS, "Convert quaternion to 4x4 column-major rotation matrix.\nQuaternion need not be unit magnitude."},
      {"to_row_matrix",  to_row_matrix, METH_VARARGS, "Convert quaternion to 4x4 row-major rotation matrix.\nQuaternion need not be unit magnitude."},
      {NULL, NULL, 0, NULL}
    };

#if PY_MAJOR_VERSION >= 3
    static PyModuleDef module_definition = {
      PyModuleDef_HEAD_INIT,
      "quaternion",
      "VRPN quaternion methods.",
      -1,
      vrpnMethods,
      NULL
    };
#endif

    bool init_types() {

      return true;
    }

    void add_types(PyObject* vrpn_module) {
      PyObject* quaternion_module = 
#if PY_MAJOR_VERSION >= 3
	PyModule_Create(&module_definition);
#else
      Py_InitModule("quaternion", vrpnMethods);
#endif

      PyModule_AddObject(vrpn_module, "quaternion", quaternion_module);
    }
  }
}
