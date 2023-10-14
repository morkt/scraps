// -*- C++ -*-
//! \file       abmp2png.cc
//! \date       Sun Jul 05 05:52:20 2015
//! \brief      convert BMP with appended alpha channel to PNG
//

#include "sysmemmap.h"
#include "png-convert.hpp"
#include <cstdio>
#include <fstream>

typedef std::wstring tstring;

bool g_scale_alpha;

void
read_bmp (const tstring& filename, std::vector<uint8_t>& image_data, unsigned& width, unsigned& height)
{
    sys::mapping::readonly in (filename);
    if (in.size() < 54)
        throw std::runtime_error ("invalid BMP size");

    sys::mapping::const_view<uint8_t> view (in);
    if ('B' != view[0] || 'M' != view[1])
        throw std::runtime_error ("not a BMP format");
    int bpp = *(const uint16_t*)&view[0x1c];
    if (0x18 != bpp)
        throw std::runtime_error ("invalid BMP bitdepth");

    width  = *(const uint32_t*)&view[0x12];
    height = *(const uint32_t*)&view[0x16];
    if (0 == width || 0 == height)
        throw std::runtime_error ("invalid BMP image dimensions");
    if (view.size() < 54 + width*height*3)
        throw std::runtime_error ("invalid BMP image size");

    size_t bmp_size = *(const uint32_t*)&view[2];
    size_t alpha_size = width*height;
    if (view.size() < bmp_size || view.size() - bmp_size < alpha_size)
        throw std::runtime_error ("no alpha channel appended");

    const unsigned dst_stride = width*4;
    image_data.resize (dst_stride*height);

    const uint8_t* src = view.data() + 54;
    const uint8_t* alpha = view.data() + bmp_size;
    /*
    uint8_t max_alpha = 0;
    if (g_scale_alpha)
    {
        for (size_t a = 0; max_alpha < 0xFF && a < alpha_size; ++a)
            if (max_alpha < alpha[a])
                max_alpha = alpha[a];
        if (0xFF == max_alpha)
            g_scale_alpha = false;
        std::printf ("max_alpha: %02X\n", max_alpha);
    }
    */
    uint8_t* dst = image_data.data() + dst_stride*(height-1);
    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < dst_stride; x += 4)
        {
            int a = *alpha++;
            if (g_scale_alpha)
                a = std::min (a * 0xFF / 0x80, 0xFF);
            dst[x] = *src++;
            dst[x+1] = *src++;
            dst[x+2] = *src++;
            dst[x+3] = a;
        }
        dst -= dst_stride;
    }
}

tstring
convert_filename (tstring filename, const wchar_t* ext)
{
    size_t dot = filename.rfind ('.');
    if (dot != tstring::npos)
        filename.replace (dot, tstring::npos, ext);
    else
        filename.append (ext);
    return filename;
}

int wmain (int argc, wchar_t* argv[])
{
    if (argc < 2)
    {
        std::puts ("usage: abmp2png [-s] FILENAME\n"
                   "    -s  scale alpha values");
        return 0;
    }
    int argN = 1;
    if (argc > 2 && 0 == std::wcscmp (argv[argN], L"-s"))
    {
        g_scale_alpha = true;
        argN++;
    }
    try
    {
        unsigned width, height;
        std::vector<uint8_t> image;
        read_bmp (argv[argN], image, width, height);

        tstring out_name (convert_filename (argv[argN], L".png"));
        png::error rc = png::encode (out_name, image.data(), width, height);
        if (png::error::none != rc)
            std::fprintf (stderr, "%S: %s\n", out_name.c_str(), png::get_error_text (rc));
        return 0;
    }
    catch (std::exception& X)
    {
        std::fprintf (stderr, "%S: %s\n", argv[argN], X.what());
        return 1;
    }
}
