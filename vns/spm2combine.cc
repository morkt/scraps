// -*- C++ -*-
//! \file       spm2combine.cc
//! \date       2019 Jan 28
//! \brief      combine SPM 2.0 images for Giga games (love clear)
//

#include "png-convert.hpp"
#include "sysmemmap.h"
#include "sysfs.h"
#include <iostream>
#include <algorithm>
#include <map>
#include <vector>
#include <windows.h>

#pragma pack(1)

struct SpriteDef
{
    uint32_t entry_count;
    uint32_t width;
    uint32_t height;
    int32_t  base_x;
    int32_t  base_y;
    int32_t  base_cx;
    int32_t  base_cy;
    uint32_t unknown1;
    uint32_t unknown2;
    uint32_t unknown3;
    uint32_t unknown4;
};

struct LayerDef
{
    uint32_t index;
    int32_t  dst_x;
    int32_t  dst_y;
    int32_t  dst_cx;
    int32_t  dst_cy;
    uint32_t width;
    uint32_t height;
    int32_t  src_x;
    int32_t  src_y;
    int32_t  src_cx;
    int32_t  src_cy;
    uint32_t unknown1;
    uint32_t unknown2;
    uint32_t unknown3;
};

#pragma pack()

struct OverlayDef
{
    std::string name;
    int         base_id;
    int         overlay_id;
};

struct Sprite : SpriteDef
{
    std::vector<LayerDef>   layers;

    Sprite (const SpriteDef& def) : SpriteDef (def)
    {
    }
};

struct Image
{
    int                     bpp;
    RECT                    bounds;
    std::vector<uint8_t>    pixels;

    explicit Image (int depth) : bpp (depth)
    {
    }
};

