// -*- C++ -*-
//! \file       u16jpstr.cc
//! \date       2022 Jun 02
//! \brief      extract japanese text in UTF-16 encoding from binary file.
//

const size_t sjis_table_size = 32512;
extern const unsigned short shift_jis_codepoints[];

#include "sysmemmap.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cctype>

typedef std::basic_string<uint8_t> bstring;

enum class codepoint : uint8_t {
    invalid = 0,
    ascii   = 0x01,
    jp      = 0x02,
};

inline codepoint operator| (codepoint a, codepoint b)
{
    return codepoint (static_cast<uint8_t> (a) | static_cast<uint8_t> (b));
}

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

class u16_tokenizer
{
    std::ostream&           m_output;

    static const uint8_t* const s_utf16_codepoints;
    static const uint8_t* init_utf16_codepoints ();

    const size_t min_token_length = 2;

public:
    explicit u16_tokenizer (std::ostream& out) : m_output (out) { }
    u16_tokenizer () : u16_tokenizer (std::cout) { }

    static bool is_jp_utf16 (wchar_t c)
    {
        return s_utf16_codepoints[c] & static_cast<uint8_t> (codepoint::jp);
    }

    static bool is_ascii (uint8_t c)
    {
        return s_utf16_codepoints[c] & static_cast<uint8_t> (codepoint::ascii);
    }

    static bool is_range_ascii (const uint8_t* begin, const uint8_t* end)
    {
        while (begin != end)
        {
            if (!is_ascii (*begin++))
                return false;
        }
        return true;
    }

    void run (const uint8_t* begin, const uint8_t* end);

private:
    void add_u16 (sequence<wchar_t>& target, wchar_t c);

    void flush_sequence (sequence<wchar_t>& target);
    void dump_sequence (const sequence<wchar_t>& target);

    // u16[0]: even-aligned utf-16le sequence
    // u16[1]: odd-aligned utf-16le sequence
    // u16[3]: single byte ascii sequence
    sequence<wchar_t>   u16[3];
};

const uint8_t* const u16_tokenizer::s_utf16_codepoints = init_utf16_codepoints();

const uint8_t* u16_tokenizer::init_utf16_codepoints ()
{
    static uint8_t table[0x10000];
    for (size_t i = 0; i < sjis_table_size; ++i)
    {
        auto codepoint = shift_jis_codepoints[i];
        if (codepoint)
            table[codepoint] |= static_cast<uint8_t> (codepoint::jp);
    }
    for (wchar_t c = ' '; c < 0x7F; ++c)
        table[c] |= static_cast<uint8_t> (codepoint::jp) | static_cast<uint8_t> (codepoint::ascii);
    return table;
}

void u16_tokenizer::
run (const uint8_t* const begin, const uint8_t* const end)
{
    const size_t length = end - begin;
    size_t pos = 0;
    while (pos < length)
    {
        const size_t current_pos = pos;
        auto byte0 = begin[pos++];
        add_u16 (u16[2], byte0);
        size_t avail = length - pos;
        if (avail > 0)
        {
            auto byte1 = begin[pos];
            wchar_t w0 = static_cast<wchar_t> (byte1 << 8 | byte0);
            add_u16 (u16[current_pos & 1], w0);
        }
    }
    if (u16[0].size() >= u16[1].size() && u16[0].bytes_size() >= u16[2].size())
    {
        if (u16[0].size() >= min_token_length)
            dump_sequence (u16[0]);
    }
    else if (u16[1].bytes_size() >= u16[2].size())
    {
        if (u16[1].size() >= min_token_length)
            dump_sequence (u16[1]);
    }
    else if (u16[2].size() >= min_token_length)
        dump_sequence (u16[2]);
}

void u16_tokenizer::
add_u16 (sequence<wchar_t>& target, wchar_t c)
{
    if (is_jp_utf16 (c))
    {
        target.push_back (c);
    }
    else if (target.size() >= min_token_length)
    {
        flush_sequence (target);
    }
    else if (!target.empty())
    {
        target.clear();
    }
}

void u16_tokenizer::
flush_sequence (sequence<wchar_t>& target)
{
    if (&target == &u16[2])
    {
        if (target.size() > u16[0].bytes_size() && target.size() > u16[1].bytes_size())
        {
            dump_sequence (target);
        }
    }
    else
    {
        size_t other = &target == &u16[0] ? 1 : 0;
        if (target.size() >= u16[other].size())
        {
            dump_sequence (target);
        }
    }
    target.clear();
}

void u16_tokenizer::
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
        std::cout << "usage: u16jpstr INPUT OUTPUT\n";
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    sys::mapping::const_view<uint8_t> view (in);
    std::ofstream out (argv[2], std::ios::out|std::ios::trunc|std::ios::binary);
    wchar_t bom = L'\xFEFF';
    out.write (reinterpret_cast<char*> (&bom), 2);

    u16_tokenizer tok (out);
    tok.run (view.begin(), view.end());
    return 0;
}
catch (std::exception& X)
{
    std::cerr << "u16jpstr: " << X.what() << std::endl;
    return 1;
}
