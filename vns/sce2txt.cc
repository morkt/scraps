// -*- C++ -*-
//! \file       sce2txt.cc
//! \date       Thu Jun 11 17:58:29 2015
//! \brief      convert SCE script to TXT
//

#include <cstring>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <iostream>
#include "sysmemmap.h"

const uint8_t key_string1[] = "crowd script yeah !";
const uint8_t key_string2[] = "crowd scenario script";
const char    key_string3[] = "CROWD MissYou Scenario Data"; // Miss You
const char    key_string4[] = "CROWD \x8F\x49\x82\xED\x82\xE8\x82\xC8\x82\xAB\x83\x81\x83\x43\x83\x68\x92\x42\x82\xCC\x96\xE9 Scenario Data"; // Owari naki Maid-tachi no Yoru

class missyou_sce
{
    size_t          pos;
    const size_t    length;
    uint8_t         data[30];
public:
    missyou_sce () : pos (0), length (30)
    {
        for (size_t i = 0; i < length; ++i)
            data[i] = i;
    }

    void set_password (const char* text)
    {
        size_t len = std::strlen (text);
        for (size_t i = 0; i < len; ++i)
            compute (text[i]);
    }

    const uint8_t* get_key () const { return data; }
    const size_t get_length () const { return length; }

private:
    void compute (uint8_t x)
    {
        pos = (pos + 1) % length;
        uint8_t a = ~*data;
        uint8_t c = data[(pos ^ a) % length];
        uint8_t b = x;
        data[pos] = x | a & c;
        if (x == a)
        {
            b = ~x;
            c = ~c;
            a = ~a;
        }
        if (b <= a)
            c = ~c;
        if (a <= c)
            b += c;
        uint8_t* a2a;
        if (b < 0x20u)
            data[pos] ^= x;
        if (b > 0x60u)
            data[(pos + 1) % length]++;
        if (a < 0x20u)
            data[(pos + 2) % length] = b + a + c;
        if (a > 0x80u)
            data[pos] = b + a - c;
        if (a > 0xC0u)
            data[(pos + 1) % length] = a ^ c;
        if ( c < 0x32u )
            data[(pos + 3) % length] = b ^ c;
        if ( c > 0xE0u )
            data[pos] = a ^ data[(pos + 1) % length];
    }
};

void decrypt (uint8_t *enc_buf, size_t enc_buf_len, const uint8_t* key_string, size_t key_length)
{
    uint32_t key = 0, shift = 0;
    for (size_t i = 0; i < enc_buf_len; i++)
    {
        uint32_t idx = (key + i) % key_length;	
        enc_buf[i] ^= (uint8_t)(key_string[idx] | (key & shift));
        if (!idx)
            key = key_string[(key + shift++) % key_length];
    }
}

void sub_449540 (uint8_t* buf, uint8_t k)
{
    uint8_t v3, v4;
    uint8_t v5, v6, v7;
    uint8_t v8, v9, v10, v11, v12, v13;
    uint8_t v14;
    uint8_t v15;
    uint8_t v16;
    uint8_t v17;
    uint8_t v18;
    uint8_t v19;
    uint8_t v20;
    uint8_t v21;
    uint8_t v22;
    uint8_t v23;
    uint8_t v24, v25, v26;
    uint8_t v27, v28, v29;

    switch (k & 7)
    {
    case 0:
        *buf += k;
        v3 = k + buf[2];
        v4 = buf[6];
        buf[3] += k + 2;
        buf[4] = v3 + 11;
        buf[8] = v4 + 7;
        break;
    case 1:
        v5 = buf[9] + buf[10];
        buf[6] = buf[15] + buf[7];
        v6 = buf[8];
        buf[2] = v5;
        v7 = buf[3];
        buf[8] = buf[1] + v6;
        buf[15] = v7 + buf[5];
        break;
    case 2:
        v8 = buf[5];
        v9 = buf[8];
        buf[1] += buf[2];
        v10 = buf[6] + v8;
        v11 = v9 + buf[7];
        v12 = buf[10];
        buf[7] = v11;
        v13 = buf[11];
        buf[5] = v10;
        buf[10] = v13 + v12;
        break;
    case 3:
        v14 = buf[5];
        v15 = buf[7];
        buf[9] = buf[1] + buf[2];
        v16 = v14 + buf[6];
        buf[12] = v15 + buf[8];
        v17 = buf[10];
        buf[11] = v16;
        buf[13] = v16 + v17;
        break;
    case 4:
        v18 = buf[4] + 71;
        *buf = buf[1] + 111;
        v19 = buf[5];
        buf[3] = v18;
        v20 = buf[15];
        buf[4] = v19 + 17;
        buf[14] = v20 + 64;
        break;
    case 5:
        v21 = buf[5];
        v22 = buf[14];
        buf[2] += buf[10];
        buf[4] = v21 + buf[12];
        v23 = buf[11];
        buf[6] = buf[8] + v22;
        buf[8] = *buf + v23;
        break;
    case 6:
        v24 = buf[13] + buf[3];
        buf[9] = buf[11] + buf[1];
        v25 = buf[5];
        buf[11] = v24;
        v26 = buf[7];
        buf[13] = buf[15] + v25;
        buf[15] = buf[9] + v26;
        /* FALL THROUGH */
    default:
        v27 = buf[6] + buf[10];
        buf[1] = buf[5] + buf[9];
        v28 = buf[11];
        buf[2] = v27;
        v29 = buf[12];
        buf[3] = buf[7] + v28;
        buf[4] = buf[8] + v29;
        break;
    }
}

