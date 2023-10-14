// -*- C++ -*-
//! \file       de_advcntl.cc
//! \date       2023 Sep 01
//! \brief      decrypt AdvCntl engine scripts.
//
// AdvCntl (Adventure Game Controller) engine by Omni-sha 『オムニ舎』
//

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: de_advcntl INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    if (in.size() < 4)
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
    sys::mapping::view<uint8_t> view (in);
    std::ofstream out (argv[2], std::ios::out|std::ios::trunc);
    if (!out)
    {
        std::fprintf (stderr, "%S: error opening output file\n", argv[2]);
        return 2;
    }
    auto ptr = view.data();
    while (ptr + 4 < view.end())
    {
        size_t sz = *reinterpret_cast<uint32_t*> (ptr);
        if (!sz)
            break;
        ptr += 4;
        if (sz > view.size() || ptr + sz > view.end())
        {
            std::fprintf (stderr, "%S: invalid input\n", argv[1]);
            return 3;
        }
        for (size_t i = 0; i < sz; ++i)
        {
            ++ptr[i];
        }
        out.write (reinterpret_cast<char*> (ptr), sz);
        out.put ('\n');
        ptr += sz;
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
