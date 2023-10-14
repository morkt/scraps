// -*- C++ -*-
//! \file       deqdo.cc
//! \date       2023 Sep 18
//! \brief      decrypt QDO scripts.
//

#include <cstdio>
#include "sysmemmap.h"

// [010706][Red-Zone] Kenkyuu Nisshi

void decrypt (uint8_t* data, size_t size)
{
    for (auto end = data+size; data != end; ++data)
    {
        *data = ~(*data - 13);
    }
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: deqdo INPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1]);
    sys::mapping::view<uint8_t> view (in);
    if (in.size() < 0xF || 0 != std::memcmp (view.data(), "QDO_SHO", 8))
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
    auto data = view.data()+0xE;
    size_t size = view.size()-0xE;
    if (view[0xC])
    {
        decrypt (data, size);
        view[0xC] = 0;
    }
    else
        std::printf ("%S: not encrypted\n", argv[1]);
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
