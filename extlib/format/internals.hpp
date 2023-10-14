/* -*- C++ -*-
 * File:        internals.hpp
 * Created:     Fri Jul 27 00:18:58 2007
 * Description: internal data structures used by ext::format
 *
 * $Id$
 */

#ifndef EXT_FORMAT_INTERNALS_HPP
#define EXT_FORMAT_INTERNALS_HPP

#include "format/forward.hpp"
#ifndef EXT_FORMAT_USE_TABLE
#include "format/table.hpp"
#endif
#include "atomic.hpp"		// for ext::atomic_swap
#include <locale>
#include <vector>
#ifdef EXT_MT
#include <boost/thread/mutex.hpp>
#endif

namespace ext {

namespace io { namespace detail {

// empty_buf (OS)
// zap contents of ostringstream OS

template<class Ch, class Tr, class Al> inline
void empty_buf (basic_ostringstream<Ch,Tr,Al>& os)
{
    static const basic_string<Ch,Tr,Al> empty_str;
    os.str (empty_str);
}

// do_fill (S, W, C, F, CENTER)
// pad contents of string S with character C to fill width W according to flags F,
// or if CENTER is TRUE center string.

template<class Ch, class Tr, class Al>
void do_fill (basic_string<Ch,Tr,Al>& s, int w, const Ch c,
	      ios::fmtflags f, bool center) 
{
    int n = w - s.size();
    if (n > 0)
    {
	if (center) 
	{
	    if (s.capacity() < static_cast<size_t> (w))
		s.reserve (w);
	    const int nright = n / 2, nleft = n - nright; 
	    s.insert (0, nleft, c);
	    s.append (nright, c);
	} 
	else if (f & ios::left)
	    s.append (n, c);
	else
	    s.insert (0, n, c);
    }
}

// write_str (SBUF, STR)
// Effects: unformatted write contents of string STR into streambuffer SBUF.
// Returns: true if write succeeded, false otherwise.

template <class Ch, class Tr, class String> inline
bool write_str (basic_streambuf<Ch,Tr>* sbuf, const String& str)
{
    const std::streamsize size = str.size();
    return size == sbuf->sputn (str.data(), size);
}

} } // namespace io::detail

namespace detail {

// ---------------------------------------------------------------------------
// stream_format_state
// set of parameters that define the format state of a stream.

template <class CharT, class Traits>
struct stream_format_state
{
    int			m_width;
    int			m_precision;
    CharT		m_fill;
    ios::fmtflags	m_flags;

    stream_format_state () : m_width(-1), m_precision(-1), m_fill(0), m_flags(ios::dec)
      	{ }
    stream_format_state (basic_ios<CharT,Traits>& os)
	{ set_by_stream (os); }

    void apply_on (basic_ios<CharT,Traits>& os) const;	// applies format_state to the stream
    template<class T>
    void apply_manip (T manipulator);			// modifies state by applying manipulator.
    void reset();					// sets to default state.
    void set_by_stream (const basic_ios<CharT,Traits>& os);	// sets to os's state.
};

// ---------------------------------------------------------------------------
// auto_locale_imbuer
// imbue stream with specified locale in current scope.

template <class Ch, class Tr>
class auto_locale_imbuer
{
    basic_ios<Ch,Tr>&	m_stream;
    std::locale		m_prevloc;
public:
    auto_locale_imbuer (basic_ios<Ch,Tr>& stream, const std::locale& loc)
	: m_stream (stream), m_prevloc (m_stream.imbue (loc))
	{ }
    ~auto_locale_imbuer () { m_stream.imbue (m_prevloc); }
};

// ---------------------------------------------------------------------------
// num_sep
// numpunct facet with thousands grouping (see [22.2.3] and [22.2.8]).

template <class CharT>
class num_sep : public std::numpunct<CharT>
{
    typedef std::numpunct<CharT>	numpunct;
    typedef std::ctype<CharT>		ctype;
    typedef CharT			char_type;

