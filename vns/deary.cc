// -*- C++ -*-
//! \file       deary.cc
//! \date       2018 Mar 29
//! \brief      Unpack abel script.
//

#include <cstdio>
#include <fstream>
#include <sstream>
#include "sysmemmap.h"
#include "lzss.h"

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: deary INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    if (in.size() < 12)
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
    sys::mapping::view<uint8_t> view (in);
    size_t packed_size = *reinterpret_cast<uint32_t*> (&view[8]);
    if (packed_size + 12 != view.size())
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
    for (auto p = view.begin()+12; p != view.end(); ++p)
        *p ^= 0x7C;
    std::stringstream strings;
    lzss_decompress (view.data()+12, packed_size, strings);
    std::ofstream out (argv[2], std::ios::out|std::ios::trunc);
    if (!out)
    {
        std::fprintf (stderr, "%S: error opening output file\n", argv[2]);
        return 1;
    }
    strings.seekg (0);
    std::string line;
    while (std::getline (strings, line, '\0'))
    {
        out << line << '\n';
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
