// -*- C++ -*-
//! \file       pngblend.cc
//! \date       Mon Oct 19 15:05:45 2015
//! \brief      alpha-blend png images.
//

#include "png-convert.hpp"
#include <algorithm>
#include <stdexcept>
#include <windows.h>

typedef std::wstring tstring;

void
copy_image (std::vector<uint8_t>& image, int image_stride, int x, int y,
            const std::vector<uint8_t>& overlay, int overlay_stride, const RECT& src)
{
    int w = src.right - src.left;
    int h = src.bottom - src.top;
    if (w <= 0 || h <= 0)
        return;

    uint8_t* dst = image.data() + y * image_stride + x * 4;
    const uint8_t* ov = overlay.data() + src.top * overlay_stride + src.left * 4;
    for (int row = 0; row < h; ++row)
    {
        std::memcpy (dst, ov, w*4);
        dst += image_stride;
        ov  += overlay_stride;
    }
}

void
alpha_blend (std::vector<uint8_t>& image, int image_stride, int x, int y,
             const std::vector<uint8_t>& overlay, int overlay_stride, const RECT& src)
{
    int w = src.right - src.left;
    int h = src.bottom - src.top;
    if (w <= 0 || h <= 0)
        return;

    uint8_t* dst = image.data() + y * image_stride + x * 4;
    const uint8_t* ov = overlay.data() + src.top * overlay_stride + src.left * 4;
    for (int row = 0; row < h; ++row)
    {
        for (int col = 0; col < w; ++col)
        {
            auto src_pixel = ov+col*4;
            auto src_alpha = src_pixel[3];
            if (src_alpha > 0)
            {
                auto dst_pixel = dst+col*4;
                if (0xFF == src_alpha || 0 == dst_pixel[3])
                {
                    std::memcpy (dst_pixel, src_pixel, 4);
                }
                else
                {
                    dst_pixel[0] = (src_pixel[0] * src_alpha + dst_pixel[0] * (0xFF - src_alpha)) / 0xFF;
                    dst_pixel[1] = (src_pixel[1] * src_alpha + dst_pixel[1] * (0xFF - src_alpha)) / 0xFF;
                    dst_pixel[2] = (src_pixel[2] * src_alpha + dst_pixel[2] * (0xFF - src_alpha)) / 0xFF;
                    dst_pixel[3] = std::max (src_alpha, dst_pixel[3]);
                }
            }
        }
        dst += image_stride;
        ov  += overlay_stride;
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
    tstring output_name;
    unsigned transparent_color = 0;
    bool ignore_coords = false;
    bool ignore_base_coords = false;
    int argN = 1;
    while (argN < argc)
    {
        if (argv[argN][0] != '-')
            break;
        if (argv[argN][1] == '-')
        {
            argN++;
            break;
        }
        if (0 == std::wcscmp (argv[argN], L"-o"))
        {
            if (++argN < argc)
                output_name = argv[argN];
        }
        else if (0 == std::wcscmp (argv[argN], L"-g"))
        {
            transparent_color = 0x00FF00;
        }
        else if (0 == std::wcscmp (argv[argN], L"-i"))
        {
            ignore_coords = true;
        }
        else if (0 == std::wcscmp (argv[argN], L"-ib"))
        {
            ignore_base_coords = true;
        }
        ++argN;
    }
    if (argN+1 >= argc)
    {
        std::puts ("usage: pngblend [-o OUTPUT] BASE OVERLAY1 OVERLAY2...\n"
                   "    -i    ignore all embedded coordinates\n"
                   "    -ib   ignore coordinates embedded into base image\n"
                   "    -g    interpret green #00FF00 color as transparent");
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
    if (ignore_coords || ignore_base_coords)
        x = y = 0;

    int image_stride = width * 4;
    RECT base = { x, y, x + static_cast<int> (width), y + static_cast<int> (height) };
//    std::printf ("%S: base_x: %d, base_y: %d\n", argv[argN], x, y);

    std::vector<uint8_t> overlay;
    unsigned overlay_w, overlay_h;
    int overlay_x, overlay_y;
    ++argN;
    while (argN < argc)
    {
        rc = png::decode (argv[argN], overlay, &overlay_w, &overlay_h, &overlay_x, &overlay_y);
        if (png::error::none != rc)
            throw file_error (argv[argN], png::get_error_text (rc));
        if (ignore_coords)
            overlay_x = overlay_y = 0;
        if (transparent_color != 0)
        {
            uint32_t* src = reinterpret_cast<uint32_t*> (overlay.data());
            uint32_t* src_end = src + (overlay.size() / 4);
            for (; src != src_end; ++src)
            {
                if ((*src & 0xFFFFFF) == transparent_color)
                    *src = 0;
            }
        }
        const int overlay_stride = overlay_w * 4;
        RECT patch = { 
            overlay_x, overlay_y,
            overlay_x + static_cast<int> (overlay_w),
            overlay_y + static_cast<int> (overlay_h)
        };
        if (patch.left < base.left || patch.top < base.top || patch.right > base.right || patch.bottom > base.bottom)
        {
            // overlay extends background
            RECT new_base;
            ::UnionRect (&new_base, &base, &patch);
            int new_width = new_base.right - new_base.left;
            int new_height = new_base.bottom - new_base.top;
            int new_stride = 4 * new_width;
            std::vector<uint8_t> new_image (new_stride * new_height);
            RECT base_src = { 0, 0, static_cast<long> (width), static_cast<long> (height) };
            copy_image (new_image, new_stride, base.left-new_base.left, base.top-new_base.top,
                        image, image_stride, base_src);

            std::swap (image, new_image);
            base = new_base;
            image_stride = new_stride;
            width = new_width;
            height = new_height;
            x = new_base.left;
            y = new_base.top;
        }
        RECT blend;
        ::IntersectRect (&blend, &base, &patch);
        int blend_x = blend.left - patch.left;
        int blend_y = blend.top - patch.top;
        int blend_w = blend.right - blend.left;
        int blend_h = blend.bottom - blend.top;
        patch = { blend_x, blend_y, blend_x + blend_w, blend_y + blend_h };
//        std::printf ("%S: patch_x: %d, patch_y: %d\n", argv[argN], blend_x, blend_y);
        alpha_blend (image, image_stride, blend.left - base.left, blend.top - base.top, overlay, overlay_stride, patch);
        ++argN;
    }

    rc = png::encode (output_name, image.data(), width, height, x, y);
    if (png::error::none != rc)
        throw file_error (output_name, png::get_error_text (rc));
    std::printf ("%S\n", output_name.c_str());

    return 0;
}
catch (file_error& X)
{
    std::fprintf (stderr, "%S: %s\n", X.filename().c_str(), X.what());
    return 1;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%s\n", X.what());
    return 1;
}
