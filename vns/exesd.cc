// -*- C++ -*-
//! \file       exesd.cc
//! \date       2017 Dec 08
//! \brief      extract ESD scripts (Tail software).
//

#include <iostream>
#include <fstream>
#include <vector>
#include "sysmemmap.h"

int main (int argc, char* argv[])
try
{
    if (argc < 3)
    {
        std::cout << "usage: exEsd INPUT OUTPUT\n";
        return 0;
    }

    sys::mapping::readonly in (argv[1]);
    sys::mapping::const_view<uint8_t> view (in);
    if (view.size() <= 8 || 0 != std::memcmp (view.data(), "HP\0", 4))
        throw std::runtime_error ("invalid input file");

    size_t unpacked_size = *reinterpret_cast<const uint32_t*> (&view[8]);
    std::unique_ptr<char[]> unpacked (new char[unpacked_size]);
    std::vector<int> tree_nodes (0xC00);

    int root_token = *reinterpret_cast<const int32_t*> (&view[12]);
    int dword_46C494 = *reinterpret_cast<const int32_t*> (&view[16]);
    int packed_size = *reinterpret_cast<const int32_t*> (&view[20]);
    int node_count = dword_46C494 + root_token - 255;
    const int32_t* tree_src = reinterpret_cast<const int32_t*> (&view[24]);
    while (node_count --> 0)
    {
        int node = 6 * tree_src[0];
        tree_nodes[node + 1] = tree_src[1];
        tree_nodes[node + 2] = tree_src[2];
        tree_src += 3;
    }
    const uint8_t* src = reinterpret_cast<const uint8_t*> (tree_src);
    int bit_num = 8;
    char* output = unpacked.get();
    uint8_t bits = 0;
    uint8_t bit_mask = 0;
    for (int i = 0; i < packed_size; ++i)
    {
        int symbol = root_token;
        do
        {
            if (bit_num > 7)
            {
                bits = *src++;
                bit_num = 0;
                bit_mask = 128;
            }
            ++bit_num;
            int node = ((bits & bit_mask) != 0) + 6 * symbol;
            bit_mask >>= 1;
            symbol = tree_nodes[node + 1];
        }
        while (tree_nodes[6 * symbol + 1] != -1);
        *output++ = symbol;
    }

    std::ofstream out (argv[2], std::ios::out|std::ios::binary|std::ios::trunc);
    if (!out)
    {
        std::cerr << argv[2] << ": error opening output file\n";
        return 1;
    }
    std::cout << argv[1] << " -> " << argv[2] << std::endl;
    out.write (unpacked.get(), unpacked_size);

    return 0;
}
catch (std::exception& X)
{
    std::cerr << argv[1] << ": " << X.what() << '\n';
    return 1;
}
