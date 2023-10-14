// -*- C++ -*-
//! \file       desndat.cc
//! \date       2018 Aug 17
//! \brief      decrypt SN.DAT
//
// [010525][Pochette] Uzukihime

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: desndat INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    for (auto p = view.begin(); p != view.end(); ++p)
    {
        if (*p > 0x20 && *p < 0xDF)
            *p ^= 0xFF;
    }
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
