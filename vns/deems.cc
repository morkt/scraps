// -*- C++ -*-
//! \file       deems.cc
//! \date       2019 Jan 04
//! \brief      decrypt Studio B-Room EMS file.
//

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"

const uint8_t g_ems_key[] = {
    0x01, 0x07, 0x0D, 0x40, 0xA0, 0x11, 0x02, 0x08, 0x0E, 0x50, 0xB0, 0x22, 0x03, 0x09,
    0x0F, 0x60, 0xC0, 0x33, 0x04, 0x0A, 0x10, 0x70, 0xD0, 0x44, 0x05, 0x0B, 0x20, 0x80,
    0xE0, 0x55, 0x06, 0x0C, 0x30, 0x90, 0xF0, 0x66
};

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: deems INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    if (in.size() < 52)
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
    sys::mapping::view<uint8_t> view (in);
    size_t k = 0;
    for (size_t i = 52; i < view.size(); ++i)
    {
        view[i] ^= g_ems_key[k++];
        if (k >= sizeof(g_ems_key))
            k = 0;
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
