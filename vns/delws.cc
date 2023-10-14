// -*- C++ -*-
//! \file       delws.cc
//! \date       2023 Sep 08
//! \brief      decrypt LWS scripts.
//
// [020412][Ciel] Maid Hunter Zero One ~Nora Maid~

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: delws INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    if (view.size() < 8 || 0 != std::memcmp (view.data(), "LW", 2))
    {
        std::fprintf (stderr, "%S: invalid LW script\n", argv[1]);
        return 1;
    }
    size_t text_pos = *reinterpret_cast<uint32_t*> (&view[4]);
    size_t text_size = *reinterpret_cast<uint32_t*> (&view[8]);
    if (text_pos > view.size() || text_size > view.size() || text_pos > view.size() - text_size)
    {
        std::fprintf (stderr, "%S: invalid LW script\n", argv[1]);
        return 1;
    }
    uint32_t* p = reinterpret_cast<uint32_t*> (&view[text_pos]);
    for (size_t i = 0; i < text_size; ++i)
    {
        p[i] ^= 0xFFFFFFFF;
    }
    std::ofstream out (argv[2], std::ios::out|std::ios::trunc);
    if (!out)
    {
        std::fprintf (stderr, "%S: unable to open output file\n", argv[2]);
        return 2;
    }
    auto t = reinterpret_cast<uint16_t*> (&view[text_pos]);
    auto t_end = reinterpret_cast<uint16_t*> (view.end());
    while (t < t_end)
    {
        if (*t < 0x2000)
        {
            if (!*t)
                out.put ('\n');
        }
        else
            out.write (reinterpret_cast<char*> (t), 2);
        ++t;
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
