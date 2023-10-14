// -*- C++ -*-
//! \file       demsf.cc
//! \date       Sun Jan 22 04:43:00 2017
//! \brief      decrypt Malie MSF scripts
//

#include "sysmemmap.h"

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
        return 0;
    sys::mapping::readwrite in (argv[1]);
    if (in.size() < 2)
        return 0;
    sys::mapping::view<uint8_t> view (in);
    uint8_t key = view[view.size()-1];
    auto end = view.end() - 2;
    for (auto p = view.begin(); p != end; ++p)
        *p ^= key;
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}
