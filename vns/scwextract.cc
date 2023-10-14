// -*- C++ -*-
//! \file       scwextract.cc
//! \date       Tue Jul 07 09:07:24 2015
//! \brief      extract text data from GsWin SCW files.
//

#include <iostream>
#include <fstream>
#include <vector>
#include "lzss.h"
#include "sysmemmap.h"
#include "membuf.hpp"

struct scw_script
{
public:
    scw_script (uint8_t* view, size_t view_size);

    void extract_text (std::ostream& out);

    static void decrypt (uint8_t* data, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
            data[i] ^= i;
    }

private:
    void read_scw4_header (uint8_t* view, size_t view_size);
    void read_scw5_header (uint8_t* view, size_t view_size);
    void unpack_data (uint8_t* data, size_t data_size);

private:
    int         version;
    bool        compressed;
    size_t      data_offset;
    size_t      packed_size;
    size_t      unpacked_size;
    size_t      number_of_commands;
    size_t      number_of_strings;
    size_t      number_of_extra;
    size_t      command_table_size;
    size_t      string_table_size;
    std::vector<uint8_t>    script_data;
};

scw_script::
scw_script (uint8_t* view, size_t view_size)
{
    if (0 == std::memcmp (view, "Scw4.x", 7))
        read_scw4_header (view, view_size);
    else if (0 == std::memcmp (view, "Scw5.x", 7))
        read_scw5_header (view, view_size);
    else
        throw std::runtime_error ("invalid input script");
    if (view_size < data_offset)
        throw std::runtime_error ("invalid input file data");

    unpack_data (view+data_offset, compressed ? packed_size : unpacked_size);

    size_t total_size = number_of_commands*8 + number_of_strings*8 + command_table_size + string_table_size;
    if (total_size > script_data.size())
        throw std::runtime_error ("invalid input file data");
}

void scw_script::
read_scw4_header (uint8_t* view, size_t view_size)
{
    version = 4;
    compressed = -1 == *(int32_t*)&view[0x14];
    unpacked_size = *(uint32_t*)&view[0x18];
    packed_size = *(uint32_t*)&view[0x1c];
    number_of_commands = *(uint32_t*)&view[0x24];
    number_of_strings = *(uint32_t*)&view[0x28];
    number_of_extra = *(uint32_t*)&view[0x2C];
    command_table_size = *(uint32_t*)&view[0x30];
    string_table_size = *(uint32_t*)&view[0x34];
    data_offset = 0x1c4;
}

void scw_script::
read_scw5_header (uint8_t* view, size_t view_size)
{
    version = 5;
    compressed = -1 == *(int32_t*)&view[0x14];
    unpacked_size = *(uint32_t*)&view[0x18];
    packed_size = *(uint32_t*)&view[0x1c];
    number_of_commands = *(uint32_t*)&view[0x24];
    number_of_strings = *(uint32_t*)&view[0x28];
    number_of_extra = *(uint32_t*)&view[0x2C];
    command_table_size = *(uint32_t*)&view[0x30];
    string_table_size = *(uint32_t*)&view[0x34];
    data_offset = 0x1c8;
}

void scw_script::
unpack_data (uint8_t* data, size_t data_size)
{
    decrypt (data, data_size);

    script_data.resize (unpacked_size);
    if (compressed)
    {
        sys::memory_buf mem_buf ((char*)script_data.data(), script_data.size(), std::ios::out);
        std::ostream out (&mem_buf);
        lzss_decompress (data, data_size, out);
    }
    else
        std::copy_n (data, data_size, script_data.data());
}

void scw_script::
extract_text (std::ostream& out)
{
    uint32_t* string_table = reinterpret_cast<uint32_t*> (script_data.data() + number_of_commands*8);
    char* string_data = reinterpret_cast<char*> (string_table) + number_of_strings*8 + number_of_extra*8 + command_table_size;
    size_t s_offset = string_data - (char*)script_data.data();
    for (size_t i = 0; i < number_of_strings; ++i)
    {
        if (0 != string_table[1])
        {
            if (string_table[0] > string_table_size
                || string_table[1] > string_table_size
                || string_table[0]+string_table[1] > string_table_size)
                throw std::runtime_error ("invalid string table");
            out.write (string_data+string_table[0], string_table[1]-1);
            out.put ('\n');
        }
        string_table += 2;
    }
}

void extract_scw3_text (const uint32_t* offset_table, size_t count, char* data, size_t size, std::ostream& out)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (*offset_table >= size)
            break;
        out << (data + *offset_table) << '\n';
        offset_table += 4;
    }
}

void extract_scw3 (uint8_t* view, size_t size, std::ostream& out)
{
    if (0x03000000 != *reinterpret_cast<uint32_t*> (&view[0x10]))
        throw std::runtime_error ("invalid SCW file version");

    size_t num1 = *(uint32_t*)&view[0x20];
    size_t num2 = *(uint32_t*)&view[0x24];
    size_t num3 = *(uint32_t*)&view[0x28];
    size_t num4 = *(uint32_t*)&view[0x38];
    size_t num5 = *(uint32_t*)&view[0x3C];
    size_t data_offset = 0x100;
    data_offset += num1 * 16;
    uint32_t* string_table1 = reinterpret_cast<uint32_t*> (view + data_offset);
    data_offset += num2 * 16;
    uint32_t* string_table2 = reinterpret_cast<uint32_t*> (view + data_offset);
    data_offset += num3 * 16;
    data_offset += num4 * 8;
    data_offset += num5 * 8;
    if (num1)
    {
        data_offset += *reinterpret_cast<uint32_t*> (&view[0x2C]);
    }
    if (num2)
    {
        auto data = view + data_offset;
        size_t size = *reinterpret_cast<uint32_t*> (&view[0x30]);
        if (size)
        {
            scw_script::decrypt (data, size);
            extract_scw3_text (string_table1, num2, reinterpret_cast<char*> (data), size, out);
            data_offset += size;
        }
    }
    if (num3)
    {
        auto data = view + data_offset;
        size_t size = *reinterpret_cast<uint32_t*> (&view[0x34]);
        if (size)
        {
            scw_script::decrypt (data, size);
            extract_scw3_text (string_table1, num2, reinterpret_cast<char*> (data), size, out);
            data_offset += size;
        }
    }
}

int main (int argc, char* argv[])
try
{
    if (argc < 3)
    {
        std::cout << "usage: scwextract INPUT OUTPUT\n";
        return 0;
    }
    sys::mapping::readwrite in (argv[1], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    if (view.size() > 0x14 && 0 == std::memcmp (view.data(), "SCW for GsWin", 13)
        && 0x03000000 == *reinterpret_cast<uint32_t*> (&view[0x10]))
    {
        std::ofstream out (argv[2], std::ios::out|std::ios::trunc);
        extract_scw3 (view.data(), view.size(), out);
    }
    else
    {
        scw_script scw (view.data(), view.size());
        std::ofstream out (argv[2], std::ios::out|std::ios::trunc);
        scw.extract_text (out);
    }
    return 0;
}
catch (std::exception& X)
{
    std::cerr << argv[1] << ": " << X.what() << '\n';
    return 1;
}
