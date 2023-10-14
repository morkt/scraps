// -*- C++ -*-
//! \file       cmpfirst.cc
//! \date       Wed Sep 14 18:41:57 2016
//! \brief      compare first bytes of file and report offset where they differ.
//

#include "sysmemmap.h"

int main (int argc, char* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: cmpfirst FILE1 FILE2");
        return 0;
    }
    sys::mapping::readonly file1 (argv[1]);
    sys::mapping::readonly file2 (argv[2]);
    size_t common_size = std::min (file1.size(), file2.size());
    if (0 == common_size)
    {
        std::printf ("%08x: complete different\n", 0);
        return 0;
    }
    sys::mapping::const_view<uint8_t> view1 (file1, 0, common_size);
    sys::mapping::const_view<uint8_t> view2 (file2, 0, common_size);
    size_t pos = 0;
    while (pos < common_size)
    {
        if (view1[pos] != view2[pos])
            break;
        ++pos;
    }
    if (pos == common_size)
    {
        if (file1.size() == file2.size())
            std::puts ("files are identical");
        else if (pos == file1.size())
            std::printf ("%s: fully included into %s\n", argv[1], argv[2]);
        else
            std::printf ("%s: fully included into %s\n", argv[2], argv[1]);
        return 0;
    }
    std::printf ("%08X: difference position\n", pos);
    return 0;
}
catch (sys::file_error& X)
{
    std::fprintf (stderr, "%s: %s\n", X.get_filename<char>(), X.what());
    return 1;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "cmpfirst: %s\n", X.what());
    return 1;
}
