// -*- C++ -*-
//! \file       sysfs.h
//! \date       Fri May 11 15:35:59 2007
//! \brief      filesystem manipulation functions.
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

#ifndef SYSPP_SYSFS_H
#define SYSPP_SYSFS_H

#include "syserror.h"
#include "sysstring.h"
#include "syshandle.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#endif

#include <boost/operators.hpp>

namespace sys {

// --- filesystem manipulation -----------------------------------------------

/// sys::chdir (DIRECTORY)

inline bool chdir (const char* dir)
{
#ifdef _WIN32
    return ::SetCurrentDirectoryA (dir);
#else
    return ::chdir (dir) != -1;
#endif
}

#ifdef _WIN32
inline bool chdir (const wchar_t* dir)
{
    return ::SetCurrentDirectoryW (dir);
}
#else
inline bool chdir (const wstring& dir)
{
    string cname;
    return wcstombs (dir, cname) && sys::chdir (cname.c_str());
}
#endif

template <typename Ch, typename Tr, typename Al>
inline bool chdir (const basic_string<Ch,Tr,Al>& dir)
{
    return sys::chdir (dir.c_str());
}

/// sys::mkdir (DIRECTORY)

inline bool mkdir (const char* dir)
{
#ifdef _WIN32
    return ::CreateDirectoryA (dir, 0);
#else
    return ::mkdir (dir, 0777) != -1;
#endif
}

#ifdef _WIN32
inline bool mkdir (const wchar_t* dir)
{
    return ::CreateDirectoryW (dir, 0);
}
#else
inline bool mkdir (const wstring& dir)
{
    string cname;
    return wcstombs (dir, cname) && sys::mkdir (cname.c_str());
}
#endif

template <typename Ch, typename Tr, typename Al>
inline bool mkdir (const basic_string<Ch,Tr,Al>& dir)
{
    return sys::mkdir (dir.c_str());
}

/// sys::rmdir (DIRECTORY)

inline bool rmdir (const char* dir)
{
#ifdef _WIN32
    return ::RemoveDirectoryA (dir);
#else
    return ::rmdir (dir) != -1;
#endif
}

#ifdef _WIN32
inline bool rmdir (const wchar_t* dir)
{
    return ::RemoveDirectoryW (dir);
}
#else
inline bool rmdir (const wstring& dir)
{
    string cname;
    return wcstombs (dir, cname) && sys::rmdir (cname.c_str());
}
#endif

template <typename Ch, typename Tr, typename Al>
inline bool rmdir (const basic_string<Ch,Tr,Al>& dir)
{
    return rmdir (dir.c_str());
}

/// sys::getcwd (BUFFER, BUFFER_SIZE)

inline bool getcwd (char* buf, size_t buf_size)
{
#ifdef _WIN32
    size_t ret = ::GetCurrentDirectoryA (buf_size, buf);
    return ret && ret <= buf_size;
#else
    return ::getcwd (buf, buf_size);
#endif
}

#ifndef _WIN32
namespace detail {
    bool wgetcwd (WChar* buf, size_t buf_size);
}
#endif

inline bool getcwd (WChar* buf, size_t buf_size)
{
#ifdef _WIN32
    size_t ret = ::GetCurrentDirectoryW (buf_size, buf);
    return ret && ret <= buf_size;
#else
    return detail::wgetcwd (buf, buf_size);
#endif
}

// sys::getcwd (DST)
// Effects: puts current working directory into DST.
// Returns: TRUE on success, FALSE otherwise.
//          DST will contain empty string on error.

template <typename char_type>
bool getcwd (basic_string<char_type>& cwd);

// sys::create_path (PATH)
// Effects: create all directories within PATH if they dont already exist.
// Returns: TRUE if path was successfully created or already existed,
//          FALSE otherwise.

template <typename char_type>
bool create_path (const basic_string<char_type>& path);

#ifndef _WIN32
template <>
inline bool create_path (const wstring& path)
{
    string cname;
    return wcstombs (path, cname) && create_path (cname);
}
#endif

// --- file manipulation functions -------------------------------------------

namespace file {

#ifdef _WIN32
typedef LONGLONG	size_type;
#else
typedef off_t		size_type;
#endif

const size_type invalid_size = size_type (-1);

/// sys::file::exists (FILENAME)

inline bool exists (const char* name)
{
#ifdef _WIN32
    return (::GetFileAttributesA (name) != 0xffffffff);
#else
    struct stat sbuf;
    return !(-1 == ::stat (name, &sbuf) && errno == ENOENT);
#endif
}

#ifdef _WIN32
inline bool exists (const wchar_t* name)
{
    return (::GetFileAttributesW (name) != 0xffffffff);
}
#else
inline bool exists (const wstring& name)
{
    string cname;
    return wcstombs (name, cname) && exists (cname.c_str());
}
#endif

template <typename Ch, typename Tr, typename Al>
inline bool exists (const basic_string<Ch,Tr,Al>& name)
{
    return exists (name.c_str());
}

/// sys::file::unlink (FILENAME)

inline bool unlink (const char* name)
{
#ifdef _WIN32
    return ::DeleteFileA (name);
#else
    return ::unlink (name) != -1;
#endif
}

#ifdef _WIN32
inline bool unlink (const wchar_t* name)
{
    return ::DeleteFileW (name);
}
#else
inline bool unlink (const wstring& name)
{
    string cname;
    return wcstombs (name, cname) && unlink (cname.c_str());
}
#endif

template <typename Ch, typename Tr, typename Al>
inline bool unlink (const basic_string<Ch,Tr,Al>& name)
{
    return unlink (name.c_str());
}

/// sys::file::rename (OLDNAME, NEWNAME)

inline bool rename (const char* oldname, const char* newname)
{
#ifdef _WIN32
    return ::MoveFileA (oldname, newname);
#else
    return ::rename (oldname, newname) != -1;
#endif
}

#ifdef _WIN32
inline bool rename (const wchar_t* oldname, const wchar_t* newname)
{
    return ::MoveFileW (oldname, newname);
}
#else
inline bool rename (const wstring& oldname,
		    const wstring& newname)
{
    string oname, nname;
    return wcstombs (oldname, oname) && wcstombs (newname, nname)
    	&& rename (oname.c_str(), nname.c_str());
}
#endif

template <typename Ch, typename Tr, typename Al>
inline bool rename (const basic_string<Ch,Tr,Al>& oldname,
		    const basic_string<Ch,Tr,Al>& newname)
{
    return rename (oldname.c_str(), newname.c_str());
}

// --- file time -------------------------------------------------------------

struct time : boost::less_than_comparable<time
	    , boost::equality_comparable<time> >
{
#ifdef _WIN32
    typedef FILETIME	ftime_type;
#else
    typedef time_t	ftime_type;
#endif
    time (ftime_type t) : m_time (t) { }

    bool operator< (const time& rhs) const { return compare (rhs) < 0; }
    bool operator== (const time& rhs) const { return compare (rhs) == 0; }

    int compare (const time& other) const
	{
#ifdef _WIN32
	    return ::CompareFileTime (&m_time, &other.m_time);
#else
	    return m_time < other.m_time? -1: m_time > other.m_time? 1: 0;
#endif
       	}

private:
    ftime_type		m_time;
};

// sys::file::get_mod_time
// Returns: last modification time of file identified by name.
// Throws: sys::file_error if modification time cannot be accessed.

inline time get_mod_time (const char* name)
{
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (::GetFileAttributesExA (name, GetFileExInfoStandard, &attr))
	return time (attr.ftLastWriteTime);
#else
    struct stat buf;
    if (-1 != ::stat (name, &buf))
	return time (buf.st_mtime);
#endif
    SYS_THROW_FILE_ERROR (name);
}

#ifdef _WIN32
inline time get_mod_time (const wchar_t* name)
{
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!::GetFileAttributesExW (name, GetFileExInfoStandard, &attr))
	SYS_THROW_FILE_ERROR (name);
    return time (attr.ftLastWriteTime);
}
#else
inline time get_mod_time (const wstring& name)
{
    string cname;
    if (!wcstombs (name, cname))
	SYS_THROW_FILE_ERROR (name.c_str());
    return get_mod_time (cname.c_str());
}
#endif

template <typename Ch, typename Tr, typename Al>
inline time get_mod_time (const basic_string<Ch,Tr,Al>& name)
{
    return get_mod_time (name.c_str());
}

// --- file size -------------------------------------------------------------

// sys::file::get_size
// Returns: size of file identified by either handle or name.  In case of error,
// sys::file::invalid_size is returned.

inline size_type get_size (sys::raw_handle handle)
{
#ifdef _WIN32
    LARGE_INTEGER size;
    if (::GetFileSizeEx (handle, &size))
	return size.QuadPart;
#else
    struct stat buf;
    if (-1 != ::fstat (handle, &buf))
	return buf.st_size;
#endif
    return invalid_size;
}

inline size_type get_size (const char* name)
{
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (::GetFileAttributesExA (name, GetFileExInfoStandard, &attr))
    {
	LARGE_INTEGER size;
	size.LowPart = attr.nFileSizeLow;
	size.HighPart = attr.nFileSizeHigh;
	return size.QuadPart;
    }
#else
    struct stat buf;
    if (-1 != ::stat (name, &buf))
	return buf.st_size;
#endif
    return invalid_size;
}

#ifdef _WIN32
inline size_type get_size (const wchar_t* name)
{
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (::GetFileAttributesExW (name, GetFileExInfoStandard, &attr))
    {
	LARGE_INTEGER size;
	size.LowPart = attr.nFileSizeLow;
	size.HighPart = attr.nFileSizeHigh;
	return size.QuadPart;
    }
    return invalid_size;
}
#else
inline size_type get_size (const wstring& name)
{
    string cname;
    if (wcstombs (name, cname))
	return get_size (cname.c_str());
    return invalid_size;
}
#endif

template <typename Ch, typename Tr, typename Al>
inline size_type get_size (const basic_string<Ch,Tr,Al>& name)
{
    return get_size (name.c_str());
}

} // namespace file

