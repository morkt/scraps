// -*- C++ -*-
//! \file       antblend.cc
//! \date       Mon Jan 04 21:55:02 2016
//! \brief      blend Izumo PNG+ANT images.
//

#include "png-convert.hpp"
#include <iostream>
#include <algorithm>
#include <windows.h>

typedef std::wstring tstring;

void
anti_blend (std::vector<uint8_t>& image, const std::vector<uint8_t>& overlay)
{
    uint8_t* dst = image.data();
    uint8_t* const dst_end = dst + image.size();
    const uint8_t* ov = overlay.data();
    while (dst < dst_end)
    {
        uint32_t* dst_pixel = reinterpret_cast<uint32_t*> (dst);
        if (0 == (*dst_pixel & 0xFFFFFF))
        {

            *dst_pixel = *reinterpret_cast<const uint32_t*> (ov);
        }
        else if (0 != ov[3])
        {
            uint8_t ov_alpha = ov[3];
            dst[0] = (ov[0] * ov_alpha + dst[0] * (0xFF - ov_alpha)) / 0xFF;
            dst[1] = (ov[1] * ov_alpha + dst[1] * (0xFF - ov_alpha)) / 0xFF;
            dst[2] = (ov[2] * ov_alpha + dst[2] * (0xFF - ov_alpha)) / 0xFF;
            dst[3] = std::max (ov_alpha, dst[3]);
        }
        dst += 4;
        ov  += 4;
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

struct file_error : std::runtime_error
{
    file_error (const tstring& filename, const char* message)
        : std::runtime_error (message)
        , m_filename (filename)
    { }

    const tstring& filename () const { return m_filename; }

private:
    tstring     m_filename;
};

int wmain (int argc, wchar_t* argv[])
try
{
    int argN = 1;
    tstring output_name;
    if (argc > 3 && 0 == std::wcscmp (argv[1], L"-o"))
    {
        output_name = argv[2];
        argN += 2;
    }
    if (argN+1 >= argc)
    {
        std::cout << "usage: pngblend [-o OUTPUT] BASE ANTI\n";
        return 0;
    }
    if (output_name.empty())
        output_name = convert_filename (argv[argN]);
    std::vector<uint8_t> image;
    unsigned width, height;
    int x, y;
    png::error rc = png::decode (argv[argN], image, &width, &height, &x, &y);
    if (png::error::none != rc)
        throw file_error (argv[argN], png::get_error_text (rc));

    const int image_stride = width * 4;

    std::vector<uint8_t> overlay;
    unsigned overlay_w, overlay_h;
    ++argN;
    rc = png::decode (argv[argN], overlay, &overlay_w, &overlay_h);
    if (png::error::none != rc)
        throw file_error (argv[argN], png::get_error_text (rc));
    if (width != overlay_w || height != overlay_h)
        throw std::runtime_error ("image dimensions don't match");
    anti_blend (image, overlay);

    rc = png::encode (output_name, image.data(), width, height, x, y);
    if (png::error::none != rc)
        throw file_error (output_name, png::get_error_text (rc));
    std::wcout << output_name << std::endl;

    return 0;
}
catch (file_error& X)
{
    std::wcerr << X.filename() << L": " << X.what() << std::endl;
}
catch (std::exception& X)
{
    std::cerr << X.what() << '\n';
    return 1;
}
