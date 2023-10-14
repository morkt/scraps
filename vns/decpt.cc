// -*- C++ -*-
//! \file       decpt.cc
//! \date       2018 Dec 13
//! \brief      decrypt Abel Software CPT script.
//

#include <cstdio>
#include <fstream>
#include <vector>
#include "sysmemmap.h"

namespace abel {
    int s_seed;

    void srand (int seed)
    {
        s_seed = seed;
    }

    int rand ()
    {
        unsigned val = 214013 * s_seed + 2531011;
        s_seed = val;
        return (val >> 16) & 0x7FFF;
    }
}

uint8_t g_decrypt_table[256];

void init_decrypt_table (unsigned seed)
{
    uint8_t cryptTable[256];
    abel::srand (seed);
    for (int i = 0; i < 256; ++i)
    {
        cryptTable[i] = i;
    }
    for (int i = 0; i < 256; ++i)
    {
        int rnd = abel::rand() % 256;
        auto t = cryptTable[i];
        cryptTable[i] = cryptTable[rnd];
        cryptTable[rnd] = t;
    }
    for (int i = 0; i < 256; ++i)
    {
        g_decrypt_table[cryptTable[i]] = i;
    }
}

void decrypt_cpt (uint8_t* data, size_t length)
{
    uint8_t prev = 0;
    for (size_t i = 0; i < length; ++i)
    {
        auto x = data[i];
        data[i] = g_decrypt_table[(x - prev) & 0xFF];
        prev = x;
    }
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: decpt INPUT OUTPUT");
        return 0;
    }
    init_decrypt_table (0x03429195);
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    decrypt_cpt (view.data(), view.size());
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
