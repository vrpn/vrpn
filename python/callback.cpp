#include "include/callback.hpp"
#include "include/Device.hpp"
#include <map>
#include <vrpn_Tracker.h>

namespace vrpn_python {

  class callbackEntry {
    PyObject *d_userdata;
    PyObject *d_callback;
    int d_counterReference;

  public:
    callbackEntry(PyObject *userdata, PyObject *callback);
    callbackEntry(const callbackEntry &other);
    ~callbackEntry();

    bool operator == (const callbackEntry &other) const;
    bool operator < (const callbackEntry &other) const;
    bool operator > (const callbackEntry &other) const;

    PyObject *getUserData() { return d_userdata ; }
    PyObject *getCallback() { return d_callback ; }

    void incrementReference() { d_counterReference ++ ; }
    void decrementReference() { d_counterReference -- ; }
    int getCounterReference() const { return d_counterReference ; }
  };

  static std::map<callbackEntry, callbackEntry *> s_callbacks;

  callbackEntry::callbackEntry(PyObject *data, PyObject *callback) :
    d_userdata(data), d_callback(callback), d_counterReference(0) {
  }

  callbackEntry::callbackEntry(const callbackEntry &other) :
    d_userdata(other.d_userdata), d_callback(other.d_callback), d_counterReference(0) {
  }

  callbackEntry::~callbackEntry() {
  }

  bool callbackEntry::operator == (const callbackEntry &other) const {
    return ((other.d_userdata == d_userdata) && (other.d_callback == d_callback));
  }

  bool callbackEntry::operator < (const callbackEntry &other) const {
    if (d_userdata < other.d_userdata) return true;
    if (d_userdata > other.d_userdata) return false;
    return (d_callback < other.d_callback);
  }

  bool callbackEntry::operator > (const callbackEntry &other) const {
    if (d_userdata > other.d_userdata) return true;
    if (d_userdata < other.d_userdata) return false;
    return (d_callback > other.d_callback);
  }

  Callback::Callback(PyObject *userdata, PyObject *callback) : d_userdata(userdata), d_callback(callback) {
    Py_INCREF(d_userdata);
    Py_INCREF(d_callback);

    callbackEntry entry(d_userdata, d_callback);

    std::map<callbackEntry, callbackEntry *>::iterator position = s_callbacks.find(entry);

    if (position != s_callbacks.end()) {
      d_entry = position->second;
    } else {
      d_entry = new callbackEntry(entry);
    }
  }

  Callback::Callback(void *data) {
    d_entry = static_cast<callbackEntry *>(data);
    d_userdata = d_entry->getUserData();
    d_callback = d_entry->getCallback();

    Py_INCREF(d_userdata);
    Py_INCREF(d_callback);
  }

  Callback::~Callback() {
    std::map<callbackEntry, callbackEntry *>::iterator position = s_callbacks.find(*d_entry);
    if (d_entry->getCounterReference() < 1) {
      if (position != s_callbacks.end()) {
	s_callbacks.erase(position);
	Py_DECREF(d_userdata);
	Py_DECREF(d_callback);
      }
      delete d_entry;
    } else {
      if (position == s_callbacks.end()) {
	Py_INCREF(d_userdata);
	Py_INCREF(d_callback);
	s_callbacks[*d_entry] = d_entry;
      }
    }
    Py_DECREF(d_userdata);
    Py_DECREF(d_callback);
  }

  void Callback::increment() {
    d_entry->incrementReference();
  }

  void Callback::decrement() {
    d_entry->decrementReference();
  }

  void Callback::get(void *pointer, PyObject *&userdata, PyObject *&callback) {
    callbackEntry* entry = static_cast<callbackEntry*>(pointer);
    userdata = entry->getUserData();
    callback = entry->getCallback();
  }
}