// --- template functions implementation--------------------------------------

#ifdef _WIN32

namespace detail
{
    inline size_t get_curdir (size_t sz, char* buffer)
	{ return GetCurrentDirectoryA (sz, buffer); }

    inline size_t get_curdir (size_t sz, wchar_t* buffer)
	{ return GetCurrentDirectoryW (sz, buffer); }
} // namespace detail

template <typename char_type>
bool getcwd (basic_string<char_type>& cwd)
{
    cwd.clear();
    size_t req_size = detail::get_curdir (0, (char_type*)0);
    local_buffer<char_type> buf (req_size);
    req_size = detail::get_curdir (buf.size(), buf.get());
    bool success = req_size && req_size < buf.size();
    if (success)
	cwd.assign (buf.get(), req_size);
    return success;
}

#else

template <>
inline bool getcwd (basic_string<char>& cwd)
{
    cwd.clear();
    local_buffer<char> buf;
    if (getcwd (buf.get(), buf.size()))
    {
	cwd.assign (buf.get());
	return true;
    }
    if (errno != ERANGE)
	return false;

    buf.reserve (1024);
    bool success = getcwd (buf.get(), buf.size());
    if (success)
	cwd.assign (buf.get());
    return success;
}

template<>
inline bool getcwd (wstring& cwd)
{
    string ccwd;
    bool success = getcwd (ccwd) && mbstowcs (ccwd, cwd);
    if (!success)
	cwd.clear();
    return success;
}

#endif

} // namespace sys

#endif /* SYSPP_SYSFS_H */
