/** @file
    @brief Header

    @date 2016

    @author
    Ryan Pavlik
    <ryan@sensics.com>
    <http://sensics.com>
*/

//                  Copyright Sensics 2016.
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE_1_0.txt or copy at
//         http://www.boost.org/LICENSE_1_0.txt)

#ifndef INCLUDED_vrpn_OwningPtr_h_GUID_A2A96325_1863_458B_A567_6FD968CB1321
#define INCLUDED_vrpn_OwningPtr_h_GUID_A2A96325_1863_458B_A567_6FD968CB1321

// Internal Includes
#include "vrpn_Assert.h"

// Library/third-party includes
// - none

// Standard includes
#include <utility>
#include <iostream>

namespace vrpn {

    /// default deleter
    template <typename T> struct DefaultDeleter {
        void operator()(T* ptr) const { 
          try {
            delete ptr;
          } catch (...) {
            std::cerr << "DefaultDeleter: delete failed" << std::endl;;
            return;
          }
        }
    };
    /// handle arrays with delete []
    template <typename T> struct DefaultDeleter<T[]> {
        void operator()(T* ptr) const {
          try {
            delete[] ptr;
          } catch (...) {
            std::cerr << "DefaultDeleter: delete failed" << std::endl;;
            return;
          }
        }
    };
    namespace traits {
        /// Default trait.
        template <typename T> struct OwningPtrPointerType {
            typedef T* type;
        };
        /// Specialization for arrays
        template <typename T> struct OwningPtrPointerType<T[]> {
            typedef T* type;
        };
    } // namespace traits

    /// A unique-ownership smart pointer, with the ability to transfer
    /// ownership, but only explicitly (aka, not like auto_ptr did it).
    ///
    /// Essentially, a hybrid of boost::scoped_ptr and std::unique_ptr that
    /// doesn't require C++11.
    ///
    /// Limitations relative to unique_ptr: no move semantics (naturally, since
    /// no C++11, and have not implemented rvalue-reference-emulation), no
    /// stateful deleters, and no casting or conversions.
    template <typename T, typename D = DefaultDeleter<T> > class OwningPtr {
    public:
        typedef T element_type;
        typedef T& reference_type;
        typedef D deleter_type;
        typedef typename traits::OwningPtrPointerType<T>::type pointer;
        typedef OwningPtr<T, D> type;

        /// Construct empty.
        OwningPtr()
            : p_(NULL)
        {
        }

        /// Construct with a raw pointer for a new object to take exclusive
        /// ownership of.
        explicit OwningPtr(pointer p)
            : p_(p)
        {
        }

        /// Destructor: deletes owned object, if any.
        ~OwningPtr() { reset(); }

        /// Deletes the owned object, if any, and takes ownership of a new
        /// object, if one is passed.
        void reset(pointer p = pointer())
        {
            pointer oldPtr(p_);
            p_ = p;
            if (oldPtr) {
                invokeDeleter(oldPtr);
            }
        }

        /// Returns the held pointer without performing the delete action.
        pointer release()
        {
            pointer ret(p_);
            p_ = NULL;
            return ret;
        }

        /// Swap pointers with another OwningPtr of the same type.
        void swap(type& other) { std::swap(p_, other.p_); }

        /// Gets pointer value
        pointer get() const { return p_; }

        /// Smart pointer operator overload
        pointer operator->() const
        {
            VRPN_ASSERT_MSG(valid(),
                            "attempt to use operator-> on a null pointer");
            return p_;
        }

        /// Smart pointer operator
        reference_type operator*() const
        {
            VRPN_ASSERT_MSG(valid(),
                            "attempt to use operator* on a null pointer");
            return *p_;
        }

        /// Checks pointer validity
        bool valid() const { return p_ != NULL; }

        /// @name Safe bool idiom - in the absence of explicit operator bool()
        /// @{
        typedef pointer type::*unspecified_bool_type;
        operator unspecified_bool_type() const
        {
            return valid() ? &type::p_ : NULL;
        }
        /// @}

        /// Redundant way of checking pointer validity
        bool operator!() const { return !valid(); }

    private:
        /// wrapper around invoking the deleter, in case the decision to not
        /// provide stateful deleters is reconsidered.
        void invokeDeleter(pointer p) { deleter_type()(p); }
        // non-copyable
        OwningPtr(OwningPtr const&);
        // Non-assignable
        OwningPtr& operator=(OwningPtr const&);

        /// The sole data member: the owned pointer.
        pointer p_;
    };

    template <typename T, typename D>
    inline void swap(OwningPtr<T, D>& lhs, OwningPtr<T, D>& rhs)
    {
        lhs.swap(rhs);
    }

    template <typename T, typename D>
    inline typename traits::OwningPtrPointerType<T>::type
    get_pointer(OwningPtr<T, D> const& ptr)
    {
        return ptr.get();
    }

    template <typename T, typename D1, typename D2>
    inline bool operator==(OwningPtr<T, D1> const& lhs,
                           OwningPtr<T, D2> const& rhs)
    {
        return lhs.get() == rhs.get();
    }

    template <typename T, typename D>
    inline bool operator==(OwningPtr<T, D> const& lhs,
                           typename traits::OwningPtrPointerType<T>::type rhs)
    {
        return lhs.get() == rhs;
    }

    template <typename T, typename D>
    inline bool operator==(typename traits::OwningPtrPointerType<T>::type lhs,
                           OwningPtr<T, D> const& rhs)
    {
        return lhs == rhs.get();
    }

    template <typename T, typename D1, typename D2>
    inline bool operator!=(OwningPtr<T, D1> const& lhs,
                           OwningPtr<T, D2> const& rhs)
    {
        return lhs.get() != rhs.get();
    }
    template <typename T, typename D>
    inline bool operator!=(OwningPtr<T, D> const& lhs,
                           typename traits::OwningPtrPointerType<T>::type rhs)
    {
        return lhs.get() != rhs;
    }

    template <typename T, typename D>
    inline bool operator!=(typename traits::OwningPtrPointerType<T>::type lhs,
                           OwningPtr<T, D> const& rhs)
    {
        return lhs != rhs.get();
    }
} // namespace vrpn

#endif // INCLUDED_vrpn_OwningPtr_h_GUID_A2A96325_1863_458B_A567_6FD968CB1321
