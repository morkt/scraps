// -*- C++ -*-
//! \file       deenz.cc
//! \date       Sat Jun 18 09:22:55 2016
//! \brief      decrypt Septer scripts.
//

#include <iostream>
#include <fstream>
#include <zlib.h>
#include "sysmemmap.h"

void uncompress (const uint8_t* input, size_t input_size, std::ostream& out)
{
    z_stream stream = { 0 };
    stream.next_in = const_cast<uint8_t*> (input);
    stream.avail_in = input_size;
    static uint8_t dest[0x1000];
    int err = inflateInit (&stream);
    if (err != Z_OK)
        throw std::runtime_error ("zlib initialization error");
    try
    {
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
                std::cerr << "zlib data error at " << std::hex << (stream.next_in - input) << '\n';
                return;
            }
            int have = 0x1000 - stream.avail_out;
            out.write ((char*)dest, have);
        } while (0 == stream.avail_out);
    }
    catch (...)
    {
        inflateEnd (&stream);
        throw;
    }
    inflateEnd (&stream);
}

int main (int argc, char* argv[])
try
{
    if (argc < 3)
    {
        std::cout << "usage: deenz INPUT OUTPUT\n";
        return 0;
    }

    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    if (in.size() <= 8 || 0 != std::memcmp (view.data(), "EEN", 3))
        throw std::runtime_error ("invalid EENZ file");
    uint32_t key = *(uint32_t*)&view[4] ^ 0xDEADBEEF;
    for (int i = 8; i < view.size(); ++i)
    {
        view[i] ^= key >> ((i & 3) << 3);
    }

    std::ofstream out (argv[2], std::ios::out|std::ios::binary|std::ios::trunc);
    if (!out)
    {
        std::cerr << argv[2] << ": error opening file for writing\n";
        return 1;
    }
    if ('Z' != view[3])
        out.write ((char*)view.data()+8, view.size()-8);
    else
        uncompress (view.data()+8, view.size()-8, out);
    return 0;
}
catch (std::exception& X)
{
    std::cerr << argv[1] << ": " << X.what() << '\n';
    return 1;
}
