// -*- C++ -*-
//! \file       eucjpstr.cc
//! \date       2022 Jun 11
//! \brief      extract japanese text in EUC-JP encoding from binary file.
//

const size_t jis_table_size = 24064;
extern const unsigned short jis_0208_codepoints[];

#include "sysmemmap.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cctype>

typedef std::basic_string<uint8_t> bstring;

template <typename CharT>
class sequence
{
public:
    typedef std::vector<CharT>  seq_type;

    const CharT* data () const { return m_seq.data(); }

    const char* bytes () const { return reinterpret_cast<const char*> (m_seq.data()); }

    void push_back (CharT c) { m_seq.push_back (c); }

    size_t size () const { return m_seq.size(); }

    size_t bytes_size () const { return size() * sizeof(CharT); }

    bool empty () const { return m_seq.empty(); }

    void clear () { m_seq.clear(); }

private:
    seq_type    m_seq;
};

class jis_tokenizer
{
    std::ostream&           m_output;

    const size_t min_token_length = 2;

public:
    explicit jis_tokenizer (std::ostream& out) : m_output (out) { }
    jis_tokenizer () : jis_tokenizer (std::cout) { }

    static wchar_t get_unicode (uint16_t jis)
    {
        if (!(((jis & 0x8080) == 0x8080) && jis >= 0xA1A1 && jis < 0xFF00))
            return 0;
        return jis_0208_codepoints[jis - 0xA180];
    }

    static bool is_ascii (uint8_t c)
    {
        return c >= 0x20 && c < 0x7F;
    }

    void run (const uint8_t* begin, const uint8_t* end);

private:
    void add_char (wchar_t c);

    void flush_sequence ();
    void dump_sequence (const sequence<wchar_t>& target);

    sequence<wchar_t>   jis;
};

void jis_tokenizer::
run (const uint8_t* const begin, const uint8_t* const end)
{
    const size_t length = end - begin;
    size_t pos = 0;
    while (pos < length)
    {
        auto byte0 = begin[pos++];
        size_t avail = length - pos;
        if (byte0 < 0x20)
        {
            flush_sequence();
        }
        else if (is_ascii (byte0))
        {
            add_char (static_cast<wchar_t> (byte0));
        }
        else if (avail > 0)
        {
            auto byte1 = begin[pos];
            wchar_t chr = get_unicode (byte0 << 8 | byte1);
            if (chr)
            {
                pos++;
                add_char (chr);
            }
            else
            {
                flush_sequence();
                // skip the whole sequence since it's neither ascii nor jis
                while (pos < length && begin[pos])
                    ++pos;
            }
        }
    }
    flush_sequence();
}

inline void jis_tokenizer::
add_char (wchar_t c)
{
    jis.push_back (c);
}

void jis_tokenizer::
flush_sequence ()
{
    if (jis.size() >= min_token_length)
        dump_sequence (jis);
    if (!jis.empty())
        jis.clear();
}

void jis_tokenizer::
dump_sequence (const sequence<wchar_t>& target)
{
    m_output.write (target.bytes(), target.bytes_size());
    m_output.write ("\n", 2);
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::cout << "usage: eucjpstr INPUT OUTPUT\n";
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    sys::mapping::const_view<uint8_t> view (in);
    std::ofstream out (argv[2], std::ios::out|std::ios::trunc|std::ios::binary);
    wchar_t bom = L'\xFEFF';
    out.write (reinterpret_cast<char*> (&bom), 2);

    jis_tokenizer tok (out);
    tok.run (view.begin(), view.end());
    return 0;
}
catch (std::exception& X)
{
    std::cerr << "u16jpstr: " << X.what() << std::endl;
    return 1;
}
