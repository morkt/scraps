// -*- C++ -*-
//! \file        sysmemmap.cc
//! \date        Mon Dec 11 23:33:54 2006
//! \brief       memory mapped objects non-inlined methods.
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

#include "sysmemmap.h"
#include "sysfs.h"

namespace sys { namespace mapping {

const detail::info detail::map_impl_common::sys_info;

detail::info::info ()
{
#ifdef _WIN32
    SYSTEM_INFO system_info;
    ::GetSystemInfo (&system_info);
    page_size = system_info.dwAllocationGranularity;
#else
    page_size = sysconf (_SC_PAGESIZE);
#endif
}

#ifdef _WIN32

void map_base::
open (sys::raw_handle file, mode_t mode, off_type file_size)
{
    if (!file_size)
    {
	file_size = file::get_size (file);
	if (file_size == static_cast<off_type> (file::invalid_size))
            SYS_THROW_SYSTEM_ERROR();
    }
    DWORD protect, map_access;
    switch (mode)
    {
    default:
    case read:	protect = PAGE_READONLY;  map_access = FILE_MAP_READ; break;
    case write:	protect = PAGE_READWRITE; map_access = FILE_MAP_WRITE; break;
    case copy:	protect = PAGE_WRITECOPY; map_access = FILE_MAP_COPY; break;
    }
    ULARGE_INTEGER sz;
    sz.QuadPart = file_size;
    sys::handle backend (::CreateFileMapping (file, NULL, protect, sz.HighPart, sz.LowPart, NULL));
    if (!backend)
        SYS_THROW_SYSTEM_ERROR();

    impl.reset (new detail::map_impl (backend, file_size, map_access));
}

#else

void map_base::
open (sys::raw_handle file, mode_t mode, off_type file_size)
{
    if (!file_size)
    {
	file_size = file::get_size (file);
	if (file::invalid_size == file_size)
	    SYS_THROW_SYSTEM_ERROR();
    }
    sys::handle backend (::dup (file));
    if (!backend)
        SYS_THROW_SYSTEM_ERROR();

    impl.reset (new detail::map_impl (backend, file_size, mode));
}

#endif /* _WIN32 */

} } // namespace sys::mapping
