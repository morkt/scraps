// -*- C++ -*-
//! \file       lzss.h
//! \date       Tue Jul 07 09:01:39 2015
//! \brief      
//

#ifndef LZSS_H
#define LZSS_H

#include <iosfwd>
#include <cstdint>

size_t lzss_decompress (const uint8_t* packed, size_t packed_size, std::ostream& out);

#endif /* LZSS_H */
