// -*- C++ -*-
//! \file       png-convert.hpp
//! \date       Mon Feb 03 01:16:09 2014
//! \brief      convert PNG images to and from BGRA pixel data.
//

#ifndef PNG_CONVERT_HPP
#define PNG_CONVERT_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <png.h>

namespace png {

using std::uint8_t;

enum class error {
    none,
    io,
    init,
    failure,
    format,
    params,
    interlace,
};

enum class format {
    bgr24,
    bgr32,
    bgra32,
};

error encode_rgb (const std::string& filename, const uint8_t* const pixel_data,
                  unsigned width, unsigned height, format frm, int off_x = 0, int off_y = 0);

error encode_rgb (const std::wstring& filename, const uint8_t* const pixel_data,
                  unsigned width, unsigned height, format frm, int off_x = 0, int off_y = 0);

inline error encode (const std::string& to_file, const uint8_t* const bgr_data,
                     size_t width, size_t height, format data_format)
    { return encode_rgb (to_file, bgr_data, width, height, data_format, 0, 0); }

inline error encode (const std::wstring& to_file, const uint8_t* const bgr_data,
                     size_t width, size_t height, format data_format)
    { return encode_rgb (to_file, bgr_data, width, height, data_format, 0, 0); }

inline error encode (const std::string& to_file, const uint8_t* const bgr_data,
                     size_t width, size_t height, int off_x = 0, int off_y = 0)
    { return encode_rgb (to_file, bgr_data, width, height, format::bgra32, off_x, off_y); }

inline error encode (const std::wstring& to_file, const uint8_t* const bgr_data,
                     size_t width, size_t height, int off_x = 0, int off_y = 0)
    { return encode_rgb (to_file, bgr_data, width, height, format::bgra32, off_x, off_y); }

error decode (std::istream& in, std::vector<uint8_t>& bgr_data,
              unsigned* const width, unsigned* const height,
              int* const off_x = 0, int* const off_y = 0);

error decode (const std::string& from_file, std::vector<uint8_t>& bgr_data,
              unsigned* const width, unsigned* const height, int* const off_x = 0, int* const off_y = 0);

error decode (const std::wstring& from_file, std::vector<uint8_t>& bgr_data,
              unsigned* const width, unsigned* const height, int* const off_x = 0, int* const off_y = 0);

error decode_grayscaled (std::istream& in, std::vector<uint8_t>& gray_data,
                         unsigned* const width, unsigned* const height,
                         int* const off_x = 0, int* const off_y = 0);

error decode_grayscaled (const std::string& from_file, std::vector<uint8_t>& gray_data,
                         unsigned* const width, unsigned* const height, int* const off_x = 0, int* const off_y = 0);

error decode_grayscaled (const std::wstring& filename, std::vector<uint8_t>& gray_data,
                         unsigned* const width, unsigned* const height, int* const off_x = 0, int* const off_y = 0);

const char* get_error_text (error num);

void read_stream (png_structp png_ptr, png_bytep data, png_size_t length);

struct io_struct
{
    png_structp png_ptr;
    png_infop   info_ptr;

    io_struct () : png_ptr (0), info_ptr (0) { }

    png_structp png () const { return png_ptr; }
    png_infop info () const { return info_ptr; }
};

struct write_struct : io_struct
{
    write_struct () : io_struct() { }

    bool create ()
    {
        png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr)
            return false;

        info_ptr = png_create_info_struct (png_ptr);
        if (!info_ptr)
            return false;
        return true;
    }

    ~write_struct ()
    {
        if (info_ptr)
            png_destroy_write_struct (&png_ptr, &info_ptr);
        else if (png_ptr)
            png_destroy_write_struct (&png_ptr, 0);
    }

    using io_struct::png;
    using io_struct::info;
};

class read_struct : io_struct
{
    png_infop   end_info;

public:
    read_struct () : io_struct(), end_info (0) { }

    bool create ()
    {
        png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr)
            return false;

        info_ptr = png_create_info_struct (png_ptr);
        if (!info_ptr)
            return false;
        
        return true;
    }

    bool create_with_end ()
    {
        if (!create())
            return false;
        
        end_info = png_create_info_struct (png_ptr);
        if (!end_info)
            return false;

        return true;
    }

    ~read_struct ()
    {
        if (end_info)
            png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
        else if (info_ptr)
            png_destroy_read_struct (&png_ptr, &info_ptr, 0);
        else if (png_ptr)
            png_destroy_read_struct (&png_ptr, 0, 0);
    }

    using io_struct::png;
    using io_struct::info;
    png_infop end () const { return end_info; }
};

} // namespace png

#endif /* PNG_CONVERT_HPP */
