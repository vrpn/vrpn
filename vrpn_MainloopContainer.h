/** @file
    @brief Header

    @date 2011

    @author
    Ryan Pavlik
    <rpavlik@iastate.edu> and <abiryan@ryand.net>
    http://academic.cleardefinition.com/
    Iowa State University Virtual Reality Applications Center
    Human-Computer Interaction Graduate Program
*/

//          Copyright Iowa State University 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// *** WARNING: This file must only be included in a .cpp or .C file, not
// in a .h file, because it includes <vector>, and we can't have any standard
// library files in the VRPN headers because that will make VRPN incompatible
// with libraries that use the other flavor of <vector.h>.

#pragma once

// Internal Includes
#include "vrpn_MainloopObject.h"

// Library/third-party includes
// - none

// Standard includes
#include <vector>

/// A container that holds and owns one or more VRPN objects,
class vrpn_MainloopContainer {
public:
    /// Constructor
    vrpn_MainloopContainer() {}
    /// Destructor: invokes clear()
    ~vrpn_MainloopContainer();

    /// Clear internal structure holding objects, deleting them
    /// in reverse order of their addition.
    void clear();

    /// Add an object wrapped by vrpn_MainloopObject.
    /// Return NULL if the object has a problem (indicated by
    /// broken()).
    vrpn_MainloopObject *add(vrpn_MainloopObject *o);

    /// Template method to automatically wrap objects
    /// with vrpn_MainloopObject before adding them.
    /// Return NULL if there is a problem with the object add.
    template <class T> T add(T o)
    {
        if (!add(vrpn_MainloopObject::wrap(o))) {
            return NULL;
        }
        return o;
    }

    /// Runs mainloop on all contained objects, in the order
    /// that they were added.
    void mainloop();

private:
    std::vector<vrpn_MainloopObject *> _vrpn;
};

/* -- inline implementations -- */

inline vrpn_MainloopContainer::~vrpn_MainloopContainer() { clear(); }

inline vrpn_MainloopObject *vrpn_MainloopContainer::add(vrpn_MainloopObject *o)
{
    // If the object is NULL, we have a problem and don't add it.
    if (!o) {
        return o;
    }

    // If the object is broken(), this indicates a problem
    // with the device.  We also do not add it and we return NULL to show.
    // that there is a problem.
    if (o->broken()) {
        fprintf(
            stderr,
            "vrpn_MainloopContainer::add() Device is broken, not adding.\n");
        return NULL;
    }

    _vrpn.push_back(o);
    return o;
}

inline void vrpn_MainloopContainer::clear()
{
    if (_vrpn.empty()) {
        return;
    }
    /// Delete in reverse order
    for (int i = int(_vrpn.size()) - 1; i >= 0; --i) {
        try {
          delete _vrpn[i];
        } catch (...) {
          fprintf(stderr, "vrpn_MainloopContainer::clear: delete failed\n");
          return;
        }
        _vrpn[i] = NULL;
    }
    _vrpn.clear();
}

inline void vrpn_MainloopContainer::mainloop()
{
    const size_t n = _vrpn.size();
    for (size_t i = 0; i < n; ++i) {
        _vrpn[i]->mainloop();
    }
}
