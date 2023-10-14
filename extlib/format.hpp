/* -*- C++ -*-
 * File:        format.hpp
 * Created:     Fri Jul 13 12:08:57 2007
 * Description: formatting of arguments according to printf-like format-string.
 *
 * format keeps compiled format specifications in static hash_map.
 * if format called more than once for the same string, compiled object is used.
 *
 * Copyright (C) 2007 ****************
 *
 * Based on boost::format class (C) Samuel Krempp 2001
 *                                  krempp@crans.ens-cachan.fr
 *
 * Permission to copy, use, modify, sell and distribute this software is granted
 * provided this copyright notice appears in all copies. This software is provided
 * "as is" without express or implied warranty, and with no claim as to its
 * suitability for any purpose.
 *
 * $Id$
 */

#ifndef EXT_FORMAT_HPP
#define EXT_FORMAT_HPP

#include "format/forward.hpp"
#include "format/errors.hpp"
#include "format/internals.hpp"
#include "format/parsing.hpp"
#include "utility.hpp"		// for io::detail::fail_guard
#include "type_traits.hpp"	// for ext::type_traits and friends
#include <algorithm>		// for std::mismatch
#include <boost/function.hpp>
#include <boost/mpl/or.hpp>
#include <cassert>

namespace ext {

// ---------------------------------------------------------------------------
// basic_format

template <class CharT, class Traits, class Alloc>
class basic_format : public detail::format_base<CharT, Traits, Alloc>
{
public: // types

    typedef CharT					char_type;
    typedef Traits					traits_type;
    typedef detail::format_base<CharT, Traits, Alloc>	base_type;
    typedef basic_ostream<CharT, Traits>		ostream_type;
    typedef typename base_type::string_type		string_type;
    typedef typename base_type::istring_type		istring_type;
    typedef typename base_type::format_state		format_state;
    typedef typename base_type::format_spec_t		format_spec_t;
    typedef typename base_type::internal_stream_t	internal_stream_t;
    typedef typename base_type::comp_format		comp_format;

public: // methods

    template <class String>
    basic_format (const String& s);
    template <class String>
    basic_format (const String& s, const std::locale& loc);

    // pass arguments through those operators
    template<class T>
    basic_format& operator% (const T& arg)
       	{ return feed<typename boost::call_traits<T>::param_type> (arg); }

    template<class T>
    basic_format& operator% (T& arg)
       	{ return feed<typename detail::non_const_param<T>::type> (arg); }
	// detail::non_const_param<T>::type yields to non-const reference to T if arg
	// is a class/struct type and to boost::call_traits<T>::param_type otherwise.

    // format::str()
    // Returns: final output as string.
    string_type str () const;

    // format::append_to (STRING)
    // Effects: append final output to STRING
    void append_to (string_type& dest) const;

    // format::write (OS)
    // Effects: put final output into stream OS.
    ostream_type& write (ostream_type& os) const;

    // format::size()
    // Returns: length of final output.
    std::streamsize size () const;

    // format::clear()
    // Effects: empty buffers and reset arguments
    void clear();

private: // types

    template <class T>
    struct initializer
    {
        struct ref_init
        {
            template <class String>
            static istring_type make_istring (const String& str)
                { return istring_type (str); }
        };
        struct ptr_init
        {
            static istring_type make_istring (const CharT* str)
                { return istring_type (str, istring_type::traits_type::length (str)); }
        };
        typedef typename
            mpl::if_<boost::is_pointer<T>, ptr_init, ref_init>::type type;
    };

    struct format_result // --- results of applying format specs to arguments
    {
	typedef boost::function<bool (const format_spec_t&, format_result&)>
				dumper_type;
	format_state		state;
	string_type		output;
	dumper_type		dumper;
    };

    typedef std::vector<format_result>	result_type;

    template <class T>
    struct copy_dumper // --- helper functor for delayed format output
    {
	typedef T		param_type;
	typedef typename boost::remove_cv<
		typename boost::remove_reference<T>::type>::type
				copy_type;
	typedef typename boost::call_traits<copy_type>::param_type
				const_param_type;

	explicit copy_dumper (basic_format* master)
	    : m_master (master), m_arg ()
	    { }
	copy_dumper (basic_format* master, const_param_type x)
	    : m_master (master), m_arg (x)
	    { }

