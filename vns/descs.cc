// -*- C++ -*-
//! \file       descs.cc
//! \date       2017 Dec 27
//! \brief      decrypt Bishop SCS file.
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
        std::puts ("usage: descs INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    if (in.size() < 4 || 0 != std::memcmp (view.data(), "PE", 2))
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
    auto data_end = view.data() + view.size();
    std::ofstream out (argv[2], std::ios::out|std::ios::trunc);
    if (!out)
    {
        std::fprintf (stderr, "%S: error opening output file\n", argv[2]);
        return 1;
    }
    auto ptr = view.data() + 2;
    while (ptr+1 < data_end)
    {
        size_t key = (ptr - view.data()) | 1;
        size_t len = *reinterpret_cast<uint16_t*> (ptr);
        ptr += 2;
        if (ptr + len > data_end)
            break;
        for (size_t i = 0; i < len; ++i)
        {
            key *= 69069;
            ptr[i] ^= key;
        }
        out.write (reinterpret_cast<char*> (ptr), len);
        out.put ('\n');
        ptr += len;
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
