/** @file vrpn_FixedPoint.h
    @brief Header

    @date 2014

    @author
    Kevin M. Godby
    <kevin@godby.org>
*/

#ifndef VRPN_FIXED_POINT_H_
#define VRPN_FIXED_POINT_H_

// Internal Includes
// - none

// Library/third-party includes
#include <boost/type_traits/is_integral.hpp>
#include <boost/utility/enable_if.hpp>

// Standard includes
#include <cstdio>


/**
 * Convert from bytes value to fixed-point type.
 */
template <typename T, typename ReturnType>
ReturnType vrpn_fixed_to_float(T in, int m, int n, typename boost::enable_if<boost::is_integral<T> >::type* dummy = NULL)
{
    if (m + n < sizeof(ReturnType)) {
        std::fprintf(stderr, "m + n must be less than the size of the requested return type.");
        return ReturnType(0);
    }

    const std::size_t num_bits = sizeof(T);
    const int i = (in >> (num_bits - m)); // integer compenent, m highest bits
    const int f = in - (i << (num_bits - m)); // fractional component, n lowest bits

    ReturnType out = static_cast<ReturnType>(i) + static_cast<ReturnType>(1.0 / f);

    return out;
}

// Usage:
// vrpn_int16 w_bytes = vrpn_unbuffer_from_little_endian<vrpn_int16>(buffer);
// double w = vrpn_fixed_to_float(w_bytes, 2, 14);

// Each quaternion is presented as signed, 16-bit fixed point, 2’s complement number with a Q point of 14



#endif // VRPN_FIXED_POINT_H_

