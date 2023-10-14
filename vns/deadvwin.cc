// -*- C++ -*-
//! \file       deadv.cc
//! \date       2023 Sep 19
//! \brief      extract text from ADVWIN scripts.
//
// reference source used [970124][Red Zone] Jinmon Yuugi
// works for PC-98 engine Adv98 as well

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include "sysmemmap.h"
#include "format.hpp"
#include "bytecode.h"

namespace adv {

namespace fs = std::filesystem;
using ext::format;

class invalid_instruction_error : bytecode_error
{
public:
    invalid_instruction_error (size_t pos, int inst) : bytecode_error (pos, format_message (inst))
    {
    }

private:
    static std::string format_message (int inst)
    {
        return ext::str (format ("invalid instruction %02X") % inst);
    }
};

class adv_bytecode_reader : public bytecode_reader
{
    typedef fs::path::string_type   path_string_type;
    typedef std::ctype<path_string_type::value_type> facet_type;

    const facet_type&           m_ctype;
    std::string                 m_buffer;
    std::vector<char>           m_text;
    std::map<int, size_t>       m_builtins;
    std::map<int, path_string_type> m_externals;
    std::map<int, const uint8_t*> subroutines;
    bool                        m_eof_opcode_reached;

    static const size_t default_string_buffer_size = 260;

public:
    adv_bytecode_reader ()
        : bytecode_reader ()
        , m_ctype (std::use_facet<facet_type> (std::locale()))
    { }

    // opcode D5 performs bytecode decryption in-place, hence readwrite requirement
    adv_bytecode_reader (sys::mapping::readwrite& in)
        : bytecode_reader (in)
        , m_ctype (std::use_facet<facet_type> (std::locale()))
    { }

    void init (sys::mapping::readwrite& in)
    {
        bytecode_reader::init (in);
    }

private:
    bool do_run () override;

    const uint8_t* init () override
    {
        m_eof_opcode_reached = false;
        return bytecode_reader::init();
    }

    int current_pos () const { return pBytecode - view.data(); }

    std::ostream& print_bytecode (const uint8_t* ptr, uint8_t bytecode) const
    {
        return log(ll_debug) << put_offset (ptr) << hex (bytecode) << '\n';
    }

    const path_string_type& toupper (path_string_type& str) const
    {
        auto begin = &str[0];
        auto end   = begin + str.length();
        m_ctype.toupper (begin, end);
        return str;
    }

    bool parse_next ();

    size_t remaining () const { return view.end() - pBytecode; }

    // do not change, this is exactly how engine checks for printable characters
    static bool is_shiftJis (uint8_t byte)
    {
        return byte >= 0x81 && byte < 0xA0 || byte >= 0xE0 && byte < 0xFD;
    }

    void process_text (uint16_t word);
    void put_char (uint16_t word, int width);
    void put_special (uint16_t word);
    void put_newline ();
    void flush_text ()
    {
        if (!m_text.empty())
            put_newline();
    }
    const std::string& escape_string (const std::string& str);

    void parse_strz (const uint8_t* ptr);
    bool parse_arg (int& arg);
    int get_arg ()
    {
        int arg;
        parse_arg (arg);
        return arg;
    }
    int skip_operand ();
    void skip_operands ();
    std::ostream& log_operands (std::ostream& out);
    std::string get_string ();
    std::string get_string_arg ();

    void opcode_0X (uint16_t word);
    void opcode_1X (uint16_t word);

    void opcode_A6 ();
    void opcode_A7 ();
    void opcode_A8 ();
    void opcode_A9 ();
    void opcode_AA ();
    void opcode_AB ();
    void opcode_AC ();
    void opcode_AD ();
    void opcode_AE ();
    void opcode_AF ();
    void opcode_B0 ();
    void opcode_B1 ();
    void opcode_B2 ();
    void opcode_B3 ();
    void opcode_B4 ();
    void opcode_B5 ();
    void opcode_B6 ();
    void opcode_B7 ();
    void opcode_B9 ();
    void opcode_BA ();
    void opcode_BB ();
    void opcode_BC ();
    void opcode_BD ();
    void opcode_BE ();
    void opcode_BF ();
    void opcode_C0 ();
    void opcode_C1 ();
    void opcode_C2 ();
    void opcode_C3 ();
    void opcode_C4 ();
    void opcode_C5 ();
    void opcode_C6 ();
    void opcode_C7 ();
    void opcode_C8 ();
    void opcode_C9 (int source);
    void opcode_CA ();
    void opcode_CB ();
    void opcode_CC ();
    void opcode_CD ();
    void opcode_CE ();
    void opcode_CF ();
    void opcode_D0 ();
    void opcode_D1 ();
    void opcode_D2 ();
    void opcode_D3 ();
    void opcode_D4 ();
    void opcode_D5 ();

    void run_builtin (unsigned id);
    void builtin_ACTE ();
    void builtin_PCLICKH2 ();
    void builtin_AVIPLAY ();
    void builtin_APPEARH ();
    void builtin_LOADIPA ();
    void builtin_EXREG ();
    void builtin_QUAKEH ();
    void builtin_SELECT ();
    void builtin_MBUFF ();
    void builtin_CAPPEAR ();
    void builtin_BLNKCSRH ();
    void builtin_CLOCKH ();
    void builtin_ICON3H ();
    void builtin_ROLL ();
    void builtin_MAKEFLAS ();
    void builtin_RECLICKH ();
    void builtin_KEEPPALH ();
    void builtin_GPCPALCH ();
    void builtin_WINDOWH ();
    void builtin_MOUSECSR ();
    void builtin_GETNAMEH ();
    void builtin_NMWIND2 ();
    void builtin_CLIB ();
    void builtin_PUSHPALH ();
    void builtin_WHITEH ();
    void builtin_GAPPEARH ();
    void builtin_BLNKCSR2 ();
    void builtin_MOUSECTR ();
    void builtin_SACTE ();
    void builtin_MAHW ();
    void builtin_LCOUNT ();
    void builtin_PUTNAMEH ();
    void builtin_OMAKE ();
    void builtin_SCRH ();
    void builtin_HDSCRH ();
    void builtin_ROTATEH ();
    void builtin_CAPPEAR2 ();
    void builtin_LOUPE ();
    void builtin_CELLWORK ();
    void builtin_PCMPLAY ();
    void builtin_MOUSENAM ();
    void builtin_ROLL2 ();
    void builtin_SCROLLSP ();
    void builtin_RANDREGH ();
    void builtin_DELTA ();
    void builtin_GETDATE ();

    void sub_pclickh2_0 ();
    void sub_pclickh2_1 ();
    void sub_pclickh2_2 ();
    void sub_pclickh2_3 ();
    void sub_pclickh2_4 ();
    void sub_pclickh2_6 ();

    void sub_aviplay_0 ();
    void sub_aviplay_1 (int arg);

    void keeppalh_1 ();

    void sub_1BADA (int w);
    void sub_1BBF0 (int a1, int a2);
    int sub_1BEB4 ();
    bool sub_1BD6E ();
    bool sub_1C08C ();
    void sub_1C545 (const char* source);
    void execute_sub ();
    void cmd_wait ();

    void skip_whitespace ();
    uint8_t get_byte_after_whitespace ();
    int str2int ();

    const uint8_t* process_B9 (int a1, int a2, int dw, int arg);
    void skip_sub (int a1, int a2);

    int word_29CBE;

