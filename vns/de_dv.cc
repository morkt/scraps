// -*- C++ -*-
//! \file       de_dv.cc
//! \date       2022 May 26
//! \brief      
//! \brief      decompress ADVEngine scripts (ACD)
//

#include "sysmemmap.h"
#include <iostream>
#include <fstream>
#include <zlib.h>

void zlib_unpack (const uint8_t* input, size_t in_size, uint8_t* output, size_t out_size)
{
    z_stream stream = { 0 };
    stream.next_in = const_cast<uint8_t*> (input);
    stream.avail_in = in_size;

    int err = inflateInit (&stream);
    if (err != Z_OK)
        throw std::runtime_error ("zlib initialization error");

    stream.next_out = output;
    stream.avail_out = out_size;
    err = inflate (&stream, Z_NO_FLUSH);
    inflateEnd (&stream);

    if (Z_STREAM_ERROR == err)
        throw std::runtime_error ("invalid compressed stream");
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::cout << "usage: de_dv INPUT OUTPUT\n";
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    sys::mapping::const_view<uint8_t> view (in);
    if (view.size() < 0x12 || 0 != std::memcmp (view.begin(), "ACD", 4))
        throw std::runtime_error ("invalid ACD file");

    size_t unpacked_size = *reinterpret_cast<const uint32_t*> (&view[0xC]);
    std::unique_ptr<uint8_t[]> output (new uint8_t[unpacked_size]);
    zlib_unpack (&view[0x10], view.size() - 0x10, output.get(), unpacked_size);
    for (size_t i = 0; i < unpacked_size; ++i)
        output[i] ^= 0xFF;

    std::ofstream out (argv[2], std::ios::binary|std::ios::trunc|std::ios::out);
    if (!out)
        throw std::runtime_error ("can't open output file");
    out.write (reinterpret_cast<char*> (output.get()), unpacked_size);
}
catch (std::exception& X)
{
    std::cerr << argv[1] << ": " << X.what() << std::endl;
    return 1;
}


