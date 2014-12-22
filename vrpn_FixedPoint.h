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
#include "vrpn_Types.h"

// Library/third-party includes
// - none

// Standard includes
// - none

namespace {

/**
 * Set as the type member of vrpn_IntegerOfSize if too many bits are required.
 * We only support up to 32 bits currently.
 */
struct vrpn_IntegerOverflow;

/**
 * \name Integer types.
 *
 * In the general case, we'll start adding bits to the required size until we
 * land on one of the specialized cases below, then return that specialized
 * case. The end result is that we return an integer type that can contain a
 * value require NUM_BITS bits.
 */
//@{
template <int NUM_BITS>
struct vrpn_IntegerOfSize {
    // An integer requiring n bits can be represented by an integer of size n+1 bits.
    typedef typename vrpn_IntegerOfSize<NUM_BITS + 1>::type type;
};

template <>
struct vrpn_IntegerOfSize<8> {
    typedef vrpn_int8 type;
};

template <>
struct vrpn_IntegerOfSize<16> {
    typedef vrpn_int16 type;
};

template <>
struct vrpn_IntegerOfSize<32> {
    typedef vrpn_int32 type;
};

template <>
struct vrpn_IntegerOfSize<64> {
    typedef vrpn_IntegerOverflow type;
};

// TODO allow for larger types if we can establish VRPN versions of them (e.g.,
// equivalents to int64_t).

//@}

} // end anonymous namespace


/**
 * A fixed-point value class. All values are signed, two's-complement.
 *
 * @tparam INTEGER_BITS The number of bits devoted to the integer part.
 * @tparam FRACTIONAL_BITS The number of bits devoted to the fractioal part.
 */
template <int INTEGER_BITS, int FRACTIONAL_BITS>
class vrpn_FixedPoint {
public:
    /**
     * Find an integer type large enough to hold INTEGER_BITS.
     */
    typedef typename vrpn_IntegerOfSize<INTEGER_BITS>::type IntegerType;

    typedef typename vrpn_IntegerOfSize<INTEGER_BITS + FRACTIONAL_BITS>::type RawType;

    /**
     * \name Constructors.
     *
     * The bits of an integral type passed to the constructor will be
     * interpreted as as fixed-point value. A floating-point type passed to the
     * constructor will be converted to a fixed-point value.
     */
    //@{
    vrpn_FixedPoint() : value_(0) { }
    vrpn_FixedPoint(vrpn_int8 x) : value_(x) { }
    vrpn_FixedPoint(vrpn_int16 x) : value_(x) { }
    vrpn_FixedPoint(vrpn_int32 x) : value_(x) { }
    vrpn_FixedPoint(vrpn_uint8 x) : value_(x) { }
    vrpn_FixedPoint(vrpn_uint16 x) : value_(x) { }
    vrpn_FixedPoint(vrpn_uint32 x) : value_(x) { }
    vrpn_FixedPoint(double x) : value_(x * (1 << FRACTIONAL_BITS)) { }
    vrpn_FixedPoint(float x) : value_(x * (1 << FRACTIONAL_BITS)) { }
    //@}

    // TODO add operators. lots and lots of operators.

    /**
     * Returns a floating-point representation of this
     * fixed-point value.
     */
    //@{
    operator vrpn_float32() const
    {
        return static_cast<vrpn_float32>(value_) / (1 << FRACTIONAL_BITS);
    }

    operator vrpn_float64() const
    {
        return static_cast<vrpn_float64>(value_) / (1 << FRACTIONAL_BITS);
    }
    //@}

    // FIXME remove these functions after debugging
    /** \name Debugging functions. */
    //@{
    RawType value() const
    {
        return value_;
    }
    //@}

private:
    RawType value_;

};

#endif // VRPN_FIXED_POINT_H_