    static const uint8_t sPlaceholder[7];
    static std::vector<std::string> s_functions;
    static std::map<uint16_t, uint16_t> s_special_symbols;
    static const uint8_t s_valid_char[256];
};

const uint8_t adv_bytecode_reader::sPlaceholder[] = { 0x81, 0x96, 0x81, 0x96, 0x81, 0x96 }; // ＊＊＊

bool adv_bytecode_reader::
do_run ()
{
    if (!pBytecodeStart)
        throw std::runtime_error ("Uninitialized bytecode reader!");
    m_buffer.reserve (default_string_buffer_size);
    std::cout << std::setfill('0') << std::uppercase;
    std::cerr << std::setfill('0') << std::uppercase;
    pBytecode = pBytecodeStart;
    while (remaining() > 1 && !m_eof_opcode_reached)
       parse_next();
    flush_text();
    return true;
}

bool adv_bytecode_reader::
parse_next ()
{
    const auto currentPos = pBytecode;
    if (remaining() >= 6 && 0 == std::memcmp (pBytecode, sPlaceholder, 6))
    {
        // performs a name substitute from previously loaded NAME.DEF (see opcode_D2)
        log(ll_cmd) << "***\n";
        pBytecode += 6;
        return true;
    }
    if (remaining() >= 4 && 0 == std::memcmp (pBytecode, "wait", 4))
    {
        // this is a workaround for strange script ZACT_FRM.MEC
        cmd_wait();
        return false;
    }
    uint16_t currentWord = get<uint16_t> (pBytecode);
    pBytecode += 2;
    if (0xFFFF == currentWord)
        return true;
    if (0xFEFF == currentWord)
    {
        m_eof_opcode_reached = true;
        log(ll_cmd) << put_offset(currentPos) << "__END__\n";
        return true;
    }
    uint8_t currentByte = currentWord;
    if (currentByte >= 0xA0 && currentByte < 0xA5)
    {
        flush_text();
        --pBytecode;
        print_bytecode (currentPos, currentByte);
        return true;
    }
    currentWord = bin::swap_word (currentWord);
    if (is_shiftJis (currentByte))
    {
        switch (currentWord)
        {
        case 0x816F:
            {
                log(ll_debug) << put_offset(currentPos) << "{\n";
//                contextTable[word_29CFA] = pBytecode;
                break;
            }
        case 0x8170:
            {
                log(ll_debug) << put_offset(currentPos) << "}\n";
//                pBytecode = contextTable[word_29CFA];
                break;
            }
        case 0x8190: // ＄
            log(ll_cmd) << "$\n";
            break;

        case 0x8197: // ＠
            log(ll_debug) << "@\n";
            break;

        default:
            process_text (currentWord);
            break;
        }
        return false;
    }
    if (0x21 == currentByte)
    {
        --pBytecode;
        parse_strz (pBytecode);
        while (pBytecode < view.end() && *pBytecode++)
            ;
        return false;
    }
    switch (currentByte & 0xF0)
    {
    case 0x00:
        opcode_0X (currentWord);
        return false;
    case 0x10:
        opcode_1X (currentWord);
        return false;
    }
    --pBytecode;
    if (0xA5 == currentByte)
    {
        put_newline();
        print_bytecode (currentPos, currentByte);
        return false;
    }
    if (currentByte >= 0x2D && currentByte <= 0x7F)
    {
        currentWord = currentByte + 0x8272;
        process_text (currentWord);
        return false;
    }
    flush_text();
    switch (currentByte)
    {
    case 0xA6: opcode_A6(); break;
    case 0xA7: opcode_A7(); break;
    case 0xA8: opcode_A8(); break;
    case 0xA9: opcode_A9(); break;
    case 0xAA: opcode_AA(); break;
    case 0xAB: opcode_AB(); break;
    case 0xAC: opcode_AC(); break;
    case 0xAD: opcode_AD(); break;
    case 0xAE: opcode_AE(); break;
    case 0xAF: opcode_AF(); break;
    case 0xB0: opcode_B0(); break;
    case 0xB1: opcode_B1(); break;
    case 0xB2: opcode_B2(); break;
    case 0xB3: opcode_B3(); break;
    case 0xB4: opcode_B4(); break;
    case 0xB5: opcode_B5(); break;
    case 0xB6: opcode_B6(); break;
    case 0xB7: opcode_B7(); break;
    case 0xB8: throw invalid_instruction_error (currentPos - view.data(), currentByte);
    case 0xB9: opcode_B9(); break;
    case 0xBA: opcode_BA(); break;
    case 0xBB: opcode_BB(); break;
    case 0xBC: opcode_BC(); break;
    case 0xBD: opcode_BD(); break;
    case 0xBE: opcode_BE(); break;
    case 0xBF: opcode_BF(); break;
    case 0xC0: opcode_C0(); break;
    case 0xC1: opcode_C1(); break;
    case 0xC2: opcode_C2(); break;
    case 0xC3: opcode_C3(); break;
    case 0xC4: opcode_C4(); break;
    case 0xC5: opcode_C5(); break;
    case 0xC6: opcode_C6(); break;
    case 0xC7: opcode_C7(); break;
    case 0xC8: opcode_C8(); break;
    case 0xC9: opcode_C9(0); break;
    case 0xCA: opcode_CA(); break;
    case 0xCB: opcode_CB(); break;
    case 0xCC: opcode_CC(); break;
    case 0xCD: opcode_CD(); break;
    case 0xCE: opcode_CE(); break;
    case 0xCF: opcode_CF(); break;
    case 0xD0: opcode_D0(); break;
    case 0xD1: opcode_D1(); break;
    case 0xD2: opcode_D2(); break;
    case 0xD3: opcode_D3(); break;
    case 0xD4: opcode_D4(); break;
    case 0xD5: opcode_D5(); break;
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x29:
        {
            log(ll_alert) << put_offset (currentPos) << "operand without operation ignored\n";
            get_arg();
            break;
        }
    default:   throw bytecode_error (currentPos - view.data(), currentByte);
    }
    return false;
}

std::string adv_bytecode_reader::
get_string ()
{
    auto pStart = reinterpret_cast<const char*> (pBytecode);
    char c;
    while ((c = get<char> (pBytecode)) != 0x22)
        ++pBytecode;
    return std::string (pStart, reinterpret_cast<const char*> (pBytecode++));
}

std::string adv_bytecode_reader::
get_string_arg ()
{
    uint8_t b;
    while ((b = get<uint8_t> (pBytecode++)) != 0x22)
        ;
    return get_string();
}

void adv_bytecode_reader::
parse_strz (const uint8_t* ptr)
{
    for (;;)
    {
        auto word = get<uint16_t> (ptr);
        uint8_t loByte = word;
        if (!loByte || 0xFF == loByte)
            break;
        if (loByte >= 0x81 && loByte <= 0x9F ||
            loByte >= 0xE0 && loByte <= 0xFC)
        {
            ptr += 2;
            word = bin::swap_word (word);
        }
        else
        {
            ++ptr;
            word = loByte;
        }
        process_text (word);
    }
}

bool adv_bytecode_reader::
parse_arg (int& ret_value)
{
    ret_value = -1;
    int code = get<uint8_t> (pBytecode);
    int sign = 1;
    if (0x2D == code)
    {
        sign = -1;
        ++pBytecode;
        code = get<uint8_t> (pBytecode);
        if (code < 0x23 || code > 0x2C)
            --pBytecode;
    }
    if (code < 0x23 || code > 0x2C)
        return false;
    ++pBytecode;
    if (0x28 == code)
    {
        code = get<uint8_t> (pBytecode++);
    }
    else if (code > 0x28)
    {
        int value = get<uint16_t> (pBytecode);
        pBytecode += 2;
        int ax = (value & 0xFF) << 7;
        ax += value >> 8;
        ax += (code - 0x29) << 14;
        code = ax;
    }
    else
    {
        code -= 0x23;
    }
    ret_value = code * sign;
    return true;
}

int adv_bytecode_reader::
skip_operand ()
{
    auto code = get<uint8_t> (pBytecode++);
    int count = 1;
    if (code >= 0x23 && code <= 0x27)
        return 1;
    if (0x28 == code)
    {
        ++pBytecode;
        return 2;
    }
    if (code >= 0x29 && code <= 0x2C)
    {
        pBytecode += 2;
        return 3;
    }
    if (0x21 == code)
    {
        while (get<uint8_t> (pBytecode))
        {
            ++pBytecode;
            ++count;
        }
        ++pBytecode;
        ++count;
        return count;
    }
    if (0x22 == code)
    {
        while (get<uint8_t> (pBytecode) != 0x22)
        {
            ++pBytecode;
            ++count;
        }
        ++pBytecode;
        ++count;
        return count;
    }
    switch (code & 0xF0)
    {
    case 0x00:
        ++pBytecode;
        return 2;
    case 0x10:
        pBytecode += 2;
        return 3;
    }
    if (!is_shiftJis (code))
        return 0;
    ++pBytecode;
    return 2;
}

void adv_bytecode_reader::
skip_operands ()
{
    int arg;
    do
    {
        auto b = get<uint8_t> (pBytecode);
        if (0x22 == b)
        {
            while (get<uint8_t> (++pBytecode) != 0x22)
                ;
            ++pBytecode;
        }
    }
    while (parse_arg (arg));
}

std::ostream& adv_bytecode_reader::
log_operands (std::ostream& out)
{
    while (remaining() > 1)
    {
        auto code = get<uint8_t> (pBytecode);
        int arg;
        if (0x22 == code)
        {
            ++pBytecode;
            auto str = get_string();
            out << format (", \"%s\"") % escape_string(str);
        }
        else if (parse_arg (arg))
        {
            out << format (", %d") % arg;
        }
        else
            break;
    }
    return out;
}

void adv_bytecode_reader::
cmd_wait ()
{
    pBytecode += 4;
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_cmd) << format ("WAIT (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
opcode_0X (uint16_t word)
{
    log(ll_debug) << format ("%02X %02X\n") % (word >> 8) % (word & 0xFF);
}

void adv_bytecode_reader::
opcode_1X (uint16_t word)
{
    sub_1BADA (word);
}

void adv_bytecode_reader::
opcode_A6 ()
{
    int arg1, arg2, arg3, arg4;
    if (parse_arg (arg1))
    {
        arg2 = get_arg();
        arg3 = get_arg();
        arg4 = get_arg();
        log(ll_cmd) << format ("SET_RECT (%d, %d, %d, %d)\n") % arg1 % arg2 % arg3 % arg4;
    }
    else
    {
        ++pBytecode;
        auto str = get_string();
        log(ll_cmd) << format ("SET_RECT (\"%s\")\n") % str;
        /*
        arg1 = str2int();
        arg2 = str2int();
        arg3 = str2int();
        arg4 = str2int();
        uint8_t b;
        do
        {
            b = get<uint8_t> (pBytecode++);
        }
        while (b != '"');
        */
    }
}

void adv_bytecode_reader::
opcode_A7 ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_debug) << format ("A7 (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
opcode_A8 ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    log(ll_debug) << format ("A8 (%d, %d, %d, %d)\n") % a1 % a2 % a3 % a4;
}

void adv_bytecode_reader::
opcode_A9 ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_debug) << format ("A9 (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
opcode_AA ()
{
    int arg = get_arg();
    log(ll_debug) << format ("AA (%d)\n") % arg;
}

void adv_bytecode_reader::
opcode_AB ()
{
    int a1;
    if (parse_arg (a1))
    {
        int a2 = get_arg();
        log(ll_debug) << format ("AB (%d, %d)\n") % a1 % a2;
    }
    else
        log(ll_debug) << put_offset (pBytecode-1) << "AB\n";
}

void adv_bytecode_reader::
opcode_AC ()
{
    int a1;
    if (parse_arg (a1) && a1 != 0)
    {
        int a2 = get_arg();
        int a3 = get_arg();
        log(ll_debug) << format ("AC (%d, %d, %d)\n") % a1 % a2 % a3;
    }
}

void adv_bytecode_reader::
opcode_AD ()
{
//    log(ll_debug) << "AD\n";
    sub_1C545 ("AD");
}

void adv_bytecode_reader::
opcode_AE ()
{
//    log(ll_debug) << "AE\n";
    sub_1C545 ("AE");
}

void adv_bytecode_reader::
opcode_AF ()
{
    log(ll_debug) << "AF\n";
    auto savePos = pBytecode;
    /*
    uint16_t word;
    do
    {
        word = get<uint16_t> (pBytecode++);
    }
    while (word != 0xFFFF);
    ++pBytecode;
    int i = 1;
    while (i != 0)
    {
        word = get<uint16_t> (pBytecode);
        pBytecode += 2;
        word = bin::swap_word (word);
        if (0xFEFF == word)
            break;
        int code = (word >> 8) & 0xF0;
        if (0x00 == code)
        {
            i = sub_1BEB4();
        }
        else if (0x10 == code)
        {
            sub_1BADA (word);
            i = sub_1BEB4();
        }
        else
        {
            pBytecode -= 2;
            execute_sub();
            break;
        }
    }
    pBytecode = savePos;
    */
}

int adv_bytecode_reader::
sub_1BEB4 ()
{
    for (;;)
    {
        uint8_t b = get<uint8_t> (pBytecode++);
        if (0xFF == b)
        {
            b = get<uint8_t> (pBytecode++);
            if (0xFE == b)
                return 0;
            if (0xFF == b)
                return 1;
        }
    }
} 

void adv_bytecode_reader::
sub_1C545 (const char* source)
{
    int i = 0;
    int arg1;
    while (parse_arg (arg1))
    {
        int arg2 = get_arg();
        log(ll_debug) << format ("%s[%d] <- (%d, %d)\n") % source % i % arg1 % arg2;
        ++i;
    }
    /*
    uint8_t b;
    do
    {
        sub_1C08C();
        b = get<uint8_t> (pBytecode - 1);
    }
    while (0xA4 == b);
    */
}

int adv_bytecode_reader::
str2int ()
{
    throw std::runtime_error ("str2int not implemented");
}

void adv_bytecode_reader::
opcode_B0 ()
{
    auto a1 = get_string_arg();
    auto a2 = get_string_arg();
    log(ll_cmd) << format ("EXEC (\"%s\", \"%s\")\n") % a1 % a2;
}

void adv_bytecode_reader::
opcode_B1 ()
{
    ++pBytecode;
    auto str = get_string();
    log(ll_cmd) << format ("LOAD_SCRIPT (\"%s\")\n") % str;
}

void adv_bytecode_reader::
opcode_B2 ()
{
    ++pBytecode;
    log(ll_debug) << "B2\n";
}

void adv_bytecode_reader::
opcode_B3 ()
{
    int arg1 = get_arg();
    int arg2 = get_arg();
    log(ll_debug) << format ("B3 (%d, %d)\n") % arg1 % arg2;
}

void adv_bytecode_reader::
opcode_B4 ()
{
    int arg = (get<uint8_t> (pBytecode) | 0x20) + 0x9F;
    pBytecode += 2;
    log(ll_debug) << format ("B4 (%X)\n") % arg;
}

void adv_bytecode_reader::
opcode_B5 ()
{
    int arg = get_arg() + 1;
    log(ll_debug) << format ("B5 (%d)\n") % arg;
    ++pBytecode;
//    auto savePos = pBytecode;
//    execute_sub();
//    pBytecode = savePos;
//    skip_sub (0xA2, 0xA3);
}

void adv_bytecode_reader::
opcode_B6 ()
{
    auto code = get<uint8_t> (pBytecode++);
    if (0x22 == code)
    {
        auto arg = get_string();
        log(ll_cmd) << format ("B6 (\"%s\")\n") % arg;
    }
    else
    {
        int arg = get_arg();
        log(ll_cmd) << format ("B6 ('%c', %d)\n") % code % arg;
    }
}

void adv_bytecode_reader::
opcode_B7 ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    log(ll_debug) << format ("B7 (%d, %d, %d)\n") % a1 % a2 % a3;
}