    char_type	m_point, m_sep;

public:
    // cant use numpunct_byname since it cant be based on modified locales
    // (i.e. name() == "*")
    explicit num_sep (const std::locale& loc, size_t refs = 0)
       	: numpunct (refs)
       	{
	    const numpunct& num_facet = std::use_facet<numpunct> (loc);
	    const ctype& ctype_facet = std::use_facet<ctype> (loc);
	    m_point = num_facet.decimal_point();
	    m_sep = ctype_facet.widen ('\'');
	    // Note: truename() and falsename() are not used with this facet
       	}
protected: // virtual functions
    char_type do_decimal_point () const { return m_point; }
    char_type do_thousands_sep () const { return m_sep; }
    std::string do_grouping () const // note std::string here
	{
	    static std::string group ("\003");
	    return group;
	}
};

// --- format_spec -----------------------------------------------------------
// class describing single formatting directive.

template <class CharT, class Traits, class Alloc>
struct format_spec
{
    typedef stream_format_state<CharT, Traits>	format_state;
    typedef light_string<CharT>	                istring_type;

    enum fmt_flags { zeropad = 1, spacepad = 2, centered = 4,
		     tabulation    =     8,
		     thousands_sep =  0x10,
		     runtime_width =  0x20,
		     runtime_prec  =  0x40,
		     integer_conv  =  0x80,
		     floating_conv = 0x100,
		     string_conv   = 0x200,
		     padding = zeropad|spacepad|centered,
		     delayed_format = runtime_width|runtime_prec,
    };
    enum arg_values { argN_no_posit   = -1, // non-positional directive. argN will be set later.
		      argN_tabulation = -2, // tabulation directive. (no argument read) 
		      argN_ignored    = -3  // ignored directive. (no argument read)
    };

    int			m_argN;		// argument number (starts at 0, ie: %1 => argN=0)
					// negative values are used for items that don't process
					// an argument (see arg_values)
    format_state	m_ref_state;	// reference state, as set from format string
    istring_type	m_postfix;	// piece of string between this item and the next
    unsigned		m_fmt_flags;	// special format flags and padding, see fmt_flags
    int			m_width_arg;	// width argument number supplied during run-time
    int			m_prec_arg;	// precision argument number

    format_spec () : m_argN(argN_no_posit), m_fmt_flags(0)
		   , m_width_arg(argN_no_posit), m_prec_arg(argN_no_posit) { }

    void compute_states (const std::ctype<CharT>& ctype);
};

// --- format_base -----------------------------------------------------------
// encapsulates non-copy-constructable format members and members independent
// of format specification.

template <class CharT, class Traits, class Alloc>
class format_base
{
public: // types

    typedef CharT					char_type;
    typedef Traits					traits_type;
    typedef basic_string<CharT, Traits, Alloc>		string_type;
    typedef light_string<CharT>		                istring_type; // immutable string
    typedef stream_format_state<CharT, Traits>		format_state;
    typedef format_spec<CharT, Traits, Alloc>		format_spec_t;
    typedef basic_ostringstream<CharT, Traits, Alloc>	internal_stream_t;
    typedef std::ctype<char_type>			ctype;
    typedef num_sep<char_type>				numpunct;

public: // methods

    format_base ();
    format_base (const std::locale& loc);
    format_base (const format_base& other);
    format_base& operator= (const format_base& other);

    // Choosing which errors will throw exceptions
    unsigned exceptions () const { return m_exceptions; }
    unsigned exceptions (unsigned newexcept);

    static unsigned set_default_exceptions (unsigned newexcept);

protected: // types

    // comp_format -- compiled format object representation

    struct comp_format
    {
	std::vector<format_spec_t>	m_spec;		// arguments format flags
	istring_type			m_prefix;	// string preceding first argument
	int				m_num_args;	// number of expected arguments
	unsigned			m_fmt_flags;	// some flags describing format
	// m_num_args may be less than or greater than spec_count() since some
	// arguments could be ignored by format specification or same argument used
	// in several format directives.

	comp_format () : m_num_args(0), m_fmt_flags(0) { }
    };

