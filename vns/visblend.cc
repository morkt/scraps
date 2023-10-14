// -*- C++ -*-
//! \file       visblend.cc
//! \date       2019 Apr 02
//! \brief      blend event images for GIGA games (love clear)
//

#include "png-convert.hpp"
#include "sysmemmap.h"
#include "sysfs.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <windows.h>

struct Image
{
    int                     bpp;
    RECT                    bounds;
    std::vector<uint8_t>    pixels;

    explicit Image (int depth) : bpp (depth), bounds ({0})
    {
    }
};

typedef std::string tstring;

void
bmp_blend (Image& image, int x, int y, const Image& overlay)
{
    if (image.bpp != overlay.bpp)
        throw std::runtime_error ("incompatible color depths");

    int overlay_width = overlay.bounds.right - overlay.bounds.left;
    int overlay_height = overlay.bounds.bottom - overlay.bounds.top;
    RECT src, dst;
    src.left = image.bounds.left + x;
    src.right = src.left + overlay_width;
    src.top = image.bounds.top + y;
    src.bottom = src.top + overlay_height;
    ::IntersectRect (&dst, &image.bounds, &src);

    int w = dst.right - dst.left;
    int h = dst.bottom - dst.top;
    if (w <= 0 || h <= 0)
    {
        std::cerr << "[bmp_blend] empty overlay\n";
        return;
    }

    int base_width = image.bounds.right - image.bounds.left;
//    int bpp = image.bpp / 8;
//    int image_stride = base_width * bpp;
//    int overlay_stride = overlay_width * bpp;
    uint32_t* dst_ptr = reinterpret_cast<uint32_t*> (image.pixels.data()) + y * base_width + x;
    const uint32_t* ov = reinterpret_cast<const uint32_t*> (overlay.pixels.data());
    for (int row = 0; row < h; ++row)
    {
        for (int col = 0; col < w; ++col)
        {
            auto src_pixel = ov[col];
            if (src_pixel & 0xFFFFFF)
            {
                dst_ptr[col] = src_pixel | 0xFF000000;
            }
        }
        dst_ptr += base_width;
        ov  += overlay_width;
    }
}

template <typename CharT>
struct path_helper
{
    static const CharT* path_delimiter;
    static const CharT  dot;
};

template<>
const char* path_helper<char>::path_delimiter = "\\/";
template<>
const wchar_t* path_helper<wchar_t>::path_delimiter = L"\\/";
template<>
const char path_helper<char>::dot = '.';
template<>
const wchar_t path_helper<wchar_t>::dot = L'.';

template <typename CharT>
std::basic_string<CharT>
change_ext (std::basic_string<CharT> filename, const std::basic_string<CharT>& ext)
{
    const size_t slash = filename.find_last_of (path_helper<CharT>::path_delimiter);
    const bool ext_starts_with_dot = !ext.empty() && ext[0] == path_helper<CharT>::dot;
    size_t dot = filename.rfind (path_helper<CharT>::dot);
    if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
    {
        if (ext_starts_with_dot)
            filename.replace (dot, std::string::npos, ext);
        else if (!ext.empty())
            filename.replace (dot, std::string::npos, path_helper<CharT>::dot+ext);
        else
            filename.erase (dot);
    }
    else if (ext_starts_with_dot)
        filename += ext;
    else if (!ext.empty())
        filename += path_helper<CharT>::dot+ext;
    return filename;
}

tstring
convert_filename (tstring filename)
{
    size_t dot = filename.rfind (path_helper<tstring::value_type>::dot);
    if (dot != tstring::npos)
        filename.insert (dot, "~");
    else
        filename += '~';
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

static inline uint32_t read_u32 (const uint8_t* data)
{
    return *reinterpret_cast<const uint32_t*> (data);
}

int main (int argc, char* argv[])
try
{
    int argN = 1;
    tstring output_dir;
    if (argc > 3 && 0 == std::strcmp (argv[1], "-o"))
    {
        output_dir = argv[2];
        argN += 2;
    }
    if (argN >= argc)
    {
        std::cout << "usage: visblend [-o OUTPUT-DIR] visual.dat\n";
        return 0;
    }
    if (!output_dir.empty())
        sys::mkdir (output_dir);

    sys::mapping::readonly in (argv[argN]);
    sys::mapping::const_view<uint8_t> view (in);
    const uint8_t* data = view.data();

    size_t header_count = read_u32 (data);
    size_t header_size = 4 + header_count * 4;
    if (header_size >= view.size())
        throw std::runtime_error ("invalid visual.dat file");

    std::string base_name, diff_name, out_name;
    Image base_image (32), diff_image (32);

    static const std::string png_ext (".png");

    data = data + header_size;
    auto data_end = view.end();
    while (data + 0x20 < data_end)
    {
        data += 0x20;
        auto name_end = std::find (data, data_end, 0);
        if (name_end == data_end)
            break;
        base_name.assign (data, name_end);
        data = name_end+1;
        name_end = std::find (data, data_end, 0);
        diff_name.assign (data, name_end);
        data = name_end+1;
        int x = read_u32 (data);
        data += 4;
        int y = read_u32 (data);
        data += 4 /*+ 8*/;
        if (diff_name.empty())
            continue;
        base_name = change_ext (base_name, png_ext);
        diff_name = change_ext (diff_name, png_ext);

        if (!sys::file::exists (base_name) || !sys::file::exists (diff_name))
            continue;

        unsigned width, height;
        auto rc = png::decode (base_name, base_image.pixels, &width, &height);
        if (png::error::none != rc)
        {
            std::cerr << base_name << ": " << png::get_error_text (rc) << std::endl;
            continue;
        }
        base_image.bounds = { 0, 0, (long)width, (long)height };
        rc = png::decode (diff_name, diff_image.pixels, &width, &height);
        if (png::error::none != rc)
        {
            std::cerr << diff_name << ": " << png::get_error_text (rc) << std::endl;
            continue;
        }
        diff_image.bounds = { 0, 0, (long)width, (long)height };

        bmp_blend (base_image, x, y, diff_image);
        if (!output_dir.empty())
            out_name = output_dir + "\\" + diff_name;
        else
            out_name = convert_filename (diff_name);

        std::cout << base_name << " + " << diff_name << " -> " << out_name << std::endl;
        rc = png::encode (out_name, base_image.pixels.data(), base_image.bounds.right, base_image.bounds.bottom);
        if (png::error::none != rc)
        {
            std::cerr << out_name << ": " << png::get_error_text (rc) << std::endl;
        }
    }
    return 0;
}
catch (file_error& X)
{
    std::cerr << X.filename() << ": " << X.what() << std::endl;
}
catch (std::exception& X)
{
    std::cerr << X.what() << std::endl;
    return 1;
}