void adv_bytecode_reader::
opcode_B9 ()
{
    int arg = get_arg() << 8;
    log(ll_debug) << format ("B9 (%X)\n") % arg;
    ++pBytecode;
    subroutines[arg] = pBytecode;
//    pBytecode = process_B9 (0xA2, 0xA3, dword_33EC2, arg);
}

void adv_bytecode_reader::
opcode_BA ()
{
    int arg = get_arg();
    log(ll_debug) << put_offset(pBytecode) << format ("BA (%04X)\n") % (arg << 8);
}

void adv_bytecode_reader::
opcode_BB ()
{
    int arg = get_arg();
    ++pBytecode;
    log(ll_debug) << format ("BB (%d)\n") % arg;
}

void adv_bytecode_reader::
opcode_BC ()
{
    log(ll_debug) << "BC\n";
    /*
    auto savePos = ++pBytecode;
    while (!sub_1C08C() || word_32C43 == 1)
    {
        if (get<uint8_t> (pBytecode - 1) == 0xA3)
            break;
    }
    pBytecode = savePos;
    skip_sub (0xA2, 0xA3);
    */
}

void adv_bytecode_reader::
opcode_BD ()
{
    log(ll_debug) << "BD\n";
    /*
    uint8_t b;
    do
    {
        sub_1C08C();
        if (1 == word_32C43 || 8 == word_32C43)
            break;
        b = get<uint8_t> (pBytecode-1);
    }
    while (b != 0xA3);
    */
}

void adv_bytecode_reader::
opcode_BE ()
{
    int a1;
    if (!parse_arg (a1))
        a1 = 0;
    log(ll_debug) << format ("BE (%d)\n") % a1;
}

void adv_bytecode_reader::
opcode_BF ()
{
    int a1;
    if (!parse_arg (a1))
        a1 = 0;
    int a2 = parse_arg (a2);
    log(ll_debug) << format ("BF (%d, %d)\n") % a1 % a2;
}

bool adv_bytecode_reader::
sub_1C08C ()
{
    for (;;)
    {
        auto currentByte = get<uint8_t> (pBytecode);
        if (currentByte == 0xA3 || currentByte == 0xA4)
        {
            sub_1BD6E();
            return false;
        }
        ++pBytecode;
        auto nextByte = get<uint8_t> (pBytecode++);
        switch (currentByte & 0xF0)
        {
        case 0x00:
            break;
        case 0x10:
            sub_1BADA (currentByte << 8 | nextByte);
            break;
        default:
            pBytecode -= 2;
            execute_sub();
            return true;
        }
    }
}

void adv_bytecode_reader::
execute_sub ()
{
    while (!parse_next())
        ;
}

void adv_bytecode_reader::
sub_1BADA (int w)
{
    auto arg = get<uint8_t> (pBytecode++);
    log(ll_debug) << format ("%02X %02X %02X\n") % (w >> 8) % (w & 0xFF) % arg;
}

bool adv_bytecode_reader::
sub_1BD6E ()
{
    int saveWord = word_29CBE;
    word_29CBE = 1;
    for (;;)
    {
        auto b = get<uint8_t> (pBytecode++);
        if (0xA2 == b)
        {
            ++word_29CBE;
        }
        else if (0xA3 == b)
        {
            --word_29CBE;
            if (0 == word_29CBE)
            {
                word_29CBE = saveWord;
                return false;
            }
        }
        else if (0xA4 == b)
        {
            if (1 == word_29CBE)
            {
                word_29CBE = saveWord;
                return true;
            }
        }
        else
        {
            --pBytecode;
            skip_operand();
        }
    }
}

const uint8_t* adv_bytecode_reader::
process_B9 (int a1, int a2, int dw, int arg)
{
    auto savePos = pBytecode;
    execute_sub();
    pBytecode = savePos;
    auto newPos = pBytecode;
    skip_sub (a1, a2);
    int length = pBytecode - newPos;
    for (int i = 0; i < length; ++i)
    {
        get<uint8_t> (newPos++);
    }
    pBytecode = savePos;
    return newPos;
}

