// -*- C++ -*-
//! \file       unadvc.cc
//! \date       2023 Oct 05
//! \brief      decompress ADVC script.
//

#include "bytecode.h"
#include <cstdio>
#include <fstream>
#include <vector>

class advc_unpacker : private bytecode_reader
{
    bool do_run () override  { return true; }

public:
    advc_unpacker (sys::mapping::map_base& in) : bytecode_reader (in)
    {
    }

    void unpack (std::vector<uint8_t>& output);

private:
    void setup_frame();

    std::vector<uint8_t> m_frame;
};

void advc_unpacker::
setup_frame ()
{
    m_frame.resize (0x1000);
    size_t dst = 0;
    for (int i = 0; i < 0x100; ++i)
    {
        for (int j = 0; j < 13; ++j)
            m_frame[dst++] = i;
    }
    // 0xD00
    for (int i = 0; i < 0x100; ++i)
        m_frame[dst++] = i;
    // 0xE00
    for (int i = 0xFF; i >= 0; --i)
        m_frame[dst++] = i;
    // 0xF00
    for (int i = 0; i < 0x80; ++i)
        m_frame[dst++] = 0;
    // 0xF80
    for (int i = 0; i < 0x6E; ++i)
        m_frame[dst++] = 0x20;
}

void advc_unpacker::
unpack (std::vector<uint8_t>& output)
{
    setup_frame();
    output.clear();
    pBytecode = pBytecodeStart;
    auto word00 = get_word();
    auto word02 = get_word();
    size_t unpacked_size = get_word();
    output.resize (unpacked_size);
    auto word06 = get_word();
    int frame_pos = 0xFEE;
    const int frame_mask = 0xFFF;
    size_t dst = 0;
    uint8_t mask = 0;
    uint8_t ctl;
    while (pBytecode < view.end() && dst < unpacked_size)
    {
        mask <<= 1;
        if (!mask)
        {
            ctl = get_byte();
            mask = 1;
        }
        if (ctl & mask)
        {
            output[dst++] = m_frame[frame_pos++ & frame_mask] = get_byte();
        }
        else
        {
            unsigned lo = get_byte();
            unsigned hi = get_byte();
            unsigned offset = (hi & 0xF0) << 4 | lo;
            int count = std::min ((hi & 0xF) + 3, unpacked_size - dst);
            while (count --> 0)
            {
                auto b = m_frame[offset++ & frame_mask];
                output[dst++] = m_frame[frame_pos++ & frame_mask] = b;
            }
        }
    }
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: unadvc INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    if (in.size() < 2)
    {
        std::fprintf (stderr, "%S: invalid input\n", argv[1]);
        return 1;
    }
    advc_unpacker advc (in);
    std::vector<uint8_t> output;
    advc.unpack (output);
    std::ofstream out (argv[2], std::ios::out|std::ios::binary|std::ios::trunc);
    if (!out)
    {
        std::fprintf (stderr, "%S: error opening output file\n", argv[2]);
        return 1;
    }
    out.write (reinterpret_cast<char*> (output.data()), output.size());
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}
