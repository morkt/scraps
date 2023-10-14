// -*- C++ -*-
//! \file       decscr.cc
//! \date       Wed Sep 14 07:43:17 2016
//! \brief      decrypt ScrPlayer scripts.
//

#include "sysmemmap.h"
#include <cstdio>
#include <fstream>

// SCR:1001 -> [990709][Love Gun] Koi no Sweet Tart wa Ikaga?
// [040319][smart] Fortune Cookie
// [070330][Melty Koubou] Rasetsu no Koku
// [031226][PANDAHOUSE] Stray Sheep ~Chijoku no Zangeshitsu~
// SCR:0004 -> [050225][PANDAHOUSE] Last Story wa Anata e.

int main (int argc, char* argv[])
try
{
    if (argc < 3)
    {
        std::printf ("usage: decscr SCRIPT OUTPUT\n");
        return 0;
    }
    const char* out_name = NULL;
    if (argc > 2)
        out_name = argv[2];
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    // tested with
    // SCR:4002
    // SCR:4007
    int version = 0;
    size_t base = 0;
    if (view.size() > 0x14)
    {
        if (0 == std::memcmp (view.data(), "SCR:1001", 8))
        {
            version = 1001;
        }
        else if (0 == std::memcmp (view.data(), "SCR:400", 7) ||
                 0 == std::memcmp (view.data(), "SCR:0004", 8))
        {
            version = 400;
            base = 0x14;
        }
        else if (0 == std::memcmp (view.data(), "SCR\x1C", 4))
        {
            version = 0x1C;
            base = 0x12;
        }
    }
    if (!version)
    {
        std::fprintf (stderr, "%s: invalid script\n", argv[1]);
        return 1;
    }
    size_t offset;
    if (400 == version)
        offset = *reinterpret_cast<uint32_t*> (&view[0x10]);
    else if (1001 == version)
        offset = *reinterpret_cast<uint16_t*> (&view[0x10]) * 10 + 0x12;
    else
        offset = *reinterpret_cast<uint16_t*> (&view[0xC]) * 8;
    offset += base;
    if (offset >= view.size())
    {
        std::fprintf (stderr, "%s: invalid script\n", argv[1]);
        return 1;
    }
    auto ptr = view.begin() + offset;
    size_t size = view.size() - offset;
    if (400 == version || 1001 == version)
    {
        size = *reinterpret_cast<uint32_t*> (ptr);
        ptr += 4;
        if (size > view.end() - ptr)
            return 0;
    }
    auto start = ptr;
    for (size_t i = 0; i < size; ++i)
        ptr[i] ^= 0x7F;

    std::ofstream out (out_name, std::ios::out|std::ios::binary|std::ios::trunc);
    if (out)
        out.write (reinterpret_cast<char*> (start), size);
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s: %s\n", argv[1], X.what());
    return 1;
}