void adv_bytecode_reader::
skip_sub (int a1, int a2)
{
    int saveWord = word_29CBE;
    word_29CBE = 1;
    while (word_29CBE)
    {
        auto b = get<uint8_t> (pBytecode++);
        if (b == a1)
        {
            ++word_29CBE;
        }
        else if (b == a2)
        {
            --word_29CBE;
        }
        else
        {
            --pBytecode;
            skip_operand();
        }
    }
    word_29CBE = saveWord;
}

void adv_bytecode_reader::
opcode_C0 ()
{
    auto b = get<uint8_t> (pBytecode);
    if (0x22 == b)
    {
        ++pBytecode;
        auto str = get_string();
        int arg;
        if (!parse_arg (arg))
            throw bytecode_error (current_pos(), "No buffer specified for external file");
        log(ll_cmd) << format ("LOAD (%d, \"%s\")\n") % arg % str;
    }
    else
    {
        int arg = get_arg();
        log(ll_debug) << format ("C0 (%d)\n") % arg;
    }
}

void adv_bytecode_reader::
opcode_C1 ()
{
    int arg = get_arg();
    ++pBytecode;
    auto str = get_string();
    log(ll_cmd) << format ("C1 (%d, \"%s\")\n") % arg % str;
}

void adv_bytecode_reader::
opcode_C2 ()
{
    int arg = get_arg();
    ++pBytecode;
    // sub_21409
    auto str = get_string();
    log(ll_cmd) << format ("C2 (%d, \"%s\")\n") % arg % str;
}

void adv_bytecode_reader::
opcode_C3 ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_debug) << format ("C3 (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
opcode_C4 ()
{
    int a1 = get_arg() & 3;
    int a2 = get_arg() - 0x4F;
    int a3, a4, a5;
    if (!parse_arg (a3))
        a3 = 0;
    if (!parse_arg (a4))
        a4 = 0;
    if (!parse_arg (a5))
        a5 = 0;
    log(ll_debug) << format ("C4 (%d, %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5;
}

void adv_bytecode_reader::
opcode_C5 ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_debug) << format ("C5 (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
opcode_C6 ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    if (a5 > 8)
        a5 = 8;
    log(ll_cmd) << format ("COPY_RECT (%d, %d, %d, %d) -> %d\n") % a1 % a2 % a3 % a4 % a5;
}

void adv_bytecode_reader::
opcode_C7 ()
{
    int a1;
    if (!parse_arg (a1) || a1 > 10)
        a1 = 10;
    log(ll_debug) << format ("C7 (%d)\n") % a1;
}

void adv_bytecode_reader::
opcode_C8 ()
{
    const auto cur_pos = pBytecode;
    ++pBytecode;
    fs::path path = get_string();
    int arg = get_arg();
    if (ll_cmd >= m_log_level)
        std::wcout << ext::wformat (L"SET_ORDINAL (\"%2%\", %1%)\n") % arg % path.native();
    path_string_type ext (path.extension());
    if (ext.empty() || toupper (ext) == L".TCM")
    {
        path_string_type wname (path.stem());
        std::string fname;
        sys::wcstombs (toupper (wname), fname);
        auto prim = std::find (s_functions.begin(), s_functions.end(), fname);
        if (prim != s_functions.end())
        {
            int prim_index = prim - s_functions.begin();
            m_builtins[arg] = prim_index;
            return;
        }
    }
    auto it = m_builtins.find (arg);
    if (it != m_builtins.end())
        m_builtins.erase (it);
    m_externals[arg] = path.native();
}

void adv_bytecode_reader::
opcode_C9 (int source)
{
    ++pBytecode;
    auto path = get_string();
    log(ll_cmd) << format ("LOAD_FILE (%d, \"%s\")\n") % source % path;
}

void adv_bytecode_reader::
opcode_CA ()
{
    auto cb = get<uint8_t> (pBytecode++);
//    cb = (cb | 0x20) + 0x9F; // map 'A'.. character to 0.. integer
    int a1, a2, a3;
    if (!parse_arg (a1))
        a1 = 1;
    if (!parse_arg (a2))
        a2 = 2;
    if (!parse_arg (a3))
        a3 = 3;
    log(ll_debug) << format ("CA ('%c', %d, %d, %d)\n") % cb % a1 % a2 % a3;
}

void adv_bytecode_reader::
opcode_CB ()
{
    int code = get_arg();
    switch (code)
    {
    case 1:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        {
            int arg = get_arg();
            log(ll_debug) << format ("CB_%d (%d)\n") % code % arg;
            break;
        }
    case 2:
        {
            ++pBytecode;
            auto arg = get_string();
            log(ll_debug) << format ("CB_2 (\"%s\")\n") % arg;
            break;
        }
    case 0:
    case 3:
    case 4:
    case 5:
        {
            int arg1 = get_arg();
            int arg2 = get_arg();
            log(ll_debug) << format ("CB_%d (%d, %d)\n") % code % arg1 % arg2;
            break;
        }

    default:
        {
            while (get<uint8_t> (pBytecode) == 0x22)
            {
                ++pBytecode;
                auto str = get_string();
                log(ll_cmd) << format ("CB_%d (\"%s\")\n") % code % str;
            }
            while (get_arg() != -1)
                ;
            break;
        }
    }
}

void adv_bytecode_reader::
opcode_CC ()
{
    int a1, a2;
    if (parse_arg (a1))
    {
        if (!parse_arg (a2))
        {
            a2 = a1;
            a1 = 0;
        }
        log(ll_debug) << format ("CC (%d, %d)\n") % a1 % a2;
    }
}

void adv_bytecode_reader::
opcode_CD ()
{
    int arg = get_arg();
    auto ptr = m_builtins.find (arg);
    if (ptr != m_builtins.end())
    {
        run_builtin (ptr->second);
    }
    else
    {
        auto it = m_externals.find (arg);
        if (it != m_externals.end())
        {
            if (ll_cmd >= m_log_level)
                std::wcout << ext::wformat (L"CALL_ORDINAL (%d) -> \"%s\"") % arg % it->second << std::flush;
            log_operands (log(ll_cmd)) << '\n';
        }
        else
            log_operands (log(ll_debug) << put_offset(pBytecode) << format ("CD (%d") % arg) << ")\n";
    }
}

void adv_bytecode_reader::
run_builtin (unsigned id)
{
    // in Adv98 these were external 'plugins' that were loaded dynamically in run-time
    // in ADVWIN they were incorporated into engine and became builtin functions
    switch (id)
    {
    case 0:     builtin_ACTE();     break;
    case 1:     builtin_PCLICKH2(); break;
    case 2:     builtin_AVIPLAY();  break;
    case 3:     builtin_APPEARH();  break;
    case 4:     builtin_LOADIPA();  break;
    case 6:     builtin_EXREG();    break;
    case 8:     builtin_QUAKEH();   break;
    case 10:    builtin_SELECT();   break;
    case 11:    builtin_MBUFF();    break;
    case 12:    builtin_CAPPEAR();  break;
    case 13:    builtin_BLNKCSRH(); break;
    case 14:    builtin_CLOCKH();   break;
    case 15:    builtin_ICON3H();   break;
    case 16:    builtin_ROLL();     break;
    case 18:    builtin_MAKEFLAS(); break;
    case 20:    builtin_RECLICKH(); break;
    case 22:    builtin_KEEPPALH(); break;
    case 23:    builtin_GPCPALCH(); break;
    case 25:    builtin_WINDOWH();  break;
    case 26:    builtin_MOUSECSR(); break;
    case 27:    builtin_GETNAMEH(); break;
    case 28:    builtin_NMWIND2();  break;
    case 29:    builtin_CLIB();     break;
    case 30:    builtin_PUSHPALH(); break;
    case 31:    builtin_WHITEH();   break;
    case 32:    builtin_GAPPEARH(); break;
    case 33:    builtin_BLNKCSR2(); break;
    case 34:    builtin_MOUSECTR(); break;
    case 35:    builtin_SACTE();    break;
    case 36:    builtin_MAHW();     break;
    case 37:    builtin_LCOUNT();   break;
    case 38:    builtin_PUTNAMEH(); break;
    case 39:    builtin_OMAKE();    break;
    case 40:    builtin_SCRH();     break;
    case 41:    builtin_HDSCRH();   break;
    case 42:    builtin_ROTATEH();  break;
    case 43:    builtin_CAPPEAR2(); break;
    case 44:    builtin_LOUPE();    break;
    case 47:    builtin_CELLWORK(); break;
    case 48:    builtin_PCMPLAY();  break;
    case 49:    builtin_MOUSENAM(); break;
    case 51:    builtin_ROLL2();    break;
    case 53:    builtin_SCROLLSP(); break;
    case 54:    builtin_RANDREGH(); break;
    case 55:    builtin_DELTA();    break;
    case 56:    builtin_GETDATE();  break;
    default:
        if (id < s_functions.size())
            throw bytecode_error (current_pos(), ext::str (format ("builtin %s[%d] not implemented") % s_functions[id] % id));
        else
            throw bytecode_error (current_pos(), ext::str (format ("invalid builtin id %02X") % id));
    }
}

void adv_bytecode_reader::
builtin_ACTE ()
{
    ++pBytecode;
    auto str = get_string();
    log(ll_cmd) << format ("ACTE (\"%s\")\n") % str;
}

void adv_bytecode_reader::
builtin_PCLICKH2 ()
{
    int arg = get_arg();
    switch (arg)
    {
    case 0: sub_pclickh2_0(); break;
    case 1: sub_pclickh2_1(); break;
    case 2: sub_pclickh2_2(); break;
    case 3: sub_pclickh2_3(); break;
    case 4: sub_pclickh2_4(); break;
    case 5: log(ll_cmd) << "PCLICKH2 (5)\n"; break;
    case 6: sub_pclickh2_6(); break;
    default: log(ll_cmd) << format ("PCLICKH2 (%d)\n"); break;
    }
}

void adv_bytecode_reader::
sub_pclickh2_0 ()
{
    auto a1 = get_string_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    log(ll_cmd) << format ("PCLICKH2 (0, \"%s\", %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5;
}

void adv_bytecode_reader::
sub_pclickh2_1 ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    auto a4 = get_string_arg();
    int a5 = get_arg();
    log(ll_cmd) << format ("PCLICKH2 (1, %d, %d, %d, \"%s\", %d)\n") % a1 % a2 % a3 % a4 % a5;
}

void adv_bytecode_reader::
sub_pclickh2_2 ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    auto a3 = get_string_arg();
    int a4 = get_arg();
    log(ll_cmd) << format ("PCLICKH2 (2, %d, %d, \"%s\", %d)\n") % a1 % a2 % a3 % a4;
}

