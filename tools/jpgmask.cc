// -*- C++ -*-
//! \file       jpgmask.cc
//! \date       Fri Dec 04 14:33:30 2015
//! \brief      apply mask to NScripter character images.
//

#include <cstdio>
extern "C" {
#include <jpeglib.h>
}
#include "png-convert.hpp"
#include <string>
#include <memory>

typedef std::wstring tstring;

void convert_masked_image (unsigned width, unsigned height, const uint8_t* input, uint8_t* output)
{
    size_t input_stride  = width * 6;
    auto mask = input_stride / 2;
    for (auto y = 0; y < height; ++y)
    {
        auto src = input;
        for (auto x = 0; x < width; ++x)
        {
            *output++ = src[2];
            *output++ = src[1];
            *output++ = src[0];
            unsigned alpha = (src[mask] + src[mask+1] + src[mask+2]) / 3;
            *output++ = ~alpha;
            src += 3;
        }
        input += input_stride;
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
try
{
    if (argc < 2)
    {
        std::puts ("usage: jpgmask FILENAME\n");
        return 0;
    }

    static_assert (sizeof(JSAMPLE) == 1, "only 8-bit JSAMPLE supported");

    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error (&jerr);
    jpeg_create_decompress (&cinfo);

    FILE* in = _wfopen (argv[1], L"rb");
    if (!in)
    {
        std::fprintf (stderr, "%S: %s\n", argv[1], std::strerror (errno));
        std::fclose (in);
        return 1;
    }
    jpeg_stdio_src (&cinfo, in);

    jpeg_read_header (&cinfo, TRUE);
    jpeg_start_decompress (&cinfo);
    unsigned width  = cinfo.output_width;
    if (width & 1)
    {
        std::fprintf (stderr, "%S: image width should be even [%d]\n", argv[1], width);
        std::fclose (in);
        return 2;
    }
    if (cinfo.output_components != 3)
    {
        std::fprintf (stderr, "%S: RGB image required\n", argv[1]);
        std::fclose (in);
        return 2;
    }
    unsigned height = cinfo.output_height;
    const size_t stride = width * cinfo.output_components;
    const size_t total_size = stride * height;

    std::unique_ptr<JSAMPLE[]> jpeg_output (new JSAMPLE[total_size]);
    std::unique_ptr<JSAMPROW[]> scanlines (new JSAMPROW[height]);

    auto line = jpeg_output.get();
    for (unsigned i = 0; i < height; ++i)
    {
        scanlines[i] = line;
        line += stride;
    }
    while (cinfo.output_scanline < height)
    {
        jpeg_read_scanlines (&cinfo, scanlines.get()+cinfo.output_scanline, height - cinfo.output_scanline);
    }
    jpeg_finish_decompress (&cinfo);
    jpeg_destroy_decompress (&cinfo);
    std::fclose (in);

    width /= 2;
    std::vector<uint8_t> image (width*height*4);
    convert_masked_image (width, height, jpeg_output.get(), image.data());

    tstring out_name (convert_filename (argv[1], L".png"));
    std::printf ("%S -> %S\n", argv[1], out_name.c_str());
    png::error rc = png::encode (out_name, image.data(), width, height, png::format::bgra32);
    if (png::error::none != rc)
        std::fprintf (stderr, "%S: %s\n", out_name.c_str(), png::get_error_text (rc));

    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "jpgmask: %s\n", X.what());
    return 1;
}
