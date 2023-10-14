// -*- C++ -*-
//! \file       bindata.h
//! \date       Tue Oct 10  2006
//! \brief      byte-swapping functions for endianness handling
//
// $Id: bindata.h,v 1.4.6.1 2006/11/17 14:15:25 rnd Exp $
//
// Copyright (C) 2006 by poddav
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//

#ifndef SYS_BINDATA_H
#define SYS_BINDATA_H

#include "sysdef.h"
#include <climits>		// for ULONG_MAX
#include <boost/cstdint.hpp>	// for intXX_t types
#include <boost/static_assert.hpp>
#if HAVE_SYS_PARAM_H
#include <sys/param.h>		// for BYTE_ORDER macro
#endif
#if SYSPP_MSC
#include <intrin.h>		// MS Visual C intinsic functions
#endif
#include <algorithm>		// for std::iter_swap

#if defined(__BYTE_ORDER__) && defined (__ORDER_LITTLE_ENDIAN__)
#   if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#       define SYSPP_BIGENDIAN 1
#   endif
#elif defined(BYTE_ORDER) && defined(LITTLE_ENDIAN)
#   if BYTE_ORDER != LITTLE_ENDIAN
#	define SYSPP_BIGENDIAN 1
#   endif
#endif

namespace bin {

using boost::int8_t;
using boost::int16_t;
using boost::int32_t;
using boost::int64_t;
using boost::uint8_t;
using boost::uint16_t;
using boost::uint32_t;
using boost::uint64_t;

inline SYSPP_constexpr bool is_big_endian ()
{
#ifdef SYSPP_BIGENDIAN
    return true;
#else
    return false;
#endif
}

inline SYSPP_constexpr bool is_little_endian ()
    { return !is_big_endian(); }

/// swap bytes in 16-bit word

inline uint16_t
swap_word (uint16_t w)
{
#if SYSPP_GNUC > 40700 && SYSPP_GNUC < 40702
    return __builtin_bswap16 (w);
#elif SYSPP_MSC
    return _byteswap_ushort (w);
#elif SYSPP_GNUC && defined(__i386__)
    __asm__ ("xchg	%b0, %h0\n\t"
	     : "=q" (w)
	     :  "0" (w));
    return (w);
#else
    return w >>8 | (w &0xff) <<8;
#endif
}

inline int16_t
swap_word (int16_t w)
{
    return static_cast<int16_t> (swap_word (static_cast<uint16_t> (w)));
}

/// swap bytes in 32-bit word

inline uint32_t
swap_dword (uint32_t dw)
{
#if SYSPP_GNUC >= 40300
    return __builtin_bswap32 (dw);
#elif SYSPP_MSC
    return _byteswap_ulong (dw);
#elif SYSPP_GNUC && defined(__i386__)
    __asm__ ("bswap	%0" : "+r" (dw));
    return (dw);
#else
    return (dw &0xff) <<24 | (dw &0xff00) <<8 | (dw >>8) &0xff00 | (dw >>24) &0xff;
#endif
}

inline int32_t
swap_dword (int32_t w)
{
    return static_cast<int32_t> (swap_dword (static_cast<uint32_t> (w)));
}

/// swap bytes in 64-bit long word

inline uint64_t
swap_qword (uint64_t qw)
{
#if SYSPP_GNUC >= 40300
    return __builtin_bswap64 (qw);
#elif SYSPP_MSC
    return _byteswap_uint64 (qw);
#else
    uint8_t* pqw = reinterpret_cast<uint8_t*> (&qw);
    std::iter_swap (pqw,     pqw + 7);
    std::iter_swap (pqw + 1, pqw + 6);
    std::iter_swap (pqw + 2, pqw + 5);
    std::iter_swap (pqw + 3, pqw + 4);
    return (qw);
#endif
}

inline int64_t
swap_qword (int64_t w)
{
    return static_cast<int64_t> (swap_qword (static_cast<uint64_t> (w)));
}

/// convert 16-bit word from little endian to architecture-specific format.

inline int16_t
little_word (int16_t w)
{
#if SYSPP_BIGENDIAN
    return swap_word (w);
#else
    return (w);
#endif
}

/// convert 32-bit word from little endian to architecture-specific format.

inline int32_t
little_dword (int32_t dw)
{
#if SYSPP_BIGENDIAN
    return swap_dword (dw);
#else
    return (dw);
#endif
}

/// convert 64-bit word from little endian to architecture-specific format.

inline int64_t
little_qword (int64_t qw)
{
#if SYSPP_BIGENDIAN
    return swap_qword (qw);
#else
    return (qw);
#endif
}

/// convert 32-bit float from little endian to architecture-specific format.

inline float
little_float (float g)
{
#if SYSPP_BIGENDIAN
    // assume float is 32-bit wide
    BOOST_STATIC_ASSERT(sizeof(float) == sizeof(int32_t));
    union {
	float		f;
	uint32_t	dw;
    };
    f = g;
    swap_dword (dw);
    return (f);
#else
    return (g);
#endif
}

/// convert 64-bit float from little endian to architecture-specific format.

inline double
little_double (double g)
{
#if SYSPP_BIGENDIAN
    BOOST_STATIC_ASSERT(sizeof(double) == sizeof(int64_t));
    union {
	double	d;
	int64_t	qw;
    };
    d = g;
    swap_qword (qw);
    return (d);
#else
    return (g);
#endif
}

/// convert 16-bit float from big endian to architecture-specific format.

inline int16_t
big_word (int16_t w)
{
#if SYSPP_BIGENDIAN
    return (w);
#else
    return swap_word (w);
#endif
}

/// convert 32-bit float from big endian to architecture-specific format.

inline int32_t
big_dword (int32_t dw)
{
#if SYSPP_BIGENDIAN
    return (dw);
#else
    return swap_dword (dw);
#endif
}

/// convert 64-bit float from big endian to architecture-specific format.

inline int64_t
big_qword (int64_t qw)
{
#if SYSPP_BIGENDIAN
    return (qw);
#else
    return swap_qword (qw);
#endif
}

// ---------------------------------------------------------------------------
// conversion functors

namespace detail
{
    template <size_t N>
    struct endian_swap_helper
    {
        template <typename IntT>
        IntT operator() (IntT x) const;
    };

