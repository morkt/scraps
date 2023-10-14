// -*- C++ -*-
//! \file       decrp.cc
//! \date       Tue Feb 07 16:51:26 2017
//! \brief      decrypt cromwell SCENE.CRP script file.
//

#include <cstdio>
#include <cstdint>
#include "sysmemmap.h"

void decrypt_string (uint8_t* data, size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        data[i] ^= (0x89 - (i+1) % 10);
    }
}

uint8_t data[] = { 0xF0, 0xE5, 0xDA, 0xE9, 0xEB, 0xE7, 0xEB, 0xEF, 0xE7, 0xA7, 0xF0 };

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: decrp SCENE.CRP");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    if (view.size() < 0x14 || 0 != std::memcmp (view.data(), "CromwellPresent.", 0x10))
        throw std::runtime_error ("invalid scene.crp file");

    uint32_t* index = reinterpret_cast<uint32_t*> (&view[0x10]);
    size_t count = index[0];
    if (count * 4 + 0x10 > view.size())
        throw std::runtime_error ("invalid scene.crp file");

    for (size_t i = 1; i <= count; ++i)
    {
        size_t offset = index[i];
        if (offset > view.size()-4)
            break;
        uint8_t* data = &view[offset];
        size_t length = *reinterpret_cast<uint32_t*> (data);
        data += 4;
        if (data + length > view.end())
            break;
        decrypt_string (data, length);
        std::fwrite (data, 1, length, stdout);
        std::putc ('\n', stdout);
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}