    class format_parser;

#ifdef EXT_FORMAT_USE_TABLE
    typedef hash_map<istring_type, comp_format>	map_type;
    struct format_table
    {
	map_type		map;
#ifdef EXT_MT
	boost::mutex		mutex;
#endif
    };
#endif // EXT_FORMAT_USE_TABLE

protected: // methods

    // default_state()
    // Returns: reference to default state of the stream.
    const format_state& default_state () const { return m_state0; }

    // default_fill()
    // Returns: character used to pad fields.
    char_type default_fill () const { return default_state().m_fill; }

    // ctype_facet()
    // Returns: reference to std::ctype facet used by format.  Reference is valid as
    // long as format object exists.
    const ctype& ctype_facet () const { return *m_ctype; }

    // format::isdigit (C)
    // Returns: true if character C is a digit in format's locale, false otherwise.
    bool isdigit (char_type c) const { return m_ctype->is (ctype::digit, c); }

    // format::signal_error<EXCEPTION> (COND)
    // Effects: if exception condition COND is enabled, throw EXCEPTION(), otherwise
    // do nothing.
    template <class Exception>
    void signal_error (unsigned condition) const
	{
	    // since ext::format could be extensively used for logging/debug output,
	    // protect from format errors during stack unwinding
	    if ((exceptions() & condition) && !std::uncaught_exception())
	       	throw Exception();
       	}

    // format::parse (SPEC_STR)
    // Effects: find compiled format specification corresponding to SPEC_STR and
    // initalize members accordingly.
    void parse (const istring_type& spec_str);

    // format::begin_put (STATE)
    // Effects: intitializes internal stream from STATE object.
    void begin_put (const format_state& state);

    // format::end_put()
    // Effects: resets internal stream to default state.
    void end_put ();

    // init_numpunct()
    // Effects: initialize custom numpunct facet for thousands grouping.
    void init_numpunct ()
	{ m_numpunct.reset (new numpunct (m_oss.getloc(), 1)); }

    // format::valid()
    // Returns: true if format spec has been parsed and spec() will returns valid
    // reference.
    bool valid () const { return m_format != 0; }

protected: // data

    internal_stream_t		m_oss;		// the internal stream
    format_state		m_state0;	// default stream state
    const ctype*		m_ctype;	// ctype facet of stream's locale
    unique_ptr<numpunct>	m_numpunct;	// numpunct facet for thousands grouping

    const comp_format*		m_format;	// pointer to compiled specification
						// or null if no specification is used
#ifdef EXT_FORMAT_USE_TABLE
    shared_ptr<comp_format>	m_format_allocated;
#else
    comp_format			m_format_compiled;
#endif
    unsigned			m_exceptions;	// exception throw policy

