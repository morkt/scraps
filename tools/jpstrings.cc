// -*- C++ -*-
//! \file       jpstrings.cc
//! \date       2017 Nov 28
//! \brief      extract strings in Shift-JIS encoding from binary file.
//

#include <string>
#include <iostream>
#include "sysmemmap.h"

extern const unsigned short shift_jis_codepoints[];

#define _   symbol_state::invalid
#define S   symbol_state::sbs
#define M   symbol_state::mbs

enum class symbol_state : uint8_t {
    invalid,
    sbs,
    mbs,
};

const symbol_state first_map[] = {
//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // 0
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // 1
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 2
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 3
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 4
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 5
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 6
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, _, // 7 
    _, M, M, M, _, _, _, M, M, M, M, M, M, M, M, M, // 8
    M, M, M, M, M, M, M, M, M, M, M, M, M, M, M, M, // 9
    _, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // A
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // B
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // C
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // D
    M, M, M, M, M, M, M, M, M, M, M, _, _, _, _, _, // E
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // F
};

const symbol_state second_map[] = {
//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // 0
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // 1
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // 2
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // 3
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 4
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 5
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 6
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, _, // 7 
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 8
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 9
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // A
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // B
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // C
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // D
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // E
    S, S, S, S, S, S, S, S, S, S, S, S, S, _, _, _, // F
};

#undef _
#undef S
#undef M

typedef std::basic_string<uint8_t> bstring;

class jp_tokenizer
{
    enum parse_state
    {
        no_char,
        sb_char,
        mb_char,
    };

    parse_state     m_state;
    std::ostream&   m_output;

public:
    jp_tokenizer (std::ostream& out) : m_state (no_char), m_output (out) { }
    jp_tokenizer () : jp_tokenizer (std::cout) { }

    void run (const uint8_t* begin, const uint8_t* end);

    const size_t min_token_length = 2;

private:
    void add_sb_symbol (uint8_t symbol);
    void add_mb_symbol (uint8_t symbol);
    void end_token();

    size_t      symbol_count;
    bstring     cur_token;
    uint8_t     prev_symbol;
};

void jp_tokenizer::
run (const uint8_t* begin, const uint8_t* end)
{
    while (begin < end)
    {
        auto cur_symbol = *begin++;
        symbol_state symbol_class = symbol_state::invalid;
        switch (m_state)
        {
        case no_char:
        case sb_char:
            symbol_class = first_map[cur_symbol];
            break;

        case mb_char:
            symbol_class = second_map[cur_symbol];
            break;
        }
        switch (symbol_class)
        {
        case symbol_state::invalid:
            end_token();
            break;
        case symbol_state::sbs:
            add_sb_symbol (cur_symbol);
            break;
        case symbol_state::mbs:
            add_mb_symbol (cur_symbol);
            break;
        }
    }
    end_token();
}

void jp_tokenizer::
add_sb_symbol (uint8_t symbol)
{
    if (mb_char == m_state)
    {
        int sjis_code = prev_symbol << 8 | symbol;
        if (sjis_code < 0x8100 || !shift_jis_codepoints[sjis_code - 0x8100])
        {
            end_token();
            return;
        }
        cur_token.push_back (prev_symbol);
    }
    prev_symbol = symbol;
    cur_token.push_back (symbol);
    ++symbol_count;
    m_state = sb_char;
}

inline void jp_tokenizer::
add_mb_symbol (uint8_t symbol)
{
    prev_symbol = symbol;
    m_state = mb_char;
}

void jp_tokenizer::
end_token ()
{
    if (cur_token.length() >= min_token_length)
    {
        m_output.write (reinterpret_cast<const char*> (cur_token.data()), cur_token.size());
        m_output.put ('\n');
    }
    symbol_count = 0;
    cur_token.clear();
    m_state = no_char;
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::cout << "usage: jpstrings FILE\n";
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    sys::mapping::const_view<uint8_t> view (in);

    jp_tokenizer tok;
    tok.run (view.begin(), view.end());
    return 0;
}
catch (std::exception& X)
{
    std::cerr << "jpstrings: " << X.what() << std::endl;
    return 1;
}
