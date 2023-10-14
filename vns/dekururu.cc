// -*- C++ -*-
//! \file       dekururu.cc
//! \date       2018 Feb 01
//! \brief      decrypt Majokko-roid Kururu-chan scripts
//

#include "sysmemmap.h"
#include <cstdio>

int main (int argc, char* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: dekururu FILE");
        return 0;
    }
    sys::mapping::readwrite in (argv[1]);
    sys::mapping::view<uint8_t> view (in);
    for (auto ptr = view.begin(); ptr != view.end(); ++ptr)
    {
        uint8_t v = *ptr ^ 0x39;
        *ptr = v >> 3 | v << 5;
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s: %s\n", argv[1], X.what());
    return 1;
}
