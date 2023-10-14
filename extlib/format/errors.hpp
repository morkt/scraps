/* -*- C++ -*-
 * File:        errors.hpp
 * Created:     Fri Jul 27 00:24:19 2007
 * Description: errors and exceptions issued by ext::format
 *
 * $Id$
 */

#ifndef EXT_FORMAT_ERRORS_HPP
#define EXT_FORMAT_ERRORS_HPP

#include <exception>

namespace ext { namespace io {

enum format_error_bits { bad_format_string_bit = 1, 
			 too_few_args_bit = 2,
			 too_many_args_bit = 4,
			 out_of_range_bit = 8,
			 invalid_argument_bit = 16,
			 all_error_bits = 0xffffffff,
			 no_error_bits = 0 };

// --- exceptions ------------------------------------------------------------

struct format_error : std::exception
{
    virtual const char* what () const throw()
    { return "ext::format_error: format generic failure"; }
};

struct bad_format_string : format_error
{
    virtual const char* what () const throw()
    { return "ext::bad_format_string: format-string is ill-formed"; }
};

struct too_few_args : format_error
{
    virtual const char* what () const throw()
    { return "ext::too_few_args: format-string referred to more arguments than were passed"; }
};

struct too_many_args : format_error
{
    virtual const char* what () const throw()
    { return "ext::too_many_args: format-string referred to less arguments than were passed"; }
};

struct out_of_range : format_error
{
    virtual const char* what () const throw()
    {
       	return "ext::out_of_range: "
	    "tried to refer to an argument (or item) number which is out of range, "
	    "according to the format string";
    }
};

struct invalid_argument : format_error
{
    virtual const char* what () const throw()
    {
       	return "ext::invalid_argument: "
	    "invalid value supplied for custom width or precision in format expression";
    }
};

} } // namespace ext::io

#endif /* EXT_FORMAT_ERRORS_HPP */