	bool operator() (const format_spec_t& specs, format_result& res)
	    { return m_master->put_dispatch<param_type> (m_arg, specs, res); }

	basic_format*	m_master;
	copy_type	m_arg;
    };

    struct string_dumper : public copy_dumper<const string_type&>
    {
	string_dumper (basic_format* master, const string_type& x)
	    : copy_dumper<const string_type&> (master, x)
	    {}
	template <class T>
	string_dumper (basic_format* master, const T& x)
	    : copy_dumper<const string_type&> (master)
	    {
	    	io::detail::empty_buf (this->m_master->stream());
		this->m_master->stream() << x;
		this->m_arg = this->m_master->stream().str();
	    }
    };

    template <typename T>
    struct dump_wrapper
    {
	typedef
	    typename boost::remove_cv<
	    typename boost::remove_reference<T>::type>::type base_type;
	typedef typename
	    mpl::if_<boost::is_pod<base_type>,
		copy_dumper<T>,
		string_dumper
	    >::type type;
    };

private: // methods

    // Note: all methods that depend on argument type T should be called with
    // explicit type specifier, eg: 'distribute<T>(x);'. This assures that argument is
    // passed in the most efficient manner (by value for small builtin types, by
    // reference otherwise).

    // format::feed(X)
    // Effects: feed argument X to format object.
    template <class T>
    basic_format& feed (T arg);

    // format::distribute(X)
    // Effects: distribute argument X among corresponding positions in format string.
    template <class T>
    void distribute (T arg);

    // put dispatchers that select proper conversion method for an argument X depending
    // on its type and formatting flags.

    template <class T> 
    bool put_dispatch (T x, const format_spec_t& specs, format_result& res);
    template <class T> 
    bool put_numloc_dispatch (T x, const format_spec_t& specs, format_result& res);
    template <class T> 
    bool put_int_dispatch (T x, const format_spec_t& specs, format_result& res);

    // format::put (X, SPEC, RES, ...)
    // Effects: convert argument X into string RES using format specification SPEC.
    // Returns: true if conversion was successful, false otherwise.
    template <class T> 
    bool put (T x, const format_spec_t& specs, format_result& res, false_type);
    template <class T> 
    bool put (T x, const format_spec_t& specs, format_result& res, detail::integral_type_tag);
    template <class T>
    bool put (T x, const format_spec_t& specs, format_result& res, detail::arithmetic_type_tag);

    // format::set_custom_width (X, RES, ZEROPAD)
    // Effects: read field width from X and store it in state member of RES.
    // if ZEROPAD is true and field is left-aligned, reset fill character.
    template <class T> 
    void set_custom_width (T x, format_result& res, bool zeropad);

    // format::set_custom_precision (X, RES)
    // Effects: read field precision from X and store it in state member of RES.
    template <class T> 
    void set_custom_precision (T x, format_result& res);

    // format::convert_arg (X, DST)
    // Effects: convert X to arithmetic type and put result into DST.
    // Returns: true if conversion was successful, false otherwise.
    template <class T, class Arithmetic>
    bool convert_arg (T x, Arithmetic& dst);
    template <class T, class Arithmetic> 
    bool convert_arg (T x, Arithmetic& dst, false_type);
    template <class T, class Arithmetic> 
    bool convert_arg (T x, Arithmetic& dst, true_type);

    void init (const istring_type& spec_str);

    // format::stream()
    // Returns: reference to internal conversion stream
    internal_stream_t& stream () { return this->m_oss; }

    // access comp_format object members

    int spec_count () const { return this->m_format->m_spec.size(); }
    int arg_total () const { return this->m_format->m_num_args; }
    const istring_type& prefix () const { return this->m_format->m_prefix; }
    unsigned fmt_flags () const { return this->m_format->m_fmt_flags; }

    // format::spec(N)
    // Returns: reference to directive N in format specification (starting from 0)
    // Requires: N < spec_count()
    const format_spec_t& spec (int n) const { return this->m_format->m_spec[n]; }

    // format::do_append_to (STR)
    // Effects: append final output to STRING (without error checking)
    void do_append_to (string_type& dest) const;

private: // data

