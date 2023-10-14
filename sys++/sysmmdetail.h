// -*- C++ -*-
//! \file       sysmmdetail.h
//! \date       Mon Dec 11 23:24:38 2006
//! \brief      sys::mapping implementation details.
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

#ifndef SYSMMDETAIL_H

#include "sysdef.h"
#include "syshandle.h"
#include "syserror.h"
#include "refcount_ptr.h"

#ifdef _WIN32

#include <windows.h>

#else

#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>

#endif

namespace sys { namespace mapping {

enum mode_t
{
    read,	// read access
    write,	// read/write access (shared access)
    copy,	// copy-on-write access (private access)
};
   
namespace detail {

struct SYSPP_DLLIMPORT info
{
    size_t	page_size;
    info ();
};

// detail::nearest_int<T>
// yields to integer type enough to hold object of type T

template <bool ulong_is_enough> struct nearest_int_impl {
    typedef boost::uint32_t type;
};

template <> struct nearest_int_impl<false> {
    typedef boost::uint64_t type;
};

template <typename T> struct nearest_int {
    typedef typename nearest_int_impl<sizeof(T) <= sizeof(boost::uint32_t)>::type type;
};

class SYSPP_DLLIMPORT map_impl_common : public refcount_base
{
public:
    // integer type wide enough to hold a pointer
    typedef nearest_int<void*>::type	int_ptr_type;

    // page_align (PTR)
    //
    // Returns: PTR aligned to nearest preceding page boundary.
    static void* page_align (void* area)
	{
	    int_ptr_type ptr = int_ptr_cast (area);
	    ptr &= ~page_mask();
	    return reinterpret_cast<void*> (ptr);
	}

    // size_align (PTR, SIZE)
    //
    // Returns: SIZE extended by offset from PTR to nearest page boundary.
    static size_t size_align (void* area, size_t size)
	{
	    int_ptr_type ptr = int_ptr_cast (area);
	    return size + (ptr & page_mask());
	}

    // XXX page size is assumed to be the power of 2
    static size_t page_size () { return sys_info.page_size; }
    static size_t page_mask () { return sys_info.page_size - 1; }

    static int_ptr_type int_ptr_cast (void* ptr)
	{ return reinterpret_cast<int_ptr_type> (ptr); }

private:
    static const info	sys_info;
};

#ifdef _WIN32

class SYSPP_DLLIMPORT map_impl : public map_impl_common
{
public:
    typedef DWORDLONG	off_type;
    typedef size_t	size_type;

    // note that handle object is passed by a reference
    // so the map_impl class could take ownership over it
    //
    map_impl (sys::handle& handle, off_type size, DWORD mode)
       	: backend (handle), backend_size (size), access (mode) { }

    void* map (off_type offset = 0, size_type size = 0)
	{
	    off_type page_offset (0);
	    if (offset)
	    {
		page_offset = offset & page_mask();
		offset &= ~static_cast<off_type> (page_mask());
		if (size) size += static_cast<size_type> (page_offset);
	    }
	    LARGE_INTEGER pos;
	    pos.QuadPart = offset;
	    char* ptr = (char*) ::MapViewOfFile (backend, access, pos.HighPart, pos.LowPart, size);
	    return ptr? ptr + page_offset: ptr;
	}

    bool unmap (void* area, size_type)
	{ return ::UnmapViewOfFile (page_align (area)); }

    bool sync (void* area, size_type size)
	{ return ::FlushViewOfFile (page_align (area), size_align (area, size)); }

    off_type get_size () const { return backend_size; }

    bool writeable () const
       	{ return access & (FILE_MAP_WRITE|FILE_MAP_COPY); }

private:

    sys::handle		backend;
    const off_type	backend_size;
    const DWORD		access;
};

#else

class map_impl : public map_impl_common
{
public:
    typedef off_t	off_type;
    typedef size_t	size_type;

    map_impl (sys::handle& handle, off_type size, mode_t mode)
       	: backend (handle), backend_size (size)
	, protect (mode == read? PROT_READ: PROT_READ|PROT_WRITE)
	, map_flags (mode == copy? MAP_PRIVATE: MAP_SHARED)
	{ }

    void* map (off_type offset = 0, size_type size = 0)
	{
	    off_type page_offset (0);
	    if (offset)
	    {
		page_offset = offset & page_mask();
		offset &= ~static_cast<off_type> (page_mask());
		if (size) size += page_offset;
	    }
	    if (size == 0)
	       	size = backend_size - offset;
	    void* ptr = ::mmap (NULL, size, protect, map_flags, backend, offset);
	    if (ptr != MAP_FAILED)
	    {
		assert (0 == (int_ptr_cast (ptr) & page_mask()));
		return static_cast<char*>(ptr) + page_offset;
	    }
	    else
		return 0;
	}

    bool unmap (void* area, size_type size)
	{ return ::munmap (page_align (area), size_align (area, size)) != -1; }

    bool sync (void* area, off_type size)
	{
	    size = size? size_align (area, size): page_size();
	    return ::msync (page_align (area), size, MS_SYNC) != -1;
       	}

    off_type get_size () const { return backend_size; }

    bool writeable () const { return protect & PROT_WRITE; }

private:

    sys::file_handle	backend;
    const off_type	backend_size;
    const int		protect;
    const int		map_flags;
};

#endif /* _WIN32 */

} } } // namespace sys::mapping::detail

#endif /* SYSMMDETAIL_H */
