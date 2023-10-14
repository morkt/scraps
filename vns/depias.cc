// -*- C++ -*-
//! \file       depias.cc
//! \date       2018 Jan 19
//! \brief      decrypt Pias text.dat file.
//

#include "sysmemmap.h"
#include <cstdio>
#include <fstream>

struct key_data
{
    unsigned    m_a;
    unsigned    m_b;

    key_data (unsigned seed) : m_a (seed), m_b (0) { }

    void set_seed (unsigned seed)
    {
        m_b = seed;
    }

    unsigned next ()
    {
        unsigned y, x;
        if (0 == m_a)
        {
            x = 0xD22;
            y = 0x849;
        }
        else if (1 == m_a)
        {
            x = 0xF43;
            y = 0x356B;
        }
        else if (2 == m_a)
        {
            x = 0x292;
            y = 0x57A7;
        }
        else
        {
            x = 0;
            y = 0;
        }
        unsigned v3 = x + m_b * y;
        unsigned v4 = 0;
        if (v3 & 0x400000)
            v4 = 1;
        if (v3 & 0x400 )
            v4 ^= 1;
        if (v3 & 1)
            v4 ^= 1;
        m_b = (v3 >> 1) | (v4 ? 0x80000000 : 0);
        return m_b;
    }
};

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: deadx INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    if (in.size() <= 4)
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
    sys::mapping::view<uint8_t> view (in);
    key_data rnd (1);
    rnd.set_seed (*reinterpret_cast<uint32_t*> (view.data()));
    for (int i = 4; i < view.size(); i++)
    {
        view[i] ^= rnd.next();
    }
    std::ofstream out (argv[2], std::ios::out|std::ios::binary|std::ios::trunc);
    if (!out)
    {
        std::fprintf (stderr, "%S: error opening output file\n", argv[2]);
        return 1;
    }
    out.write (reinterpret_cast<char*> (view.data()+4), view.size()-4);
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}
