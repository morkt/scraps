// -*- C++ -*-
//! \file       deags32i.cc
//! \date       2018 Mar 28
//! \brief      decrypt AGS32i files.
//

#include <cstdint>
#include <fstream>
#include <cstdio>
#include "sysmemmap.h"

const uint32_t g_default_key = 0x20041001;

inline unsigned rotL (unsigned v, int count)
{
    count &= 0x1F;
    return v << count | v >> (32-count);
}

void decrypt (uint8_t* data, size_t size, uint32_t key)
{
    for (size_t i = 0; i < size; i += 4)
    {
        int rem = (i >> 2) % 31;
        int div = (i >> 2) / 31;
        unsigned t = rotL (key + div, rem);
        size_t chunk = std::min (4u, size-i);
        for (int j = 0; j < chunk; ++j)
            data[i+j] ^= t >> (j << 3);
    }
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: deags32i INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    if (in.size() < 4)
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
