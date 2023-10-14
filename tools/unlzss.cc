// -*- C++ -*-
//! \file       unlzss.cc
//! \date       Wed Jul 01 00:11:55 2015
//! \brief      LZSS-decopress file.
//

#include <fstream>
#include <iostream>
#include "sysmemmap.h"
#include "lzss.h"

int main (int argc, char* argv[])
try
{
    if (argc < 3)
    {
        std::cout << "usage: unlz INPUT OUTPUT\n";
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    sys::mapping::const_view<uint8_t> view (in);
    std::ofstream out (argv[2], std::ios::out|std::ios::binary|std::ios::trunc);
    if (!out)
    {
        std::cerr << argv[2] << ": error opening output file\n";
        return 1;
    }
    lzss_decompress (view.data(), view.size(), out);
    return 0;
}
catch (std::exception& X)
{
    std::cerr << X.what() << '\n';
    return 1;
}