void adv_bytecode_reader::
sub_pclickh2_3 ()
{
    do
    {
        auto a1 = get_string_arg();
        int a2 = get_arg();
        log(ll_cmd) << format ("PCLICKH2 (3, \"%s\", %d)\n") % a1 % a2;
    }
    while (get<uint8_t> (pBytecode) == 0x22);
}

void adv_bytecode_reader::
sub_pclickh2_4 ()
{
    int a1 = get_arg();
    log(ll_cmd) << format ("PCLICKH2 (4, %d") % a1;
    format str_format (", \"%s\"");
    std::string a2;
    do
    {
        a2 = get_string_arg();
        log(ll_cmd) << str_format % a2;
    }
    while (get<uint8_t> (pBytecode) == 0x22);
    format num_format (", %d");
    int a3;
    while (parse_arg (a3))
    {
        log(ll_cmd) << num_format % a3;
    }
    log(ll_cmd) << ")\n";
}

void adv_bytecode_reader::
sub_pclickh2_6 ()
{
    auto a1 = get_string_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    log(ll_cmd) << format ("PCLICKH2 (6, \"%s\", %d, %d)\n") % a1 % a2 % a3;
}

void adv_bytecode_reader::
builtin_AVIPLAY ()
{
    int code = get_arg();
    switch (code)
    {
    case 0: sub_aviplay_0(); break;
    case 1: sub_aviplay_1 (1); break;
    case 2:
    case 4:
        {
            int a1 = get_arg();
            int a2 = get_arg();
            log(ll_cmd) << format ("AVIPLAY (%d, %d, %d)\n") % code % a1 % a2;
            break;
        }
    case 3: log(ll_cmd) << "AVIPLAY (MCI_CLOSE)\n"; break;
    case 7:
        {
            int arg = (get_arg() & 0x3FF) + 1;
            log(ll_cmd) << format ("AVIPLAY (7, %d)\n") % arg;
            break;
        }
    default: throw bytecode_error (current_pos(), ext::str (format ("AVIPLAY_%d not implemented") % code));
    }
}

void adv_bytecode_reader::
sub_aviplay_0 ()
{
    sub_aviplay_1(1);
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_cmd) << format ("AVIPLAY (0, %d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
sub_aviplay_1 (int arg)
{
    auto str = get_string_arg();
    opcode_C9 (1);
    log(ll_cmd) << format ("AVIPLAY (%d, \"%s\")\n") % arg % str;
}

void adv_bytecode_reader::
builtin_APPEARH ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    int a6 = get_arg();
    log(ll_cmd) << format ("APPEARH (%d, %d, %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5 % a6;
}

void adv_bytecode_reader::
builtin_LOADIPA ()
{
    opcode_C9 (1);
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_cmd) << format ("LOADIPA (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
builtin_EXREG ()
{
    int a1;
    if (parse_arg (a1))
    {
        auto a2 = get_string_arg();
        log_operands (log(ll_cmd) << format ("EXREG (%d, \"%s\"") % a1 % escape_string (a2)) << ")\n";
    }
    else if (get<uint8_t> (pBytecode) == 0x22)
    {
        ++pBytecode;
        auto str = get_string();
        log(ll_cmd) << format ("EXREG (\"%s\")\n") % escape_string (str);
    }
}

void adv_bytecode_reader::
builtin_MBUFF ()
{
    auto arg = get_string_arg();
    log(ll_cmd) << format ("MBUFF (\"%s\")\n") % arg;
}

void adv_bytecode_reader::
builtin_QUAKEH ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    int a6 = get_arg();
    log(ll_cmd) << format ("QUAKEH (%d, %d, %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5 % a6;
}

void adv_bytecode_reader::
builtin_SELECT ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_cmd) << format ("SELECT (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
builtin_CAPPEAR ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    log(ll_cmd) << format ("CAPPEAR (%d, %d, %d, %d)\n") % a1 % a2 % a3 % a4;
}

void adv_bytecode_reader::
builtin_BLNKCSRH ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    auto a3 = get_string_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    int a6 = get_arg();
    int a7 = get_arg();
    int a8 = get_arg();
    log(ll_cmd) << format ("BLNKCSRH (%d, %d, \"%s\", %d, %d, %d, %d, %d)\n") % a1 % a2 % escape_string(a3) % a4 % a5 % a6 % a7 % a8;
}

void adv_bytecode_reader::
builtin_CLOCKH ()
{
    int a1 = get_arg();
    auto a2 = get_string_arg();
    auto a3 = get_string_arg();
    auto a4 = get_string_arg();
    log(ll_cmd) << format ("CLOCKH (%d, \"%s\", \"%s\", \"%s\")\n") % a1 % a2 % a3 % a4;
}

void adv_bytecode_reader::
builtin_ICON3H ()
{
    int a1 = get_arg();
    switch (a1)
    {
    case 0:
    case 2:
        {
            int a2 = get_arg();
            log(ll_cmd) << format ("ICON3H (%d, %d)\n") % a1 % a2;
            break;
        }
    case 1:
        {
            int a2 = get_arg();
            int a3 = get_arg();
            int a4 = get_arg();
            int a5 = get_arg();
            int a6 = get_arg();
            log(ll_cmd) << format ("ICON3H (%d, %d, %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5 % a6;
            break;
        }
    case 3:
    default:
        log(ll_cmd) << format ("ICON3H (%d)\n") % a1;
        break;
    }
}

void adv_bytecode_reader::
builtin_ROLL ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    int a6 = get_arg();
    int a7 = get_arg();
    log(ll_cmd) << format ("ROLL (%d, %d, %d, %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5 % a6 % a7;
}

void adv_bytecode_reader::
builtin_MAKEFLAS ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_cmd) << format ("MAKEFLAS (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
builtin_RECLICKH ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    log_operands (log(ll_cmd) << format ("RECLICKH (%d, %d") % a1 % a2) << ")\n";
}

void adv_bytecode_reader::
builtin_KEEPPALH ()
{
    int a1 = get_arg();
    switch (a1)
    {
    case 0:
        {
            int a2 = get_arg();
            int a3 = get_arg();
            log(ll_cmd) << format ("KEEPPALH (%d, %d, %d)\n") % a1 % a2 % a3;
            break;
        }
    case 1:
        keeppalh_1();
        break;
    case 2:
        log(ll_cmd) << "KEEPPALH (2) -> malloc\n";
        break;
    case 3:
        log(ll_cmd) << "KEEPPALH (3) -> free\n";
        break;
    default:
        log(ll_cmd) << format ("KEEPPALH (%d)\n") % a1;
        break;
    }
}

void adv_bytecode_reader::
keeppalh_1 ()
{
    int a1 = get_arg();
    switch (a1)
    {
    case 0:
    case 1:
        {
            int a2 = get_arg();
            log(ll_cmd) << format ("KEEPPALH (1, %d, ...)\n") % a1;
            if (a2)
            {
                int arg;
                while (parse_arg (arg) && 1 == arg)
                    ;
            }
            break;
        }
    case 2:
        {
            int a2 = get_arg();
            int a3 = get_arg();
            log(ll_cmd) << "KEEPPALH (1, 2, ...)\n";
            if (a3)
            {
                int arg;
                while (parse_arg (arg) && 1 == arg)
                    ;
            }
            break;
        }
    default:
        log(ll_cmd) << format ("KEEPPALH (%d)\n") % a1;
    }
}


