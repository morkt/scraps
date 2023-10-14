// -*- C++ -*-
//! \file       syserror.cc
//! \date       Thu Oct 14 05:21:28 2010
//! \brief      system exceptions implementation.
//
// Copyright (C) 2010 by poddav
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

#include "syserror.h"
#ifndef _WIN32
#include <cstring>	// for std::strerror
#else
#include "winmem.hpp"	// for sys::mem::local
#endif

namespace sys {

#ifdef _WIN32

void error_info::
set_system_message ()
{
    if (m_error_code != NO_ERROR)
    {
	// XXX FLAW
	// In non-unicode envirnoment (UNICODE not defined) FormatMessage returns
	// string in user locale-dependent encoding.  Later calls to sys::mbstowcs
	// incorrectly threat this string as UTF-8.
	TCHAR *msg_buf;
	if (::FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			     NULL, m_error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			     (LPTSTR) &msg_buf, 0, NULL))
	{
	    sys::mem::local sentry (msg_buf);
	    size_t len = std::char_traits<TCHAR>::length (msg_buf);
	    std::locale loc;
	    while (len && std::isspace (msg_buf[len - 1], loc))
		--len;
	    m_system_message.assign (msg_buf, len);
	}
	else
	    m_system_message.assign (string ("Unknown system error"));
    }
}

#else // _WIN32

void error_info::
set_system_message ()
{
    if (m_error_code)
    {
	const char* msg = 0;
#if HAS_STRERROR_R
	local_buffer<char> msg_buf;
	if (-1 != strerror_r (m_error_code, msg_buf.get(), msg_buf.size()))
	    msg = msg_buf.get();
#else
	msg = std::strerror (m_error_code);
#endif
	if (msg && *msg)
	    m_system_message.assign (msg, std::strlen (msg));
	else
	    m_system_message.assign (string ("Unknown system error"));
    }
}

#endif // _WIN32

} // namespace sys