    template<> template <typename IntT>
    inline IntT endian_swap_helper<1>:: operator() (IntT x) const
    { return x; }

    template<> template <typename IntT>
    inline IntT endian_swap_helper<2>:: operator() (IntT x) const
    { return swap_word (static_cast<uint16_t> (x)); }

    template<> template <typename IntT>
    inline IntT endian_swap_helper<4>:: operator() (IntT x) const
    { return swap_dword (static_cast<uint32_t> (x)); }

    template<> template <typename IntT>
    inline IntT endian_swap_helper<8>:: operator() (IntT x) const
    { return swap_qword (static_cast<uint64_t> (x)); }
} // namespace detail

template <typename IntT>
struct litendian_convert
{
    typedef IntT argument_type;
    typedef IntT result_type;
    result_type operator() (argument_type x) const;
};

template <typename IntT>
struct bigendian_convert
{
    typedef IntT argument_type;
    typedef IntT result_type;
    result_type operator() (argument_type x) const;
};

template <typename IntT>
struct endian_swap : public detail::endian_swap_helper<sizeof (IntT)>
{
    typedef IntT argument_type;
    typedef IntT result_type;
//    result_type operator() (argument_type x) const;
};

template<>
inline int8_t litendian_convert<int8_t>::operator() (int8_t x) const
{ return (x); }

template<>
inline int16_t litendian_convert<int16_t>::operator() (int16_t x) const
{ return little_word (x); }

template<>
inline int32_t litendian_convert<int32_t>::operator() (int32_t x) const
{ return little_dword (x); }

template<>
inline int64_t litendian_convert<int64_t>::operator() (int64_t x) const
{ return little_qword (x); }

template<>
inline uint8_t litendian_convert<uint8_t>::operator() (uint8_t x) const
{ return (x); }

template<>
inline uint16_t litendian_convert<uint16_t>::operator() (uint16_t x) const
{ return little_word (x); }

template<>
inline uint32_t litendian_convert<uint32_t>::operator() (uint32_t x) const
{ return little_dword (x); }

template<>
inline uint64_t litendian_convert<uint64_t>::operator() (uint64_t x) const
{ return little_qword (x); }

template<>
inline int8_t bigendian_convert<int8_t>::operator() (int8_t x) const
{ return (x); }

template<>
inline int16_t bigendian_convert<int16_t>::operator() (int16_t x) const
{ return big_word (x); }

template<>
inline int32_t bigendian_convert<int32_t>::operator() (int32_t x) const
{ return big_dword (x); }

template<>
inline int64_t bigendian_convert<int64_t>::operator() (int64_t x) const
{ return big_qword (x); }

template<>
inline uint8_t bigendian_convert<uint8_t>::operator() (uint8_t x) const
{ return (x); }

template<>
inline uint16_t bigendian_convert<uint16_t>::operator() (uint16_t x) const
{ return big_word (x); }

template<>
inline uint32_t bigendian_convert<uint32_t>::operator() (uint32_t x) const
{ return big_dword (x); }

template<>
inline uint64_t bigendian_convert<uint64_t>::operator() (uint64_t x) const
{ return big_qword (x); }

#if 0
template<>
inline int8_t endian_swap<int8_t>::operator() (int8_t x) const
{ return (x); }

template<>
inline int16_t endian_swap<int16_t>::operator() (int16_t x) const
{ return swap_word (x); }

template<>
inline int32_t endian_swap<int32_t>::operator() (int32_t x) const
{ return swap_dword (x); }

template<>
inline int64_t endian_swap<int64_t>::operator() (int64_t x) const
{ return swap_qword (x); }

template<>
inline uint8_t endian_swap<uint8_t>::operator() (uint8_t x) const
{ return (x); }

template<>
inline uint16_t endian_swap<uint16_t>::operator() (uint16_t x) const
{ return swap_word (x); }

template<>
inline uint32_t endian_swap<uint32_t>::operator() (uint32_t x) const
{ return swap_dword (x); }

template<>
inline uint64_t endian_swap<uint64_t>::operator() (uint64_t x) const
{ return swap_qword (x); }
#endif

// ---------------------------------------------------------------------------
// bit scan functions

/// bit_scan_msb (MASK)
/// \return the number of leading 0-bits in MASK, starting at the most significant bit
///	    position, or ULONG_MAX if there's no 1-bits in MASK.
inline unsigned long bit_scan_msb (uint32_t mask)
{
#if SYSPP_MSC
    unsigned long index;
    return _BitScanReverse (&index, mask)? (31-index): ULONG_MAX;
#elif SYSPP_GNUC >= 40300
    return mask? __builtin_clz (mask): ULONG_MAX;
#else
    if (!mask)
       	return ULONG_MAX;
    unsigned long index = 0;
    while (mask >>= 1)
	++index;
    return 31-index;
#endif
}

inline unsigned long bit_scan_msb (uint64_t mask)
{
#if SYSPP_MSC && (defined(_M_IA64) || defined(_M_AMD64))
    unsigned long index;
    return _BitScanReverse64 (&index, mask)? (63-index): ULONG_MAX;
#elif SYSPP_GNUC >= 40300
    return mask? __builtin_clzll (mask): ULONG_MAX;
#else
    unsigned long index = ULONG_MAX;
    if (mask >> 32)
	index = bit_scan_msb (static_cast<uint32_t> (mask >> 32));
    if (index == ULONG_MAX)
    {
	index = bit_scan_msb (static_cast<uint32_t> (mask & 0xffffffff));
	if (index != ULONG_MAX)
	    index += 32;
    }
    return index;
#endif
}

inline unsigned long bit_scan_msb (uint8_t mask)
{
    unsigned long index = bit_scan_msb (static_cast<uint32_t> (mask));;
    return index == ULONG_MAX? ULONG_MAX: index - 24;
}

inline unsigned long bit_scan_msb (uint16_t mask)
{
    unsigned long index = bit_scan_msb (static_cast<uint32_t> (mask));;
    return index == ULONG_MAX? ULONG_MAX: index - 16;
}

} // namespace bin

#endif /* SYS_BINDATA_H */
