/* -*- C++ -*-
 * File:        utility.hpp
 * Created:     Tue Mar 20 08:21:32 2007
 * Description: utility functions.
 *
 * $Id$
 */

#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <ios>	// for std::basic_stream

namespace ext {

// ---------------------------------------------------------------------------
// std::min and std::max pass arguments by reference, introducing redundant
// overhead for builtin types.

template <class Tp>
inline Tp min (Tp lhs, Tp rhs) { return lhs < rhs? lhs: rhs; }

template <class Tp>
inline Tp max (Tp lhs, Tp rhs) { return rhs < lhs? lhs: rhs; }

// ---------------------------------------------------------------------------
// I/O helpers

namespace io { namespace detail {

using std::ios_base;
using std::basic_ios;
using std::basic_streambuf;

// class fail_guard
// helper class for exception-safe stream i/o errors handling.
// This class intended for use in custom operator<< and operator>> implementations.
// Create object of this class on stack and call clear() method after input/output
// successfully finished.  Call addstate(ios_base::eofbit) when end of file reached.
// Upon function exit, either by return statement or thrown exception, fail_guard
// transfers its state to controlled stream.

template <class CharT, class Traits = std::char_traits<CharT> >
class fail_guard
{
public:
    typedef basic_ios<CharT, Traits>	ios_type;
    typedef typename ios_type::iostate	iostate;

    fail_guard (ios_type& s) : m_stream (s), m_state (ios_base::failbit) { }
    ~fail_guard () { m_stream.setstate (m_state); }

    // clear state of controlled stream (set goodbit)
    void clear () { m_state = ios_base::goodbit; }

    // set controlled stream state to STATE
    void setstate (iostate state) { m_state = state; }

    // add STATE bits to controlled stream state
    void addstate (iostate state) { m_state |= state; }

private:
    ios_type&		m_stream;
    iostate		m_state;
};

template <class Ch, class Tr>
bool write_chars_aux (basic_streambuf<Ch,Tr>* sbuf, std::streamsize n, Ch chr)
{
    const size_t buf_bit_size = 6;
    const std::streamsize buf_size = 1 << buf_bit_size;
    const size_t buf_mask = buf_size-1;
    Ch buf[buf_size];
    std::fill_n (buf, min (n, buf_size), chr);
    if (std::streamsize portion = n & buf_mask)
	if (portion != sbuf->sputn (buf, portion))
	    return false;
    for (size_t count = static_cast<size_t> (n >> buf_bit_size); count; --count)
	if (buf_size != sbuf->sputn (buf, buf_size))
	    return false;
    return true;
}

// write_char (SBUF, N, CHR)
// Effects: write N copies of character CHR into streambuffer SBUF.
// Returns: true if write succeeded, false otherwise.

template <class Ch, class Tr> inline
bool write_char (basic_streambuf<Ch,Tr>* sbuf, std::streamsize n, Ch chr)
{
    if (n > 0)
    {
	if (Tr::eq_int_type (sbuf->sputc (chr), Tr::eof()))
	    return false;
	if (--n)
	    return write_chars_aux (sbuf, n, chr);
    }
    return true;
}

} // namespace detail

using detail::fail_guard;

} // namespace io

// ---------------------------------------------------------------------------
// C string equality

template <class CharT>
struct eq_c_str
{
    bool operator() (const CharT* lhs, const CharT* rhs) const
	{
            do
                if (*lhs != *rhs++)
                    return false;
            while (*lhs++);
            return true;
	}
};

} // namespace ext

#endif /* UTILITY_HPP */
