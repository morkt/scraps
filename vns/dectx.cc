// -*- C++ -*-
//! \file       dectx.cc
//! \date       2018 Jan 13
//! \brief      decrypt CTX scripts (Uncanny!)
//

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"

namespace {
    void decrypt (uint8_t* data, size_t size)
    {
        unsigned key = 0x4B5AB4A5;
        for (size_t i = 0; i < size; ++i)
        {
            uint8_t x = key ^ data[i];
            data[i] = x;
            key = ((key << 9) | (key >> 23) & 0x1F0) ^ x;
        }
    }
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: dectx INPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1]);
    if (in.size() < 2)
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
    sys::mapping::view<uint8_t> view (in);
    decrypt (view.data(), view.size());
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
