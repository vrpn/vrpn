/** @file
    @brief Header

    @date 2015

    @author
    Ryan Pavlik
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2015 Sensics, Inc.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef INCLUDED_vrpn_EndpointContainer_h_GUID_DB073DE8_5BBC_46BF_255B_71264D47A639
#define INCLUDED_vrpn_EndpointContainer_h_GUID_DB073DE8_5BBC_46BF_255B_71264D47A639

// Internal Includes
#include "vrpn_Types.h"
#include "vrpn_Configure.h"

// Library/third-party includes
// - none

// Standard includes
#include <vector>
#include <assert.h>
#include <stddef.h> // for NULL

class VRPN_API vrpn_Endpoint;
class VRPN_API vrpn_Endpoint_IP;
class vrpn_Connection;

/// @brief Function pointer to an endpoint allocator.
typedef vrpn_Endpoint_IP *(*vrpn_EndpointAllocator)(
    vrpn_Connection *connection, vrpn_int32 *numActiveConnections);

namespace vrpn {

    /// @brief Combines the function pointer for an Endpoint Allocator with its
    /// two arguments into a single callable object, with the ability to
    /// override the last parameter at call time.
    class BoundEndpointAllocator {
    public:
        BoundEndpointAllocator()
            : epa_(NULL)
            , conn_(NULL)
            , numActiveEndpoints_(NULL)
        {
        }
        BoundEndpointAllocator(vrpn_EndpointAllocator epa,
                               vrpn_Connection *conn,
                               vrpn_int32 *numActiveEndpoints = NULL)
            : epa_(epa)
            , conn_(conn)
            , numActiveEndpoints_(numActiveEndpoints)
        {
        }

        typedef vrpn_Endpoint_IP *return_type;

        /// @brief Default, fully pre-bound
        return_type operator()() const
        {
            assert(epa_);
            if (!epa_) {
                return NULL;
            }
            return (*epa_)(conn_, numActiveEndpoints_);
        }

        /// @brief Overload, with alternate num active connnection pointer.
        return_type operator()(vrpn_int32 *alternateNumActiveEndpoints) const
        {
            assert(epa_);
            if (!epa_) {
                return NULL;
            }
            return (*epa_)(conn_, alternateNumActiveEndpoints);
        }

        /// @brief Creates a new bound allocator with a different num endpoints
        /// pointer.
        BoundEndpointAllocator
        create_alternate(vrpn_int32 *alternateNumActiveEndpoints) const
        {
            return BoundEndpointAllocator(epa_, conn_,
                                          alternateNumActiveEndpoints);
        }

    private:
        vrpn_EndpointAllocator epa_;
        vrpn_Connection *conn_;
        vrpn_int32 *numActiveEndpoints_;
    };

    class EndpointIterator;

    /** @brief Container for endpoints, held by pointer.

    To check if we have room, use this: `if (d_endpoints.full()) {}` instead of
    the old code looking like this: `if (which_end >= vrpn_MAX_ENDPOINTS)`

    Usage example for iteration:

    ~~~
    for (vrpn::EndpointIterator it = d_endpoints.begin(), e = d_endpoints.end();
         it != e; ++it) {
        it->pack_type_description(which)
    }
    ~~~

    */
    class EndpointContainer {
    public:
        typedef vrpn_Endpoint_IP T;
        typedef T &reference;
        typedef T *pointer;
        typedef vrpn_Endpoint *base_pointer;

    private:
        typedef std::vector<pointer> container_type;

    public:
        typedef container_type::size_type size_type;
        typedef EndpointIterator iterator;
        typedef EndpointIterator const_iterator;

        EndpointContainer();

        ~EndpointContainer();

        void clear();

        /// @brief Shorthand for get_by_index(0)
        pointer front() const { return get_by_index(0); }

        /// @brief Allocates an endpoint using the given allocator in the
        /// container, returning a reference to the new endpoint.
        pointer allocate(BoundEndpointAllocator const &allocator);

        /// @brief Goes through and gets rid of the NULL entries.
        void compact();

        /// @brief Can we no longer accommodate a new endpoint?
        bool full() const;

        /// @brief Checks to see if an index is both in-range and pointing to a
        /// still-extant object
        bool is_valid(size_type i) const;

        /// @brief Destroys the object at index i, if it exists.
        /// @return true if there was something for us to delete
        bool destroy(size_type i);

        /// @brief Destroys the contained endpoint by address.
        /// @return true if there was something for us to delete
        bool destroy(base_pointer endpoint);

        pointer get_by_index(size_type i) const;

        /// @brief Get size of container including NULL elements that haven't
        /// been compacted yet.
        size_type get_full_container_size() const;

        /// @brief Get an iterator to the beginning that skips nulls.
        /// Invalidated by compacting.
        iterator begin() const;

        /// @brief Get an iterator suitable only for testing to see if we're
        /// "done"
        iterator end() const;

    private:
        typedef container_type::iterator raw_iterator;
        typedef container_type::const_iterator raw_const_iterator;
        /// @name Internal iterator methods
        /// @{
        raw_iterator begin_() { return container_.begin(); }
        raw_const_iterator begin_() const { return container_.begin(); }
        raw_iterator end_() { return container_.end(); }
        raw_const_iterator end_() const { return container_.end(); }
        // @}
        container_type container_;
        bool needsCompact_;
    };

    /// @brief An iterator (ForwardIterator concept, I believe) that goes
    /// forward in an EndpointContainer skipping the NULLs.
    ///
    /// All end() iterators compare equal to each other and to the
    /// default-constructed iterator. They are the only invalid iterators:
    /// incrementing an iterator past the end makes it the same as the
    /// default-constructed iterator.
    ///
    /// That is, for all EndpointIterators it, we enforce the class invariant
    /// `it.valid() || (it == EndpointIterator())`
    class EndpointIterator {
    public:
        // typedef EndpointIteratorBase<ContainerType> type;
        typedef EndpointIterator type;
        typedef EndpointContainer const container_type;
        typedef container_type::pointer pointer;
        typedef container_type::reference reference;
        typedef container_type::size_type size_type;

        /// @brief Default constructor, equal to all other default-constructed
        /// instances and all end()
        EndpointIterator()
            : index_(0)
            , container_(NULL)
        {
        }

        /// @brief Constructor with container, points to beginning of container.
        EndpointIterator(container_type &container)
            : index_(0)
            , container_(&container)
        {
            enforce_invariant_();
        }

        EndpointIterator(container_type &container, size_type index)
            : index_(index)
            , container_(&container)
        {
            enforce_invariant_();
        }

        /// @brief Does this iterator refer to a valid element?
        ///
        /// Class invariant: valid() || (*this == type())
        /// That is, there is only one invalid value.
        bool valid() const
        {
            return container_ && container_->is_valid(index_);
        }

        /// @brief Extract the pointer.
        pointer get_pointer() const { return container_ ? (get_raw_()) : NULL; }

        /// @brief Implicit conversion operator to pointer.
        operator pointer() const { return get_pointer(); }

        /// @brief prefix ++ operator, increments (and skips any nulls)
        type &operator++()
        {
            if (!valid()) {
                // Early out if we're already at the end
                return *this;
            }

            // Increment until we either go out of bounds or get a non-null
            // entry
            do {
                index_++;
            } while (index_in_bounds_() && (get_raw_() == NULL));

            // We may have run out of elements, so check the invariant
            enforce_invariant_();

            return *this;
        }

        /// @name Smart pointer idiom operators
        /// @{
        pointer operator->() const { return get_pointer(); }

        reference operator*() const { return *get_raw_(); }
        /// @}

        bool operator==(type const &other) const
        {
            return (container_ == other.container_) && (index_ == other.index_);
        }
        bool operator!=(type const &other) const
        {
            return (container_ != other.container_) || (index_ != other.index_);
        }

    private:
        /// @brief Function to verify an iterator to enforce the class
        /// invariant.
        void enforce_invariant_()
        {
            if (!valid()) {
                /// Assign from default-constructed iterator, to be the same as
                /// end()
                *this = type();
            }
        }

        /// @brief get, without checking validity of container_ first!
        pointer get_raw_() const { return container_->get_by_index(index_); }

        /// @brief Helper to check index vs container bounds, without checking
        /// validity of container_ first!
        bool index_in_bounds_() const
        {
            return index_ < container_->get_full_container_size();
        }

        size_type index_;
        container_type *container_;
    };

    // Inline Implementations //

    inline bool
    EndpointContainer::is_valid(EndpointContainer::size_type i) const
    {
        return (i < get_full_container_size()) && (NULL != container_[i]);
    }

    inline EndpointContainer::pointer
    EndpointContainer::get_by_index(size_type i) const
    {
        if (!is_valid(i)) {
            return NULL;
        }
        return container_[i];
    }
    inline EndpointContainer::size_type
    EndpointContainer::get_full_container_size() const
    {
        return container_.size();
    }

    inline EndpointIterator EndpointContainer::begin() const
    {
        return EndpointIterator(*this);
    }

    inline EndpointIterator EndpointContainer::end() const
    {
        return EndpointIterator();
    }

} // namespace vrpn

#endif // INCLUDED_vrpn_EndpointContainer_h_GUID_DB073DE8_5BBC_46BF_255B_71264D47A639
