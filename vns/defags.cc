// -*- C++ -*-
//! \file       defags.cc
//! \date       Wed Sep 14 05:02:12 2016
//! \brief      decrypt fAGS scripts.
//

#include "sysmemmap.h"
#include <cstdio>
#include <fstream>

void decrypt (uint8_t* data, size_t size, uint32_t seed)
{
    size /= 4;
    if (0 == size)
        return;
    uint16_t ctl[32];
    uint32_t key[32];

    for (int i = 0; i < 32; ++i)
    {
        uint32_t code = 0;
        uint32_t k = seed;
        for (int j = 0; j < 16; ++j)
        {
            code = (k ^ (k >> 1)) << 15 | (code & 0xFFFF) >> 1;
            k >>= 2;
        }		
        key[i] = seed;
        ctl[i] = code;
        seed = seed << 1 | seed >> 31;
    }
    uint32_t* data32 = reinterpret_cast<uint32_t*> (data);
    for (int i = 0; i < size; ++i)
    {
        auto s = *data32;
        auto code = ctl[i & 0x1F];
        unsigned d = 0;
        unsigned v3 = 3;
        unsigned v2 = 2;
        unsigned v1 = 1;
        for (int j = 0; j < 16; ++j)
        {
            if (0 != (code & 1))
            {
                d |= (s & v1) << 1 | (s >> 1) & (v2 >> 1);
            }
            else
            {
                d |= s & v3;
            }
            code >>= 1;
            v3 <<= 2;
            v2 <<= 2;
            v1 <<= 2;
        }
        *data32++ = d ^ key[i & 0x1F];
    }
}

void dump_text (const uint8_t* data, size_t size, std::ostream& out)
{
    size /= 2;
    const uint16_t* input = reinterpret_cast<const uint16_t*> (data);
    size_t src = 0;
    while (src < size)
    {
        while (src + 1 <= size && input[src])
        {
            auto symbol = input[src++];
            if (symbol & 0xFF)
            {
                if ((symbol & 0xFF) != 0xFF)
                {
                    out.put (symbol & 0xFF);
                    out.put (symbol >> 8);
                }
            }
            else
                out.put (symbol >> 8);
        }
        ++src;
        out.put ('\n');
    }
}

int main (int argc, char* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: defags SCRIPT [TEXTOUT]");
        return 0;
    }
    sys::mapping::readwrite in (argv[1]);
    sys::mapping::view<uint8_t> view (in);
    if (view.size() < 0x10 || 0 != std::memcmp (view.data(), "fAGS", 4))
        throw std::runtime_error ("not a fAGS script");
    auto ptr = view.begin() + 8;
    int remaining = view.size() - 8;
    while (remaining > 0x10)
    {
        size_t section_size = *reinterpret_cast<uint32_t*> (ptr+4);
        size_t header_size  = *reinterpret_cast<uint32_t*> (ptr+8);
        if (header_size > section_size)
            break;
        auto data = ptr + header_size;
        size_t data_size = section_size - header_size;
        if (0 == std::memcmp (ptr, "cTEX", 4) ||
            0 == std::memcmp (ptr, "cFNM", 4))
        {
            uint32_t key = *reinterpret_cast<uint32_t*> (ptr+12);
            decrypt (data, data_size, key);
            if (0 == std::memcmp (ptr, "cTEX", 4) && argc > 2)
            {
                std::ofstream out (argv[2], std::ios::out|std::ios::binary|std::ios::trunc);
                if (out)
                    dump_text (data, data_size, out);
            }
        }
        else if (0 == std::memcmp (ptr, "cCOD", 4))
        {
            uint32_t key = *reinterpret_cast<uint32_t*> (ptr+16);
            decrypt (data, data_size, key);
        }
        ptr += section_size;
        remaining -= section_size;
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s: %s\n", argv[1], X.what());
    return 1;
}
