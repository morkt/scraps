/* -*- C++ -*-
 * File:        table.hpp
 * Created:     Fri Jul 27 00:42:24 2007
 * Description: ext::format hash function implementation.
 *
 * $Id$
 */

#ifndef EXT_FORMAT_TABLE_HPP
#define EXT_FORMAT_TABLE_HPP

#ifdef _MSC_VER // disable nasty warning (what's Koenig lookup for they think?)
#pragma warning(push)
#pragma warning(disable: 4675) // resolved overload was found by argument-dependent lookup
#endif

#include "hash_map.hpp"
#include "format/forward.hpp"	// for basic_string declaration

namespace ext {

#if !defined(EXT_USE_STD_STRING) || defined(__GNUC__)
template <class Ch, class Tr, class Al>
std::size_t hash_value (const basic_string<Ch,Tr,Al>& str)
{
    typedef basic_string<Ch,Tr,Al>		string_type;
    typedef typename string_type::size_type	size_type;

    unsigned long h = 0xdeadbeef;
    if (size_type size = str.size())
    {
	size_type stride = (size / 16) + 1;
	size -= stride;	// protect against size near str.max_size()
	for (size_type i = 0; i <= size; i += stride)
	    h = h*33 + Tr::to_int_type (str[i]);
    }
    return std::size_t(h);
}
#endif

} // namespace ext

#ifdef __GNUC__

namespace __gnu_cxx {
#ifdef EXT_USE_STD_STRING
    using std::basic_string;
#else
    using ext::basic_string;
#endif
}

#if __GNUC__ < 4

namespace __gnu_cxx {
    template<> template <class Ch, class Tr, class Al>
    struct hash<basic_string<Ch,Tr,Al> >
    {
	size_t operator() (const basic_string<Ch,Tr,Al>& str) const
	    { return ext::hash_value (str); }
    };
} // namespace __gnu_cxx

#else // __GNUC__ >= 4

namespace std {
    template<> template <class Ch, class Tr, class Al>
    struct hash<__gnu_cxx::basic_string<Ch,Tr,Al> >
    {
	size_t operator() (const __gnu_cxx::basic_string<Ch,Tr,Al>& str) const
	    { return ext::hash_value (str); }
    };
}

#endif // __GNUC__ < 4
#endif // __GNUC__

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* EXT_FORMAT_TABLE_HPP */
