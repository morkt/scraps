// -*- C++ -*-
//! \file       pngalpha.cc
//! \date       Tue Jul 07 16:35:00 2015
//! \brief      add alpha channel to PNG file.
//

#include "png-convert.hpp"
#include <iostream>
#include <algorithm>

std::wstring
convert_filename (std::wstring filename)
{
    size_t dot = filename.rfind (L'.');
    if (dot != std::wstring::npos)
        filename.insert (dot, L"~");
    else
        filename += L'~';
    return filename;
}

std::wstring    base_filename;
bool            g_ignore_offs;
bool            g_premultiply;
bool            g_quiet;

int wmain (int argc, wchar_t* argv[])
try
{
    int argN = 1;
    std::wstring out_name;
    while (argN < argc)
    {
        if (argN+1 < argc && 0 == std::wcscmp (argv[argN], L"-o"))
        {
            out_name = argv[argN+1];
            argN += 2;
        }
        else if (0 == std::wcscmp (argv[argN], L"-i"))
        {
            g_ignore_offs = true;
            ++argN;
        }
        else if (0 == std::wcscmp (argv[argN], L"-p"))
        {
            g_premultiply = true;
            ++argN;
        }
        else if (0 == std::wcscmp (argv[argN], L"-q"))
        {
            g_quiet = true;
            ++argN;
        }
        else
            break;
    }
    if (argc < argN + 2)
    {
        std::cout << "usage: pngalpha [-o OUTPUT] INPUT.png [~] MASK.png\n"
                     "    -i    ignore embedded coordinates\n"
                     "    -p    premulitply colors by alpha value\n"
                     "    -q    be quiet\n"
                     "specifying '~' will invert mask before applying\n";
        return 0;
    }
    uint8_t invert_mask = 0;
    base_filename = argv[argN];
    const wchar_t* mask_filename = argv[argN+1];
    if (argc > argN + 2 && 0 == std::wcscmp (argv[argN+1], L"~"))
    {
        invert_mask = 0xff;
        mask_filename = argv[argN+2];
    }

    std::vector<uint8_t> image;
    unsigned width, height;
    int base_x, base_y;
    png::error rc = png::decode (base_filename, image, &width, &height, &base_x, &base_y);
    if (png::error::none != rc)
        throw std::runtime_error (png::get_error_text (rc));

    std::vector<uint8_t> mask;
    unsigned mask_w, mask_h;
    int mask_x, mask_y;
    rc = png::decode_grayscaled (mask_filename, mask, &mask_w, &mask_h, &mask_x, &mask_y);
    if (png::error::none != rc)
    {
        std::wcerr << mask_filename << ": " << png::get_error_text (rc) << std::endl;
        return 1;
    }

    int offset_y = g_ignore_offs ? 0 : (mask_y - base_y);
    int offset_x = g_ignore_offs ? 0 : (mask_x - base_x);

    if (offset_y > 0)
    {
        unsigned top_block = offset_y*width;
        for (unsigned i = 0; i < top_block; ++i)
            image[i*4+3] = 0;
    }

    int region_x = std::max (offset_x, 0);
    int region_y = std::max (offset_y, 0);
    int region_w = std::min ((int)width, offset_x+(int)mask_w);
    int region_h = std::min ((int)height, offset_y+(int)mask_h);

    unsigned base_stride = width * 4;
    for (int y = region_y; y < region_h; ++y)
    {
        auto image_pos = image.begin() + y * base_stride + 3;
        int x;
        for (x = 0; x < region_x; ++x)
        {
            *image_pos = 0;
            image_pos += 4;
        }
        auto mask_pos = mask.begin() + (y-offset_y)*mask_w;
        if (offset_x < 0)
            mask_pos -= offset_x;
        for ( ; x < region_w; ++x)
        {
            uint8_t alpha = *mask_pos++;
            if (invert_mask)
                alpha = ~alpha;
            *image_pos = alpha;
            if (alpha < 0xFF && g_premultiply)
            {
                image_pos[-3] = image_pos[-3] * alpha / 0xFF;
                image_pos[-2] = image_pos[-2] * alpha / 0xFF;
                image_pos[-1] = image_pos[-1] * alpha / 0xFF;
            }
            image_pos += 4;
        }
        for ( ; x < width; ++x)
        {
            *image_pos = 0;
            image_pos += 4;
        }
    }

    for (auto i = region_h * base_stride + 3; i < image.size(); i += 4)
        image[i] = 0;

    if (out_name.empty())
        out_name = convert_filename (base_filename);
    if (!g_quiet)
        std::wcout << base_filename << L" + " << mask_filename << L" -> " << out_name << std::endl;
    rc = png::encode (out_name, image.data(), width, height, mask_x, mask_y);
    if (png::error::none != rc)
    {
        std::wcerr << out_name << ": " << png::get_error_text (rc) << std::endl;
        return 1;
    }

    return 0;
}
catch (std::exception& X)
{
    std::wcerr << base_filename << L": " << X.what() << std::endl;
    return 1;
}
