// -*- C++ -*-
//! \file       dehyptext.cc
//! \date       2023 Aug 29
//! \brief      decrypt HyperWorks TEXT resources.
//

#include "sysmemmap.h"
#include <cstdio>
#include <fstream>

uint16_t decrypt_char (int ax)
{
    uint8_t ah = ax >> 8;
    uint8_t al = ax;
    if (ah < 0x21 || ah > 0x7F || al < 0x21 || al > 0x7F)
        return 0;
    ax -= 0xDE82;
    ah = ax >> 9;
    al = ax;
    if (!(ax & 0x100))
    {
        al -= 0x5E + (al < 0xDE);
    }
    ah ^= 0x20;
    return ah << 8 | al;
}

uint16_t decrypt_char_2 (int ax)
{
    uint8_t ah = ax >> 8;
    uint8_t al = ax;
    if (ah < 0x21 || ah > 0x7F || al < 0x21 || al > 0x7F)
        return 0;
    int lo;
    if (ax & 0x100)
    {
        lo = al + 0x1F;
        if (lo >= 0x7F)
            lo = al + 0x20;
    }
    else
    {
        lo = al + 0x7E;
    }
    int hi = (ah + 0xE1) >> 1;
    if (hi >= 0xA0)
        hi += 0x40;
    return (lo | hi << 8) & 0xFFFF;
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: dehyptext INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);

    if (view.size() < 4 || *reinterpret_cast<uint16_t*> (view.data()) != 0)
    {
        std::fprintf (stderr, "%S: invalid input file.\n", argv[1]);
        return 1;
    }
    char buffer[2];
    std::ofstream out (argv[2], std::ios::out|std::ios::trunc);
    size_t sz = *reinterpret_cast<uint16_t*> (view.data());
    if (sz > view.size())
    {
        std::fprintf (stderr, "%S: invalid input file.\n", argv[1]);
        return 1;
    }
    auto src = view.data() + 2 + sz;
    while (src + 1 < view.end())
    {
        auto word = *reinterpret_cast<uint16_t*> (src);
        if (0x7621 == word || 0x7622 == word)
        {
            out << "\\x" << std::hex << word << '\n';
            src += 2;
            continue;
        }
        if (*src > 0x7F)
        {
            if (*src == 0xB1)
            {
                src += 2;
                size_t skip = *reinterpret_cast<uint16_t*> (src);
                if (skip > view.end() - src)
                {
                    std::fprintf (stderr, "%S: invalid control sequence at %08X\n", argv[1], src- view.data());
                    return 2;
                }
                src += 2 + skip;
                continue;
            }
            std::fprintf (stderr, "%S: invalid sequence at %08X\n", argv[1], src - view.data());
            return 2;
        }
        if (!*src)
        {
            out.put ('\n');
            if (!src[1])
            {
                std::fprintf (stderr, "%S: end of data at %08X\n", argv[1], src - view.data());
                break;
            }
            ++src;
            continue;
        }
        uint16_t ax = *reinterpret_cast<uint16_t*> (src);
        ax = decrypt_char (ax);
        if (ax)
        {
            buffer[0] = ax >> 8;
            buffer[1] = ax;
            out.write (buffer, 2);
        }
        else
        {
            std::fprintf (stderr, "%S: ignored sequence at %08X\n", argv[1], src - view.data());
        }
        src += 2;
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}
