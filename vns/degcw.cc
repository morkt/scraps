// -*- C++ -*-
//! \file       degcw.cc
//! \date       2022 May 11
//! \brief      decrypt OCSW scripts.
//

#include "sysmemmap.h"
#include <fstream>
#include <iostream>

const unsigned default_key = 0x12345678;

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::cout << "usage: degcw INPUT OUTPUT\n";
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    if (view.size() < 0xC || 0 != std::memcmp (view.data(), "OCSW 1", 7)
        || 0x45 != *reinterpret_cast<uint16_t*> (&view[7]))
        throw std::runtime_error ("invalid OCSW file");
    size_t size = *reinterpret_cast<uint32_t*> (&view[9]);
    if (size > view.size() - 0xE)
        throw std::runtime_error ("incompatible OCSW file");
    uint8_t* ptr = &view[0xD];
    auto end_ptr = ptr + size;
    uint32_t key = default_key;
    uint8_t check_sum = 0;
    while (ptr != end_ptr)
    {
        uint8_t c = key ^ *ptr;
        check_sum += c;
        key = key >> 1 | key << 31;
        *ptr++ = c;
    }
    if (check_sum != *ptr)
        throw std::runtime_error ("checksum mismatch");
    std::ofstream out (argv[2], std::ios::out|std::ios::trunc|std::ios::binary);
    if (!out)
        throw std::runtime_error ("failed to open output file");
    out.write (reinterpret_cast<char*> (&view[0xD]), size);
    return 0;
}
catch (std::exception& X)
{
    std::cerr << argv[1] << ": " << X.what() << std::endl;
    return 1;
}
