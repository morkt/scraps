// -*- C++ -*-
//! \file       decsrp.cc
//! \date       Fri Mar 18 22:55:58 2016
//! \brief      decrypt srp scripts from Tmr-Hiro engine.
//

#include "sysmemmap.h"

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: decsrp FILE");
        return 0;
    }
    sys::mapping::readwrite in (argv[1]);
    if (in.size() < 5)
        throw std::runtime_error ("invalid SRP script");
    sys::mapping::view<uint8_t> view (in);
    for (uint8_t* p = view.begin() + 4; p != view.end(); ++p)
    {
        if ('\n' == *p)
            continue;
        uint8_t v = *p ^ 0xA;
        *p = v << 4 | v >> 4;
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}
