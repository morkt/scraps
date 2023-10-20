// -*- C++ -*-
//! \file       mpx2txt.cc
//! \date       Fri Jan 20 09:57:44 2017
//! \brief      extract text from Complet's MPX scripts.
//

#include "sysmemmap.h"
#include <fstream>

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: mpx2txt INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    if (view.size() < 8 || 0 != std::memcmp (view.data(), "Mp1", 3))
    {
        std::fprintf (stderr, "%S: invalid MPX file.\n", argv[1]);
        return 1;
    }
    int version = view[3] - '0';
    if (version < 6 || version > 7)
    {
        std::fprintf (stderr, "%S: not supported MPX file version.\n", argv[1]);
        return 1;
    }
    size_t offset = *reinterpret_cast<uint16_t*> (&view[4]);
    size_t size   = *reinterpret_cast<uint16_t*> (&view[6]);
    if (offset >= view.size() || size >= view.size() || offset + size > view.size())
    {
        std::fprintf (stderr, "%S: invalid MPX file.\n", argv[1]);
        return 1;
    }
    auto begin = view.data() + offset;
    for (auto end = begin + size; begin != end; ++begin)
    {
        *begin ^= 0x24;
        if (!*begin)
            *begin = '\n';
    }
    std::ofstream out (argv[2], std::ios::out|std::ios::trunc);
    if (!out)
    {
        std::fprintf (stderr, "%S: unable to open file for writing\n", argv[2]);
        return 1;
    }
    char* data = reinterpret_cast<char*> (view.data() + offset);
    out.write (data, size);
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}
