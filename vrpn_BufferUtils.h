/** @file
	@brief Internal header providing unbuffering facilities for a number of types.

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

#pragma once
#ifndef INCLUDED_vrpn_BufferUtils_h_GUID_6a741cf1_9fa4_4064_8af0_fa0c6a16c810
#define INCLUDED_vrpn_BufferUtils_h_GUID_6a741cf1_9fa4_4064_8af0_fa0c6a16c810

// Internal Includes
#include "vrpn_Shared.h"

// Library/third-party includes
// - none

// Standard includes
#include <cassert> // for assert
#include <cstring> // for std::memcpy

#if !( defined(_WIN32) && defined(VRPN_USE_WINSOCK_SOCKETS) )
#include <netinet/in.h>
#endif

/// @brief Contains overloaded hton() and ntoh() functions that forward
/// to their correctly-typed implementations.
namespace vrpn_byte_order {
	namespace detail {
		/// Traits class to get the uint type of a given size
		template<int TypeSize>
		struct uint_traits;

		template<> struct uint_traits<1> {
			typedef vrpn_uint8 type;
		};
		template<> struct uint_traits<2> {
			typedef vrpn_uint16 type;
		};
		template<> struct uint_traits<4> {
			typedef vrpn_uint32 type;
		};
	}

	/// host to network byte order for 8-bit uints is a no-op
	inline vrpn_uint8 hton(vrpn_uint8 hostval) {
		return hostval;
	}

	/// network to host byte order for 8-bit uints is a no-op
	inline vrpn_uint8 ntoh(vrpn_uint8 netval) {
		return netval;
	}

	/// host to network byte order for 16-bit uints
	inline vrpn_uint16 hton(vrpn_uint16 hostval) {
		return htons(hostval);
	}

	/// network to host byte order for 16-bit uints
	inline vrpn_uint16 ntoh(vrpn_uint16 netval) {
		return ntohs(netval);
	}

	/// host to network byte order for 32-bit uints
	inline vrpn_uint32 hton(vrpn_uint32 hostval) {
		return htonl(hostval);
	}

	/// network to host byte order for 32-bit uints
	inline vrpn_uint32 ntoh(vrpn_uint32 netval) {
		return ntohl(netval);
	}

	/// host to network byte order for 64-bit floats, using vrpn htond
	inline vrpn_float64 hton(vrpn_float64 hostval) {
		return htond(hostval);
	}

	/// network to host byte order for 64-bit floats, using vrpn ntohd
	inline vrpn_float64 ntoh(vrpn_float64 netval) {
		return ntohd(netval);
	}

	/// Templated hton that type-puns to the same-sized uint type
	/// as a fallback for those types not explicitly defined above.
	template<typename T>
	inline T hton(T input) {
		union {
			T asInput;
			typename detail::uint_traits<sizeof(T)>::type asInt;
		} inVal, outVal;
		inVal.asInput = input;
		outVal.asInt = hton(inVal.asInt);
		return outVal.asInput;
	}

	/// Templated ntoh that type-puns to the same-sized uint type
	/// as a fallback for those types not explicitly defined above.
	template<typename T>
	inline T ntoh(T input) {
		union {
			T asInput;
			typename detail::uint_traits<sizeof(T)>::type asInt;
		} inVal, outVal;
		inVal.asInput = input;
		outVal.asInt = ntoh(inVal.asInt);
		return outVal.asInput;
	}
}

namespace detail {
	template<typename T>
	struct remove_const {
		typedef T type;
	};

	template<typename T>
	struct remove_const<const T> {
		typedef T type;
	};
} // end of namespace detail

/// Function template to unbuffer values from a buffer stored in network
/// byte order. Specify the type to extract T as a template parameter.
/// The templated buffer type ByteT will be deduced automatically.
/// The input pointer will be advanced past the unbuffered value.
template<typename T, typename ByteT>
static inline T vrpn_unbuffer_from_little_endian(ByteT * & input) {
	using namespace vrpn_byte_order;

	/// @todo make this a static assertion
	assert(sizeof(ByteT) == 1);

	/// Union to allow type-punning
	union {
		typename ::detail::remove_const<ByteT>::type bytes[sizeof(T)];
		T value;
	};

	/// Swap known little-endian into big-endian (aka network byte order)
	for (int i = 0, j = sizeof(T) - 1; i < sizeof(T); ++i, --j) {
		bytes[i] = input[j];
	}

	/// Advance input pointer
	input += sizeof(T);

	/// return value in host byte order
	return ntoh(value);
}

/// Function template to unbuffer values from a buffer stored in network
/// byte order. Specify the type to extract T as a template parameter.
/// The templated buffer type ByteT will be deduced automatically.
/// The input pointer will be advanced past the unbuffered value.
template<typename T, typename ByteT>
inline T vrpn_unbuffer(ByteT * & input) {
	using namespace vrpn_byte_order;

	/// @todo make this a static assertion
	assert(sizeof(ByteT) == 1);

	/// Union to allow type-punning and ensure alignment
	union {
		typename ::detail::remove_const<ByteT>::type bytes[sizeof(T)];
		T value;
	};

	/// Copy bytes into union
	std::memcpy(bytes, input, sizeof(T));

	/// Advance input pointer
	input += sizeof(T);

	/// return value in host byte order
	return ntoh(value);
}

#endif // INCLUDED_vrpn_BufferUtils_h_GUID_6a741cf1_9fa4_4064_8af0_fa0c6a16c810
