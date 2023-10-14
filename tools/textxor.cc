// -*- C++ -*-
//! \file       textxor.cc
//! \date       2018 Jan 04
//! \brief      xor text file.
//

#include "sysmemmap.h"
#include <iostream>
#include <fstream>

#define _   0
#define S   1
#define M   2

const uint8_t first_map[] = {
//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // 0
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // 1
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 2
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 3
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 4
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 5
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // 6
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, _, // 7 
    _, M, M, M, _, _, _, M, M, M, M, M, M, M, M, M, // 8
    M, M, M, M, M, M, M, M, M, M, M, M, M, M, M, M, // 9
    _, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // A
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // B
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // C
    S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, // D
    M, M, M, M, M, M, M, M, M, M, M, _, _, _, _, _, // E
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // F
};

int main (int argc, char* argv[])
try
{
    if (argc < 3)
    {
        std::cout << "usage: textxor FILENAME HEXKEY\n";
        return 0;
    }
    errno = 0;
    char* endptr;
    long key = std::strtol (argv[2], &endptr, 16);
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
        uint8_t sym = *it;
        if (sym == '\r' || sym == '\n' || sym == ' ' || sym > 0xE0)
            continue;
        *it = sym ^ key;
        if (2 == first_map[*it] && std::next (it) != view.end())
        {
            sym = *++it;
            if (sym < 0xDF)
                sym ^= key;
            *it = sym;
        }
    }
    return 0;
}
catch (std::exception& X)
{
    std::cerr << X.what() << '\n';
    return 1;
}


