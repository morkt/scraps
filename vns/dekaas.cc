// -*- C++ -*-
//! \file       dekaas.cc
//! \date       2023 Sep 08
//! \brief      decrypt KAAS game script.
//

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: dekaas INPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1]);
    sys::mapping::view<uint8_t> view (in);
    if (view.size() < 4)
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
    size_t pos = *reinterpret_cast<uint32_t*> (view.data());
    size_t count = *reinterpret_cast<uint32_t*> (&view[4]);
    if (pos >= view.size() || count <= pos || pos * 4 >= view.size())
    {
        std::fprintf (stderr, "%S: invalid input [1]\n", argv[1]);
        return 1;
    }
    count = count * 2 - pos * 2;
    if (pos * 4 > view.size() - count)
    {
        std::fprintf (stderr, "%S: invalid input [2]\n", argv[1]);
        return 1;
    }
    uint16_t* data = reinterpret_cast<uint16_t*> (view.data() + 4 * pos);
    do
    {
        *data++ ^= 1;
    }
    while (--count > 0);
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
