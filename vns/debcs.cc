// -*- C++ -*-
//! \file       debcs.cc
//! \date       Tue Mar 21 23:02:56 2017
//! \brief      TanukiSoft BCS file decompressor.
//

#include <cstdio>
#include <fstream>
#include <vector>
#include "sysmemmap.h"
#include "blowfish.h"

char g_tnk_key[] = "TLibDefKey";

size_t bcs_decompress (const uint8_t* packed, size_t packed_size, std::vector<uint8_t>& output)
{
    const size_t FrameSize = 0x1000;
    const int FrameFill = 0;
    const int FrameInitPos = 0xFEE;
    const unsigned frame_mask = FrameSize-1;

    sys::local_buffer<uint8_t> frame (FrameSize);
    std::fill_n (frame.begin(), FrameSize, FrameFill);
    int frame_pos = FrameInitPos;

    auto dst = output.begin();
    auto out_end = output.end();
    auto const packed_end = packed + packed_size;
    auto src = packed;
    while (src != packed_end && dst < out_end)
    {
        int ctl = *src++;
        for (int bit = 1; src != packed_end && bit != 0x100; bit <<= 1)
        {
            if (ctl & bit)
            {
                uint8_t b = *src++;
                frame[frame_pos++ & frame_mask] = b;
                *dst++ = b;
            }
            else
            {
                if (packed_end - src < 2)
                    break;
                int lo = *src++;
                int hi = *src++;
                int offset = (hi & 0xF0) << 4 | lo;
                int count = 3 + (~hi & 0xF);
                for (int i = 0; i < count && dst < out_end; ++i)
                {
                    uint8_t v = frame[offset++ & frame_mask];
                    frame[frame_pos++ & frame_mask] = v;
                    *dst++ = v;
                }
            }
        }
    }
    return dst - output.begin();
}

void tnk_decrypt (uint8_t* data, size_t length, std::ostream& out)
{
    CBlowFish blowfish;
    blowfish.Initialize (reinterpret_cast<unsigned char*> (g_tnk_key), std::strlen (g_tnk_key));
    size_t align_length = length & ~7u;
    if (align_length > 0)
        blowfish.Decode (data, data, align_length);
    out.write (reinterpret_cast<char*> (data), length);
}

void gms_descramble (uint8_t* data)
{
    auto v1 = data + 9;
    for (int i = 0; i < 2; ++i)
    {
        auto tmp = v1[4];
        v1[4] = *v1;
        *v1 = tmp;
        v1 += 2;
    }
    v1 = data + 8;
    for (int i = 0; i < 2; ++i)
    {
        auto tmp = *(v1 - 4);
        *(v1 - 4) = *v1;
        *v1 = tmp;
        v1 += 2;
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
    sys::mapping::readonly in (argv[1]);
    sys::mapping::const_view<uint8_t> view (in);
    bool is_encrypted = 0 == std::memcmp (view.data(), "TSV", 4);
    bool is_bcs = is_encrypted || 0 == std::memcmp (view.data(), "BCS", 4);
    if (view.size() < 0x18 || !is_bcs)
    {
        std::fprintf (stderr, "%S: invalid BSV file\n", argv[1]);
        return 1;
    }
    size_t unpacked_size = *reinterpret_cast<const uint32_t*> (&view[4]);
    std::vector<uint8_t> output (unpacked_size);
    bcs_decompress (view.data()+0x18, view.size()-0x18, output);
    std::ofstream out (argv[2], std::ios::out|std::ios::binary);
    if (!out)
    {
        std::fprintf (stderr, "%S: error opening output file\n", argv[2]);
        return 1;
    }
    if (is_encrypted)
    {
        auto it = std::search (output.begin(), output.end(), "TNK", &4["TNK"]);
        if (it != output.end() && (output.end() - it) > 0xC)
        {
    //        std::printf ("%S: found TNK at %08X\n", argv[1], it - output.begin());
            auto tnk = &*it;
            size_t tnk_size = *reinterpret_cast<const uint32_t*> (tnk+8);
            if (tnk_size <= (output.end() - it))
            {
                tnk_decrypt (tnk+0xC, tnk_size, out);
                return 0;
            }
        }
    }
    else if (is_bcs)
    {
        auto it = std::search (output.begin(), output.end(), "GMS", &4["GMS"]);
        if (it != output.end() && (output.end() - it) > 0x10)
        {
            size_t gms_size = output.end() - it;
            auto gms = &*it;
            gms_descramble (gms);
            unpacked_size = *reinterpret_cast<uint32_t*> (gms+12);
            std::vector<uint8_t> gms_data (unpacked_size);
            bcs_decompress (gms+0x10, gms_size-0x10, gms_data);
            for (auto& p : gms_data)
                p ^= 0xFF;
            output.swap (gms_data);
        }
    }
    out.write (reinterpret_cast<char*> (output.data()), output.size());
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