    // s_default_exceptions is statically initialized to zero (io::no_error_bits),
    // so ext::Format follows printf behavior to silently ignore formatting errors.
    // In order to turn on exceptions in created basic_format objects, issue a call
    //
    //	    ext::format::set_default_exceptions (ext::io::all_error_bits);
    //
    // or with any other combination of error_bits.  Note that format objects that
    // was created prior to set_default_exceptions call will still use their local
    // exception policy.  Also note that policy change is global and affects all
    // running threads.         XXX consider make this variable thread-local
    static atomic_type		s_default_exceptions;

#ifdef EXT_FORMAT_USE_TABLE
    // format keeps compiled format specifications in this static hash_map.
    // Access to map is serialized between threads if EXT_MT preprocessor symbol is
    // defined during compilation.
    static format_table		s_table;	// table of used formats
#endif
};

// ---------------------------------------------------------------------------
// Definitions
// ---------------------------------------------------------------------------

// stream_format_state::apply_on(OS)
// Effects: set the state of stream OS according to our params.

template<class Ch, class Tr> inline
void stream_format_state<Ch,Tr>::apply_on (basic_ios<Ch,Tr>& os) const
{
    if (m_width >= 0)
	os.width (m_width);
    if (m_precision >= 0)
	os.precision (m_precision);
    if (m_fill != 0)
	os.fill (m_fill);
    os.flags (m_flags);
}

// stream_format_state::set_by_stream(OS)
// Effects: set our params according to the state of stream OS.

template<class Ch, class Tr> inline
void stream_format_state<Ch,Tr>::set_by_stream (const basic_ios<Ch,Tr>& os) 
{
    m_flags = os.flags();
    m_width = static_cast<int> (os.width());
    m_precision = static_cast<int> (os.precision());
    m_fill = os.fill();
}

// stream_format_state::apply_manip(MANIP)
// Effects: modify our params according to the manipulator.

template<class Ch, class Tr> template<class T> inline
void stream_format_state<Ch,Tr>::apply_manip (T manipulator) 
{
    basic_ostringstream<Ch,Tr> ss;
    apply_on (ss);
    ss << manipulator;
    set_by_stream (ss);
}

// stream_format_state::reset()
// Effects: set our params to standard's default state

template<class Ch, class Tr> inline
void stream_format_state<Ch,Tr>::reset() 
{
    m_width = -1; m_precision = -1; m_fill = 0; 
    m_flags = ios::dec; 
}

// format_spec::compute_states (CTYPE)
// Effects: preform flags post-processing, since some flags are mutually exclusive
// and some has complex consequences on stream state.

template<class Ch, class Tr, class Al>
void format_spec<Ch,Tr,Al>:: compute_states (const std::ctype<Ch>& ct)
{
    if (m_fmt_flags & zeropad) 
    {
	// If the 0 and -/= flags both appear, the 0 flag is ignored
	if ((m_ref_state.m_flags & ios::left) || (m_fmt_flags & centered))
	    m_fmt_flags &= ~zeropad;
	else 
	{ 
	    m_ref_state.m_fill = ct.widen ('0'); 
	    m_ref_state.m_flags |= ios::internal;
	}
    }
    if (m_fmt_flags & string_conv)
    {
	m_fmt_flags &= ~(spacepad|thousands_sep);
	// ignore custom precision for 'c' specification
	if ((m_fmt_flags & runtime_prec)
	    && (m_ref_state.m_precision == 1))
	    m_fmt_flags &= ~runtime_prec;
    }
    // If the 'space' and '+' flags both appear, the space flag is ignored
    // Spacepad works for signed conversions only
    else if (m_ref_state.m_flags & (ios::showpos|ios::hex|ios::oct))
	m_fmt_flags &= ~spacepad;
}

// ---------------------------------------------------------------------------
// format_base implementation
// ---------------------------------------------------------------------------

template <class Ch, class Tr, class Al>
atomic_type format_base<Ch,Tr,Al>::s_default_exceptions;

#ifdef EXT_FORMAT_USE_TABLE
template <class Ch, class Tr, class Al>
typename format_base<Ch,Tr,Al>::format_table format_base<Ch,Tr,Al>::s_table;
#endif

template <class Ch, class Tr, class Al>
format_base<Ch,Tr,Al>::format_base ()
    : m_oss(), m_state0 (m_oss)
    , m_ctype (&std::use_facet<ctype> (m_oss.getloc()))
    , m_format (0)
    , m_exceptions (s_default_exceptions)
{
}

template <class Ch, class Tr, class Al>
format_base<Ch,Tr,Al>::format_base (const std::locale& loc)
    : m_oss(), m_state0 (m_oss)
    , m_ctype (&std::use_facet<ctype> (loc))
    , m_format (0)
    , m_exceptions (s_default_exceptions)
{
    m_oss.imbue (loc);
}

template <class Ch, class Tr, class Al>
format_base<Ch,Tr,Al>::format_base (const format_base& other)
    : m_oss(), m_state0 (other.m_state0)
#ifdef EXT_FORMAT_USE_TABLE
    , m_format (other.m_format)
    , m_format_allocated (other.m_format_allocated)
#else
    , m_format (0)
    , m_format_compiled (other.m_format_compiled)
#endif
    , m_exceptions (other.m_exceptions)
{
    std::locale loc = other.m_oss.getloc();
    m_oss.imbue (loc);
    m_ctype = &std::use_facet<ctype> (loc);
    if (other.m_numpunct)
	m_numpunct.reset (new numpunct (loc, 1));
#ifndef EXT_FORMAT_USE_TABLE
    if (other.m_format)
	m_format = &m_format_compiled;
#endif
}

template <class Ch, class Tr, class Al>
format_base<Ch,Tr,Al>& format_base<Ch,Tr,Al>::
operator= (const format_base& other)
{
    if (this != &other)
    {
	std::locale loc = other.m_oss.getloc();
	m_oss.imbue (loc);
	m_ctype = &std::use_facet<ctype> (loc);
	if (other.m_numpunct)
	    m_numpunct.reset (new numpunct (loc, 1));

	m_state0 = other.m_state0;
#ifndef EXT_FORMAT_USE_TABLE
	if (other.m_format)
	{
	    m_format_compiled = other.m_format_compiled;
	    m_format = &m_format_compiled;
	}
	else
	    m_format = 0;
#else
	m_format_allocated = other.m_format_allocated;
	m_format = other.m_format;
#endif
	m_exceptions = other.m_exceptions;
    }
    return *this;
}

template <class Ch, class Tr, class Al> inline
unsigned format_base<Ch,Tr,Al>::exceptions (unsigned newexcept) 
{ 
    std::swap (m_exceptions, newexcept);
    return newexcept;
}

template <class Ch, class Tr, class Al> inline
unsigned format_base<Ch,Tr,Al>::set_default_exceptions (unsigned newexcept) 
{
    return (unsigned) atomic_swap (s_default_exceptions, static_cast<atomic_type> (newexcept));
}

template <class Ch, class Tr, class Al> inline
void format_base<Ch,Tr,Al>::begin_put (const format_state& state)
{
    io::detail::empty_buf (m_oss);
    m_oss.clear();
    state.apply_on (m_oss);
}

template <class Ch, class Tr, class Al> inline
void format_base<Ch,Tr,Al>::end_put ()
{
    m_state0.apply_on (m_oss);
}

// format::parse (SPEC_STR)
// Effects: find compiled format specification corresponding to SPEC_STR and
// initalize members accordingly.

template <class Ch, class Tr, class Al>
void format_base<Ch,Tr,Al>:: parse (const istring_type& spec_str)
{
    const char_type arg_mark = this->ctype_facet().widen ('%');
    if (traits_type::find (spec_str.data(), spec_str.size(), arg_mark))
    {
#ifndef EXT_FORMAT_USE_TABLE
	format_parser parser (this, arg_mark);
	parser.compile (spec_str, m_format_compiled);
	m_format = &m_format_compiled;
#else // EXT_FORMAT_USE_TABLE
#ifdef EXT_MT
	boost::mutex::scoped_try_lock lock (s_table.mutex);
	if (lock.owns_lock())
	{
#endif
	    typename map_type::iterator it = s_table.map.find (spec_str);
	    if (it == s_table.map.end())
	    {
		// it's cheaper to insert empty comp_format object and erase it on error,
		// than insert fully initialized comp_format in case of success.
		it = s_table.map.insert (std::make_pair (spec_str, comp_format())).first;
		try {
		    format_parser parser (this, arg_mark);
		    parser.compile (spec_str, it->second);
		} catch (...) {
		    s_table.map.erase (it);
		    throw;
		}
	    }
	    m_format = &it->second;
#ifdef EXT_MT
	}
	else // don't wait for lock, compile from scratch
	{
	    format_parser parser (this, arg_mark);
	    m_format_allocated.reset (new comp_format);
	    parser.compile (spec_str, *m_format_allocated);
	    m_format = m_format_allocated.get();
	}
#endif
#endif // EXT_FORMAT_USE_TABLE
    }
}

} // namespace detail

} // namespace ext

#endif /* EXT_FORMAT_INTERNALS_HPP */
