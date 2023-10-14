// -*- C++ -*-
//! \file       syserror.h
//! \date       Sat Oct 09 18:06:14 2010
//! \brief      system error exception classes supporting wide strings.
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

#ifndef SYSERROR_H
#define SYSERROR_H

#include "sysstring.h"
#include <exception>
#include <locale>
#include <memory>
#ifdef SYSPP_NO_CPP0X
#include <boost/shared_ptr.hpp>
#endif
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#endif

namespace sys {

#ifndef SYSPP_NO_CPP0X
using std::shared_ptr;
#else
using boost::shared_ptr;
#endif

// work-around macros for mingw runtime bug

#define SYS_THROW_SYSTEM_ERROR() do {\
    int __err = sys::error_info::get_last_error();\
    throw sys::generic_error (__err); } while (0)

#define SYS_THROW_GENERIC_ERROR(object) do {\
    int __err = sys::error_info::get_last_error();\
    throw sys::generic_error (__err, object); } while (0)

#define SYS_THROW_FILE_ERROR(filename) do {\
    int __err = sys::error_info::get_last_error();\
    throw sys::file_error (__err, filename); } while (0)

class error_info
{
public:
    error_info () : m_error_code (get_last_error())
	{ set_system_message(); }

    explicit error_info (int code) : m_error_code (code)
	{ set_system_message(); }

    template <typename CharT>
    explicit error_info (const CharT* object)
	: m_error_code (get_last_error())
	, m_object (object)
	{ set_system_message(); }

    template <typename Ch, typename Tr, typename Al>
    explicit error_info (const basic_string<Ch,Tr,Al>& object)
	: m_error_code (get_last_error())
	, m_object (object)
	{ set_system_message(); }

    template <typename CharT>
    error_info (const CharT* object, const CharT* message)
	: m_error_code (get_last_error())
	, m_custom_message (message)
	, m_object (object)
	{ set_system_message(); }

    template <typename Ch, typename Tr, typename Al>
    explicit error_info (const basic_string<Ch,Tr,Al>& object,
			 const basic_string<Ch,Tr,Al>& message)
	: m_error_code (get_last_error())
	, m_custom_message (message)
	, m_object (object)
	{ set_system_message(); }

    static int get_last_error();

    template <typename CharT>
    const CharT* what ()
	{
	    if (m_what.empty())
		make_what<CharT>();
	    return m_what.get_string<CharT>().c_str();
	}

    int get_error_code () const { return m_error_code; }
    uni_string& get_object () { return m_object; }
    uni_string& get_system_message () { return m_system_message; }
    uni_string& get_custom_message () { return m_custom_message; }

private:
    SYSPP_DLLIMPORT void set_system_message();

    template <typename CharT>
    void make_what ();

private:
    int			m_error_code;
    uni_string		m_system_message;
    uni_string		m_custom_message;
    uni_string		m_object;
    uni_string		m_what;
};

class error_sentry
{
public:
    error_sentry () : m_errno (error_info::get_last_error()) {}

    // return true if system error changed its state since sentry was created

    operator bool() const { return m_errno != error_info::get_last_error(); }

    bool operator!() const { return m_errno == error_info::get_last_error(); }

    void reset () { m_errno = error_info::get_last_error(); }

private:
    int		m_errno;
};

class generic_error : public std::exception
{
public:
    generic_error () : m_info (new error_info) { }

    explicit generic_error (int errnum) : m_info (new error_info (errnum)) { }

    template <typename CharT>
    generic_error (int errnum, const CharT* object) : m_info (new error_info (errnum))
        { m_info->get_object().assign (object); }

    template <typename CharT>
    explicit generic_error (const CharT* object)
	: m_info (new error_info (object))
       	{ }

    template <typename Ch, typename Tr, typename Al>
    explicit generic_error (const basic_string<Ch,Tr,Al>& object)
       	: m_info (new error_info (object)) { }

    template <typename CharT>
    explicit generic_error (const CharT* object, const CharT* message)
	: m_info (new error_info (object, message)) { }

    template <typename Ch, typename Tr, typename Al>
    explicit generic_error (const basic_string<Ch,Tr,Al>& object,
   			    const basic_string<Ch,Tr,Al>& message)
       	: m_info (new error_info (object, message)) { }

    ~generic_error () noexcept { }

    const char* what () const throw() { return get_description<char>(); }

    int get_error_code () const { return m_info->get_error_code(); }

    template <typename CharT>
    const CharT* get_system_message () const
       	{ return m_info->get_system_message().template get_string<CharT>().c_str(); }

    template <typename CharT>
    const CharT* get_object () const
       	{ return m_info->get_object().template get_string<CharT>().c_str(); }

    template <typename CharT>
    const CharT* get_message () const
       	{ return m_info->get_custom_message().template get_string<CharT>().c_str(); }

    template <typename CharT>
    const CharT* get_description () const
	{ return m_info->what<CharT>(); }

protected:
    shared_ptr<error_info>	m_info;
};

class file_error : public generic_error
{
public:
    template <typename CharT>
    explicit file_error (const CharT* filename) : generic_error (filename) { }

    template <typename CharT>
    file_error (int error, const CharT* filename) : generic_error (error, filename) { }

    template <typename Ch, typename Tr, typename Al>
    explicit file_error (const basic_string<Ch,Tr,Al>& filename)
       	: generic_error (filename) { }

    template <typename CharT>
    file_error (const CharT* filename, const CharT* custom_msg)
	: generic_error (filename, custom_msg) { }

    template <typename Ch, typename Tr, typename Al>
    explicit file_error (const basic_string<Ch,Tr,Al>& filename,
			 const basic_string<Ch,Tr,Al>& custom_msg)
       	: generic_error (filename, custom_msg) { }

    template <typename CharT>
    const CharT* get_filename () const { return get_object<CharT>(); }
};

// --- error_info ------------------------------------------------------------

inline int error_info::
get_last_error()
{
#ifdef _WIN32
    return ::GetLastError();
#else
    return errno;
#endif
}

template <typename CharT>
void error_info::make_what ()
{
    const CharT colon[3] = { ':', ' ', 0 };

    basic_string<CharT> what_str;

    if (!m_object.empty())
	what_str = m_object.get_string<CharT>();
    if (!m_custom_message.empty())
    {
	if (!what_str.empty())
	    what_str += colon;
	what_str += m_custom_message.get_string<CharT>();
    }
    if (!m_system_message.empty())
    {
	if (!what_str.empty())
	    what_str += colon;
	what_str += m_system_message.get_string<CharT>();
    }
    if (what_str.empty())
	m_what.assign ("Unknown error");
    else
	m_what.assign (what_str);
}

} // namespace sys

#if SYSPP_SAMPLE_USAGE
{
    // ...
}
catch (sys::generic_error& X)
{
    MessageBox (0, X.get_description<TCHAR>(), TEXT("Run-time Error"), MB_OK|MB_ICONASTERISK);
}
catch (std::exception& X)
{
    MessageBoxA (0, X.what(), "Run-time Error", MB_OK|MB_ICONERROR);
}
#endif

#endif /* SYSERROR_H */
