// -*- C++ -*-
//! \file       desda.cc
//! \date       2023 Sep 26
//! \brief      extract text from Squadra D bytecode
//

#include <cstdio>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include "sysmemmap.h"

class bytecode_error : public std::runtime_error
{
    size_t  pos;

public:
    bytecode_error (size_t pos, const char* message) : std::runtime_error (message), pos (pos)
    {
    }

    bytecode_error (size_t pos, uint8_t bytecode) : std::runtime_error (format_message (bytecode)), pos (pos)
    {
    }

    size_t get_pos () const { return pos; }

private:
    std::string format_message (uint8_t bytecode)
    {
        std::ostringstream err;
        err << "unknown bytecode " << std::setfill('0') << std::uppercase << std::hex << std::setw(2)
            << static_cast<int> (bytecode);
        return err.str();
    }
};

class bytecode_reader
{
    sys::mapping::const_view<uint8_t> view;
    const uint8_t* const    pBytecodeStart;
    const uint8_t*          pBytecode;
    std::vector<char>       buffer;

public:
    bytecode_reader (sys::mapping::map_base& in)
        : view (in)
        , pBytecodeStart (init())
        , buffer (1024)
    {
    }

    bool run ();

private:
    const uint8_t* init ()
    {
        if (!view.size())
            throw std::runtime_error ("no bytecode");
        size_t bytecode_start = 0;
        return view.data();
    }

    void decrypt_string (const uint8_t* str, size_t len);
    void print_string (size_t len);

    template <typename T>
    T get (const uint8_t* ptr)
    {
        if (ptr + sizeof(T) > view.end())
            throw bytecode_error (pBytecode - view.data(), "Failed attempt to access data out of script bounds");
        return *reinterpret_cast<const T*> (ptr);
    }

    std::ostream& put_offset (const uint8_t* ptr, int width = 8)
    {
        return std::cout << std::hex << std::setw(width) << (ptr - view.data()) << ':';
    }

    uint16_t get_byte ()
    {
        return get<uint8_t> (pBytecode++);
    }

    uint16_t get_word ()
    {
        auto word = get<uint16_t> (pBytecode);
        pBytecode += 2;
        return word;
    }

    uint32_t get_dword ()
    {
        auto dword = get<uint32_t> (pBytecode);
        pBytecode += 4;
        return dword;
    }
};

bool bytecode_reader::run ()
{
    std::cout << std::setfill('0') << std::uppercase;
    pBytecode = pBytecodeStart;
    while (pBytecode < view.end())
    {
        const auto currentPos = pBytecode;
        auto bytecode = get_byte();
        switch (bytecode)
        {
        case 0xFF:
            put_offset (currentPos) << "__END__\n";
            return true;
        case 0x01:
            put_offset (currentPos) << "SDA_OPEN\n";
            break;
        case 0x00:
        case 0x03:
        case 0x05:
        case 0x0A:
        case 0x0B:
        case 0x0D:
        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x19:
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x28:
        case 0x29:
        case 0x2D:
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x36:
        case 0x38:
        case 0x39:
        case 0x3C:
        case 0x3D:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4A:
        case 0x55:
        case 0x56:
        case 0x5E:
        case 0x5F:
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x64:
        case 0x65:
        case 0x69:
        case 0x6C:
        case 0x6E:
        case 0x6F:
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0xFE:
            break;
        case 0x35:
        case 0x37:
        case 0x3A:
        case 0x3B:
        case 0x4F:
            get_word();
            break;
        case 0x54:
            get_dword();
            break;
        case 0x2B: 
        case 0x2E:
//            put_offset (currentPos) << "PLA_PLAY_START\n";
            break;
        case 0x2C:
            put_offset (currentPos) << "PLA_PLAY_OPEN\n";
            break;
        case 0x5B:
            put_offset (currentPos) << "EXEC_SCRIPT\n";
            break;
        case 0x5C:
            put_offset (currentPos) << "CALL_SCRIPT\n";
            break;
        case 0x5D:
            put_offset (currentPos) << "EXEC_AT\n";
            break;
        case 0x4C:
            put_offset (currentPos) << "JUMP\n";
            break;
        case 0x4E:
            {
                size_t len = get_word();
                decrypt_string (pBytecode, len);
                pBytecode += len;
                break;
            }
        default:
            throw bytecode_error (currentPos - view.data(), bytecode);
        }
    }
    return true;
}

void bytecode_reader::
decrypt_string (const uint8_t* str, size_t len)
{
    uint8_t key = len * 7 + 85;
    if (len > view.size() || str + len > view.end())
        throw bytecode_error (str - view.data(), "invalid string");
    if (len > buffer.size())
        buffer.resize (len);
    for (size_t i = 0; i < len; ++i)
        buffer[i] = str[i] ^ key;
    if (len)
        print_string (len);
}

void bytecode_reader::
print_string (size_t len)
{
    std::cout.write (buffer.data(), len);
    std::cout.put ('\n');
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: desda INPUT");
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    bytecode_reader reader (in);
    return reader.run() ? 0 : 1;
}
catch (bytecode_error& X)
{
    std::fprintf (stderr, "%S:%04X: %s\n", argv[1], X.get_pos(), X.what());
    return 1;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}
