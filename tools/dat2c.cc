// -*- C++ -*-
//! \file       dat2c.cc
//! \date       Wed Jun 17 08:45:22 2015
//! \brief      convert binary data to C source.
//

#include <string>
#include <cstdio>
#include "sysmemmap.h"

int main (int argc, char* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: dat2c FILENAME");
        return 0;
    }
    const bool use_blocks = false;
    sys::mapping::readonly in (argv[1]);
//    sys::mapping::const_view<uint8_t> dat (in);
    sys::mapping::const_view<uint32_t> dat (in);
    int count = 0;
    for (auto it = dat.begin(); it < dat.end(); ++it)
    {
        if (use_blocks && !(count & 0xFF))
            std::puts ("{");
        std::printf ("0x%08X,", *it);
//        std::printf ("0x%02X,", *it);
//        std::printf ("%5d,", *it);
//        std::printf ("%02X: %08X\n", count++, *it);
        if (use_blocks && 0xFF == (count & 0xFF))
            std::puts ("\n},");
        else if ((count + 1) & 0x7)
            putchar (' ');
        else
            putchar ('\n');
        ++count;
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
