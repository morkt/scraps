// -*- C++ -*-
//! \file       z2nl.cc
//! \date       2018 Aug 13
//! \brief      replace zero bytes with '\n' in file.
//

#include "sysmemmap.h"
#include <cstdio>

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::printf ("replace all null bytes with new line characters\n"
                     "usage: z2nl FILENAME\n"
                     "WARNING: changes are made IN PLACE\n");
        return 0;
    }
    sys::mapping::readwrite in (argv[1]);
    if (in.size() == 0)
        return 0;
    sys::mapping::view<uint8_t> view (in);
    for (auto p = view.begin(); p != view.end(); ++p)
    {
        if (!*p)
            *p = '\n';
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
