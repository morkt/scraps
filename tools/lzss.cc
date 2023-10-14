// -*- C++ -*-
//! \file       lzss.cc
//! \date       Tue Jul 07 09:00:28 2015
//! \brief      LZSS decompression routine.
//

#include <iostream>
#include "sysstring.h"

size_t lzss_decompress (const uint8_t* packed, size_t packed_size, std::ostream& out)
{
    const size_t FrameSize = 0x1000;
    const int FrameFill = 0;
    const int FrameInitPos = 0xfee;
    const unsigned frame_mask = FrameSize-1;

    sys::local_buffer<uint8_t> frame (FrameSize);
    std::fill_n (frame.begin(), FrameSize, FrameFill);
    int frame_pos = FrameInitPos;

    size_t total = 0;
    auto const packed_end = packed + packed_size;
    auto src = packed;
    while (src != packed_end)
    {
        int ctl = *src++;
        for (int bit = 1; src != packed_end && bit != 0x100; bit <<= 1)
        {
            if (ctl & bit)
            {
                uint8_t b = *src++;
                frame[frame_pos++] = b;
                frame_pos &= frame_mask;
                out.put (static_cast<char> (b));
                ++total;
            }
            else
            {
                if (packed_end - src < 2)
                    return total;
                int lo = *src++;
                int hi = *src++;
                int offset = (hi & 0xF0) << 4 | lo;
                int count = 3 + (hi & 0xF);
                for (int i = 0; i < count; ++i)
                {
                    uint8_t v = frame[offset++];
                    offset &= frame_mask;
                    frame[frame_pos++] = v;
                    frame_pos &= frame_mask;
                    out.put (static_cast<char> (v));
                }
                total += count;
            }
        }
    }
    return total;
}
