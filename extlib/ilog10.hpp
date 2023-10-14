// -*- C++ -*-
//! \file       ilog10.hpp
//! \date       Thu Aug 16 18:37:37 2012
//! \brief      fast implementation of log10 function for integer types.
//

#ifndef EXT_ILOG10_HPP
#define EXT_ILOG10_HPP

#include "bindata.h"

namespace ext {

// ilog10 (x)
// Returns: floor (log10 (fabs (x))) if x != 0, otherwise returns 0.

template <typename Integer>
unsigned ilog10 (Integer x);

namespace detail {

template <size_t bits>
struct pow10_int_traits { };

template <>
struct pow10_int_traits<32>
{
    typedef bin::uint32_t int_type;
    static const unsigned width = 32;
    static int_type pow10 (unsigned n)
    {
        static int_type const powers[] = {
            1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000
        };
        return powers[n];
    }
};

template <>
struct pow10_int_traits<64>
{
    typedef bin::uint64_t int_type;
    static const unsigned width = 64;
    static int_type pow10 (unsigned n)
    {
        static int_type const powers[] = {
            1ULL, 10ULL, 100ULL, 1000ULL, 10000ULL, 100000ULL, 1000000ULL, 10000000ULL,
            100000000ULL, 1000000000ULL, 10000000000ULL, 100000000000ULL, 1000000000000ULL,
            10000000000000ULL, 100000000000000ULL, 1000000000000000ULL, 10000000000000000ULL,
            100000000000000000ULL, 1000000000000000000ULL, 10000000000000000000ULL
        };
        return powers[n];
    }
};

template <> struct pow10_int_traits<8>  : public pow10_int_traits<32> { };
template <> struct pow10_int_traits<16> : public pow10_int_traits<32> { };

template <size_t width>
struct ilog10_impl
{
    typedef pow10_int_traits<width> traits;
    typedef typename traits::int_type argument_type;
    static unsigned do_ilog10 (argument_type x)
    {
        unsigned long n = bin::bit_scan_msb (x);
        if (ULONG_MAX != n)
        {
            n = (traits::width - n) * 1233 / 4096;
            return n - (x < traits::pow10 (n));
        }
        else
            return 0;
    }
};


template <bool is_signed>
struct ilog10_signed
{
    template <typename Signed>
    static unsigned do_abs (Signed x)
    {
        typedef ilog10_impl<std::numeric_limits<Signed>::digits+1> ilog10;
        if (x != std::numeric_limits<Signed>::min())
        {
            if (x < 0) x = -x;
            return ilog10::do_ilog10 (static_cast<typename ilog10::argument_type> (x));
        }
        else
            return std::numeric_limits<Signed>::digits10;
    }
};

template <>
struct ilog10_signed<false>
{
    template <typename Unsigned>
    static unsigned do_abs (Unsigned x)
    {
        typedef ilog10_impl<std::numeric_limits<Unsigned>::digits> ilog10;
        return ilog10::do_ilog10 (static_cast<typename ilog10::argument_type> (x));
    }
};

} // namespace detail

template <typename Integer>
inline unsigned ilog10 (Integer x)
{
    static_assert (std::numeric_limits<Integer>::is_integer, "ilog10(X) requires integer X");
    return detail::ilog10_signed<std::numeric_limits<Integer>::is_signed>::do_abs (x);
}

} // namespace ext

#endif /* EXT_ILOG10_HPP */