void adv_bytecode_reader::
builtin_GPCPALCH ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_cmd) << format ("GPCPALCH (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
builtin_WINDOWH ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    int a6 = get_arg();
    int a7 = get_arg();
    int a8 = get_arg();
    int a9 = get_arg();
    int a10 = get_arg();
    log(ll_cmd) << format ("WINDOWH (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5 % a6 % a7 % a8 % a9 % a10;
}

void adv_bytecode_reader::
builtin_MOUSECSR ()
{
    int a1 = get_arg();
    if (a1 < 0x65)
    {
        int a2 = get_arg();
        log(ll_cmd) << format ("MOUSECSR (%d, %d)\n") % a1 % a2;
        return;
    }
    switch (a1 - 0x65)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        log_operands (log(ll_cmd) << format ("MOUSECSR ('%c'") % a1) << ")\n";
        break;
    default:
        throw bytecode_error (current_pos(), "invalid MOUSECSR operand");
    }
}

void adv_bytecode_reader::
builtin_GETNAMEH ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_cmd) << format ("GETNAMEH (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
builtin_NMWIND2 ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    int a6 = get_arg();
    int a7 = get_arg();
    int a8 = get_arg();
    int a9 = get_arg();
    int a10 = get_arg();
    log(ll_cmd) << format ("NMWIND2 (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d") % a1 % a2 % a3 % a4 % a5 % a6 % a7 % a8 % a9 % a10;
    while (get<uint8_t> (pBytecode) == 0x22)
    {
        auto str = get_string_arg();
        log(ll_cmd) << ", \"" << str << '"';
    }
    log(ll_cmd) << ")\n";
}

void adv_bytecode_reader::
builtin_CLIB ()
{
    int a1 = get_arg();
    switch (a1)
    {
    case 0:
        {
            int a2 = get_arg();
            int a3 = get_arg();
            int a4 = get_arg();
            log(ll_cmd) << format ("CLIB (%d, %d, %d, %d)\n") % a1 % a2 % a3 % a4;
            break;
        }
    case 1:
    case 2:
        {
            int a2 = get_arg();
            int a3 = get_arg();
            auto a4 = get_string_arg();
            log(ll_cmd) << format ("CLIB (%d, %d, %d, \"%s\")\n") % a1 % a2 % a3 % a4;
            break;
        }
    default:
        log(ll_cmd) << format ("CLIB (%d)\n") % a1;
        break;
    }
}

void adv_bytecode_reader::
builtin_PUSHPALH ()
{
    int a1 = get_arg();
    if (0 == a1 || 1 == a1)
    {
        int a2 = get_arg();
        int a3 = get_arg();
        log(ll_cmd) << format ("PUSHPALH (%d, %d, %d)\n") % a1 % a2 %a3;
    }
}

void adv_bytecode_reader::
builtin_WHITEH ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_cmd) << format ("WHITEH (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
builtin_GAPPEARH ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    int a6 = get_arg();
    log(ll_cmd) << format ("GAPPEARH (%d, %d, %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5 % a6;
}

void adv_bytecode_reader::
builtin_BLNKCSR2 ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    auto a3 = get_string_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    int a6 = get_arg();
    int a7 = get_arg();
    log(ll_cmd) << format ("BLNKCSR2 (%d, %d, \"%s\", %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5 % a6 % a7;
}

void adv_bytecode_reader::
builtin_MOUSECTR ()
{
    int code = get_arg();
    switch (code)
    {
    case 0:
    case 1:
        break; // NOP
    case 2:
    case 3:
        log(ll_cmd) << format ("MOUSECTR (%d)\n") % code;
        break;
    case 4:
        {
            int a1 = get_arg();
            int a2 = get_arg();
            log(ll_cmd) << format ("MOUSECTR (%d, %d, %d)\n") %code % a1 % a2;
            break;
        }
    case 5:
    case 6:
        {
            int a1 = get_arg();
            log(ll_cmd) << format ("MOUSECTR (%d, %d)\n") %code % a1;
            break;
        }
    default:
        throw bytecode_error (current_pos(), ext::str (format ("MOUSECTR(%d) not implemented") % code));
    }
}

void adv_bytecode_reader::
builtin_SACTE ()
{
    int code = get_arg();
    switch (code)
    {
    case 0:
    case 1:
        {
            int arg = get_arg();
            log(ll_cmd) << format ("SACTE (%d, %d)\n") % code % arg;
            break;
        }
    case 2:
    case 3:
        {
            int a1 = get_arg();
            int a2 = get_arg();
            int a3 = get_arg();
            int a4 = get_arg();
            int a5 = get_arg();
            int a6 = get_arg();
            log(ll_cmd) << format ("SACTE (%d, %d, %d, %d, %d, %d, %d, %d)\n") % code % a1 % a2 % a3 % a4 % a5 % a6;
            break;
        }
    case 4:
    case 5: // NOP
        break;
    default:
        throw bytecode_error (current_pos(), ext::str (format ("SACTE(%d) not implemented") % code));
    }
}

void adv_bytecode_reader::
builtin_MAHW ()
{
    int code = get_arg();
    switch (code)
    {
    case 0:
        {
            auto arg = get_string_arg();
            log(ll_cmd) << format ("MAHW (%d, \"%s\")\n") % code % arg;
            break;
        }
    case 2:
    case 10:
    case 18:
    case 26:
    case 3:
    case 11:
    case 19:
    case 27:
        {
            auto arg = get_arg();
            log(ll_cmd) << format ("MAHW (%d, %d)\n") % code % arg;
            break;
        }
    case 1:
    case 9:
    case 17:
    case 25:
        {
            int a1 = get_arg();
            int a2 = get_arg();
            int a3 = get_arg();
            int a4 = get_arg();
            int a5 = get_arg();
            int a6 = get_arg();
            int a7 = get_arg();
            log(ll_cmd) << format ("MAHW (%d, %d, %d, %d, %d, %d, %d, %d)\n") % code % a1 % a2 % a3 % a4 % a5 % a6 % a7;
            break;
        }
    case 4:
    case 12:
    case 20:
    case 28:
    case 5:
    case 13:
    case 21:
    case 29:
    case 6:
    case 14:
    case 22:
    case 30:
        {
            int a1 = get_arg();
            auto a2 = get_string_arg();
            log(ll_cmd) << format ("MAHW (%d, %d, \"%s\")\n") % code % a1 % a2;
            break;
        }
    default:
        throw bytecode_error (current_pos(), ext::str (format ("MAHW(%d) not implemented") % code));
    }
}

void adv_bytecode_reader::
builtin_LCOUNT ()
{
    auto arg = get_string_arg();
    log(ll_cmd) << format ("LCOUNT (\"%s\")\n") % arg;
}

void adv_bytecode_reader::
builtin_PUTNAMEH ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_cmd) << format ("PUTNAMEH (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
builtin_OMAKE ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3, a4;
    if (!parse_arg (a3))
        a3 = 0;
    if (!parse_arg (a4))
        a4 = 0x3FF;
    log(ll_cmd) << format ("OMAKE (%d, %d, %d, %d)\n") % a1 % a2 % a3 % a4;
}

void adv_bytecode_reader::
builtin_SCRH ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    int a6 = get_arg();
    int a7 = get_arg();
    int a8 = get_arg();
    log(ll_cmd) << format ("SCRH (%d, %d, %d, %d, %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5 % a6 % a7 % a8;
}

void adv_bytecode_reader::
builtin_HDSCRH ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    log(ll_cmd) << format ("HDSCRH (%d, %d, %d)\n") % a1 % a2 % a3;
}

void adv_bytecode_reader::
builtin_ROTATEH ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    int a6 = get_arg();
    int a7 = get_arg();
    log(ll_cmd) << format ("ROTATEH (%d, %d, %d, %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5 % a6 % a7;
}

void adv_bytecode_reader::
builtin_CAPPEAR2 ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    log(ll_cmd) << format ("CAPPEAR2 (%d, %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5;
}

void adv_bytecode_reader::
builtin_LOUPE ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    int a4 = get_arg();
    int a5 = get_arg();
    int a6 = get_arg();
    int a7 = get_arg();
    int a8 = get_arg();
    int a9 = get_arg();
    log(ll_cmd) << format ("LOUPE (%d, %d, %d, %d, %d, %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5 % a6 % a7 % a8 % a9;
}

void adv_bytecode_reader::
builtin_CELLWORK ()
{
    auto a1 = get_string_arg();
    log(ll_cmd) << format ("CELLWORK (\"%s\")\n") % a1;
}

