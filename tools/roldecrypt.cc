// -*- C++ -*-
//! \file       roldecrypt.cc
//! \date       2018 Sep 23
//! \brief      apply binary rol to whole file.
//

#include "sysmemmap.h"
#include <iostream>
#include <fstream>
#include <cwchar>

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::cout << "usage: roldecrypt FILENAME SHIFT\n";
        return 0;
    }
    errno = 0;
    wchar_t* endptr;
    long shift = std::wcstol (argv[2], &endptr, 10);
    if (0 != errno || shift > 8)
        throw std::runtime_error ("invalid shift");
    if (!(shift & 7))
    {
        std::cout << "zero shift: X rol 0 = X\n";
        return 0;
    }

    sys::mapping::readwrite in (argv[1]);
    sys::mapping::view<uint8_t> view (in);
    for (auto it = view.begin(); it != view.end(); ++it)
    {
        uint8_t x = *it;
        *it = x << shift | x >> (8 - shift);
    }
    return 0;
}
catch (std::exception& X)
{
    std::cerr << X.what() << '\n';
    return 1;
}