void decrypt_anim (uint8_t* enc_buf, size_t enc_buf_len, std::vector<uint8_t>& dec)
{
    uint8_t buf[16];
    std::memcpy (buf, enc_buf+4, sizeof(buf));

    size_t dec_buf_len = enc_buf_len - 0x14;
    if (!dec_buf_len)
        return;

    dec.resize (dec_buf_len);
    uint8_t* src = enc_buf + 0x14;
    size_t v12 = 0;
    uint8_t* dst = dec.data();
    for (size_t v13 = 0; v13 < dec_buf_len; ++v13)
    {
        dst[v13] = (buf[v12] | *src) & ~(buf[v12] & *src);
        ++v12;
        if (16 == v12)
        {
            v12 = 0;
            sub_449540 (buf, dst[v13 - 1]);
        }
        ++src;
    }
}

inline void usage ()
{
    std::cout << "usage: sce2txt [-x METHOD] INPUT OUTPUT\n";
}

int main (int argc, char* argv[])
try
{
    if (argc < 3)
    {
        usage();
        return 0;
    }
    std::string method = "1";
    int argN = 1;
    if (argc > 3)
    {
        if (0 != std::strcmp (argv[1], "-x") || argc < 5)
        {
            usage();
            return 0;
        }
        method = argv[2];
        if (method != "1" && method != "2" && method != "missyou" && method != "maid")
        {
            std::cerr << "method should be 1/2/missyou/maid\n";
            return 1;
        }
        argN = 3;
    }
    sys::mapping::readwrite in (argv[argN], sys::mapping::writecopy);
    sys::mapping::view<uint8_t> view (in);
    uint8_t* data = view.data();
    size_t   size = view.size();
    if (size > 24 && 0x01000000 == *(uint32_t*)data)
    {
        std::vector<uint8_t> dec_buf;
        decrypt_anim (data, size, dec_buf);
        std::ofstream out (argv[argN+1], std::ios::out|std::ios::binary|std::ios::trunc);
        out.write ((const char*)dec_buf.data(), dec_buf.size());
        return 0;
    }
    else if ("1" == method)
    {
        decrypt (data, size, key_string1, 0x12);
    }
    else if ("2" == method && size > 0xc0)
    {
        size = std::min (*(uint32_t*)&view[0xa0], size-0xc0);
        data += 0xc0;
        decrypt (data, size, key_string2, 0x15);
    }
    else if ("missyou" == method)
    {
        missyou_sce key;
        key.set_password (key_string3);
        decrypt (data, size, key.get_key(), key.get_length());
    }
    else if ("maid" == method)
    {
        missyou_sce key;
        key.set_password (key_string4);
        decrypt (data, size, key.get_key(), key.get_length());
    }
    std::ofstream out (argv[argN+1], std::ios::out|std::ios::binary|std::ios::trunc);
    out.write ((const char*)data, size);
    return 0;
}
catch (std::exception& X)
{
    std::cerr << X.what() << '\n';
    return 1;
}
