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

#include "vrpn_Assert.h"

// Library/third-party includes
// - none

// Standard includes
#include <vector>
#include <stddef.h> // for NULL

class VRPN_API vrpn_Endpoint;
class VRPN_API vrpn_Endpoint_IP;

namespace vrpn {

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

        /// @brief Constructor of empty container.
        EndpointContainer();

        /// @brief Destructor - includes a call to clear()
        ~EndpointContainer();

        /// @brief Tells each held endpoint in turn to drop the connection then
        /// deletes it
        void clear();

        /// @brief Shorthand for get_by_index(0)
        pointer front() const { return get_by_index(0); }

        /// @brief Given the result of an endpoint allocator, if it's non-NULL,
        /// takes ownership of it.
        /// @return the input pointer
        template <typename T> T *acquire(T *endpoint)
        {
            acquire_(endpoint);
            return endpoint;
        }

        /// @brief Goes through and gets rid of the NULL entries.
        void compact();

        /// @brief Can we no longer accommodate a new endpoint?
        bool full() const;

        /// @brief Checks to see if an index is both in-range and pointing to a
        /// still-extant object
        bool is_valid(size_type i) const;

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
        /// @name Internal raw iterators and methods
        /// @{
        typedef container_type::iterator raw_iterator;
        typedef container_type::const_iterator raw_const_iterator;
        raw_iterator begin_() { return container_.begin(); }
        raw_const_iterator begin_() const { return container_.begin(); }
        raw_iterator end_() { return container_.end(); }
        raw_const_iterator end_() const { return container_.end(); }
        // @}
        /// @name Internal helper methods
        /// @{
        /// @brief Implementation of acquire for the stored pointer type.
        void acquire_(pointer endpoint);
        /// @brief Do actual compact once we've determined it's necessary.
        void compact_();

        /// @}
        container_type container_;
        bool needsCompact_;
    };

#define VRPN_ECITERATOR_ASSERT_INVARIANT()                                     \
    VRPN_ASSERT_MSG(valid() != equal_to_default_(),                            \
                    "Class invariant for EndpointIterator")

    /// @brief An iterator that goes forward in an EndpointContainer skipping
    /// the NULLs, that also acts a bit like a pointer/smart pointer (can treat
    /// it as a vrpn_Endpoint *)
    ///
    /// Because we know at design time that it iterates through pointers,
    /// we have pointer-related operator overloads that mean there's no need to
    /// double-dereference.
    ///
    /// Fulfills the InputIterator concept:
    /// http://en.cppreference.com/w/cpp/concept/InputIterator
    ///
    /// All end() iterators compare equal to each other and to the
    /// default-constructed iterator. They are the only invalid iterators:
    /// incrementing an iterator past the end makes it the same as the
    /// default-constructed iterator.
    ///
    /// That is, for all EndpointIterators it, we enforce the class invariant
    /// `it.valid() || (it == EndpointIterator())` (and that's actually an XOR)
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
            VRPN_ASSERT_MSG(equal_to_default_(),
                            "Default constructed value should be equal to "
                            "default: verifies that 'equal_to_default_()' is "
                            "equivalent to '*this == EndpointIterator()'");
            VRPN_ASSERT_MSG(!valid(),
                            "Default constructed value should not be valid");
        }

        /// @brief Constructor with container, points to beginning of container.
        EndpointIterator(container_type &container)
            : index_(0)
            , container_(&container)
        {
            // Advance index as required to maintain the class invariant.
            skip_nulls_();
            VRPN_ECITERATOR_ASSERT_INVARIANT();
        }

        /// @brief Constructor with container and raw index into container.
        EndpointIterator(container_type &container, size_type index)
            : index_(index)
            , container_(&container)
        {
            // Advance index as required to maintain the class invariant.
            skip_nulls_();
            VRPN_ECITERATOR_ASSERT_INVARIANT();
        }

        /// @brief Does this iterator refer to a valid element?
        ///
        /// Class invariant: valid() || (*this == type())
        /// That is, there is only one invalid value.
        bool valid() const
        {
            return container_ && container_->is_valid(index_);
        }

        /// @brief Extract the pointer (NULL if iterator is invalid)
        pointer get_pointer() const
        {
            VRPN_ECITERATOR_ASSERT_INVARIANT();
            // Only need to condition on container validity: invalid indexes
            // safely return null from get_raw_()
            return container_ ? (get_raw_()) : NULL;
        }

        /// @brief Implicit conversion operator to pointer.
        operator pointer() const
        {
            VRPN_ECITERATOR_ASSERT_INVARIANT();
            return get_pointer();
        }

        /// @brief prefix ++ operator, increments (and skips any nulls)
        type &operator++()
        {
            /// Invariant might be invalid here, since the user might have just
            /// deleted something.
            if (equal_to_default_()) {
                // Early out if we're already the end sentinel (default
                // constructor value)
                return *this;
            }

            // Increment until we either go out of bounds or get a non-null
            // entry
            index_++;
            skip_nulls_();
            VRPN_ECITERATOR_ASSERT_INVARIANT();
            return *this;
        }

        /// @name Smart pointer idiom operators
        /// @{
        pointer operator->() const
        {
            VRPN_ECITERATOR_ASSERT_INVARIANT();
            return get_pointer();
        }

        reference operator*() const
        {
            VRPN_ECITERATOR_ASSERT_INVARIANT();
            return *get_raw_();
        }
        /// @}

        /// @name Comparison operators, primarily for loop use
        /// @{
        bool operator==(type const &other) const
        {
            return (container_ == other.container_) && (index_ == other.index_);
        }
        bool operator!=(type const &other) const
        {
            return (container_ != other.container_) || (index_ != other.index_);
        }
        /// @}

    private:
        bool equal_to_default_() const
        {
            return (NULL == container_) && (index_ == 0);
        }
        void skip_nulls_()
        {
            while (index_in_bounds_() && (get_raw_() == NULL)) {
                index_++;
            }
            // We may have run out of elements, so check the invariant
            enforce_invariant_();
        }
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
        ///
        /// Note that the container handles cases where the index is out of
        /// range by returning NULL, so that's safe.
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
#undef VRPN_ECITERATOR_ASSERT_INVARIANT

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

    // making this condition inline so that it has minimal overhead if
    // we don't actually need to perform a compaction.
    inline void EndpointContainer::compact()
    {
        if (needsCompact_) {
            compact_();
        }
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
