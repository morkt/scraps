// -*- C++ -*-
//! \file       cst2txt.cc
//! \date       Mon Oct 19 17:46:51 2015
//! \brief      extract text from CatSystem scripts.
//

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <zlib.h>
#include "sysmemmap.h"

#pragma pack(push,1)
struct CstHeader
{
  uint32_t length;
  uint32_t entry_count;
  uint32_t table2_offset; // from end of header
  uint32_t data_offset;   // from end of header
};

struct CstEntry
{
  uint32_t entry_count;
  uint32_t start_index;
};
#pragma pack(pop)

void
extract_script (const uint8_t* src, size_t src_size, std::ostream& out)
{
    if (src_size <= 0x10)
        throw std::runtime_error ("invalid CST file");
    const CstHeader* header = reinterpret_cast<const CstHeader*> (src);
    const CstEntry*  index  = reinterpret_cast<const CstEntry*> (src+0x10);
    if (header->data_offset > src_size-0x10 || header->table2_offset > src_size-0x10)
        throw std::runtime_error ("invalid CST file format (1)");

    const uint32_t* entries = reinterpret_cast<const uint32_t*> (src+0x10+header->table2_offset);
    const char* data        = reinterpret_cast<const char*> (src+0x10+header->data_offset);
    const char* data_end    = reinterpret_cast<const char*> (src+src_size);

    out << std::hex;
    for (int i = 0; i < header->entry_count; ++i)
    {
        for (int j = 0; j < index[i].entry_count; ++j)
        {
            auto str = data + entries[index[i].start_index + j];
            if (str < data || str >= data_end || data_end-str < 2 || *str != 1)
                throw std::runtime_error ("invalid CST file format (2)");
            if (*str != 1)
                throw std::runtime_error ("invalid CST file format (3)");
            ++str;
            if (0x30 == *str)
                ++str;
            else if (0x20 != *str && 0x21 != *str)
                out << "\\x" << std::setw(2) << std::setfill('0') << (int)*str++ << ' ';

            auto str_end = std::find (str, data_end, 0);
            out.write (str, str_end-str);
            out.put ('\n');
        }
        out.put ('\n');
    }
}

void
uncompress_cst (const uint8_t* src, size_t src_size, std::vector<uint8_t>& out, size_t out_size)
{
    z_stream stream = { 0 };
    stream.next_in = const_cast<uint8_t*> (src);
    stream.avail_in = src_size;
    out.resize (out_size);

    int err = inflateInit (&stream);
    if (err != Z_OK)
        throw std::runtime_error ("zlib initialization error");

    stream.next_out = out.data();
    stream.avail_out = out.size();
    err = inflate (&stream, Z_NO_FLUSH);
    inflateEnd (&stream);

    if (Z_STREAM_ERROR == err)
        throw std::runtime_error ("invalid compressed stream");
}

int main (int argc, char* argv[])
try
{
    if (argc < 3)
    {
        std::cout << "usage: cst2txt INPUT OUTPUT\n";
        return 0;
    }

    sys::mapping::readonly in (argv[1]);
    sys::mapping::const_view<uint8_t> view (in);
    if (in.size() <= 0x10)
        throw std::runtime_error ("invalid CST file");
    std::vector<uint8_t> unpacked_cst;
    const uint8_t* cst_data = view.data();
    size_t cst_size = view.size();
    if (0 == std::memcmp (view.data(), "CatScene", 8))
    {
        size_t unpacked_size = *(const uint32_t*)&view[12];
        size_t packed_size = std::min<size_t> (*(const uint32_t*)&view[8], view.size()-0x10);
        uncompress_cst (view.data()+0x10, packed_size, unpacked_cst, unpacked_size);
        cst_data = unpacked_cst.data();
        cst_size = unpacked_cst.size();
    }
    std::ofstream out (argv[2], std::ios::out|std::ios::trunc);
    if (!out)
    {
        std::cerr << argv[2] << ": error opening file for writing\n";
        return 1;
    }
    extract_script (cst_data, cst_size, out);
    return 0;
}
catch (std::exception& X)
{
    std::cerr << argv[1] << ": " << X.what() << '\n';
    return 1;
}
