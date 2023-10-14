// -*- C++ -*-
//! \file       rtfenc.cc
//! \date       2018 Jul 09
//! \brief      convert rtf files to text encoding.
//

#include "sysmemmap.h"
#include <fstream>
#include <cstdio>

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: rtfenc INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    sys::mapping::const_view<char> view (in);
    std::ofstream out (argv[2], std::ios::out|std::ios::trunc|std::ios::binary);
    if (!out)
    {
        std::fprintf (stderr, "%S: error opening output file\n", argv[2]);
        return 1;
    }
    bool last_char_was_hex = false;
    std::string hex;
    for (auto p = view.begin(); p != view.end(); ++p)
    {
        if (*p != '\\')
        {
            out.put (*p);
        }
        else
        {
            if (std::next (p) != view.end())
            {
                if (p[1] == '\\')
                {
                    out.write ("\\\\", 2);
                    ++p;
                }
                else if (p[1] == '\'' && (view.end() - p) >= 4)
                {
                    hex.assign (p+2, p+4);
                    int x;
                    int rc = std::sscanf (hex.c_str(), "%x", &x);
                    if (1 == rc)
                    {
                        out.put (static_cast<char> (x));
                        p += 3;
                        last_char_was_hex = true;
                    }
                }
                else if (p[1] == '{' && last_char_was_hex)
                {
                    out.put (p[1]);
                    ++p;
                }
                else
                {
                    out.put ('\\');
                }
            }
        }
    }
    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
