// -*- C++ -*-
//! \file       deomi.cc
//! \date       2023 Sep 04
//! \brief      decrypt OMI-ScriptEngine files.

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"

void decrypt (uint8_t* data, size_t length, uint32_t key)
{
    for (size_t i = 0; i < length; ++i)
    {
        data[i] = ((data[i] << 7) | (data[i] >> 1)) - key;
        key = 5 * key - 3;
    }
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: deomi INPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1]);
    sys::mapping::view<uint8_t> view (in);
    decrypt (view.data(), view.size(), 7654321);
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