void adv_bytecode_reader::
builtin_PCMPLAY ()
{
    int a1;
    if (0x22 == get<uint8_t> (pBytecode))
    {
        ++pBytecode;
        auto str = get_string();
        if (!parse_arg (a1))
            a1 = 0;
        log(ll_cmd) << format ("PCMPLAY (\"%s\", %d)\n") % str % a1;
    }
    else
    {
        a1 = get_arg();
        int a2;
        if (!parse_arg (a2))
            a2 = 1;
        switch (a2)
        {
        default:
        case 0:
        case 1:
        case 2:
        case 3:
            log(ll_cmd) << format ("PCMPLAY (%d, %d)\n") % a1 % a2;
            break;
        case 4:
            {
                int a3 = get_arg();
                log(ll_cmd) << format ("PCMPLAY (%d, %d, %d)\n") % a1 % a2 % a3;
                break;
            }
        case 5:
            {
                int a3 = get_arg();
                int a4 = get_arg();
                int a5 = get_arg();
                log(ll_cmd) << format ("PCMPLAY (%d, %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5;
                break;
            }
        }
    }
}

void adv_bytecode_reader::
builtin_MOUSENAM ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3 = get_arg();
    auto a4 = get_string_arg();
    int a5 = get_arg();
    int a6 = get_arg();
    log(ll_cmd) << format ("MOUSENAM (%d, %d, %d, \"%s\", %d, %d)\n") % a1 % a2 % a3 % a4 % a5 % a6;
}

void adv_bytecode_reader::
builtin_ROLL2 ()
{
    int a1 = get_arg();
    switch (a1)
    {
    case 0:
        {
            int a2 = get_arg();
            int a3 = get_arg();
            int a4 = get_arg();
            int a5 = get_arg();
            int a6 = get_arg();
            int a7 = get_arg();
            int a8 = get_arg();
            log(ll_cmd) << format ("ROLL2 (%d, %d, %d, %d, %d, %d, %d, %d)\n") % a1 % a2 % a3 % a4 % a5 % a6 % a7 % a8;
            break;
        }
    default:
    case 1:
    case 2:
    case 3: log(ll_cmd) << format ("ROLL2 (%d)\n") % a1; break;
    }
}

void adv_bytecode_reader::
builtin_SCROLLSP ()
{
    int arg;
    if (!parse_arg (arg))
        return;
    log(ll_cmd) << format ("SCROLLSP (%d") % arg;
    format arg_format (", %d");
    while (parse_arg (arg))
    {
        log(ll_cmd) << arg_format % arg;
    }
    log(ll_cmd) << ")\n";
}

void adv_bytecode_reader::
builtin_RANDREGH ()
{
    auto arg = get_string_arg();
    log(ll_cmd) << format ("RANDREGH (\"%s\")\n") % arg;
}

void adv_bytecode_reader::
builtin_DELTA ()
{
    int a1 = get_arg();
    switch (a1)
    {
    default:
        log_operands (log(ll_cmd) << format ("DELTA (%d") % a1) << ")\n";
        break;
    case 0:
        {
            int a2 = get_arg();
            if (a2 > 0x10)
                a2 = 0;
            auto a3 = get_string_arg();
            int a4 = get_arg();
            log(ll_cmd) << format ("DELTA (%d, %d, \"%s\", %d)\n") % a1 % a2 % a3 % a4;
            break;
        }
    case 1:
        {
            int a2 = get_arg();
            if (a2 > 0x10)
                a2 = 0;
            auto a3 = get_string_arg();
            log(ll_cmd) << format ("DELTA (%d, %d, \"%s\")\n") % a1 % a2 % a3;
            break;
        }
    case 2:
        {
            int a2 = get_arg();
            if (a2 > 0x10)
                a2 = 0;
            int a3 = get_arg();
            int a4 = get_arg();
            log(ll_cmd) << format ("DELTA (%d, %d, %d, %d)\n") % a1 % a2 % a3 % a4;
            break;
        }
    case 3:
        {
            int a2 = get_arg();
            if (a2 > 0x10)
                a2 = 0;
            log(ll_cmd) << format ("DELTA (%d, %d)\n") % a1 % a2;
        }
    }
}

void adv_bytecode_reader::
builtin_GETDATE ()
{
    int code = get_arg();
    switch (code)
    { 
    case 0:
        {
            auto arg = get_string_arg();
            log(ll_cmd) << format ("GETDATE (%d, \"%s\")\n") % code % arg;
            break;
        }
    case 8:
        {
            int arg = get_arg();
            log(ll_cmd) << format ("GETDATE (%d, %d)\n") % code % arg;
            break;
        }
    default:
        throw bytecode_error (current_pos(), ext::str (format ("GETDATE(%d) not implemented") % code));
    }
}

void adv_bytecode_reader::
skip_whitespace ()
{
    uint8_t b;
    do
    {
        b = get<uint8_t> (pBytecode++);
    }
    while (b == 0x20 || b == '\t');
    --pBytecode;
}