typedef std::wstring tstring;

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
            if (src_pixel & 0xFF000000)
            {
                dst_ptr[col] = src_pixel;
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

std::map<std::string, Image> image_cache;

const Image& load_image (const std::string& filename, const LayerDef& sprite)
{
    auto it = image_cache.find (filename);
    if (it != image_cache.end())
        return it->second;

    std::string png_name = change_ext (filename, std::string (".png"));
    it = image_cache.insert (std::make_pair (filename, Image (32))).first;
    try
    {
        unsigned width, height;
        png::error rc = png::decode (png_name, it->second.pixels, &width, &height);
        if (png::error::none != rc)
            throw std::runtime_error (png_name+": file not found");
        if (width != sprite.width || height != sprite.height)
            throw std::runtime_error (png_name+": dimensions don't match");

        auto& rect = it->second.bounds;
        rect.left = sprite.src_x;
        rect.top  = sprite.src_y;
        rect.right = sprite.src_x + width;
        rect.bottom = sprite.src_y + height;
        return it->second;
    }
    catch (...)
    {
        image_cache.erase (it);
        throw;
    }
}

const Image& load_fil (const std::string& filename, const Sprite& sprite)
{
    auto it = image_cache.find (filename);
    if (it != image_cache.end())
        return it->second;
    sys::mapping::readonly in (filename);
    sys::mapping::const_view<uint8_t> view (in);
    if (view.size() != sprite.width * sprite.height)
        throw std::runtime_error (filename+": invalid alpha channel size");

    it = image_cache.insert (std::make_pair (filename, Image (8))).first;
    it->second.pixels.resize (view.size());
    std::copy (view.begin(), view.end(), it->second.pixels.begin());
    auto& rect = it->second.bounds;
    rect.left = sprite.base_x;
    rect.top  = sprite.base_y;
    rect.right = sprite.base_x + sprite.width;
    rect.bottom = sprite.base_y + sprite.height;
    return it->second;
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
    tstring output_dir;
    if (argc > 3 && 0 == std::wcscmp (argv[1], L"-o"))
    {
        output_dir = argv[2];
        argN += 2;
    }
    if (argN >= argc)
    {
        std::cout << "usage: spmcombine [-o OUTPUT-DIR] SPM-FILE\n";
        return 0;
    }
    if (!output_dir.empty())
        sys::mkdir (output_dir);

    sys::mapping::readonly in (argv[argN]);
    if (in.size() < 17)
        throw file_error (argv[argN], "invalid SPM file");
    sys::mapping::const_view<uint8_t> view (in);
    if (0 != std::memcmp (view.data(), "SPM VER-2.00", 13))
        throw file_error (argv[argN], "invalid SPM file");
    const uint8_t* data_end = view.end();

    std::vector<Sprite> sprites;

    auto data = &view[13];
    size_t entry_count  = *(const uint32_t*)data;
    data += 4;
    for (size_t i = 0; i < entry_count; ++i)
    {
        if (data + sizeof(SpriteDef) > data_end)
            throw file_error (argv[argN], "premature end of file");
        sprites.push_back (*reinterpret_cast<const SpriteDef*> (data));
        auto& sprite = sprites.back();
        data += sizeof(SpriteDef);
        for (size_t j = 0; j < sprite.entry_count; ++j)
        {
            if (data + sizeof(LayerDef) > data_end)
                throw file_error (argv[argN], "premature end of file");
            auto layer = reinterpret_cast<const LayerDef*> (data);
            sprite.layers.push_back (*layer);
            data += sizeof(LayerDef);
        }
    }
    if (data_end - data <= 4)
        throw file_error (argv[argN], "premature end of file");
    size_t name_count = *(const uint32_t*)data;
    if (!name_count)
        throw file_error (argv[argN], "[SPM] invalid number of partnames");

    data += 4;
    std::vector<std::string> partnames (name_count);
    for (auto& part : partnames)
    {
        if (data >= data_end)
            throw file_error (argv[argN], "premature end of file");
        auto name_end = std::find (data, data_end, 0);
        if (name_end == data)
            throw file_error (argv[argN], "[SPM] invalid filename");
        part.assign (data, name_end);
        data = name_end+1;
    }

    if (data_end - data <= 8)
        throw file_error (argv[argN], "premature end of file");
    size_t unknown_count = *reinterpret_cast<const uint32_t*> (data);
    size_t part_count = *reinterpret_cast<const uint32_t*> (data+4);
    if (!part_count)
        throw file_error (argv[argN], "[SPM] invalid number of parts");

    data += 8;
    std::vector<OverlayDef> overlays (part_count);
    for (auto& ovl : overlays)
    {
        auto name_end = std::find (data, data_end, 0);
        if (name_end == data)
            throw file_error (argv[argN], "[SPM] invalid filename");
        ovl.name.assign (data, name_end);
        data = name_end+1;
        if (data + 0x14 > data_end)
            throw file_error (argv[argN], "premature end of file");
        ovl.base_id = *reinterpret_cast<const uint32_t*> (data);
        ovl.overlay_id = *reinterpret_cast<const uint32_t*> (data+0x10);
        data += 0x14;
    }

    tstring base_name = change_ext (tstring (argv[argN]), tstring(L""));
    for (size_t i = 0; i < entry_count; ++i)
    {
        const auto& sprite = sprites[i];
        tstring sprite_name = base_name + L'_' + std::to_wstring (i) + L".png";
        if (sprite.layers.empty())
            continue;

        const std::string& base_layer_name = partnames[sprite.layers[0].index];
        try
        {
            Image base_layer = load_image (base_layer_name, sprite.layers[0]);
            std::wcout << sprite_name << std::endl;
            for (size_t i = 1; i < sprite.layers.size(); ++i)
            {
                const auto& layer = sprite.layers[i];
                if (layer.index >= name_count)
                    throw std::runtime_error ("invalid sprite index");
                int x = layer.dst_x - sprite.base_x;
                int y = layer.dst_y - sprite.base_y;
//                std::cout << "> " << partnames[layer.index] << ": [" << x << ", " << y << "]\n";
                const Image& overlay = load_image (partnames[layer.index], layer);
                bmp_blend (base_layer, x, y, overlay);
            }
            auto alpha_layer_name = change_ext (base_layer_name, std::string (".fil"));
            if (sys::file::exists (alpha_layer_name))
            {
                const auto& alpha = load_fil (alpha_layer_name, sprite);
                auto alpha_src = alpha.pixels.data();
                for (size_t i = 3; i < base_layer.pixels.size(); i += 4)
                    base_layer.pixels[i] = *alpha_src++;
            }
            if (!output_dir.empty())
                sprite_name = output_dir + L"\\" + sprite_name;
            auto rc = png::encode (sprite_name, base_layer.pixels.data(), sprite.width, sprite.height);
            if (png::error::none != rc)
            {
                std::wcerr << sprite_name << std::flush;
                std::cerr << ": " << png::get_error_text (rc) << std::endl;
            }
        }
        catch (std::exception& X)
        {
            std::cerr << X.what() << std::endl;
        }
    }
    return 0;
}
catch (file_error& X)
{
    std::wcerr << X.filename() << L": " << X.what() << std::endl;
}
catch (std::exception& X)
{
    std::cerr << X.what() << std::endl;
    return 1;
}
