// -*- C++ -*-
//! \file       decdsf.cc
//! \date       Sat Feb 20 16:33:06 2016
//! \brief      decrypt Crime Rhyme DSF script file.

#include "sysmemmap.h"
#include <cstdio>

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
        return 0;
    sys::mapping::readwrite in (argv[1]);
    sys::mapping::view<uint8_t> view (in);
    if (view.size() < 2)
        return 0;
    auto last = view.end()-1;
    if (0xA == *last)
        return 0;
    uint8_t key = *last ^ 0xA;
    if (0xD != (last[-1] ^ key))
    {
        std::fprintf (stderr, "%S: key guess failed\n", argv[1]);
        return 1;
    }
    for (auto p = view.begin(); p != view.end(); ++p)
        *p ^= key;
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
