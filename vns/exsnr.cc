// -*- C++ -*-
//! \file       exsnr.cc
//! \date       2018 Sep 05
//! \brief      decompress lzss-compressed snr scripts.
//

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"
#include "lzss.h"

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: exsnr INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    sys::mapping::const_view<uint8_t> view (in);
    if (view.size() <= 0x10 || 0 != std::memcmp (view.data(), "snr", 4))
    {
        std::fprintf (stderr, "%S: invalid snr file\n", argv[1]);
        return 1;
    }
    std::ofstream out (argv[2], std::ios::out|std::ios::binary|std::ios::trunc);
    if (!out)
    {
        std::fprintf (stderr, "%S: error opening output file\n", argv[2]);
        return 1;
    }
    lzss_decompress (view.data()+0x10, view.size()-0x10, out);
//   lzss_decompress (view.data()+8, view.size()-8, out);
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s: %s\n", argv[1], X.what());
    return 1;
}


