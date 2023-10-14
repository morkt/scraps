// -*- C++ -*-
//! \file       sysatomic.h
//! \date       Fri Mar 23 11:31:06 2007
//! \brief      atomic exchange/add inline functions.
//
// Copyright (C) 2007 by poddav
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

#ifndef SYSATOMIC_HPP
#define SYSATOMIC_HPP

#include "bindata.h"
#if SYSPP_MSC
#include <intrin.h>
#endif

namespace sys {

#if SYSPP_MSC || SYSPP_WIN32 && !SYSPP_GNUC && !SYSPP_CLANG
typedef long atomic_type;
#else
typedef bin::int32_t atomic_type;
#endif

// perform an atomic addition and return initial value.

atomic_type atomic_add (volatile atomic_type& value, atomic_type increment);

// atomically replace value with replacement, return previous value

atomic_type atomic_swap (volatile atomic_type& value, atomic_type replacement);

// atomically read value

atomic_type atomic_get (volatile atomic_type& value);

// ---------------------------------------------------------------------------
// implementation

#if SYSPP_CLANG || SYSPP_GNUC && __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4

inline atomic_type atomic_add (volatile atomic_type& value, atomic_type increment)
{
    return __sync_fetch_and_add (&value, increment);
}

inline atomic_type atomic_swap (volatile atomic_type& value, atomic_type replacement)
{
    return __sync_lock_test_and_set (&value, replacement);
}

inline atomic_type atomic_get (volatile atomic_type& value)
{
    return __sync_val_compare_and_swap (&value, 0, 0);
}

#elif SYSPP_GNUC && defined(__i386__)

inline atomic_type atomic_add (volatile atomic_type& value, atomic_type increment)
{
    atomic_type result;
    __asm__ __volatile__ (
	"lock\n\t"
	"xadd	%1, %0"
	: "+m" (value), "=r" (result)
	: "1"  (increment)
    );
    return result;
}

inline atomic_type atomic_swap (volatile atomic_type& value, atomic_type replacement)
{
    atomic_type result;
    __asm__ __volatile__ (
	"lock\n\t"
	"xchg	%1, %0"
	: "+m" (value), "=r" (result)
	: "1"  (replacement)
    );
    return result;
}

inline atomic_type atomic_get (volatile atomic_type& value)
{
    atomic_type result;
    __asm__ __volatile__ (
        "xor	%%eax, %%eax\n\t"
	"lock\n\t"
	"cmpxchg	%1, %0"
	: "+m" (value), "=a" (result)
    );
    return result;
}

#elif SYSPP_MSC

inline atomic_type atomic_add (volatile atomic_type& value, atomic_type increment)
{
    return _InterlockedExchangeAdd (&value, increment);
}

inline atomic_type atomic_swap (volatile atomic_type& value, atomic_type replacement)
{
    return _InterlockedExchange (&value, replacement);
}

inline atomic_type atomic_get (volatile atomic_type& value)
{
    return _InterlockedCompareExchange (&value, 0, 0);
}

#elif defined(_WIN32)

extern "C" atomic_type __stdcall InterlockedExchangeAdd (volatile atomic_type*, atomic_type);
extern "C" atomic_type __stdcall InterlockedExchange (volatile atomic_type*, atomic_type);
extern "C" atomic_type __stdcall InterlockedCompareExchange (volatile atomic_type*, atomic_type, atomic_type);

inline atomic_type atomic_add (volatile atomic_type& value, atomic_type increment)
{
    return InterlockedExchangeAdd (&value, increment);
}

inline atomic_type atomic_swap (volatile atomic_type& value, atomic_type replacement)
{
    return InterlockedExchange (&value, replacement);
}

inline atomic_type atomic_get (volatile atomic_type& value)
{
    return InterlockedCompareExchange (&value, 0, 0);
}

#else

#warning Unknown environment -- atomic functions are not thread-safe

inline atomic_type atomic_add (volatile atomic_type& value, atomic_type increment)
{
    atomic_type previous = value;
    value += increment;
    return previous;
}

inline atomic_type atomic_swap (volatile atomic_type& value, atomic_type replacement)
{
    atomic_type previous = value;
    value = replacement;
    return previous;
}

inline atomic_type atomic_get (volatile atomic_type& value)
{
    return value;
}

#endif

} // namespace sys

#endif /* SYSATOMIC_HPP */
