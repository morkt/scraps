// -*- C++ -*-
//! \file       pngmask.cc
//! \date       Sat Jan 16 09:17:42 2016
//! \brief      apply embedded mask to character images.
//

#include "png-convert.hpp"
#include <string>
#include <memory>

typedef std::wstring tstring;

void convert_masked_image (unsigned width, unsigned height, const uint8_t* input, uint8_t* output, bool inverse_mask = false)
{
    size_t input_stride  = width * 8;
    auto mask = input_stride / 2;
    for (auto y = 0; y < height; ++y)
    {
        auto src = input;
        for (auto x = 0; x < width; ++x)
        {
            *output++ = src[0];
            *output++ = src[1];
            *output++ = src[2];
            unsigned alpha = (src[mask] + src[mask+1] + src[mask+2]) / 3;
            if (inverse_mask)
                alpha = ~alpha;
            *output++ = alpha;
            src += 4;
        }
        input += input_stride;
    }
}

tstring
convert_filename (tstring filename)
{
    size_t dot = filename.rfind (L'.');
    if (dot != tstring::npos)
        filename.insert (dot, L"~");
    else
        filename += L'~';
    return filename;
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: pngmask [-i] FILENAME\n  -i  inverse mask values");
        return 0;
    }
    bool inverse_mask = false;
    int argN = 1;
    if (argc > 2 && !std::wcscmp (argv[argN], L"-i"))
    {
        inverse_mask = true;
        ++argN;
    }

    tstring in_name = argv[argN];
    std::vector<uint8_t> input_image;
    unsigned width, height;
    int x, y;
    png::error rc = png::decode (in_name, input_image, &width, &height, &x, &y);
    if (png::error::none != rc)
        throw std::runtime_error (png::get_error_text (rc));
    if (width & 1)
    {
        std::fprintf (stderr, "%S: image width should be even [%d]\n", in_name.c_str(), width);
        return 2;
    }
    width /= 2;
    std::vector<uint8_t> image (width*height*4);
    convert_masked_image (width, height, input_image.data(), image.data(), inverse_mask);

    tstring out_name (convert_filename (in_name));
    std::printf ("%S -> %S\n", in_name.c_str(), out_name.c_str());
    rc = png::encode (out_name, image.data(), width, height, png::format::bgra32);
    if (png::error::none != rc)
        std::fprintf (stderr, "%S: %s\n", out_name.c_str(), png::get_error_text (rc));

    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "pngmask: %s\n", X.what());
    return 1;
}
