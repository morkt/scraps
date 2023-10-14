//! \file       deism.cs
//! \date       2023 Sep 02
//! \brief      decrypt strings in ISM scripts
//
// print decrypted strings to stdout
//

#include <cstdio>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <unordered_map>
#include "sysmemmap.h"

class bytecode_error : public std::runtime_error
{
    size_t  pos;

public:
    bytecode_error (size_t pos, const char* message) : std::runtime_error (message)
    {
    }

    size_t get_pos () const { return pos; }
};

class bytecode_reader
{
    sys::mapping::const_view<uint8_t> view;
    const uint8_t* const pBytecodeStart;
    const uint8_t*      pBytecode;
    std::vector<char>   buffer;
    std::unordered_map<const uint8_t*, std::string> cache;

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
        if (view.size() < 0x14 || 0 != std::memcmp (view.data(), "ISM SCRIPT", 11))
            throw std::runtime_error ("invalid input");
        size_t bytecode_start = get<uint32_t> (&view[0x10]);
        if (bytecode_start < 0x14 || bytecode_start >= view.size())
            throw std::runtime_error ("no bytecode");
        return &view[bytecode_start];
    }

    void decrypt_string (const uint8_t* str, size_t len, uint8_t key);
    void print_string (const uint8_t* str, size_t len);

    template <typename T>
    T get (const uint8_t* ptr)
    {
        if (ptr + sizeof(T) > view.end())
            throw bytecode_error (pBytecode - view.data(), "Failed attempt to access data out of script bounds");
        return *reinterpret_cast<const T*> (ptr);
    }

    std::ostream& put_offset (const uint8_t* ptr, int width = 8)
    {
        return std::cout << std::hex << std::setw(width) << (ptr - view.data());
    }
};

bool bytecode_reader::run ()
{
    std::cout << std::setfill('0') << std::uppercase;
    pBytecode = pBytecodeStart;
    while (pBytecode < view.end())
    {
        switch (*pBytecode)
        {
        case 0xFF:
            put_offset (pBytecode) << " -> END_SCRIPT\n";
            ++pBytecode;
            return true;
        case 0x0F: // switch jump?
            pBytecode += 9;
            break;
        case 0x20: // relative jump
        case 0x24: // call func by address
            {
//                size_t pos = get<uint32_t> (pBytecode+1);
//                put_offset (pBytecode) << ": JUMP ";
//                put_offset (pBytecode + pos, 4).put ('\n');
                // pBytecode += get<uint32_t> (pBytecode+1);
                pBytecode += 5;
                break;
            }
        case 0x21: // conditional jump [JMPZ]
        case 0x23: // call func by name
        case 0x30: // push dword?
        case 0x31:
        case 0x38: // get var?
        case 0x3A:
            pBytecode += 5;
            break;
        case 0x39:
        case 0x3B:
            pBytecode += 6;
            break;
        case 0x00:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E: // XOR?
        case 0x10:
        case 0x11: // ADD
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16: // EQ
        case 0x17:
        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
        case 0x25: // move control flow to other script?
        case 0x28: // CREATE_THREAD
        case 0x29: // KILL_THREAD
        case 0x2C: // SET_VARIABLE
        case 0x2E: // call script?
        case 0x2D: // jump to address in stack?
        case 0x2F: // RET?
        case 0x35: // NOP
        case 0x36: // NOP
        case 0x40:
        case 0x41:
        case 0x43:
        case 0x44: // SUBSTRING
        case 0x46: // SPRINTF
        case 0x47:
        case 0x50:
        case 0x52:
        case 0x83:
        case 0x86:
        case 0x88: // START_EFFECT
        case 0x89:
        case 0x8B:
        case 0x8C:
        case 0x8D:
        case 0x8E:
        case 0x91:
        case 0x92:
        case 0xA0:
        case 0xB0:
        case 0xB1:
        case 0xB2:
        case 0xB3:
        case 0xC0:
        case 0xC1: // GET_CURSOR_POS
        case 0xD0: // LOAD_IMAGE
        case 0xD2:
        case 0xE0:
        case 0xE1:
        case 0xE2:
        case 0xE3:
        case 0xE5:
        case 0xF0:
        case 0xF1:
        case 0xF2:
        case 0xF4:
        case 0xF5:
        case 0xF7:
        case 0xFB: // NOP
        case 0xFC: // open file?
        case 0xFD:
        case 0xFE:
            ++pBytecode;
            break;
        case 0xF3: // call function?
            pBytecode += 2;
            break;
        case 0x45: // push string by ref
            {
                size_t pos = get<uint32_t> (pBytecode+1);
                uint8_t key = pos;
                auto strPtr = pBytecodeStart + pos;
                size_t len = strPtr[1];
                strPtr += 2;
                if (0xFF == len)
                {
                    len = get<uint32_t> (strPtr);
                    strPtr += 4;
                }
                decrypt_string (strPtr, len, key);
                pBytecode += 5;
                break;
            }
        case 0x33: // push string
            {
                uint8_t key = pBytecode - pBytecodeStart;
                size_t len = pBytecode[1];
                pBytecode += 2;
                if (0xFF == len)
                {
                    len = get<uint32_t> (pBytecode);
                    pBytecode += 4;
                }
                decrypt_string (pBytecode, len, key);
                pBytecode += len;
                break;
            }
        default:
            std::fprintf (stderr, "%08X: unknown bytecode %02X\n", pBytecode - view.begin(), *pBytecode);
            return false;
        }
        if (*pBytecode == 5)
            ++pBytecode;
    }
    return true;
}