    istring_type		m_prefix;	// string preceding first argument
						// in format specification
    int				m_cur_arg;	// next argument number
    mutable /*!*/ result_type	m_result;	// results of applying spec to arguments
    mutable /*!*/ bool		m_dumped;	// true only after call to str() or <<
};

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

template <class Ch, class Tr, class Al> template <class String>
basic_format<Ch,Tr,Al>:: basic_format (const String& s)
    : m_cur_arg(0), m_dumped(false)
{
    init (initializer<String>::type::make_istring (s));
}

template <class Ch, class Tr, class Al> template <class String>
basic_format<Ch,Tr,Al>:: basic_format (const String& s, const std::locale& loc)
    : base_type (loc), m_cur_arg(0), m_dumped(false)
{
    init (initializer<String>::type::make_istring (s));
}

// format::init (SPEC_STR)
// Effects: initialize format object to correspond SPEC_STR format specification.

template <class Ch, class Tr, class Al>
void basic_format<Ch,Tr,Al>:: init (const istring_type& spec_str)
{
    this->parse (spec_str);
    if (this->valid())
    {
	m_prefix = this->prefix();
	// if format uses thousands separator, initialize numpunct facet
	if (this->fmt_flags() & format_spec_t::thousands_sep)
	    this->init_numpunct();
	const int spec_count = this->spec_count();
	m_result.resize (spec_count);
	for (int i = 0; i < spec_count; ++i)
	    m_result[i].state = this->spec(i).m_ref_state;
    }
    else
	m_prefix = spec_str;
}

// format::clear()
// Effects: empty the string buffers and make the format object ready for formatting
// a new set of arguments.

template <class Ch, class Tr, class Al>
void basic_format<Ch,Tr,Al>:: clear ()
{
    if (this->valid())
    {
	const int spec_count = this->spec_count();
	for (int i = 0; i < spec_count; ++i)
	{
	    m_result[i].state = this->spec(i).m_ref_state;
	    m_result[i].output.clear();
	    m_result[i].dumper.clear();
	}
    }
    m_cur_arg = 0;
    m_dumped = false;
}

// ---------------------------------------------------------------------------
// format argument distribution and dispatching {{{

// format::feed(X)
// Effects: feed argument X to format object.

template<class Ch, class Tr, class Al> template <class T> 
inline basic_format<Ch,Tr,Al>& basic_format<Ch,Tr,Al>:: feed (T x) 
{
    if (m_dumped) clear();
    distribute<T> (x);
    ++m_cur_arg;
    return *this;
}

// format::distribute(X)
// Effects: distribute argument X among corresponding positions in format string.

template <class Ch, class Tr, class Al> template <class T> 
void basic_format<Ch,Tr,Al>:: distribute (T x) 
{
    if (!this->valid() || m_cur_arg >= this->arg_total())
    {
       	// too many variables have been supplied !
	this->template signal_error<io::too_many_args> (io::too_many_args_bit);
        // ignore remaining arguments
	return;
    }
    for (int i = 0; i < this->spec_count(); ++i)
    {
	const format_spec_t& specs = this->spec(i);

        if (specs.m_argN == format_spec_t::argN_ignored)
            continue;

	// note that same positional argument could be used for custom width,
	// precision and output.

	if (specs.m_width_arg == m_cur_arg)
	    set_custom_width<T> (x, m_result[i], specs.m_fmt_flags & format_spec_t::zeropad);

	if (specs.m_prec_arg == m_cur_arg)
	    set_custom_precision<T> (x, m_result[i]);

  	if (specs.m_argN == m_cur_arg)
	{
	    if (specs.m_fmt_flags & format_spec_t::delayed_format
		&& (m_cur_arg < specs.m_width_arg || m_cur_arg < specs.m_prec_arg))
		// delay format until all arguments are available
	    {
		typedef typename dump_wrapper<T>::type dumper_type;
		m_result[i].dumper = dumper_type (this, x);
	    }
	    else
		put_dispatch<T> (x, specs, m_result[i]);
	}
    }
}

// format::put_dispatch (X, SPEC, RES)
// Effects: select proper conversion method for X depending on its type and
// formatting flags.

template <class Ch, class Tr, class Al> template <class T> 
inline bool basic_format<Ch,Tr,Al>::
put_dispatch (T x, const format_spec_t& specs, format_result& res)
{
    if (!(specs.m_fmt_flags & format_spec_t::thousands_sep))
	return put_int_dispatch<T> (x, specs, res);
    else
	return put_numloc_dispatch<T> (x, specs, res);
}

// format::put_numloc_dispatch (X, SPEC, RES)
// Effects: imbues internal stream with thousands grouping facet and continues
// argument conversion.

template <class Ch, class Tr, class Al> template <class T> 
bool basic_format<Ch,Tr,Al>::
put_numloc_dispatch (T x, const format_spec_t& specs, format_result& res)
{
    assert (this->m_numpunct);
    std::locale numloc (stream().getloc(), this->m_numpunct.get());
    detail::auto_locale_imbuer<Ch,Tr> loc_saver (stream(), numloc);
    return put_int_dispatch<T> (x, specs, res);
}

// format::put_int_dispatch (X, SPEC, RES)
// Effects: fires up conversion of argument X according to its type and format
// conversion flag.

template <class Ch, class Tr, class Al> template <class T> 
inline bool basic_format<Ch,Tr,Al>::
put_int_dispatch (T x, const format_spec_t& specs, format_result& res)
{
    typedef typename boost::remove_reference<T>::type arg_type;
    // metafunction that determines format argument type tag
    typedef typename
       	mpl::eval_if<
//	    boost::is_integral<arg_type>,
    	    mpl::or_<boost::is_integral<arg_type>, boost::is_pointer<arg_type> >,
	    mpl::identity<detail::integral_type_tag>, mpl::identity<false_type>
	>::type integral_or_false_type;
    typedef typename mpl::eval_if<boost::is_arithmetic<arg_type>,
	    mpl::identity<detail::arithmetic_type_tag>, mpl::identity<false_type>
	>::type arithmetic_or_false_type;

    if (specs.m_fmt_flags & format_spec_t::integer_conv)
	return put<T> (x, specs, res, integral_or_false_type());
    else if (specs.m_fmt_flags & format_spec_t::floating_conv)
	return put<T> (x, specs, res, arithmetic_or_false_type());
    else // arithmetic argument doesnt treated specially
	return put<T> (x, specs, res, false_type());
}

// format::set_custom_width (X, RES, ZEROPAD)
// Effects: read field width from X and store it in state member of RES.
// if ZEROPAD is true and field is left-aligned, reset fill character.

template <class Ch, class Tr, class Al> template <class T> 
void basic_format<Ch,Tr,Al>::
set_custom_width (T x, format_result& res, bool zeropad)
{
    int width;
    if (convert_arg<T> (x, width))
    {
	if (width < 0)
	{
	    res.state.m_flags &= ~ios::adjustfield;
	    res.state.m_flags |=  ios::left;
	    width = -width;
	    if (zeropad)
		res.state.m_fill = this->default_fill();
	}
	res.state.m_width = width;
    }
    else
	this->template signal_error<io::invalid_argument> (io::invalid_argument_bit);
} 

// format::set_custom_precision (X, RES)
// Effects: read field precision from X and store it in state member of RES.

template <class Ch, class Tr, class Al> template <class T> 
void basic_format<Ch,Tr,Al>::
set_custom_precision (T x, format_result& res)
{
    int prec;
    if (convert_arg<T> (x, prec))
    {
	if (prec >= 0)
	    res.state.m_precision = prec;
    }
    else
	this->template signal_error<io::invalid_argument> (io::invalid_argument_bit);
}

// format::convert_arg (X, DST)
// Effects: convert X to arithmetic type and put result into DST.
// Returns: true if conversion was successful, false otherwise.

template <class Ch, class Tr, class Al> template <class T, class Dest>
inline bool basic_format<Ch,Tr,Al>::
convert_arg (T x, Dest& dst)
{
    typedef typename mpl::eval_if_c<
        boost::is_arithmetic<T>::value && boost::is_arithmetic<Dest>::value,
	mpl::identity<true_type>, mpl::identity<false_type>
    >::type are_both_arithmetic;
    return convert_arg<T> (x, dst, are_both_arithmetic());
}

// format::convert_arg (X, DST, false_type)
// Effects: convert X to type Dest and put result into DST if conversion succeeded.
// Returns: true if conversion was successful, false otherwise.

template <class Ch, class Tr, class Al> template <class Src, class Dest>
bool basic_format<Ch,Tr,Al>::
convert_arg (Src x, Dest& dst, false_type)
{
    basic_stringstream<Ch,Tr,Al> in_str;
    if (in_str << x) in_str >> dst;
    return !in_str.fail();
}

// format::convert_arg (X, DST, true_type)
// specialization of convert_arg for arithmetic X and DST.

template <class Ch, class Tr, class Al> template <class Src, class Dest> 
inline bool basic_format<Ch,Tr,Al>::
convert_arg (Src x, Dest& dst, true_type)
{
    typedef typename boost::remove_reference<Src>::type arg_type;
    dst = static_cast<Dest> (x);
    // FIXME this check prevents silent rounding of floating point X
    // consider using boost::numeric_cast (it throws on range loss though)
//    return static_cast<arg_type>(dst) == x;
    return true;
}

// }}}------------------------------------------------------------------------
// inserters {{{

// format::put (X, SPEC, RES, integral_type_tag)
// Effects: convert integral X into string RES using format specification SPEC.
// Returns: true if conversion was successful, false otherwise.

template <class Ch, class Tr, class Al> template <class T>
bool basic_format<Ch,Tr,Al>::
put (T x, const format_spec_t& specs, format_result& res, detail::integral_type_tag)
{
    typedef typename boost::remove_cv<
	    typename boost::remove_reference<T>::type>::type arg_type;
    this->begin_put (res.state);

    int field_width = static_cast<int> (stream().width());
    const bool centered = field_width > 0
			&& (specs.m_fmt_flags & format_spec_t::centered);
    const bool spacepad = std::numeric_limits<arg_type>::is_signed
			&& (specs.m_fmt_flags & format_spec_t::spacepad);
    char_type fill = stream().fill();
    const ios::fmtflags flags = stream().flags();
    int num_width = res.state.m_precision;

    if (num_width >= 0)
    {
	if ((flags & ios::showbase) && (flags & ios::hex) && x != 0)
	    num_width += 2; // '0x' prepended
	else if (std::numeric_limits<arg_type>::is_signed   // if type is signed
		 && ((flags & ios::showpos)		    // and either showpos flag
		     || detail::is_negative(x))		    // or x is negative
		 && !(flags & (ios::hex|ios::oct)))	    // and base is decimal
	    num_width += 1; // then either '+' or '-' prepended
	const char_type zero_char = this->ctype_facet().widen ('0');
	// in case of zero precision and zero X output is empty
	if (num_width > 0 || x != 0)
	{
	    stream().width (num_width);
	    stream().fill (zero_char);
	    stream().setf (ios::internal, ios::adjustfield);
	    stream() << detail::integer_cast<arg_type>::do_cast (x);
	}
	if (fill == zero_char)
	    fill = this->default_fill();
    }
    else
    {
       	if (centered)
    	    stream().width (0);
	else if (spacepad && field_width > 0 && !detail::is_negative(x))
	    stream().width (--field_width);

	// force integral promotion
	stream() << detail::integer_cast<arg_type>::do_cast (x);
    }
    res.output = stream().str();

    if (field_width > 0)
	io::detail::do_fill (res.output, field_width, fill, flags, centered);
    if (spacepad && (res.output.empty() || isdigit (res.output[0])))
	res.output.insert (0, 1, this->default_fill()); // insert 1 space at pos 0

    this->end_put();
    return stream().good();
}

// format::put (X, SPEC, RES, arithmetic_type_tag)
// Effects: treat arithmetic X as floating point number ant convert it into string
// RES using format specification SPEC.
// Returns: true if conversion was successful, false otherwise.

template <class Ch, class Tr, class Al> template <class T>
bool basic_format<Ch,Tr,Al>::
put (T x, const format_spec_t& specs, format_result& res, detail::arithmetic_type_tag)
{
    this->begin_put (res.state);

    int field_width = static_cast<int> (stream().width());
    const bool centered = field_width > 0 && (specs.m_fmt_flags & format_spec_t::centered);
    const bool spacepad = specs.m_fmt_flags & format_spec_t::spacepad;

    if (centered)
	stream().width (0);
    else if (spacepad && field_width > 0 && !detail::is_negative(x))
	stream().width (--field_width);

    stream() << static_cast<typename detail::float_cast<T,double>::type> (x);
    res.output = stream().str();

    // we have to pad only for center alignment, otherwise result is already padded
    // by stream.
    if (centered)
	io::detail::do_fill (res.output, field_width, stream().fill(),
			     stream().flags(), centered);
    if (spacepad && (res.output.empty() || isdigit (res.output[0])))
	res.output.insert (0, 1, this->default_fill()); // insert 1 space at pos 0

    this->end_put();
    return stream().good();
}

// format::put (X, SPEC, RES, false_type)
// Effects: convert X into string RES using format specification SPEC.
// Returns: true if conversion was successful, false otherwise.

template <class Ch, class Tr, class Al> template <class T> 
bool basic_format<Ch,Tr,Al>::
put (T x, const format_spec_t& specs, format_result& res, false_type)
{
    this->begin_put (res.state);

    const int width = static_cast<int> (stream().width());
    int truncate = -1;
    if (specs.m_fmt_flags & format_spec_t::string_conv)
    {
	truncate = res.state.m_precision;
	stream().precision (this->default_state().m_precision);
    }
    const ios::fmtflags flags = stream().flags();
    const bool two_phased_filling = (flags & ios::internal) && truncate < 0;
    // two phased filling used when zero-padding requested and output size exceedes
    // specified width

    if (width > 0 && !two_phased_filling)
	stream().width (0); // output will be padded manually

    // *** argument conversion ***
    stream() << x;

    res.output = stream().str();
    int output_size = res.output.size();

    if (truncate >= 0 && truncate < output_size)
    {
	res.output.resize (truncate);
	output_size = truncate;
    }

    // If the first character of a signed conversion is not a sign, or if a signed
    // conversion results in no characters, a space is prefixed to the result.
    const bool spacepad = specs.m_fmt_flags & format_spec_t::spacepad;
    if (spacepad && (res.output.empty() || isdigit (res.output[0])))
	res.output.insert (0, 1, this->default_fill()); // insert 1 space at pos 0

    if (two_phased_filling && output_size > width)
    {
	// either it was one big arg and we have nothing to do about it,
	// either it was multi-output with first output filling up all width..
	io::detail::empty_buf (stream());
	stream().width (0);
	stream() << x;
	string_type tmp = stream().str();  // minimal-length output

	if (spacepad && (tmp.empty() || isdigit (tmp[0])))
	    tmp.insert (0, 1, this->default_fill()); // insert 1 space at pos 0
	int pad = width - tmp.size();
	if (pad > 0)
	{
	    typename string_type::iterator diff =
	       	std::mismatch (tmp.begin(), tmp.end(), res.output.begin()).first;
	    // insert PAD fill characters at position DIFF
	    tmp.insert (diff, pad, stream().fill());
	}
	res.output.swap (tmp); 
    }
    else if (width > 0) // other cases that need do_fill
    {
	bool centered = specs.m_fmt_flags & format_spec_t::centered;
	io::detail::do_fill (res.output, width, stream().fill(), flags, centered);
    }
    this->end_put();
    return stream().good();
}

// }}}------------------------------------------------------------------------
// final output {{{

// format::size()
// Effects: calculate final ouput length, expanding delayed arguments if needed.
// Returns: length of the final output.

template <class Ch, class Tr, class Al>
std::streamsize basic_format<Ch,Tr,Al>:: size () const
{
    std::streamsize sz = m_prefix.size();
    if (this->valid())
    {
	for (int i = 0; i < this->spec_count(); ++i) 
	{
	    const format_spec_t& spec = this->spec(i);
	    if (spec.m_fmt_flags == format_spec_t::tabulation)
	    {
		std::streamsize n = m_result[i].state.m_width - sz;
		if (n > 0) sz += n;
	    }
	    else if (m_result[i].dumper)
	    {
		m_result[i].dumper (spec, m_result[i]);
		m_result[i].dumper.clear();
	    }
	    sz += m_result[i].output.size() + spec.m_postfix.size();
	}
    }
    return sz;
}

// format::append_to(STR)

template <class Ch, class Tr, class Al>
void basic_format<Ch,Tr,Al>::
append_to (typename basic_format<Ch,Tr,Al>::string_type& dest) const
{
    m_dumped = true;
    if (!this->valid() || this->spec_count() == 0)
    {
        dest.append (m_prefix.data(), m_prefix.size());
        return;
    }

    if (m_cur_arg < this->arg_total())
	// not enough variables have been supplied !
	this->template signal_error<io::too_few_args> (io::too_few_args_bit);

    this->do_append_to (dest);
}

// format::str()
// Returns: final ouput as string.

template <class Ch, class Tr, class Al>
typename basic_format<Ch,Tr,Al>::string_type basic_format<Ch,Tr,Al>::
str () const
{
    m_dumped = true;
    if (!this->valid() || this->spec_count() == 0)
	return string_type (m_prefix.data(), m_prefix.size());

    if (m_cur_arg < this->arg_total())
	// not enough variables have been supplied !
	this->template signal_error<io::too_few_args> (io::too_few_args_bit);

    string_type res;
    this->do_append_to (res);
    return res;
}

template <class Ch, class Tr, class Al>
void basic_format<Ch,Tr,Al>::
do_append_to (typename basic_format<Ch,Tr,Al>::string_type& dest) const
{
    dest.reserve (dest.size() + size());
    dest.append (m_prefix.data(), m_prefix.size());
    for (int i = 0; i < this->spec_count(); ++i) 
    {
	const format_spec_t& spec = this->spec(i);
	if (spec.m_fmt_flags & format_spec_t::tabulation)
	{
	    int n = m_result[i].state.m_width - dest.size();
	    if (n > 0)
		dest.append (n, m_result[i].state.m_fill);
	}
	else if (spec.m_argN != format_spec_t::argN_ignored)
	    dest += m_result[i].output;

	dest.append (spec.m_postfix.data(), spec.m_postfix.size());
    }
}

// write (OS)
// Effects: performance-tuned "os << this->str()"
// Returns: OS

template <class Ch, class Tr, class Al>
basic_ostream<Ch,Tr>& basic_format<Ch,Tr,Al>::
write (basic_ostream<Ch,Tr>& os) const
{
    m_dumped = true;
    if (this->valid() && m_cur_arg < this->arg_total())
       	// not enough variables have been supplied !
	this->template signal_error<io::too_few_args> (io::too_few_args_bit);

    typename ostream_type::sentry ok (os);
    io::detail::fail_guard<Ch,Tr> guard (os);

    if (ok)
    {
	bool write_ok = io::detail::write_str (os.rdbuf(), m_prefix);
	if (write_ok && this->valid())
	{
	    std::streamsize char_count = m_prefix.size();
	    const int spec_count = this->spec_count();
	    for (int i = 0; i < spec_count; ++i) 
	    {
		const format_spec_t& spec = this->spec(i);
		if (spec.m_fmt_flags & format_spec_t::tabulation)
		{
		    std::streamsize n = m_result[i].state.m_width - char_count;
		    if (n > 0)
		    {
			write_ok = io::detail::write_char (os.rdbuf(), n, m_result[i].state.m_fill);
			char_count += n;
		    }
		}
                else if (spec.m_argN != format_spec_t::argN_ignored)
                {
		    if (m_result[i].dumper)
		    {
			m_result[i].dumper (spec, m_result[i]);
			m_result[i].dumper.clear();
		    }
		    write_ok = io::detail::write_str (os.rdbuf(), m_result[i].output);
		    char_count += m_result[i].output.size();
		}
		if (!write_ok) break;
		write_ok = io::detail::write_str (os.rdbuf(), spec.m_postfix);
		if (!write_ok) break;
		char_count += spec.m_postfix.size();
	    }
	}
	if (write_ok)
	    guard.clear();
    }
    return os;
}

// }}}------------------------------------------------------------------------
// non-member functions

template <class Ch, class Tr, class Al>
inline basic_string<Ch,Tr,Al> str (const basic_format<Ch,Tr,Al>& f)
{
    return f.str();
}

// operator<< (OS, FORMAT)
// Effects: FORMAT.write (OS)

template <class Ch, class Tr, class Al>
inline basic_ostream<Ch,Tr>& 
operator<< (basic_ostream<Ch,Tr>& os, const basic_format<Ch,Tr,Al>& f) 
{
    return f.write (os);
}

} // namespace ext

#endif /* EXT_FORMAT_HPP */
