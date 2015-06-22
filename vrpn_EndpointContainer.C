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

namespace vrpn {
    namespace {
        template <typename T> struct EndpointCloser {
        public:
            void operator()(T *obj)
            {
                if (obj) {
                    obj->drop_connection();
                    delete obj;
                }
            }
        };
    } // namespace

    /// @brief Helper function required to cast NULL to the right pointer type,
    /// for the sake of STL algorithms, since we aren't allowed to use nullptr
    /// yet.
    static inline EndpointContainer::pointer getNullEndpoint()
    {
        return reinterpret_cast<EndpointContainer::pointer>(NULL);
    }

    EndpointContainer::EndpointContainer()
        : needsCompact_(false)
    {
    }

    EndpointContainer::~EndpointContainer() { clear(); }

    void EndpointContainer::clear()
    {
        ::std::for_each(begin_(), end_(), EndpointCloser<T>());
        container_.clear();
    }

    void EndpointContainer::compact()
    {
        if (needsCompact_) {
            raw_iterator it = std::remove(begin_(), end_(), getNullEndpoint());
            container_.resize(it - end_());
            needsCompact_ = false;
        }
    }

    bool EndpointContainer::full() const
    {
        /// @todo this is actually fairly arbitrary - we can go up to around
        /// max_size()
        return !(get_full_container_size() < vrpn_MAX_ENDPOINTS - 1);
    }

    bool EndpointContainer::destroy(EndpointContainer::size_type i)
    {
        bool valid = is_valid(i);
        delete container_[i]; // safe to delete null
        container_[i] = NULL;
        needsCompact_ |= valid;
        return valid;
    }

    bool EndpointContainer::destroy(EndpointContainer::base_pointer endpoint)
    {
        raw_iterator it = std::find(begin_(), end_(), endpoint);
        if (it != end_()) {
            delete *it;
            *it = NULL;
            needsCompact_ = true;
            return true;
        }
        return false;
    }

} // namespace vrpn