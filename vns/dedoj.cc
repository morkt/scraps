// -*- C++ -*-
//! \file       dedoj.cc
//! \date       2017 Nov 26
//! \brief      decompress DOJ scripts from SYSD engine (Nakanai Neko).
//

#include <cstdio>
#include <fstream>
#include "sysmemmap.h"
#include "lzss.h"

int wmain (int argc, wchar_t* argv[])
{
    if (argc < 3)
    {
        std::puts ("usage: dedoj INPUT OUTPUT");
        return 0;
    }
    try
    {
        sys::mapping::readonly in (argv[1]);
        sys::mapping::const_view<uint8_t> view (in);
        if (view.size() < 0x18 || 0 != std::memcmp (view.data(), "CC", 2))
            throw std::runtime_error ("invalid DOJ file [1]");
        size_t count = *reinterpret_cast<const uint16_t*> (&view[2]);
        size_t index_size = 6 * count;
        if (view.size() < 4 + index_size + 16)
            throw std::runtime_error ("invalid DOJ file [2]");

        const uint8_t* dd_header = &view[4+index_size];
        size_t data_size = *reinterpret_cast<const uint32_t*> (&dd_header[4]);
        if (0 != std::memcmp (dd_header, "DD", 2) || data_size <= 9)
            throw std::runtime_error ("invalid DOJ file [3]");
        sys::mapping::const_view<uint8_t> dd_view (in, 4+index_size+12);
        if (dd_view.size() < data_size)
            throw std::runtime_error ("invalid DOJ file [4]");
        std::ofstream out (argv[2], std::ios::out|std::ios::binary|std::ios::trunc);
        if (!out)
        {
            std::fprintf (stderr, "%S: error opening output file\n", argv[2]);
            return 1;
        }
        if (!dd_view[0])
        {
            out.write (reinterpret_cast<const char*> (dd_view.data()), dd_view.size());
        }
        else
        {
            lzss_decompress (dd_view.data()+9, data_size-9, out);
        }
        return 0;
    }
    catch (std::exception& X)
    {
        std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
        return 1;
    }
}
