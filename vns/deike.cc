// -*- C++ -*-
//! \file       deike.cc
//! \date       2018 Mar 18
//! \brief      decompress 'ike' files.
//

#include <cstdio>
#include <fstream>
#include <vector>
#include "sysmemmap.h"

class bit_stream
{
    const uint8_t*  m_src;
    const uint8_t*  const m_end;
    int             m_bits;

public:
    bit_stream (const uint8_t* input, size_t size)
        : m_src (input), m_end (input + size), m_bits (2)
    {
    }

    uint8_t read_byte ()
    {
        if (m_src == m_end)
            throw std::runtime_error ("end of stream");
        return *m_src++;
    }

    uint8_t get_bit ()
    {
        int bit = m_bits & 1;
        m_bits >>= 1;
        if (1 == m_bits)
        {
            if (m_src+2 > m_end)
                throw std::runtime_error ("end of stream");
            m_bits = *reinterpret_cast<const uint16_t*> (m_src) | 0x10000;
            m_src += 2;
        }
        return bit;
    }
};

void ike_decompress (const uint8_t* input, size_t input_size, std::vector<uint8_t>& output)
{
    size_t unpacked_size = input[11] + ((input[12] + (input[10] >> 2 << 8)) << 8);
    output.resize (unpacked_size);
    bit_stream bits (input+13, input_size-13);
    bits.get_bit();
    size_t dst = 0;
    while (dst < unpacked_size)
    {
        int offset, shift, count;
        if (bits.get_bit() != 0)
        {
            output[dst++] = bits.read_byte();
            continue;
        }
        if (bits.get_bit() != 0)
        {
            offset = bits.read_byte() | -0x100;
            shift = 0;
            if (bits.get_bit() == 0)
                shift += 0x100;
            if (bits.get_bit() == 0)
            {
                offset -= 0x200;
                if (bits.get_bit() == 0)
                {
                    shift <<= 1;
                    if (bits.get_bit() == 0)
                        shift += 0x100;
                    offset -= 0x200;
                    if (bits.get_bit() == 0)
                    {
                        shift <<= 1;
                        if (bits.get_bit() == 0)
                            shift += 0x100;
                        offset -= 0x400;
                        if (bits.get_bit() == 0)
                        {
                            offset -= 0x800;
                            shift <<= 1;
                            if (bits.get_bit() == 0)
                                shift += 0x100;
                        }
                    }
                }
            }
            offset -= shift;
            if (bits.get_bit() != 0)
            {
                count = 3;
            }
            else if (bits.get_bit() != 0)
            {
                count = 4;
            }
            else if (bits.get_bit() != 0)
            {
                count = 5;
            }
            else if (bits.get_bit() != 0)
            {
                count = 6;
            }
            else if (bits.get_bit() != 0)
            {
                if (bits.get_bit() != 0)
                    count = 8;
                else
                    count = 7;
            }
            else if (bits.get_bit() != 0)
            {
                count = bits.read_byte() + 17;
            }
            else
            {
                count = 9;
                if (bits.get_bit() != 0)
                    count = 13;
                if (bits.get_bit() != 0)
                    count += 2;
                if (bits.get_bit() != 0)
                    count++;
            }
        }
        else
        {
            offset = bits.read_byte() | -0x100;
            if (bits.get_bit() != 0)
            {
                offset -= 0x100;
                if (bits.get_bit() == 0)
                    offset -= 0x400;
                if (bits.get_bit() == 0)
                    offset -= 0x200;
                if (bits.get_bit() == 0)
                    offset -= 0x100;
            }
            else if (offset == -1)
            {
                if (bits.get_bit() == 0)
                    break;
                else
                    continue;
            }
            count = 2;
        }
        for (int i = 0; i < count; ++i)
            output[dst+i] = output[dst+offset+i];
        dst += count;
    }
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: deike ARCHIVE OUTPUT-FILE");
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    sys::mapping::const_view<uint8_t> view (in);
    if (view.size() < 0xF || 0 != std::memcmp (&view[2], "ike", 3))
        throw std::runtime_error ("invadid 'ike' file");

    std::vector<uint8_t> buffer;
    ike_decompress (view.data(), view.size(), buffer);
    std::ofstream out (argv[2], std::ios::out|std::ios::trunc|std::ios::binary);
    if (!out)
    {
        std::fprintf (stderr, "%S: unable to open file for writing\n", argv[2]);
        return 1;
    }
    out.write (reinterpret_cast<char*> (buffer.data()), buffer.size());
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}
