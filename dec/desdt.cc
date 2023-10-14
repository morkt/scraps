// -*- C++ -*-
//! \file       desdt.cc
//! \date       2023 Sep 29
//! \brief      
//

#include "bytecode.h"
#include <vector>
#include <cstdio>

class sdt_reader : public bytecode_reader
{
    std::vector<char>       buffer;

    bool        flag_7C30;
    uint16_t    word_7C32;

public:
    sdt_reader (sys::mapping::map_base& in)
        : bytecode_reader (in)
    {
        buffer.reserve (1024);
    }

protected:
    bool run () override;

private:
    void parse_text (bool flush = true);
    void flush_text ();
    uint16_t convert_char (uint16_t chr);
    void put_char (uint16_t chr);
    void add_text (const char* str);

    void opcode_30 ();
    void opcode_E0 ();
    void opcode_file (uint8_t op);
    void opcode_call (uint8_t op);
    void parse_string();
};

bool sdt_reader::
do_run ()
{
    pBytecode = pBytecodeStart;
    while (pBytecode < view.end())
    {
        const auto currentPos = pBytecode;
        auto bytecode = get_byte();
        switch (bytecode)
        {
        case 0x10:
        case 0x18:
            parse_text();
            flag_7C30 = false;
            break;

        case 0x1B:
        case 0xF0:
        case 0xF1:
            break;

        case 0x30:
            opcode_30();
            break;

        case 0x50:
        case 0x53:
            opcode_call (bytecode);
            pBytecode += 2;
            break;

        case 0x80:
        case 0x81:
        case 0xB0:
            opcode_file (bytecode);
            break;

        case 0x8A:
            ++pBytecode;
            opcode_file (bytecode);
            break;

        case 0x12:
        case 0x13:
        case 0x82:
        case 0x84:
        case 0x92:
        case 0xA0:
        case 0xA1:
        case 0xFA:
            pBytecode += 1;
            break;

        case 0x20:
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x45:
        case 0x85:
        case 0x87:
        case 0x8B:
        case 0xE1:
        case 0xE2:
            pBytecode += 2;
            break;

        case 0x90:
            pBytecode += 5;
            break;

        case 0x91:
            pBytecode += 3;
            break;

        case 0xE0:
            opcode_E0();
            break;

        default:
            throw bytecode_error (currentPos - view.data(), bytecode);
        }
    }
    return true;
}

void sdt_reader::
opcode_30 ()
{
    std::cout << put_offset(pBytecode-1) << hex(0x30);
    auto a1 = get_byte();
    auto a2 = get_byte();
    std::cout << ' ' << hex(a1) << ' ' << hex(a2);
    while (*pBytecode != 0xFF)
    {
        std::cout << ' ' << hex(*pBytecode);
        pBytecode += 4;
        std::cout << ' ' << hex(get_byte());
        parse_text (false);
        std::cout << " <";
        std::cout.write (buffer.data(), buffer.size());
        std::cout << ">";
        buffer.clear();
    }
    ++pBytecode;
    std::cout << '\n';
}

void sdt_reader::
opcode_call (uint8_t op)
{
    std::clog << put_offset(pBytecode-1) << "CALL[" << hex(op) << ']';
    do
    {
        auto code = get<uint8_t> (pBytecode);
        std::clog << ' ' << hex(code);
        pBytecode += 3;
    }
    while (*pBytecode++);
    std::clog << '\n';
}

void sdt_reader::
opcode_file (uint8_t op)
{
    auto code = get_byte();
    parse_string();
    std::cout << '#' << hex(op) << ' ' << hex(code) << ' ';
    flush_text();
}

void sdt_reader::
opcode_E0 ()
{
    parse_string();
    std::cout << '#' << hex(0xE0) << ' ';
    flush_text();
}

void sdt_reader::
parse_string ()
{
    buffer.clear();
    while (pBytecode < view.end())
    {
        auto chr = get<char> (pBytecode++);
        if (!chr)
            break;
        buffer.push_back (chr);
    }
}

void sdt_reader::
parse_text (bool flush)
{
    while (pBytecode < view.end())
    {
        const auto currentPos = pBytecode;
        uint16_t code = get_byte();
        if (0 == code)
            break;
        if (0x20 == code && !word_7C32)
        {
            code = get_byte();
            if ('c' == code || 'C' == code)
            {
                ++pBytecode;
            }
            else if ('$' == code)
            {
//                std::cout << "$\n";
            }
            else if ('w' == code || 'W' == code)
            {
                int n = (get_byte() - 0x30) * 10;
                std::cout << "#W " << std::dec << n << '\n';
            }
            else if ('p' == code || 'P' == code)
            {
                std::cout << "#P\n";
            }
            else if ('r' == code || 'R' == code)
            {
                int x = (get_byte() - 0x30) << 1;
                std::cout << "#R " << std::dec << x << '\n';
            }
            else if ('x' == code || 'X' == code)
            {
//                std::cout << "#X\n";
                word_7C32 = 1;
            }
            else
                --pBytecode;
            continue;
        }
        if (0x7E == code && !word_7C32)
        {
//            add_text ("<7E>");
            flag_7C30 = false;
        }
        else if (0x7F == code && !word_7C32)
        {
//            add_text ("<7F>");
            flag_7C30 = true;
        }
        else if (word_7C32)
        {
            add_text ("***");
            word_7C32 = 0;
            --pBytecode;
            continue;
        }
        else if (0x7D == code) // BITBLT
            pBytecode += 1;
        else if (code >= 0x21 && code < 0x7D)
        {
            if (flag_7C30)
                code += 0x2500;
            else
                code += 0x2400;
            auto sjis = convert_char (code);
            put_char (sjis);
        }
        else
        {
            code = code << 8 | get_byte();
            code ^= 0x0A0A;
            put_char (code);
        }
    }
    if (flush)
        flush_text();
}

uint16_t sdt_reader::
convert_char (uint16_t chr)
{
    uint8_t al = chr >> 8;
    uint8_t ah = chr;
    al -= 0x21;
    uint8_t bit = al & 1;
    al >>= 1;
    if (bit)
        ah += 0x5E;
    ah += 0x1F + (ah >= 0x60);
    al += 0x81;
    if (al >= 0xA0)
        al += 0x40;
    uint16_t result = al << 8 | ah;
    return result;
}

void sdt_reader::
put_char (uint16_t chr)
{
    if (chr > 0xFF)
        buffer.push_back (static_cast<char> (chr >> 8));
    buffer.push_back (static_cast<char> (chr));
}

void sdt_reader::
add_text (const char* str)
{
    size_t len = std::strlen (str);
    buffer.insert (buffer.end(), str, str + len);
}

void sdt_reader::
flush_text ()
{
    std::cout.write (buffer.data(), buffer.size()).put ('\n');
    buffer.clear();
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: desdt INPUT");
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    sdt_reader reader (in);
    return reader.run() ? 0 : 1;
}
catch (bytecode_error& X)
{
    std::fprintf (stderr, "%S:%04X: %s\n", argv[1], X.get_pos(), X.what());
    return 1;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}
