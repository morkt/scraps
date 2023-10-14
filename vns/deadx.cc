// -*- C++ -*-
//! \file       deadx.cc
//! \date       2017 Dec 07
//! \brief      decrypt saiki scripts (Dolls Front).
//

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"

namespace {
    void decrypt (uint8_t* data, size_t size)
    {
        const size_t encrypted_length = size;
        data[0] ^= 0xFF;
        uint8_t t = ~data[1];
        data[1] = t << 1 | t >> 7;
        int shift = 1;
        auto count = data[0];
        auto data_end = data + size;
        for (auto ptr = data+2; ptr != data_end; ++ptr)
        {
            *ptr = *ptr << shift | *ptr >> (8 - shift);
            if (++shift >= 7)
                shift = 1;
            if (--count == 0)
            {
                if (shift <= 4)
                    count = data[1];
                else
                    count = data[0];
                shift = 1;
            }
        }
    }
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: deadx INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    if (in.size() < 2)
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
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
