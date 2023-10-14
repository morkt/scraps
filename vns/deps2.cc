// -*- C++ -*-
//! \file       deps2.cc
//! \date       2019 Mar 25
//! \brief      CMVS engine script unpacker.
//

#include <fstream>
#include "sysmemmap.h"

uint8_t rot_byte_r (uint8_t x, int count)
{
    count &= 0x7;
    return x >> count | x << (8 - count);
}

void decrypt_ps2 (uint8_t* data, size_t length)
{
    unsigned key = *reinterpret_cast<uint32_t*> (&data[12]);
    int shift = static_cast<int> (key >> 20) % 5 + 1;
    key = (key >> 24) + (key >> 3);
    for (int i = 0x30; i < length; ++i)
    {
        data[i] = rot_byte_r (static_cast<uint8_t> (key ^ (data[i] - 0x7Cu)), shift);
    }
}

void unpack_lzss (const uint8_t* data, size_t size, std::ostream& out)
{
    static uint8_t frame[0x800];
    int frame_pos = 0x7DF;
    int unpacked_size = *reinterpret_cast<const int32_t*> (&data[0x28]);
    out.write (reinterpret_cast<const char*> (data), 0x30);
    int src = 0x30;
    int dst = 0;
    int ctl = 1;
    while (dst < unpacked_size && src < size)
    {
        if (1 == ctl)
            ctl = data[src++] | 0x100;
        if (0 != (ctl & 1))
        {
            auto b = data[src++];
            out.put (b);
            frame[frame_pos++] = b;
            frame_pos &= 0x7FF;
            dst++;
        }
        else
        {
            int lo = data[src++];
            int hi = data[src++];
            int offset = lo | (hi & 0xE0) << 3;
            int count = (hi & 0x1F) + 2;
            dst += count;
            for (int i = 0; i < count; ++i)
            {
                auto b = frame[(offset + i) & 0x7FF];
                out.put (b);
                frame[frame_pos++] = b;
                frame_pos &= 0x7FF;
            }
        }
        ctl >>= 1;
    }
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: debcs INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    if (view.size() < 0x30 || 0 != std::memcmp (view.data(), "PS2A", 4))
    {
        std::fprintf (stderr, "%S: invalid PS2A file\n", argv[1]);
        return 1;
    }
    decrypt_ps2 (view.data(), view.size());
    std::ofstream out (argv[2], std::ios::out|std::ios::binary);
    if (!out)
    {
        std::fprintf (stderr, "%S: error opening output file\n", argv[2]);
        return 1;
    }
    unpack_lzss (view.data(), view.size(), out);
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