void bytecode_reader::
decrypt_string (const uint8_t* str, size_t len, uint8_t key)
{
    if (0xFF == key)
        key = 0;
    if (len > view.size() || str + len > view.end())
        throw bytecode_error (str - view.data(), "invalid string");
    if (len > buffer.size())
        buffer.resize (len);
    for (size_t i = 0; i < len; ++i)
        buffer[i] = ~str[i] ^ key;
    print_string (str, len);
}

void bytecode_reader::
print_string (const uint8_t* pos, size_t len)
{
    /*
    if (cache.find (pos) != cache.end())
        return;
    std::string str (buffer.data(), len);
    put_offset (pos) << ": " << str << '\n';
    cache[pos] = std::move (str);
    */
    std::cout.write (buffer.data(), len);
    std::cout.put ('\n');
}

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 2)
    {
        std::puts ("usage: deism INPUT");
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    bytecode_reader reader (in);
    return reader.run() ? 0 : 1;
}
catch (bytecode_error& X)
{
    std::fprintf (stderr, "%08X: %s\n", X.get_pos(), X.what());
    return 1;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}

#if 0 // bytecode map
      0 -> sub_10003D40
      5 -> sub_10003CF0
      6 -> sub_10003CF0
      9 -> sub_10003810
    0Bh -> sub_10005D70
    0Ch -> sub_10006770
    0Dh -> sub_10006830
    0Eh -> sub_10005FA0
    0Fh -> sub_10005380
    10h -> sub_10006BB0
    11h -> sub_10005A30
    12h -> sub_10005BD0
    13h -> sub_10005CB0
    14h -> sub_10006050
    15h -> sub_10006140
    16h -> sub_10006230
    17h -> sub_10006470
    18h -> sub_10006530
    19h -> sub_10006350
    1Ah -> sub_100065F0
    1Bh -> sub_100066B0
    1Ch -> sub_10005E40
    1Dh -> sub_10005EF0
    1Eh -> sub_100068F0
    1Fh -> sub_10006970
    20h -> sub_10005310
    21h -> sub_10005320
    22h -> sub_10003D40
    23h -> sub_10003FF0
    24h -> sub_10005310
    25h -> sub_10004070
    28h -> sub_10004930
    29h -> sub_10004B60
    2Ah -> sub_10004C70
    2Bh -> sub_10004E50
    2Ch -> sub_10005470
    2Dh -> sub_10005560
    2Eh -> sub_100051C0
    2Fh -> sub_10005150
    30h -> sub_10003DA0
    31h -> sub_10003DD0
    32h -> sub_10003F90
    33h -> ism_string
    34h -> sub_10003810
    35h -> ___inc_tmpoff
    36h -> ___inc_tmpoff
    37h -> sub_10006B30
    38h -> sub_10004E80
    39h -> sub_10004EC0
    3Ah -> sub_10006B00
    3Bh -> sub_100069F0
    3Ch -> sub_10004FD0
    3Dh -> sub_10005030
    3Eh -> sub_10006B00
    3Fh -> sub_100069F0
    40h -> sub_10004210
    41h -> sub_10004280
    42h -> sub_100042F0
    43h -> sub_10004440
    44h -> sub_10004570
    45h -> sub_10003EE0
    46h -> sub_10004830
    47h -> sub_10004360
    50h -> sub_10006C70
    51h -> sub_10006D90
    52h -> sub_10006CE0
    60h -> sub_10005610
    61h -> sub_10005880
    80h -> sub_10007240
    81h -> sub_10007F60
    82h -> sub_10006E00
    83h -> sub_10007040
    84h -> sub_10008770
    85h -> sub_10006F40
    86h -> sub_10007120
    87h -> sub_10007120
    88h -> sub_10008050
    89h -> sub_100082D0
    8Ah -> sub_10008340
    8Bh -> sub_10008500
    8Ch -> sub_10008EB0
    8Dh -> sub_10008FF0
    8Eh -> sub_100094E0
    8Fh -> sub_10007430
    90h -> sub_10007700
    91h -> sub_10007980
    92h -> sub_10009160
    93h -> sub_10008540
    94h -> sub_10007C90
    95h -> sub_10009350
    A0h -> sub_100096B0
    A1h -> sub_10009760
    B0h -> sub_10009A10
    B1h -> sub_10009D10
    B2h -> sub_10009DD0
    B3h -> sub_10009ED0
    C0h -> sub_10009800
    C1h -> sub_100098E0
    D0h -> sub_100088C0
    D2h -> sub_10008AB0
    D3h -> sub_10008CA0
    E0h -> sub_10009F90
    E1h -> sub_1000A150
    E2h -> sub_1000A250
    E3h -> sub_1000A330
    E4h -> sub_1000A400
    E5h -> sub_10009F90
    E6h -> sub_1000A1D0
    F0h -> sub_1000A770
    F1h -> sub_1000A840
    F2h -> sub_1000AB20
    F3h -> sub_1000AC10
    F4h -> sub_1000A630
    F5h -> sub_1000A6F0
    F6h -> sub_1000A540
    F7h -> sub_1000A600
    F8h -> sub_1000AAF0
    FAh -> sub_1000A4D0
    FBh -> ___inc_tmpoff
    FCh -> sub_1000A8C0
    FDh -> sub_1000AA10
    FEh -> sub_1000AC80
    FFh -> sub_1000ACA0
#endif
