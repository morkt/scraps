// -*- C++ -*-
//! \file       refcount_ptr.h
//! \date       Thu Dec 07 09:31:56 2006
//! \brief      reference counting pointer implementation.
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

#ifndef SYS_REFCOUNT_PTR_H
#define SYS_REFCOUNT_PTR_H

#include "sysdef.h"
#include "sysatomic.h"
#include <algorithm>	// for std::swap
#ifdef SYSPP_REFCOUNT_PTR_USE_AUTO_PTR
#include <memory>	// for std::auto_ptr
#endif

namespace sys {

class SYSPP_DLLIMPORT refcount_base
{
    mutable volatile sys::atomic_type ref_count;

    template<typename T> friend class refcount_ptr;

public:
    refcount_base() : ref_count(0) { }
};

//  refcount_ptr  ------------------------------------------------------------
//
//  Requirements: For ptr of type T*, ptr->ref_count must be initialized to 0

template<typename T>
class SYSPP_DLLIMPORT refcount_ptr
{
    T*		ptr;

    template<typename U> friend class refcount_ptr;

public:
    typedef T element_type;

    // XXX enabled implicit conversion
    //
    /*explicit*/ refcount_ptr (T* p = 0) : ptr (p)
       	{ if (ptr) sys::atomic_add (ptr->ref_count, 1); }

    refcount_ptr (const refcount_ptr& other) : ptr (other.ptr)
       	{ if (ptr) sys::atomic_add (ptr->ref_count, 1); }

    refcount_ptr (refcount_ptr&& other) : ptr (other.ptr)
        { other.ptr = 0; }

    ~refcount_ptr () { dispose(); }

    template<typename U>
    refcount_ptr (const refcount_ptr<U>& r) : ptr (r.ptr)
       	{ if (ptr) sys::atomic_add (ptr->ref_count, 1); }

    template<typename U>
    refcount_ptr (refcount_ptr<U>&& other) : ptr (other.ptr)
        { other.ptr = 0; }

#if SYSPP_REFCOUNT_PTR_USE_AUTO_PTR
    template<typename U>
    refcount_ptr (std::auto_ptr<U>& r)
       	{
	    ptr = r.release();
	    if (ptr) sys::atomic_swap (ptr->ref_count, 1);
	}

    template<typename U>
    refcount_ptr& operator= (std::auto_ptr<U>& r)
       	{
	    dispose();
	    ptr = r.release();
	    if (ptr) sys::atomic_swap (ptr->ref_count, 1);
	    return *this;
	}
#endif // SYSPP_REFCOUNT_PTR_USE_AUTO_PTR

    refcount_ptr& operator= (const refcount_ptr& r)
	{
	    share (r.ptr);
	    return *this;
	}

    refcount_ptr& operator= (refcount_ptr&& r)
        {
	    if (ptr != r.ptr)
	    {
		dispose();
		ptr = r.ptr;
		r.ptr = 0;
	    }
            return *this;
        }

    template<typename U>
    refcount_ptr& operator= (const refcount_ptr<U>& r)
	{
	    share (r.ptr);
	    return *this;
	}

    template<typename U>
    refcount_ptr& operator= (refcount_ptr<U>&& r)
        {
	    if (ptr != r.ptr)
	    {
		dispose();
		ptr = r.ptr;
		r.ptr = 0;
	    }
            return *this;
        }

    bool operator< (const refcount_ptr& other) const { return ptr < other.ptr; }

    void reset (T* p = 0) { share (p); }

    T& operator* () const   { return *ptr; }
    T* operator-> () const  { return ptr; }
    T* get () const         { return ptr; }

    // implicit conversion to "bool"

    typedef T* (refcount_ptr::*bool_type)() const;

    operator bool_type() const { return ptr == 0? 0: &refcount_ptr::get; }

    bool operator! () const { return ptr == 0; }

    long use_count () const { return ptr ? ptr->ref_count : 0; }
    bool unique () const { return ptr && ptr->ref_count == 1; }

    void swap (refcount_ptr<T>& other) { std::swap (ptr, other.ptr); }

private:
    void dispose ()
	{
	    if (ptr && sys::atomic_add (ptr->ref_count, -1) == 1)
		delete ptr;
	}

    void share (T* other)
	{
	    if (ptr != other)
	    {
		dispose();
		ptr = other;
		if (ptr) sys::atomic_add (ptr->ref_count, 1);
	    }
	}
};

template<typename T1, typename T2>
inline bool operator== (const refcount_ptr<T1>& lhs, const refcount_ptr<T2>& rhs)
{
    return lhs.get() == rhs.get();
}

template<typename T1, typename T2>
inline bool operator!= (const refcount_ptr<T1>& lhs, const refcount_ptr<T2>& rhs)
{
    return lhs.get() != rhs.get();
}

// get_pointer() enables boost::mem_fn to recognize refcount_ptr

template <typename T>
inline T* get_pointer (const refcount_ptr<T>& p)
{
    return p.get();
}

} // namespace sys

#endif /* SYS_REFCOUNT_PTR_H */
