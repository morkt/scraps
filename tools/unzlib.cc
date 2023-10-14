// -*- C++ -*-
//! \file       unzlib.cc
//! \date       Fri Jul 17 08:00:42 2015
//! \brief      inflate zlib-deflated stream.
//

#include <iostream>
#include <fstream>
#include <cstdio>
#include <zlib.h>
#include "sysmemmap.h"

int main (int argc, char* argv[])
try
{
    if (argc < 3)
    {
        std::cout << "usage: unzlib INPUT OUTPUT\n";
        return 0;
    }

    sys::mapping::readonly in (argv[1]);
    sys::mapping::const_view<uint8_t> view (in);

    z_stream stream = { 0 };
    stream.next_in = const_cast<uint8_t*> (view.data());
    stream.avail_in = view.size();
    static uint8_t dest[0x1000];
    int err = inflateInit (&stream);
    if (err != Z_OK)
    {
        std::cerr << "zlib initialization error\n";
        return 2;
    }
    std::ofstream out;
    do
    {
        stream.next_out = dest;
        stream.avail_out = sizeof(dest);
        err = inflate (&stream, Z_NO_FLUSH);
        if (Z_STREAM_ERROR == err)
            throw std::runtime_error ("invalid compressed stream");
        switch (err)
        {
        case Z_NEED_DICT:
            err = Z_DATA_ERROR;
            /* FALL THROUGH */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd (&stream);
            std::cerr << "zlib data error at " << std::hex << (stream.next_in - view.data()) << '\n';
            return 3;
        }
        if (!out.is_open())
        {
            out.open (argv[2], std::ios::out|std::ios::binary);
            if (!out)
            {
                std::cerr << argv[2] << ": unable to open output file for writing\n";
                return 4;
            }
        }
        int have = 0x1000 - stream.avail_out;
        out.write ((char*)dest, have);
    } while (0 == stream.avail_out);
    uint8_t* z_end = stream.next_in;
    inflateEnd (&stream);
    std::printf ("%s -> %s [EOF:%08X]\n", argv[1], argv[2], (z_end - view.data()));
    return 0;
}
catch (std::exception& X)
{
    std::cerr << argv[1] << ": " << X.what() << '\n';
    return 1;
}
