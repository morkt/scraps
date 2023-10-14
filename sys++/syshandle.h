// -*- C++ -*-
//! \file        syshandle.h
//! \date        Tue Nov 28 07:28:39 2006
//! \brief       generic handle class.
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

#ifndef SYSHANDLE_H
#define SYSHANDLE_H

#include "sysdef.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace sys {

//! \class generic_handle
//! \brief exception-safe wrapper for handle objects

enum {
    win_invalid_handle		= 0,
    win_invalid_file		= -1,
    posix_invalid_handle	= -1,
};

#ifdef _WIN32
typedef HANDLE	raw_handle;
#else
typedef int	raw_handle;
#endif

template <typename Type>
raw_handle handle_cast (Type h)
#ifdef _WIN32
    { return reinterpret_cast<sys::raw_handle> (static_cast<LONG_PTR> (h)); }
#else
    { return static_cast<sys::raw_handle> (h); }
#endif

namespace detail {
    struct base_handle
    {
	static bool close_handle (raw_handle h)
	{
#ifdef _WIN32
    	    return ::CloseHandle (h);
#else
	    return ::close (h) != -1;
#endif
	}
    };
} // namespace detail

template <long invalid_handle_value, class BaseHandle = detail::base_handle>
class SYSPP_DLLIMPORT generic_handle : private BaseHandle
{
public:
    typedef raw_handle	handle_type;

    generic_handle (handle_type h = invalid_handle())
       	: handle (h)
       	{ }

    generic_handle (generic_handle& other)
	: handle (other.release())
	{ }

    ~generic_handle () { close(); }

    generic_handle& operator= (handle_type h)
	{ reset (h); return *this; }

    generic_handle& operator= (generic_handle& other)
	{
	    if (&other != this) reset (other.release());
	    return *this;
	}

    template <long ihv>
    generic_handle& operator= (generic_handle<ihv>& other)
	{
	    if (&other != this)
	    {
		if (other.valid())
		    reset (other.release());
		else
		    close();
	    }
	    return *this;
	}

    bool close ()
	{
	    bool rc = false;
	    if (handle != invalid_handle())
	    {
		rc = this->close_handle (handle);
		handle = invalid_handle();
	    }
	    return rc;
	}

    void reset (handle_type new_h = invalid_handle())
	{
	    if (handle != new_h)
	    {
		close();
		handle = new_h;
	    }
	}

    bool valid () const { return valid (handle); }

    bool operator! () const { return !valid(); }

    // implicit conversion operator
    //
    operator handle_type () const { return handle; }

    operator bool () const { return valid(); }

    handle_type get () const { return handle; }

    handle_type release ()
	{
	    handle_type h = handle;
	    handle = invalid_handle();
	    return h;
	}

    SYSPP_static_constexpr handle_type invalid_handle ()
       	{
#ifdef _WIN32
	    return reinterpret_cast<handle_type> (static_cast<LONG_PTR> (invalid_handle_value));
#else
	    return static_cast<handle_type> (invalid_handle_value);
#endif
       	}

    static bool valid (raw_handle h) { return h != invalid_handle(); }

private:
    handle_type		handle;
};

#ifdef _WIN32
typedef generic_handle<win_invalid_handle>	handle;
typedef generic_handle<win_invalid_file>	file_handle;
#else
typedef generic_handle<posix_invalid_handle>	handle;
typedef generic_handle<posix_invalid_handle>	file_handle;
#endif

}

#endif /* SYSHANDLE_H */
