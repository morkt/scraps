/* -*- C++ -*-
 * File:        forward.hpp
 * Created:     Fri Jul 27 00:23:02 2007
 * Description: ext::format forward declarations
 *
 * $Id$
 */

#ifndef EXT_FORMAT_FORWARD_HPP
#define EXT_FORMAT_FORWARD_HPP

#include "lstring.hpp"
#ifdef EXT_USE_STD_STRING
#include <sstream>		// for std::basic_stringstream
#else
#include "sstream.hpp"		// for ext::basic_stringstream
#endif
#include <memory>

namespace ext {

// ----------------------------------------------------- namespace imports ---

using std::ios;
using std::basic_ios;
using std::basic_ostream;
using std::basic_streambuf;
#ifdef EXT_USE_STD_STRING
using std::basic_stringstream;
using std::basic_ostringstream;
#endif
using std::unique_ptr;
using std::shared_ptr;

// -------------------------------------------------- forward declarations ---

template <class CharT, class Traits = std::char_traits<CharT>,
	  class Alloc = std::allocator<CharT> >
class basic_format;

typedef basic_format<char>	format;
typedef basic_format<wchar_t>	wformat;

} // namespace ext

#endif /* EXT_FORMAT_FORWARD_HPP */
