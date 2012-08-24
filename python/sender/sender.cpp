#include <include/interface.hpp>
#include "include/Poser.hpp"
#include "include/Text_Sender.hpp"
#include <include/definition.hpp>

namespace vrpn_python {
  namespace sender {
#if PY_MAJOR_VERSION >= 3
    static PyModuleDef module_definition = {
      PyModuleDef_HEAD_INIT,
      "sender",
      "VRPN sender classes.",
      -1,
      NULL
    };
#endif

    bool init_types() {
      if (!vrpn_python::Poser::_definition::init_type()) return false;
      if (!vrpn_python::Text_Sender::_definition::init_type()) return false;

      return true;
    }

    void add_types(PyObject* vrpn_module) {
      PyObject* sender_module = 
#if PY_MAJOR_VERSION >= 3
	PyModule_Create(&module_definition);
#else
        Py_InitModule("sender", NULL);
#endif

      PyModule_AddObject(vrpn_module, "sender", sender_module);

      vrpn_python::Poser::_definition::add_type(sender_module);
      vrpn_python::Text_Sender::_definition::add_type(sender_module);
    }
  }
}
