/** @file
    @brief Implementation

    @date 2015

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2015 Sensics, Inc.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Internal Includes
#include "vrpn_EndpointContainer.h"
#include "vrpn_Connection.h"

// Library/third-party includes
// - none

// Standard includes
#include <algorithm>

#undef VRPN_EC_VERBOSE

#ifdef VRPN_EC_VERBOSE
#include <iostream>
#define VRPN_EC_TRACE(X)                                                       \
    do {                                                                       \
        std::cerr << "  [EC " << this << (needsCompact_ ? "*" : " ") << "] "   \
                  << X << std::endl;                                           \
    } while (0)
#else
#define VRPN_EC_TRACE(X) ((void)0)
#endif

namespace vrpn {
    namespace {
        template <typename T> struct EndpointCloser {
        public:
            void operator()(T *obj)
            {
                if (obj) {
                    obj->drop_connection();
                    try {
                      delete obj;
                    } catch (...) {
                      fprintf(stderr, "EndpointCloser: delete failed\n");
                      return;
                    }
                }
            }
        };
    } // namespace

    /// @brief Helper function required to cast NULL to the right pointer type,
    /// for the sake of STL algorithms, since we aren't allowed to use nullptr
    /// yet.
    static inline EndpointContainer::pointer getNullEndpoint()
    {
        return static_cast<EndpointContainer::pointer>(NULL);
    }

    EndpointContainer::EndpointContainer()
        : needsCompact_(false)
    {
        VRPN_EC_TRACE("Constructor.");
    }

    EndpointContainer::~EndpointContainer()
    {
        VRPN_EC_TRACE("Destructor - about to call clear.");
        clear();
    }

    void EndpointContainer::clear()
    {
        if (container_.empty()) {
            // Early out if there's nothing to clear.
            return;
        }
        VRPN_EC_TRACE("Clear.");
        ::std::for_each(begin_(), end_(), EndpointCloser<T>());
        container_.clear();
    }

    void EndpointContainer::compact_()
    {
        size_type before = get_full_container_size();
        raw_iterator it = std::remove(begin_(), end_(), getNullEndpoint());
        container_.resize(it - begin_());
        needsCompact_ = false;
        VRPN_EC_TRACE("Compact complete: was " << before << ", now "
                                               << get_full_container_size());
    }

    bool EndpointContainer::full() const
    {
        /// @todo this is actually fairly arbitrary - we can go up to around
        /// max_size()
        return !(get_full_container_size() < vrpn_MAX_ENDPOINTS - 1);
    }

    bool EndpointContainer::destroy(EndpointContainer::base_pointer endpoint)
    {
        if (!endpoint) {
            return false;
        }
        raw_iterator it = std::find(begin_(), end_(), endpoint);
        if (it != end_()) {
            needsCompact_ = true;
            VRPN_EC_TRACE(endpoint << " destroyed at location "
                                   << (it - begin_()));
            try {
              delete *it;
            } catch (...) {
              fprintf(stderr, "EndpointContainer::destroy: delete failed\n");
              return false;
            }
            *it = NULL;
            return true;
        }
        return false;
    }

    void EndpointContainer::acquire_(EndpointContainer::pointer endpoint)
    {
        if (NULL != endpoint) {

            VRPN_EC_TRACE(endpoint << " acquired at location "
                                   << get_full_container_size());
            container_.push_back(endpoint);
        }
    }

} // namespace vrpn
