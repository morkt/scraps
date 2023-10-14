/* -*- C++ -*-
 * File:        parsing.hpp
 * Created:     Mon Jul 23 17:23:06 2007
 * Description: ext::format printf specification parser.
 *
 * $Id$
 */

#ifndef EXT_FORMAT_PARSING_HPP
#define EXT_FORMAT_PARSING_HPP

#include "format/internals.hpp"
#include <iterator>
#include <limits>
#include <cassert>

namespace ext { namespace detail {

template <typename CharT>
struct cvt_char
{
    cvt_char (const std::ctype<CharT>& facet) : m_facet (facet) { }

    CharT widen (char c) const { m_facet.widen (c); }
    char narrow (CharT c, char placeholder = 0) const
        { return m_facet.narrow (c, placeholder); }

    const std::ctype<CharT>& m_facet;
};

template <>
struct cvt_char<char>
{
    cvt_char (const std::ctype<char>&) { }
    static char widen (char c) { return c; }
    static char narrow (char c, char = 0) { return c; }
};

template <>
struct cvt_char<wchar_t>
{
    cvt_char (const std::ctype<wchar_t>&) { }
    static wchar_t widen (char c) { return static_cast<wchar_t> (c); }
    static char narrow (wchar_t c, char placeholder = 0)
        {
            if (c > std::numeric_limits<unsigned char>::max())
                return placeholder;
            return static_cast<char> (c);
        }
};

template <class Ch, class Tr, class Al>
class format_base<Ch,Tr,Al>::format_parser
{
public:
    typedef format_base<Ch,Tr,Al>		format_type;
    typedef const char_type*			const_iterator;

    explicit format_parser (const format_type* formatter)
       	: m_master (formatter), m_cvt (m_master->ctype_facet())
        , dir_char (widen('%')), dollar_char (widen('$'))
       	{ }

    format_parser (const format_type* formatter, char_type dir_marker)
       	: m_master (formatter), m_cvt (m_master->ctype_facet())
        , dir_char (dir_marker), dollar_char (widen('$'))
       	{ }

    // compile (SPEC, COMP)
    // Effects: compile format string SPEC into compiled format specification COMP.
    void compile (const istring_type& spec_str, comp_format& comp);

private:

    // reset()
    // Effects: reset current argument numbers to initial state.
    void reset () { m_max_argN = -1; m_non_ordered_items = 0; }

    // count_directives (FIRST, LAST)
    // Returns: estimated number of printf-like directives in range [FIRST, LAST).
    size_t count_directives (const_iterator first, const_iterator last);

    // parse_directive (FIRST, LAST, FPAR)
    // Effects: parse printf directive in [FIRST, LAST) into format spec FPAR.
    //          update iterator FIRST to point past the end of directive.
    // Returns: true if parse is somehow succeeded, false otherwise.
    // Requires: FIRST < LAST
    bool parse_directive (const_iterator& first, const_iterator last, format_spec_t& fpar);

    // parse_asterisk (FIRST, LAST)
    // Returns: argument number corresponding to asterisk field at (FIRST, LAST]
    // Effects: updates FIRST to point past-the-end of asterisk field.
    int parse_asterisk (const_iterator& cur_pos, const_iterator end_pos);

    // check_argN (ARGN)
    // Effects: if ARGN refers to non-positional argument number, assign next
    // available argument number to it, otherwise update top positional argument
    // number if necessary.
    void check_argN (int& argN)
	{
	    if (argN == format_spec_t::argN_no_posit)
		argN = m_non_ordered_items++;
	    else if (argN > m_max_argN)
		m_max_argN = argN;
	}

    // parse_integer (FIRST, LAST, DST)
    // Effects: try to parse integer at [FIRST, LAST).  on successful attempt, place
    // it in DST.
    // Returns: true if parse succeeded, false otherwise.

