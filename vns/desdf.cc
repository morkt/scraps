// -*- C++ -*-
//! \file       desdf.cc
//! \date       2018 Mar 19
//! \brief      decrypt SDF scripts.
//

#include <cstdio>
#include <fstream>
#include <string>

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: deadx INPUT OUTPUT");
        return 0;
    }
    std::ifstream in (argv[1], std::ios::in);
    if (!in)
    {
        std::fprintf (stderr, "%S: error opening input file\n", argv[1]);
        return 1;
    }
    std::ofstream out (argv[2], std::ios::out|std::ios::binary|std::ios::trunc);
    if (!out)
    {
        std::fprintf (stderr, "%S: error opening output file\n", argv[2]);
        return 1;
    }
    std::string line;
    while (std::getline (in, line))
    {
        for (auto& ch : line)
            ch -= 1;
        out.write (line.data(), line.size());
        out.put ('\n');
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}

