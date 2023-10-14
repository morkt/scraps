// -*- C++ -*-
//! \file       deciv.cc
//! \date       Fri Jan 29 06:34:20 2016
//! \brief      decode c's ware 'scenario.civ'
//

#include <iostream>
#include <fstream>
#include "sysmemmap.h"

int main (int argc, char* argv[])
try
{
    if (argc < 3)
    {
        std::cout << "usage: deciv INPUT OUTPUT\n";
        return 0;
    }

    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    if (in.size() <= 0x40)
        throw std::runtime_error ("invalid input file");

    sys::mapping::view<uint8_t> view (in);
    if (0 != std::memcmp (view.data(), "CFLR", 4))
        throw std::runtime_error ("invalid input file");

    size_t part1_size = *(uint32_t*)&view[0x34];
    size_t part2_size = *(uint32_t*)&view[0x0C];

    if (0x40 + part1_size + part2_size > view.size())
        throw std::runtime_error ("invalid input file");

    uint8_t* part1 = &view[0x40];
    uint8_t* part2 = &view[0x40+part1_size];
    int count = *(int32_t*)part1;
    auto ptr = part1 + 4;
    for (int i = 0; i < count; ++i)
    {
        int len = *(int32_t*)ptr;
        ptr += 8;
        for (int j = 0; j < len; ++j)
        {
            *ptr = *ptr << 4 | *ptr >> 4;
            ++ptr;
        }
    }

    std::ofstream out (argv[2], std::ios::out|std::ios::binary|std::ios::trunc);
    if (!out)
    {
        std::cerr << argv[2] << ": error opening output file\n";
        return 1;
    }
    out.write ((char*)view.data(), view.size());

    return 0;
}
catch (std::exception& X)
{
    std::cerr << argv[1] << ": " << X.what() << '\n';
    return 1;
}