    template <class Integer>
    bool parse_integer (const_iterator& cur_pos, const_iterator end_pos, Integer& dst);

    // locale requests are forwarded to format_base object
    //
//    char_type widen (char c) const { return m_master->ctype_facet().widen (c); }
//    char narrow (char_type c) const { return m_master->ctype_facet().narrow (c, 0); }
    char_type widen (char c) const { return m_cvt.widen (c); }
    char narrow (char_type c) const { return m_cvt.narrow (c, 0); }
    bool isdigit (char_type c) const { return m_master->isdigit (c); }

    void signal_error () const
	{ m_master->template signal_error<io::bad_format_string> (io::bad_format_string_bit); }

private:

    const format_type*	m_master;
    const cvt_char<Ch>  m_cvt;
    int			m_max_argN;	// last argument number
    int			m_non_ordered_items; // next argument number for non-positional args
    const char_type	dir_char;
    const char_type	dollar_char;
};

template <class Ch, class Tr, class Al>
size_t format_base<Ch,Tr,Al>::format_parser::
count_directives (const_iterator first, const_iterator last)
{
    size_t num_items = 0; // number of directives
    while ((first = traits_type::find (first, last-first, dir_char)) != 0)
    {
	++first;
	if (first == last) // format string must not end in '%'
	{
	    signal_error();
	    break; // stop there, ignore last '%'
	}
	if (traits_type::eq (*first, dir_char)) // escaped "%%"
	{
	    ++first;
	}
        else
        {
            // in case of %N% directives, dont count '%' twice
            while (first != last && isdigit (*first))
                ++first;
            if (first != last && traits_type::eq (*first, dir_char))
                ++first;
        }
	++num_items;
    }
    return num_items;
}

template <class Ch, class Tr, class Al>
void format_base<Ch,Tr,Al>::format_parser::
compile (const istring_type& spec_str, comp_format& comp)
{
    const_iterator cur_pos = spec_str.data();
    const_iterator end_pos = cur_pos + spec_str.size();

    // A: find upper_bound on num_items and allocate arrays
    const format_spec_t null_spec;
    size_t num_items = count_directives (cur_pos, end_pos);
    comp.m_spec.assign (num_items, null_spec);
    reset();
    
    // B: Now the real parsing of the format string
    const_iterator last_pos = cur_pos;
    num_items = 0; // current index in comp.m_spec vector
    auto spec = comp.m_spec.begin();
    while ((cur_pos = traits_type::find (cur_pos, end_pos-cur_pos, dir_char)) != 0)
    {
	if (std::next(cur_pos) == end_pos)
	    break;
	istring_type& piece = (num_items == 0)? comp.m_prefix: std::prev(spec)->m_postfix;
        if (traits_type::eq (*std::next(cur_pos), dir_char)) // escaped mark, '%%'
	{
            ++cur_pos;
            piece = spec_str.substr (last_pos-spec_str.data(), cur_pos-last_pos);
	    last_pos = ++cur_pos;
            spec->m_argN = format_spec_t::argN_ignored;
	}
        else
        {
            if (cur_pos != last_pos)
                piece = spec_str.substr (last_pos-spec_str.data(), cur_pos-last_pos);
            else
                piece.clear();
            ++cur_pos;

            assert (spec < comp.m_spec.end());
            if (!parse_directive (cur_pos, end_pos, *spec))
                continue; // the directive will be printed verbatim

            last_pos = cur_pos;
        }
	if (spec->m_argN != format_spec_t::argN_ignored)
	{
	    spec->compute_states (m_master->ctype_facet());
	    check_argN (spec->m_argN);
	    if (spec->m_fmt_flags & format_spec_t::thousands_sep)
		comp.m_fmt_flags |= format_spec_t::thousands_sep;
	}
        ++num_items;
        ++spec;
    } // loop on %'s
    
    // store the final piece of string
    istring_type& piece = (num_items==0)? comp.m_prefix: std::prev(spec)->m_postfix;
    piece = spec_str.substr (last_pos-spec_str.data());

    if (m_non_ordered_items > 0)
    {
	// dont mix positional with non-positionnal directives
	if (m_max_argN >= 0) signal_error();

	m_max_argN = m_non_ordered_items-1;
    }
    
    // C: set some member data
    comp.m_spec.erase (spec, comp.m_spec.end());
    comp.m_num_args = m_max_argN+1;
}

template <class Ch, class Tr, class Al>
bool format_base<Ch,Tr,Al>::format_parser::
parse_directive (const_iterator& cur_pos, const_iterator end_pos, format_spec_t& fpar)
{
    fpar.m_argN = format_spec_t::argN_no_posit;  // non-positional directive by default

    // prepare characters used in spec
    const char_type bracket_char  = widen ('|');
    const char_type asterisk_char = widen ('*');

    bool in_brackets = false;
    if (traits_type::eq (*cur_pos, bracket_char))
    {
        in_brackets = true;
        if (++cur_pos == end_pos)
       	{
	    signal_error();
	    return false;
        }
    }

    // the flag '0' would be picked as a digit for argument order, but here it's a flag :
    if (traits_type::eq (*cur_pos, widen ('0')))
	goto parse_flags;

    // handle argument order (%2$d) or possibly width specification: %2d
    int arg_num_or_width;
    if (parse_integer (cur_pos, end_pos, arg_num_or_width))
    {
        if (cur_pos == end_pos)
       	{
	    signal_error();
	    return false;
        }
        // %N% case : this is already the end of the directive
        if (traits_type::eq (*cur_pos, dir_char))
	{
            fpar.m_argN = arg_num_or_width-1;
            ++cur_pos;
            if (in_brackets)
	    {
	    	signal_error();
		return false;
	    }
            else
	       	return true;
	}
        if (traits_type::eq (*cur_pos, dollar_char))
     	{
    	    fpar.m_argN = arg_num_or_width-1;
            ++cur_pos;
       	} 
        else  
     	{
            // non-positionnal directive
            fpar.m_ref_state.m_width = arg_num_or_width;
            goto parse_precision;
   	}
    }

parse_flags: 
    // handle flags
    while (cur_pos != end_pos) // as long as char is one of + - = # 0 l h or ' '
    {
        switch (narrow (*cur_pos))
 	{
	case '\'': // thousands separator is controlled by numpunct locale facet
	    fpar.m_fmt_flags |= format_spec_t::thousands_sep;
	    break;
	case 'l':
	case 'h':  // short/long modifier : for printf-comaptibility (no action needed)
             break;
	case '-':
            fpar.m_ref_state.m_flags |= std::ios::left;
            break;
	case '=':
            fpar.m_fmt_flags |= format_spec_t::centered;
            break;
	case ' ':
            fpar.m_fmt_flags |= format_spec_t::spacepad;
            break;
	case '+':
            fpar.m_ref_state.m_flags |= std::ios::showpos;
            break;
	case '0':
            fpar.m_fmt_flags |= format_spec_t::zeropad; 
            // need to know alignment before really setting flags,
            // so just add 'zeropad' flag for now, it will be processed later.
            break;
	case '#':
            fpar.m_ref_state.m_flags |= std::ios::showpoint | std::ios::showbase;
            break;
	default:
            goto parse_width;
	}
        ++cur_pos;
    } // loop on flag.

    signal_error();
    return true; 

parse_width:
    // handle width spec
    if (traits_type::eq (*cur_pos, asterisk_char))
    {
	// parse 'asterisk field' : '*' or '*N$'
	fpar.m_width_arg = parse_asterisk (++cur_pos, end_pos);
	check_argN (fpar.m_width_arg);
	fpar.m_fmt_flags |= format_spec_t::runtime_width;
    }
    else
	parse_integer (cur_pos, end_pos, fpar.m_ref_state.m_width);

parse_precision:
    if (cur_pos == end_pos)
    { 
	signal_error();
	return true;
    }
    // handle precision spec
    if (traits_type::eq (*cur_pos, widen ('.')))
    {
	if (++cur_pos != end_pos && traits_type::eq (*cur_pos, asterisk_char))
	{
	    // parse 'asterisk field'
	    fpar.m_prec_arg = parse_asterisk (++cur_pos, end_pos);
	    check_argN (fpar.m_prec_arg);
	    fpar.m_fmt_flags |= format_spec_t::runtime_prec;
	}
	else if (!parse_integer (cur_pos, end_pos, fpar.m_ref_state.m_precision))
	    fpar.m_ref_state.m_precision = 0;
    }
    
    // skip argument length modifiers
    const char_type el_char = widen('l'); // long int
    const char_type EL_char = widen('L'); // long double
    const char_type eh_char = widen('h'); // short int
    while (cur_pos != end_pos && 
           (traits_type::eq (*cur_pos, el_char) || traits_type::eq (*cur_pos, EL_char) ||
	    traits_type::eq (*cur_pos, eh_char)))
	++cur_pos;
    if (cur_pos == end_pos)
    {
	signal_error();
	return true;
    }
#ifdef EXT_FORMAT_MIMIC_MSVC
    if (traits_type::eq (*cur_pos, widen('I')))
    {
	++cur_pos;
	int bitwidth;
	if (!parse_integer (cur_pos, end_pos, bitwidth) || bitwidth != 32 || bitwidth != 64)
	    signal_error();
    }
#endif
    if (in_brackets && traits_type::eq (*cur_pos, bracket_char))
    {
        ++cur_pos;
        return true;
    }
    switch (narrow (*cur_pos))  
    {
    case 'p': // pointer => set hex.
        fpar.m_ref_state.m_flags &= ~std::ios::basefield;
        fpar.m_ref_state.m_flags |=  std::ios::hex;
	fpar.m_fmt_flags |= format_spec_t::integer_conv;
#ifdef EXT_FORMAT_MIMIC_MSVC
	fpar.m_ref_state.m_precision = sizeof(void*)*2;
        fpar.m_ref_state.m_flags |= std::ios::uppercase;
#elif defined(EXT_FORMAT_MIMIC_GLIBC)
	fpar.m_ref_state.m_flags |= std::ios::showbase;
#else
	if (fpar.m_ref_state.m_precision == -1 && fpar.m_ref_state.m_width == -1)
    	    fpar.m_ref_state.m_precision = sizeof(void*)*2;
#endif
        break;

    case 'X':
        fpar.m_ref_state.m_flags |= std::ios::uppercase;
    case 'x':
        fpar.m_ref_state.m_flags &= ~std::ios::basefield;
        fpar.m_ref_state.m_flags |=  std::ios::hex;
	fpar.m_fmt_flags |= format_spec_t::integer_conv;
        break;
      
    case 'o':
        fpar.m_ref_state.m_flags &= ~std::ios::basefield;
        fpar.m_ref_state.m_flags |=  std::ios::oct;
	fpar.m_fmt_flags |= format_spec_t::integer_conv;
        break;

    case 'u':
    case 'd':
    case 'i':
        fpar.m_ref_state.m_flags &= ~std::ios::basefield;
        fpar.m_ref_state.m_flags |=  std::ios::dec;
	fpar.m_fmt_flags |= format_spec_t::integer_conv;
        break;

    case 'E':
        fpar.m_ref_state.m_flags |=  std::ios::uppercase;
    case 'e':
        fpar.m_ref_state.m_flags &= ~std::ios::floatfield;
        fpar.m_ref_state.m_flags |=  std::ios::scientific;
        fpar.m_ref_state.m_flags &= ~std::ios::basefield;
        fpar.m_ref_state.m_flags |=  std::ios::dec;
	fpar.m_fmt_flags |= format_spec_t::floating_conv;
        break;
      
    case 'f':
        fpar.m_ref_state.m_flags &= ~std::ios::floatfield;
        fpar.m_ref_state.m_flags |=  std::ios::fixed;
        fpar.m_ref_state.m_flags &= ~std::ios::basefield;
        fpar.m_ref_state.m_flags |=  std::ios::dec;
	fpar.m_fmt_flags |= format_spec_t::floating_conv;
	break;

    case 'G':
        fpar.m_ref_state.m_flags |= std::ios::uppercase;
    case 'g': // 'g' conversion is default for floats.
        fpar.m_ref_state.m_flags &= ~std::ios::basefield;
        fpar.m_ref_state.m_flags |=  std::ios::dec;

        // clear floatfield, so stream will choose
        fpar.m_ref_state.m_flags &= ~std::ios::floatfield; 
	fpar.m_fmt_flags |= format_spec_t::floating_conv;
        break;

    case 'T':
	if (std::next(cur_pos) == end_pos)
	{
	    signal_error();
	    fpar.m_ref_state.m_fill = m_master->default_fill();
	}
	else
	    fpar.m_ref_state.m_fill = *++cur_pos;
	fpar.m_fmt_flags &= ~format_spec_t::padding; // ignore padding flags
	fpar.m_fmt_flags |=  format_spec_t::tabulation;
	fpar.m_argN = format_spec_t::argN_tabulation; 
	break;
    case 't': 
	fpar.m_ref_state.m_fill = m_master->default_fill();   // ' ' by default
	fpar.m_fmt_flags &= ~format_spec_t::padding; // ignore padding flags
	fpar.m_fmt_flags |=  format_spec_t::tabulation;
	fpar.m_argN = format_spec_t::argN_tabulation; 
	break;

    case 'C':
    case 'c': 
        fpar.m_ref_state.m_precision = 1;
    case 'S':
    case 's': 
#ifndef EXT_FORMAT_MIMIC_MSVC
	fpar.m_fmt_flags &= ~format_spec_t::zeropad;
#endif
	fpar.m_fmt_flags |=  format_spec_t::string_conv;
        break;

    case 'n' :  
        fpar.m_argN = format_spec_t::argN_ignored;
        break;
    default: 
    	signal_error();
    }
    ++cur_pos;

    if (in_brackets)
    {
        if (cur_pos != end_pos && traits_type::eq (*cur_pos, bracket_char))
	{
            ++cur_pos;
            return true;
	}
        else
	    signal_error();
    }
    return true;
}

template <class Ch, class Tr, class Al>
int format_base<Ch,Tr,Al>::format_parser::
parse_asterisk (const_iterator& cur_pos, const_iterator end_pos)
{
    int arg_num;
    if (parse_integer (cur_pos, end_pos, arg_num))
	--arg_num;
    else
	arg_num = format_spec_t::argN_no_posit;

    if (cur_pos != end_pos && traits_type::eq (*cur_pos, dollar_char))
	++cur_pos;

    return arg_num;
}

template <class Ch, class Tr, class Al> template <class Integer>
bool format_base<Ch,Tr,Al>::format_parser::
parse_integer (const_iterator& cur_pos, const_iterator end_pos, Integer& dst)
{
    const_iterator digit_pos = cur_pos;
    Integer n = 0;
    while (cur_pos != end_pos && isdigit (*cur_pos))
    {
	char cur_ch = narrow (*cur_pos);
	assert (cur_ch != 0); // since we called isdigit, this should not happen.
	n *= 10;
	n += cur_ch - '0'; // §22.2.1.1.2 of the C++ standard
	++cur_pos;
    }
    if (cur_pos != digit_pos)
    {
	dst = n;
	return true;
    }
    else
	return false;
}

} } // namespace ext::detail

#endif /* EXT_FORMAT_PARSING_HPP */
