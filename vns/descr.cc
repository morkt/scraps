// -*- C++ -*-
//! \file       descr.cc
//! \date       2018 Oct 24
//! \brief      Lune Adv System script decoder.
//
// [010413][Sarang] Tokyo Yuuyuu
//

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: deadx INPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1]);
    if (in.size() < 2)
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
    sys::mapping::view<uint16_t> view (in);
    for (auto& x : view)
    {
        if (x <= 0xFF00)
            x = (((x - 0x7EC1) >> 8) & 0xFF) + ((x + 0x813F) << 8);
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
