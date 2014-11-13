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

#ifndef INCLUDED_vrpn_ConnectionPtr_h_GUID_52044DCC_1409_4F8B_FC18_0F80285ABDBE
#define INCLUDED_vrpn_ConnectionPtr_h_GUID_52044DCC_1409_4F8B_FC18_0F80285ABDBE

// Internal Includes
// - none

// Library/third-party includes
#include <vrpn_Connection.h>
#include <vrpn_MainloopObject.h>

// Standard includes
// - none

/// @brief A shared pointer class for holding on to vrpn_Connection instances,
/// using the existing "intrusive reference counting" automatically.
class vrpn_ConnectionPtr {
public:
    /// Explicit constructor from a non-smart connection pointer
    explicit vrpn_ConnectionPtr(vrpn_Connection* c = NULL)
        : _p(c)
    {
        if (_p) {
            _p->addReference();
        }
    }

    /// Copy constructor from smart pointer
    vrpn_ConnectionPtr(vrpn_ConnectionPtr const& other)
        : _p(other._p)
    {
        if (_p) {
            _p->addReference();
        }
    }

    /// Assignment operator from smart pointer
    vrpn_ConnectionPtr& operator=(vrpn_ConnectionPtr const& other)
    {
        if (this == &other || _p == other._p) {
            /// self-assignment is a no-op
            return *this;
        }

        reset();
        if (other._p) {
            _p = other._p;
            _p->addReference();
        }
        return *this;
    }

    /// Assignment operator from non-smart pointer
    vrpn_ConnectionPtr& operator=(vrpn_Connection* other)
    {
        if (_p == other) {
            /// self-assignment is a no-op
            return *this;
        }

        reset();
        if (other) {
            _p = other;
            _p->addReference();
        }
        return *this;
    }

    /// Destructor - decrements the contained reference count, if applicable
    ~vrpn_ConnectionPtr() { reset(); }

    /// Clears the contained pointer and decrements the reference count, if
    /// applicable
    void reset()
    {
        if (_p) {
            _p->removeReference();
        }
        _p = NULL;
    }

    /// Gets the contained "non-smart" pointer. You are responsible
    /// for calling vrpn_Connection::addReference() if you want to
    /// affect connection lifetime with this pointer!
    /// (Included VRPN devices take care of this by default)
    vrpn_Connection* get() const { return _p; }

    /// @name Smart Pointer operators
    /// @{
    vrpn_Connection& operator*() { return *_p; }

    vrpn_Connection const& operator*() const { return *_p; }

    vrpn_Connection* operator->() { return _p; }

    vrpn_Connection const* operator->() const { return _p; }
    /// @}

    bool operator!() const { return !_p; }

    /// @name Safe Bool Idiom
    /// @{
    typedef vrpn_Connection* vrpn_ConnectionPtr::*unspecified_bool_type;
    operator unspecified_bool_type() const
    {
        return (this->operator!()) ? &vrpn_ConnectionPtr::_p : NULL;
    }
    /// @}

    /// @name Connection creation functions
    /// Use these function, rather than initializing a vrpn_ConnectionPtr with
    /// results of vrpn_create_server_connection() - this will correctly handle
    /// the default reference added by the vrpn_create_server_connection()
    /// function. Identical signatures are provided for your convenience
    /// @{
    static vrpn_ConnectionPtr
    create_server_connection(int port = vrpn_DEFAULT_LISTEN_PORT_NO,
                             const char* local_in_logfile_name = NULL,
                             const char* local_out_logfile_name = NULL,
                             const char* NIC_NAME = NULL)
    {
        return vrpn_ConnectionPtr(
            vrpn_create_server_connection(port, local_in_logfile_name,
                                          local_out_logfile_name, NIC_NAME),
            false);
    }

    static vrpn_ConnectionPtr
    create_server_connection(const char* cname,
                             const char* local_in_logfile_name = NULL,
                             const char* local_out_logfile_name = NULL)
    {
        return vrpn_ConnectionPtr(
            vrpn_create_server_connection(cname, local_in_logfile_name,
                                          local_out_logfile_name),
            false);
    }
    /// @}

private:
    /// Private constructor used by the connection creation functions, allowing
    /// creation of a vrpn_ConnectionPtr without increasing the reference count
    /// since the construction functions in VRPN automatically do this once.
    vrpn_ConnectionPtr(vrpn_Connection* c, bool needsAddRef)
        : _p(c)
    {
        if (_p && needsAddRef) {
            _p->addReference();
        }
    }

    /// Contained pointer
    vrpn_Connection* _p;

    /// Dummy function supporting the safe bool idiom
    void this_type_does_not_support_comparisons() const {}
};

/// Equality operator for connection smart pointers
/// @relates vrpn_ConnectionPtr
inline bool operator==(const vrpn_ConnectionPtr& lhs,
                       const vrpn_ConnectionPtr& rhs)
{
    return lhs.get() == rhs.get();
}

/// Inequality operator for connection smart pointers
/// @relates vrpn_ConnectionPtr
inline bool operator!=(const vrpn_ConnectionPtr& lhs,
                       const vrpn_ConnectionPtr& rhs)
{
    return lhs.get() != rhs.get();
}

#if 0
/// @name Poisoning operators for connection smart pointers
/// Supporting the safe bool idiom
/// @todo why does this not work right?
/// @relates vrpn_ConnectionPtr
/// @{
template <typename T>
bool operator!=(const T& lhs, const vrpn_ConnectionPtr& rhs) {
	rhs.this_type_does_not_support_comparisons();
	return false;
}

template <typename T>
bool operator==(const T& lhs, const vrpn_ConnectionPtr& rhs) {
	rhs.this_type_does_not_support_comparisons();
	return false;
}

template <typename T>
bool operator!=(const vrpn_ConnectionPtr& lhs, const T& rhs) {
	lhs.this_type_does_not_support_comparisons();
	return false;
}

template <typename T>
bool operator==(const vrpn_ConnectionPtr& lhs, const T& rhs) {
	lhs.this_type_does_not_support_comparisons();
	return false;
}
/// @}
#endif

/// Namespace enclosing internal implementation details
namespace detail {
    template <class T> class TypedMainloopObject;

    /// Specialization of vrpn_MainloopObject for holding connections that
    /// are maintained by vrpn_ConnectionPtr smart pointers.
    template <>
    class TypedMainloopObject<vrpn_ConnectionPtr> : public vrpn_MainloopObject {
    public:
        explicit TypedMainloopObject(vrpn_ConnectionPtr o)
            : _instance(o)
        {
            if (!o) {
                throw vrpn_MainloopObject::
                    CannotWrapNullPointerIntoMainloopObject();
            }
        }

        virtual bool broken() { return (!_instance->doing_okay()); }

        virtual void mainloop() { _instance->mainloop(); }

    protected:
        virtual void* _returnContained() const { return _instance.get(); }

    private:
        vrpn_ConnectionPtr _instance;
    };
} // end of namespace detail

#endif // INCLUDED_vrpn_ConnectionPtr_h_GUID_52044DCC_1409_4F8B_FC18_0F80285ABDBE