void adv_bytecode_reader::
opcode_CE ()
{
    auto b = get<uint8_t> (pBytecode);
    std::string str;
    if (0x22 == b)
    {
        ++pBytecode;
        str = get_string();
    }
    int a1 = get_arg();
    int a2 = get_arg();
    if (!str.empty())
        log(ll_cmd) << format ("CE (\"%s\", %d, %d)\n") % str % a1 % a2;
    else
        log(ll_debug) << format ("CE (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
opcode_CF ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    int a3, a4;
    if (!parse_arg (a3))
        a3 = 0;
    if (!parse_arg (a4))
        a4 = 0;
    log(ll_debug) << format ("CF (%d, %d, %d, %d)\n") % a1 % a2 % a3 % a4;
}

void adv_bytecode_reader::
opcode_D0 ()
{
    int word = get<uint16_t> (pBytecode);
    uint8_t lo = word;
    std::string str;
    if (0x22 == lo)
    {
        ++pBytecode;
        str = get_string();
        log(ll_cmd) << format ("SOUND (\"%s\")\n") % str;
    }
    else
    {
        switch (word |= 0x2020)
        {
        case 0x6573: // 'se'
            {
                if (0x22 == get_byte_after_whitespace())
                {
                    ++pBytecode;
                    str = get_string();
                    if (!parse_arg (word))
                        word = 0;
                    log(ll_cmd) << format ("SOUND ('se', %d, \"%s\")\n") % word % str;
                }
                else
                {
                    int a1 = get_arg();
                    if (!parse_arg (word))
                        word = 1;
                    int a2;
                    if (!parse_arg (a2))
                        a2 = 1;
                    if (0 == a2)
                        a2 = -1;
                    log(ll_cmd) << format ("SOUND ('se', %d, %d, %d)\n") % word % a1 % a2;
                }
                break;
            }
        case 0x6463: // 'cd'
            {
                if (0x22 == get_byte_after_whitespace())
                {
                    ++pBytecode;
                    str = get_string();
                    log(ll_cmd) << format ("SOUND ('cd', %d, \"%s\")\n") % word % str;
                }
                else
                {
                    int a1 = get_arg();
                    if (!parse_arg (word))
                        word = 1;
                    int a2;
                    if (!parse_arg (a2) || 0 == a2)
                        a2 = -1;
                    log(ll_cmd) << format ("SOUND ('cd', %d, %d, %d)\n") % word % a1 % a2;
                }
                break;
            }
        case 0x646D: // 'md'
            {
                if (0x22 == get_byte_after_whitespace())
                {
                    ++pBytecode;
                    str = get_string();
                    if (!parse_arg (word))
                        word = 0;
                    log(ll_cmd) << format ("SOUND ('md', %d, \"%s\")\n") % word % str;
                }
                else
                {
                    int a1 = get_arg();
                    if (!parse_arg (word))
                        word = 1;
                    int a2;
                    if (!parse_arg (a2) || 0 == a2)
                        a2 = -1;
                    log(ll_cmd) << format ("D0 ('md', %d, %d, %d)\n") % word % a1 % a2;
                }
                break;
            }
        default:
            {
                word = get_arg();
                switch (word)
                {
                case 1:
                case 3:
                    {
                        int a1 = get_arg();
                        log(ll_debug) << format ("D0 (%d, %d)\n") % word % a1;
                        break;
                    }
                case 2:
                    log(ll_debug) << "D0 (2)\n";
                    break;
                default:
                    {
                        int a1 = get_arg();
                        log(ll_debug) << format ("D0 (%d, %d)\n") % word % a1;
                        break;
                    }
                }
                break;
            }
        }
    }
}

uint8_t adv_bytecode_reader::
get_byte_after_whitespace ()
{
    pBytecode += 2;
    uint8_t b;
    do
    {
        b = get<uint8_t> (pBytecode++);
    }
    while (b == 0x20);
    --pBytecode;
    return b;
}

void adv_bytecode_reader::
opcode_D1 ()
{
    int a1, a2, a3;
    if (parse_arg (a1))
    {
        switch (a1)
        {
        case 0:
        case 1:
            if (!parse_arg (a2))
                a2 = 0;
            if (!parse_arg (a3))
                a3 = 1;
            log(ll_debug) << format ("D1 (%d, %d, %d)\n") % a1 % a2 % a3;
            break;
        case 2:
            {
                int a4;
                if (parse_arg (a4))
                {
                    if (!parse_arg (a2))
                        a2 = 0;
                    if (!parse_arg (a3))
                        a3 = 1;
                    log(ll_debug) << format ("D1 (%d, %d, %d, %d)\n") % a1 % a2 % a3 % a4;
                }
                break;
            }
        default:
            a2 = get_arg();
            log(ll_debug) << format ("D1 (%d, %d)\n") % a1 % a2;
            break;
        }
    }
}

void adv_bytecode_reader::
opcode_D2 ()
{
    log(ll_cmd) << "LOAD_NAME (\"NAME.DEF\")\n";
}

void adv_bytecode_reader::
opcode_D3 ()
{
    int a1 = get_arg();
    int a2 = get_arg();
    log(ll_debug) << format ("D3 (%d, %d)\n") % a1 % a2;
}

void adv_bytecode_reader::
opcode_D4 ()
{
    int a1 = get_arg();
    log(ll_debug) << format ("D4 (%d)\n") % a1;
}

void adv_bytecode_reader::
opcode_D5 ()
{
    int arg = get_arg();
    log(ll_cmd) << format ("DECRYPT (%02X)\n") % arg;
    if (arg != 0)
    {
        auto ptr = const_cast<uint8_t*> (pBytecode); // presumably underlying map allows
                                                     // writes, since our constructor
                                                     // requires readwrite maps
        auto end = view.end() - 4;
        while (ptr < end)
        {
            uint8_t v = *ptr;
            *ptr++ = (v << 4 | v >> 4) ^ arg;
        }
    }
}

void adv_bytecode_reader::
process_text (uint16_t word)
{
    uint8_t hi = word >> 8;
    uint8_t lo = word;
    if (hi)
    {
        put_char (word, 2);
    }
    else if (0x0D == lo)
    {
        // new line
        put_newline();
    }
    else if (lo >= 0x20)
    {
        word = lo + 0x2900;
        if (lo <= 0x7E)
        {
            put_char (word, 1);
        }
        else if (lo >= 0xA0 && lo <= 0xDF)
        {
            put_char (word, 1);
        }
    }
}

std::map<uint16_t, uint16_t> adv_bytecode_reader::s_special_symbols = {
    { 0xEBA9, 0 },
    { 0xEBAB, 0x8169 }, //（
    { 0xEBAC, 0x816A }, // ）
    { 0xEBAF, 0x8149 }, // !
    { 0xEBB0, 0x8148 }, // ?
    { 0xEBC5, 0x8160 }, // 〜
    { 0xEC51, 0x815B }, // ー
    /*
    { 0x86A0, 0 },
    { 0x86A1, 0 },
    { 0x86A2, 0 },
    { 0x86A3, 0 },
    { 0x86A4, 0 },
    { 0xEB9F, 0 }, // heart
    { 0xEBA1, 0 },
    { 0xEBA7, 0 },
    { 0xEBA8, 0 }, // drops
    { 0xEBAD, 0 },
    { 0xEBAE, 0 },
    { 0xEBB6, 0 }, // drops
    { 0xEBBB, 0 },
    { 0xEBBD, 0 }, // pounding heart
    { 0xEBBE, 0 }, // blowing wind
    { 0xEBD5, 0 },
    { 0xEBE6, 0 },
    { 0xEBFA, 0 },
    { 0xEBFC, 0 },
    { 0xEC41, 0 },
    { 0xEC43, 0 },
    { 0xEC44, 0 },
    { 0xEC45, 0 },
    { 0xEC46, 0 },
    { 0xEC47, 0 },
    { 0xEC50, 0 },
    { 0xEC54, 0 },
    */
};

const uint8_t adv_bytecode_reader::s_valid_char[256] = {
//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 2
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 3
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 4
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 5
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 6
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, // 7 
    0, 2, 2, 2, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 8
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 9
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // C
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // D
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, // E
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F
};

void adv_bytecode_reader::
put_char (uint16_t word, int width) // sub_21A66
{
    if (2 == width)
    {
        uint8_t first = word >> 8;
        if (s_valid_char[first] != 2)
        {
            auto it = s_special_symbols.find (word);
            if (it != s_special_symbols.end() && it->second)
            {
                word = it->second;
                width = word < 0x100 ? 1 : 2;
            }
            else
                width = 0;
        }
    }
    if (1 == width)
    {
        m_text.push_back (static_cast<char> (word & 0xFF));
    }
    else if (2 == width)
    {
        m_text.push_back (static_cast<char> (word >> 8));
        m_text.push_back (static_cast<char> (word & 0xFF));
    }
    else
    {
        put_special (word);
    }
}

void adv_bytecode_reader::
put_special (uint16_t word)
{
    auto code = (format ("\\x{%04X}") % word).str();
    m_text.insert (m_text.end(), code.begin(), code.end());
}

void adv_bytecode_reader::
put_newline ()
{
    log(ll_text).write (m_text.data(), m_text.size()).put ('\n');
    m_text.clear();
}

const std::string& adv_bytecode_reader::
escape_string (const std::string& str)
{
    m_buffer.clear();
    bool buffer_used = false;
    for (auto ptr = str.begin(); ptr != str.end(); ++ptr)
    {
        uint8_t byte = static_cast<uint8_t> (*ptr);
        uint8_t len = s_valid_char[byte];
        if (!len && is_shiftJis (byte) && std::next(ptr) != str.end())
        {
            uint16_t word = byte << 8 | static_cast<uint8_t> (*std::next(ptr));
            if (!buffer_used && ptr != str.begin())
            {
                m_buffer.append (str.begin(), ptr);
            }
            auto it = s_special_symbols.find (word);
            if (it == s_special_symbols.end() || !it->second)
            {
                (format ("\\x{%04X}") % word).append_to (m_buffer);
            }
            else
            {
                word = it->second;
                m_buffer.push_back (static_cast<char> (word >> 8));
                m_buffer.push_back (static_cast<char> (word & 0xFF));
            }
            ++ptr;
            buffer_used = true;
        }
        else if (1 == len || (len > 1 && std::next(ptr) != str.end()))
        {
            if (buffer_used)
                m_buffer.append (&*ptr, len);
            if (len > 1)
                ++ptr;
        }
        else
        {
            if (!buffer_used && ptr != str.begin())
            {
                m_buffer.append (str.begin(), ptr);
            }
            (format ("\\x{%02X}") % byte).append_to (m_buffer);
            buffer_used = true;
        }
    }
    if (buffer_used)
        return m_buffer;
    else
        return str;
}

std::vector<std::string> adv_bytecode_reader::s_functions = {
    // XXX order is significant
    "ACTE",
    "PCLICKH2",
    "AVIPLAY",
    "APPEARH",
    "LOADIPA",
    "MCLICK",
    "EXREG",
    "PCLICK2H",
    "QUAKEH",
    "Q2TITLE",
    "SELECT",
    "MBUFF",
    "CAPPEAR",
    "BLNKCSRH",
    "CLOCKH",
    "ICON3H",
    "ROLL",
    "GPCFLASH",
    "MAKEFLAS",
    "LOADFLAS",
    "RECLICKH",
    "SELECTMP",
    "KEEPPALH",
    "GPCPALCH",
    "VPALH",
    "WINDOWH",
    "MOUSECSR",
    "GETNAMEH",
    "NMWIND2",
    "CLIB",
    "PUSHPALH",
    "WHITEH",
    "GAPPEARH",
    "BLNKCSR2",
    "MOUSECTR",
    "SACTE",
    "MAHW",
    "LCOUNT",
    "PUTNAMEH",
    "OMAKE",
    "SCRH",
    "HDSCRH",
    "ROTATEH",
    "CAPPEAR2",
    "LOUPE",
    "CYCLEARH",
    "PALCOPY",
    "CELLWORK",
    "PCMPLAY",
    "MOUSENAM",
    "ADJUSTH",
    "ROLL2",
    "WKSCRH",
    "SCROLLSP",
    "RANDREGH",
    "DELTA",
    "GETDATE",
};

} // namespace adv

int wmain (int argc, wchar_t* argv[])
{
    using namespace adv;

    if (argc < 2)
    {
        std::cout << "usage: deadv [-v] [START.MES...] INPUT\n"
                     "    -v  verbose output (dump all bytecodes)\n";
        return 0;
    }
    logging log_level = ll_cmd;
    int argn = 1;
    try
    {
        if (argc > 2 && 0 == std::wcscmp (argv[argn], L"-v"))
        {
            log_level = ll_debug;
            ++argn;
        }
        adv_bytecode_reader reader;
        reader.set_log_level (ll_none);
        int last_arg = argc - 1;
        while (argn < last_arg)
        {
            if (0 == std::wcscmp (argv[argn], argv[last_arg]))
                break;
            sys::mapping::readwrite start (argv[argn], sys::mapping::writecopy);
            reader.init (start);
            if (!reader.run())
            {
                std::fprintf (stderr, "%S: startup parse failed\n", argv[argn]);
                return 1;
            }
            ++argn;
        }
        sys::mapping::readwrite in (argv[argn], sys::mapping::writecopy);
        reader.init (in);
        reader.set_log_level (log_level);
        return reader.run() ? 0 : 1;
    }
    catch (bytecode_error& X)
    {
        std::fprintf (stderr, "%S:%08X: %s\n", argv[argn], X.get_pos(), X.what());
        return 1;
    }
    catch (std::exception& X)
    {
        std::fprintf (stderr, "%S: %s\n", argv[argn], X.what());
        return 1;
    }
}
