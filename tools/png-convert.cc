// -*- C++ -*-
//! \file       png-convert.cc
//! \date       Mon Feb 03 00:10:48 2014
//! \brief      convert RGBA data to/from PNG format.
//
//
// The oFFs chunk contains:
//   X position:     4 bytes (signed integer)
//   Y position:     4 bytes (signed integer)
//   Unit specifier: 1 byte
//
// The following values are legal for the unit specifier:
//   0: unit is the pixel (true dimensions unspecified)
//   1: unit is the micrometer
//

#include <fstream>
#include <png.h>
#include <setjmp.h>
#include "png-convert.hpp"

namespace png {

void
read_stream (png_structp png_ptr, png_bytep data, png_size_t length)
{
    std::istream* in = static_cast<std::istream*> (png_get_io_ptr (png_ptr));
    if (!in->read ((char*)data, length) || static_cast<size_t> (in->gcount()) != length)
        png_error (png_ptr, "file read error");
}

void
write_stream (png_structp png_ptr, png_bytep data, png_size_t length)
{
    void* io_ptr = png_get_io_ptr (png_ptr);
    static_cast<std::ostream*> (io_ptr)->write ((char*)data, length);
}

void
flush_stream (png_structp png_ptr)
{
    void* io_ptr = png_get_io_ptr (png_ptr);
    static_cast<std::ostream*> (io_ptr)->flush();
}


void on_error (png_structp png_ptr, const char* msg)
{
    throw std::runtime_error (msg);
}

bool
has_transparency (const uint8_t* pixel_data, size_t width, size_t height)
{
    for (auto data_end = pixel_data + width*height*4; pixel_data != data_end; pixel_data += 4)
        if (0xff != pixel_data[3])
            return true;
    return false;
}

error
encode_rgb (std::ostream& out, write_struct& png, const uint8_t* const pixel_data,
            unsigned width, unsigned height, format frm, int off_x, int off_y)
{
    // ---------------------------------------------------------------------------
    // no local objects should be declared below this point
    //
    if (setjmp (png_jmpbuf (png.png_ptr)))
        return error::failure;

    // size of the IDAT chunks
    png_set_compression_buffer_size (png.png_ptr, 256*1024);

    int color_type = PNG_COLOR_TYPE_RGB;
    if (format::bgra32 == frm && has_transparency (pixel_data, width, height))
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;

    png_set_write_fn (png.png_ptr, &out, write_stream, flush_stream);
    png_set_IHDR (png.png_ptr, png.info_ptr, width, height, 8, color_type,
                  PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    if (off_x || off_y)
        png_set_oFFs (png.png_ptr, png.info_ptr, off_x, off_y, PNG_OFFSET_PIXEL);
    png_write_info (png.png_ptr, png.info_ptr);

    png_set_bgr (png.png_ptr);
    if (format::bgr24 != frm && !(PNG_COLOR_MASK_ALPHA & color_type))
        png_set_filler (png.png_ptr, 0, PNG_FILLER_AFTER);

    size_t stride = format::bgr24 == frm ? 3*width : 4*width;
    const uint8_t* image_ptr = pixel_data;
    for (size_t i = 0; i < height; ++i)
    {
        png_write_row (png.png_ptr, const_cast<uint8_t*> (image_ptr));
        image_ptr += stride;
    }
    png_write_end (png.png_ptr, NULL);

    return error::none;
}

error
encode_rgb (const std::string& filename, const uint8_t* const pixel_data,
            unsigned width, unsigned height, format frm, int off_x, int off_y)
{
    if (!width || !height)
        return error::params;

    write_struct png;
    if (!png.create())
        return error::init;

    std::ofstream out (filename, std::ios::out|std::ios::binary|std::ios::trunc);
    if (!out)
        return error::io;

    return encode_rgb (out, png, pixel_data, width, height, frm, off_x, off_y);
}

error
encode_rgb (const std::wstring& filename, const uint8_t* const pixel_data,
            unsigned width, unsigned height, format frm, int off_x, int off_y)
{
    if (!width || !height)
        return error::params;

    write_struct png;
    if (!png.create())
        return error::init;

    std::ofstream out (filename, std::ios::out|std::ios::binary|std::ios::trunc);
    if (!out)
        return error::io;

    return encode_rgb (out, png, pixel_data, width, height, frm, off_x, off_y);
}

error
decode (std::istream& in, std::vector<uint8_t>& bgr_data,
        unsigned* const width, unsigned* const height, int* const off_x, int* const off_y)
{
    if (!width || !height)
        return error::params;

    char header[8];
    if (!in.read (header, 8) || in.gcount() != 8
        || 0 != png_sig_cmp ((png_bytep)header, 0, 8))
        return error::format;

    read_struct read;
    if (!read.create())
        return error::init;

    std::vector<png_bytep>  row_pointers;

    // ---------------------------------------------------------------------------
    // no local objects should be declared below this point
    //
    if (setjmp (png_jmpbuf (read.png())))
        return error::failure;

    png_set_read_fn (read.png(), &in, read_stream);
    png_set_sig_bytes (read.png(), 8);

    png_read_info (read.png(), read.info());

    *width  = png_get_image_width (read.png(), read.info());
    *height = png_get_image_height (read.png(), read.info());

    if (!*width || !*height)
        return error::format;

    int color_type = png_get_color_type (read.png(), read.info());
    int bit_depth = png_get_bit_depth (read.png(), read.info());

    if (PNG_COLOR_TYPE_PALETTE == color_type)
        png_set_palette_to_rgb (read.png());
    else if (PNG_COLOR_TYPE_GRAY == color_type || PNG_COLOR_TYPE_GRAY_ALPHA == color_type)
        png_set_gray_to_rgb (read.png());

    if (png_get_valid (read.png(), read.info(), PNG_INFO_tRNS))
        png_set_tRNS_to_alpha (read.png());
    else if (!(PNG_COLOR_MASK_ALPHA & color_type))
        png_set_filler (read.png(), 0xff, PNG_FILLER_AFTER);

    if (16 == bit_depth)
        png_set_strip_16 (read.png());

    png_set_bgr (read.png());

    png_read_update_info (read.png(), read.info());

    if (off_x) *off_x = png_get_x_offset_pixels (read.png(), read.info());
    if (off_y) *off_y = png_get_y_offset_pixels (read.png(), read.info());

    const size_t row_size = *width * 4;
    bgr_data.resize (row_size * *height);

    row_pointers.reserve (*height);
    png_bytep row_ptr = bgr_data.data();
    for (size_t row = *height; row > 0; --row)
    {
        row_pointers.push_back (row_ptr);
        row_ptr += row_size;
    }
    png_read_image (read.png(), row_pointers.data());
    png_read_end (read.png(), 0);

    return error::none;
}

error decode (const std::string& from_file, std::vector<uint8_t>& bgr_data,
              unsigned* const width, unsigned* const height, int* const off_x, int* const off_y)
{
    std::ifstream in (from_file, std::ios::in|std::ios::binary);
    if (!in)
        return error::io;
    return decode (in, bgr_data, width, height, off_x, off_y);
}

error decode (const std::wstring& from_file, std::vector<uint8_t>& bgr_data,
              unsigned* const width, unsigned* const height, int* const off_x, int* const off_y)
{
    std::ifstream in (from_file, std::ios::in|std::ios::binary);
    if (!in)
        return error::io;
    return decode (in, bgr_data, width, height, off_x, off_y);
}

error
decode_grayscaled (std::istream& in, std::vector<uint8_t>& gray_data,
                   unsigned* const width, unsigned* const height, int* const off_x, int* const off_y)
{
    if (!width || !height)
        return error::params;

    char header[8];
    if (!in.read (header, 8) || in.gcount() != 8
        || 0 != png_sig_cmp ((png_bytep)header, 0, 8))
        return error::format;

    read_struct read;
    if (!read.create())
        return error::init;

    png_set_error_fn (read.png(), nullptr, on_error, nullptr);

    // ---------------------------------------------------------------------------
    // no local objects should be declared below this point
    //
    if (setjmp (png_jmpbuf (read.png())))
        return error::failure;

    png_set_read_fn (read.png(), &in, read_stream);
    png_set_sig_bytes (read.png(), 8);

    png_read_info (read.png(), read.info());

    *width  = png_get_image_width (read.png(), read.info());
    *height = png_get_image_height (read.png(), read.info());

    if (!*width || !*height)
        return error::format;

    if (PNG_INTERLACE_NONE != png_get_interlace_type (read.png(), read.info()))
        return error::interlace;

    int color_type = png_get_color_type (read.png(), read.info());
    int bit_depth = png_get_bit_depth (read.png(), read.info());

    if (PNG_COLOR_TYPE_PALETTE == color_type)
    {
        png_set_palette_to_rgb (read.png());
        png_set_rgb_to_gray_fixed (read.png(), 2, -1, -1);
        if (bit_depth < 8)
            png_set_expand_gray_1_2_4_to_8 (read.png());
    }
    else if (PNG_COLOR_TYPE_RGB == color_type || PNG_COLOR_TYPE_RGB_ALPHA == color_type)
        png_set_rgb_to_gray_fixed (read.png(), 2, -1, -1);
    else if (PNG_COLOR_TYPE_GRAY == color_type && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8 (read.png());
    if (16 == bit_depth)
        png_set_strip_16 (read.png());
    if (color_type & PNG_COLOR_MASK_ALPHA)
        png_set_strip_alpha (read.png());

    png_read_update_info (read.png(), read.info());

    if (off_x) *off_x = png_get_x_offset_pixels (read.png(), read.info());
    if (off_y) *off_y = png_get_y_offset_pixels (read.png(), read.info());

    const size_t row_size = *width;
    gray_data.resize (row_size * *height);
    uint8_t* row_ptr = gray_data.data();
    for (size_t row = *height; row > 0; --row)
    {
         png_read_row (read.png(), row_ptr, NULL);
         row_ptr += row_size;
    }
    png_read_end (read.png(), 0);

    return error::none;
}

error
decode_grayscaled (const std::string& from_file, std::vector<uint8_t>& gray_data,
                   unsigned* const width, unsigned* const height, int* const off_x, int* const off_y)
{
    std::ifstream in (from_file, std::ios::in|std::ios::binary);
    if (!in)
        return error::io;
    return decode_grayscaled (in, gray_data, width, height, off_x, off_y);
}

error
decode_grayscaled (const std::wstring& filename, std::vector<uint8_t>& gray_data,
                   unsigned* const width, unsigned* const height, int* const off_x, int* const off_y)
{
    std::ifstream in (filename, std::ios::in|std::ios::binary);
    if (!in)
        return error::io;
    return decode_grayscaled (in, gray_data, width, height, off_x, off_y);
}

const char*
get_error_text (error num)
{
    switch (num)
    {
    case error::none:       return "no error";
    case error::io:         return "i/o error";
    case error::init:       return "initialization error";
    case error::failure:    return "unknown error";
    case error::format:     return "invalid PNG format";
    case error::params:     return "unexpected parameters";
    case error::interlace:  return "interlaced images not supported";
    }
    return "PNG library error";
}

} // namespace png
