// -*- C++ -*-
//! \file       xordecrypt.cc
//! \date       Thu Jul 23 08:56:07 2015
//! \brief      apply xor to whole file.
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
        std::cout << "usage: xordecrypt FILENAME HEXKEY\n";
        return 0;
    }
    errno = 0;
    wchar_t* endptr;
    long key = std::wcstol (argv[2], &endptr, 16);
    if (0 != errno)
        throw std::runtime_error ("invalid key");
    if (0 == key)
    {
        std::cout << "zero key: X xor 0 = X\n";
        return 0;
    }

    sys::mapping::readwrite in (argv[1]);
    sys::mapping::view<uint8_t> view (in);
    for (auto it = view.begin(); it != view.end(); ++it)
    {
        *it ^= key;
    }
    return 0;
}
catch (std::exception& X)
{
    std::cerr << X.what() << '\n';
    return 1;
}
