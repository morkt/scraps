// -*- C++ -*-
//! \file       demubin.cc
//! \date       2019 May 22
//! \brief      decrypt Artel 'Mu' binary scripts.
//

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: deadx INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    if (view.size() < 2 || 0 != std::memcmp (view.data(), "Mu", 2))
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
    std::ofstream out (argv[2], std::ios::out|std::ios::binary|std::ios::trunc);
    if (!out)
    {
        std::fprintf (stderr, "%S: error opening output file\n", argv[2]);
        return 1;
    }
    size_t src = 2;
    uint8_t key = 8;
    auto data = view.data();
    while (src + 16 < view.size())
    {
        for (size_t n = 0; n < 16; ++n)
        {
            data[src+n] ^= key++;
        }
        out.write (reinterpret_cast<char*> (data+src), 16);
        src += 17;
        ++key;
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
