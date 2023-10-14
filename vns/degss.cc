// -*- C++ -*-
//! \file       degss.cc
//! \date       2018 Oct 13
//! \brief      decrypt Agsi32 GSS files.
//

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"

unsigned g_default_key = 0x20041105;

namespace {
    unsigned rotl (unsigned arg, int shift)
    {
        return arg << shift | arg >> (32 - shift);
    }

    void decrypt (uint8_t* data, size_t size, unsigned key)
    {
        for (size_t i = 0; i < size; )
        {
            int rem = (i >> 2) % 31;
            unsigned div = (i >> 2) / 31;
            unsigned t = rotl (key + div, rem);
            data[i++] ^= t;
            data[i++] ^= t >> 8;
            data[i++] ^= t >> 16;
            data[i++] ^= t >> 24;
        }
    }
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: degss INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    if (in.size() < 2)
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
    sys::mapping::view<uint8_t> view (in);
    decrypt (view.data(), view.size(), g_default_key);
    std::ofstream out (argv[2], std::ios::out|std::ios::binary|std::ios::trunc);
    if (!out)
    {
        std::fprintf (stderr, "%S: error opening output file\n", argv[2]);
        return 1;
    }
    out.write (reinterpret_cast<char*> (view.data()), view.size());
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
