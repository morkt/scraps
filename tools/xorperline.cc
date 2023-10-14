// -*- C++ -*-
//! \file       xorperline.cc
//! \date       2018 Oct 07
//! \brief      apply xor to each line.
//
// used in Digital Monkey *.dm scripts
//

#include <iostream>
#include <fstream>
#include <string>

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::cout << "usage: xorperline FILENAME HEXKEY\n";
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

    std::ifstream in (argv[1]);
    std::string line;
    while (std::getline (in, line))
    {
        for (char& pch : line)
            pch ^= key;
        std::cout << line << '\n';
    }
    return 0;
}
catch (std::exception& X)
{
    std::cerr << X.what() << '\n';
    return 1;
}
