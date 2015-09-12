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
#include <cstddef>  // for NULL

namespace vrpn {

    namespace detail {
        /**
         * Set as the type member of vrpn_IntegerOfSize if too many bits are
         * required.
         * We only support up to 32 bits currently.
         */
        struct IntegerOverflow;

        /**
         * \name Integer types.
         *
         * In the general case, we'll start adding bits to the required size
         *until we
         * land on one of the specialized cases below, then return that
         *specialized
         * case. The end result is that we return an integer type that can
         *contain a
         * value require NUM_BITS bits.
         */
        ///@{
        template <int NUM_BITS> struct IntegerOfSize {
            // An integer requiring n bits can be represented by an integer of
            // size n+1 bits.
            typedef typename IntegerOfSize<NUM_BITS + 1>::type type;
        };

        template <> struct IntegerOfSize<8> {
            typedef vrpn_int8 type;
        };

        template <> struct IntegerOfSize<16> {
            typedef vrpn_int16 type;
        };

        template <> struct IntegerOfSize<32> {
            typedef vrpn_int32 type;
        };

        template <> struct IntegerOfSize<64> {
            typedef IntegerOfSize type;
        };
        template <int NUM_BITS> struct UnsignedIntegerOfSize {
            // An integer requiring n bits can be represented by an integer of
            // size n+1 bits.
            typedef typename UnsignedIntegerOfSize<NUM_BITS + 1>::type type;
        };

        template <> struct UnsignedIntegerOfSize<8> {
            typedef vrpn_uint8 type;
        };

        template <> struct UnsignedIntegerOfSize<16> {
            typedef vrpn_uint16 type;
        };

        template <> struct UnsignedIntegerOfSize<32> {
            typedef vrpn_uint32 type;
        };

        /// @todo allow for larger types if we can establish VRPN versions of
        /// them (e.g., equivalents to uint64_t).

        ///@}
        template <int BITS, bool SIGNED = true>
        struct IntegerOfSizeAndSignedness : IntegerOfSize<BITS> {
        };
        template <int BITS>
        struct IntegerOfSizeAndSignedness<BITS, false>
            : UnsignedIntegerOfSize<BITS> {
        };
    } // namespace detail

    /**
     * A fixed-point value class. All values are signed, two's-complement, by
     * default.
     *
     * @tparam INTEGER_BITS The number of bits devoted to the integer part.
     * @tparam FRACTIONAL_BITS The number of bits devoted to the fractional
     * part.
     */
    template <int INTEGER_BITS, int FRACTIONAL_BITS, bool SIGNED = true>
    class FixedPoint {
    public:
        /**
         * Find an integer type large enough to hold INTEGER_BITS.
         */
        typedef typename detail::IntegerOfSizeAndSignedness<
            INTEGER_BITS, SIGNED>::type IntegerType;

        typedef typename detail::IntegerOfSizeAndSignedness<
            INTEGER_BITS + FRACTIONAL_BITS, SIGNED>::type RawType;

        /**
         * \name Constructors.
         *
         * The bits of an integral type passed to the constructor will be
         * interpreted as as fixed-point value. A floating-point type passed to
         * the constructor will be converted to a fixed-point value.
         */
        //@{
        FixedPoint()
            : value_(0)
        {
        }
        explicit FixedPoint(
            typename detail::IntegerOfSizeAndSignedness<8, SIGNED>::type x)
            : value_(x)
        {
        }
        explicit FixedPoint(
            typename detail::IntegerOfSizeAndSignedness<16, SIGNED>::type x)
            : value_(x)
        {
        }
        explicit FixedPoint(
            typename detail::IntegerOfSizeAndSignedness<32, SIGNED>::type x)
            : value_(x)
        {
        }
        explicit FixedPoint(double x)
            : value_(x * (1 << FRACTIONAL_BITS))
        {
        }
        explicit FixedPoint(float x)
            : value_(x * (1 << FRACTIONAL_BITS))
        {
        }
        //@}

        /// @todo add operators, lots and lots of operators?

        /**
         * Returns a floating-point representation of this
         * fixed-point value.
         */
        template <typename T> T get() const
        {
            return get(reinterpret_cast<TypeWrapper<T> *>(NULL));
        }

        /** \name Debugging functions. */
        ///@{
        /// @todo remove these functions after debugging
        RawType value() const { return value_; }
        ///@}

    private:
        template <typename T> struct TypeWrapper;
        vrpn_float32 get(TypeWrapper<vrpn_float32> *) const
        {
            return static_cast<vrpn_float32>(value_) / (1 << FRACTIONAL_BITS);
        }

        vrpn_float64 get(TypeWrapper<vrpn_float64> *) const
        {
            return static_cast<vrpn_float64>(value_) / (1 << FRACTIONAL_BITS);
        }

        RawType value_;
    };

} // namespace vrpn

#endif // VRPN_FIXED_POINT_H_
