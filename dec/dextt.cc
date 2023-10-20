// -*- C++ -*-
//! \file       dextt.cc
//! \date       2023 Oct 17
//! \brief      decrypt Xuse XTT file.
//

#include "bytecode.h"

class xtt_reader : public bytecode_reader
{
public:
    xtt_reader (sys::mapping::map_base& in)
        : bytecode_reader (in) { }

    const uint8_t default_key = 0x47;

protected:
    bool do_run () override;

    void parse_text (size_t length);
    void decrypt_text (std::wstring& text) const;

    std::wstring buffer;
    std::string utf8_buffer;
};

bool xtt_reader::
do_run ()
{
    pBytecode = pBytecodeStart;
    while (pBytecode + 1 < view.end())
    {
        int count = get_byte();
        size_t length = get_byte();
        if (length != 0)
        {
            parse_text (length+1);
            continue;
        }
        if (!count)
            break;
        while (count --> 0)
        {
            ++pBytecode;
            length = get_byte() + 1;
            parse_text (length);
        }
    }
    return true;
}

void xtt_reader::
parse_text (size_t length)
{
    if (pBytecode + length > view.end())
        throw_error (pBytecode-1, "invalid length");
    buffer.assign (reinterpret_cast<const wchar_t*> (pBytecode), length/2);
    decrypt_text (buffer);
    sys::u16tou8 (buffer, utf8_buffer);
    std::cout << utf8_buffer << '\n';
    pBytecode += length;
}

void xtt_reader::
decrypt_text (std::wstring& text) const
{
    const uint16_t key = default_key << 8 | default_key;
    for (auto& c : text)
        c ^= key;
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: dextt INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    xtt_reader reader (in);
    if (!reader.is_valid())
        throw std::runtime_error ("invalid XTT file");
    return reader.run() ? 0 : 1;
}
catch (bytecode_error& X)
{
    std::fprintf (stderr, "%S:%08X: %s\n", argv[1], X.get_pos(), X.what());
    return 1;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}
