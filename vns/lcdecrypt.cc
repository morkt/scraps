// -*- C++ -*-
//! \file       lcdecrypt.cc
//! \date       2018 Sep 20
//! \brief      extract text from Lazycrew script.
//

#include "sysmemmap.h"

void decrypt_string (uint8_t* data, size_t length, unsigned key)
{
    for (size_t i = 0; i < length; ++i)
    {
        data[i] ^= key;
        key = data[i] ^ ((key << 9) | (key >> 23) & 0x1F0);
    }
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: lcdecrypt FILE");
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    if (in.size() < 0x14)
        throw std::runtime_error ("invalid script");
    sys::mapping::const_view<uint8_t> header (in, 0, 0x14);
    unsigned total = *reinterpret_cast<const uint32_t*> (&header[0xC]);
    unsigned offset = *reinterpret_cast<const uint32_t*> (&header[0x10]);
    if (offset >= in.size())
        throw std::runtime_error ("invalid script");
    sys::mapping::view<uint8_t> view (offset, in.size()-offset);
    auto data = view.data();
    auto data_end = data + view.size();
    unsigned count = 0;
    while (data != data_end)
    {
        auto code = *data++;
        if (0xFF == code)
            break;
        else if (1 == code)
        {
            if (data_end-data < 4)
                throw std::runtime_error ("invalid script");
//            auto cmd = *reinterpret_cast<uint32_t*> (data);
//            printf ("[%X]\n", cmd);
            data += 4;
        }
        else if (2 == code)
        {
            if (data_end-data < 2)
                throw std::runtime_error ("invalid script");
            size_t length = *reinterpret_cast<uint16_t*> (data);
            data += 2;
            if (data_end - data < length)
                throw std::runtime_error ("invalid script");
            decrypt_string (data, length, 1264235685);
            size_t str_length = length;
            while (str_length > 0 && !data[str_length-1])
                --str_length;
            std::fwrite (data, 1, str_length, stdout);
            std::putchar ('\n');
            data += length;
        }
        else
            throw std::runtime_error ("invalid script");
        ++count;
    }
    std::printf ("%d records\n", count);
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}


