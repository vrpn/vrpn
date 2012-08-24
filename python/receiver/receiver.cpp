#include <include/interface.hpp>
#include "include/Tracker.hpp"
#include "include/Analog.hpp"
#include "include/Button.hpp"
#include "include/Dial.hpp"
#include "include/Text_Receiver.hpp"
#include <include/definition.hpp>

namespace vrpn_python {
  namespace receiver {

#if PY_MAJOR_VERSION >= 3
    static PyModuleDef module_definition = {
      PyModuleDef_HEAD_INIT,
      "receiver",
      "VRPN receiver classes.",
      -1,
      NULL
    };
#endif

  bool init_types() {
    if (!Tracker::_definition::init_type()) return false;
    if (!Analog::_definition::init_type()) return false;
    if (!Button::_definition::init_type()) return false;
    if (!Dial::_definition::init_type()) return false;
    if (!Text_Receiver::_definition::init_type()) return false;

    return true;
  }

  void add_types(PyObject* vrpn_module) {
    PyObject* receiver_module = 
#if PY_MAJOR_VERSION >= 3
      PyModule_Create(&module_definition);
#else
      Py_InitModule("receiver", NULL);
#endif

    PyModule_AddObject(vrpn_module, "receiver", receiver_module);

    Tracker::_definition::add_type(receiver_module);
    Analog::_definition::add_type(receiver_module);
    Button::_definition::add_type(receiver_module);
    Dial::_definition::add_type(receiver_module);
    Text_Receiver::_definition::add_type(receiver_module);
  }
}
}
