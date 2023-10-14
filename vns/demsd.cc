// -*- C++ -*-
//! \file       demsd.cc
//! \date       2018 Jun 06
//! \brief      decrypt Muse engine script files (MSD)
//

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"

namespace {
    void decrypt (uint8_t* data, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
        {
            auto sym = data[i];
            if (sym >= 0x80 && sym < 0xA0)
                data[i] += 0x60;
            else if (sym >= 0xE0)
                data[i] -= 0x60;
            else if (sym >= 0x20 && sym < 0x60)
                data[i] += 0x80;
            else if (sym >= 0xA0 && sym < 0xE0)
                data[i] += 0x80;
        }
    }
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: demsd INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    decrypt (view.data(), view.size());
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
