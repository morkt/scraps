// -*- C++ -*-
//! \file       depias.cc
//! \date       2023 Oct 18
//! \brief      decompile PiasSys engine bytecode.
//
// [981030][Pias] Nariyuki Romance
//

#include "bytecode.h"
#include <vector>
#include <map>
#include <unordered_set>

extern const unsigned short pias_sjis_encoding[];

class pias_reader : public bytecode_reader
{
    std::vector<uint16_t>   buffer;
    std::map<int, std::vector<unsigned> > m_arrays;
    std::unordered_set<int> jumps;
    bool    m_decrypted;
    int     m_version;

    static const int default_version = 2;

public:
    pias_reader (sys::mapping::readwrite& in)
        : bytecode_reader (in)
        , m_decrypted (false)
        , m_version (default_version)
    { }

    void decrypt ();

    void set_version (int version) { m_version = version; }

protected:
    bool do_run () override;

    const uint8_t* start () const override
    {
        return m_decrypted ? view.data() + 4 : view.data();
    }

    unsigned get_integer ();
    int get_short ();

    void get_string ();
    void print_text ();
    void print_char (uint16_t w);
};

struct key_transform
{
    unsigned    m_a;
    unsigned    m_b;

    key_transform (unsigned seed_a) : m_a (seed_a), m_b (0) { }

    void set_seed (unsigned seed_b)
    {
        m_b = seed_b;
    }

    unsigned next ()
    {
        unsigned y, x;
        if (0 == m_a)
        {
            x = 0xD22;
            y = 0x849;
        }
        else if (1 == m_a)
        {
            x = 0xF43;
            y = 0x356B;
        }
        else if (2 == m_a)
        {
            x = 0x292;
            y = 0x57A7;
        }
        else
        {
            x = 0;
            y = 0;
        }
        unsigned v3 = x + m_b * y;
        unsigned v4 = 0;
        if (v3 & 0x400000)
            v4 = 1;
        if (v3 & 0x400 )
            v4 ^= 1;
        if (v3 & 1)
            v4 ^= 1;
        m_b = (v3 >> 1) | (v4 ? 0x80000000 : 0);
        return m_b;
    }
};

bool pias_reader::
do_run ()
{
    jumps.clear();
    while (pBytecode < view.end())
    {
        int offset = pBytecode - pBytecodeStart;
        if (jumps.count (offset))
            log(ll_cmd) << put_offset(pBytecode) << '\n';
        auto opcode = get_byte();
        switch (opcode)
        {
        case 0: // TEXT
            {
                get_string();
                print_text();
                break;
            }
        case 1:
            log(ll_cmd) << "CLR\n";
            break;
        case 2:
            log(ll_debug) << "WND\n";
            get_integer();
            get_integer();
            get_integer();
            get_integer();
            break;
        case 3:
            {
                auto r = get_byte();
                auto g = get_byte();
                auto b = get_byte();
                log(ll_cmd) << "COL #" << hex(r) << hex(g) << hex(b) << '\n';
            }
            break;
        case 4:
            log(ll_debug) << "WAT " << get_integer() << '\n';
            break;
        case 5:
            log(ll_cmd) << "MILLISECWAIT " << get_integer() << '\n';
            break;
        case 6:
            log(ll_cmd) << "SPD " << get_integer() << '\n';
            break;
        case 7:
            log(ll_debug) << "CR\n";
            break;
        case 8:
            {
                int arg = get_short();
                log(ll_cmd) << "SELECT " << arg << '\n';
                int count = get_byte();
                for (int i = 0; i < count; ++i)
                {
                    get_string();
                    log(ll_text) << '[' << i << "]:";
                    print_text();
                }
                break;
            }
        case 9:
        case 0xB:
            {
                int arg = get_byte();
                log(ll_debug) << "OP_" << hex(opcode) << ' ' << arg << '\n';
                break;
            }
        case 0xA:
            {
                int arg1 = get_byte();
                int arg2 = get_byte();
                int arg3 = get_byte();
                log(ll_debug) << "OP_0A " << arg1 << ',' << arg2 << ',' << arg3 << '\n';
                break;
            }
        case 0xC:
            {
                int arg = get_integer();
                log(ll_cmd) << "GRAPH " << hex(arg) << '\n';
                break;
            }
        case 0xD:
            {
                int arg = get_byte();
                log(ll_cmd) << "LOAD_FONT " << arg << '\n';
                break;
            }
        case 0xE:
            {
                int x = get_integer();
                int y = get_integer();
                log(ll_cmd) << "OP_0E " << x << ',' << y << '\n';
                break;
            }
        case 0x10:
            get_integer();
            break;
        case 0x13:
            {
                int x = get_integer();
                int y = get_integer();
                int w = get_integer();
                int h = get_integer();
                log(ll_cmd) << "RECT " << x << ',' << y << ',' << w << ',' << h << '\n';
                break;
            }
        case 0x15:
            {
                int arg = get_integer();
                log(ll_debug) << "OP_15 " << arg << '\n';
                break;
            }
        case 0x40:
            {
                int arg1 = get_short();
                int arg2 = get_short();
                log(ll_debug) << "LET Var[" << arg1 << "]:=Var[" << arg2 << "]\n";
                break;
            }
        case 0x41:
            log(ll_cmd) << "ADD\n";
            get_short();
            get_short();
            break;
        case 0x42:
            log(ll_cmd) << "SUB\n";
            get_short();
            get_short();
            break;
        case 0x50:
            {
                int addr = get_integer();
                log(ll_cmd) << "JMP " << hex(addr) << '\n';
                jumps.insert(addr);
                break;
            }
        case 0x51:
            {
                int addr = get_integer();
                log(ll_cmd) << "GOSUB " << hex(addr) << '\n';
                jumps.insert(addr);
                break;
            }
        case 0x52:
            log(ll_cmd) << "RETURN\n";
            if (!jumps.count (pBytecode-pBytecodeStart))
                log(ll_cmd) << put_offset(pBytecode) << '\n';
            break;
        case 0x53:
            {
                int arg1 = get_short();
                int arg2 = get_integer();
                int arg3 = get_short();
                int adr = get_integer();
                jumps.insert (adr);
                log(ll_cmd) << "CHKJMP " << arg1 << ',' << arg2 << ',' << arg3 << ':' << hex(adr) << '\n';
                break;
            }
        case 0x5F:
            log(ll_cmd) << "END\n";
            break;
        case 0x60:
            log(ll_cmd) << "SURFACE\n";
            break;
        case 0x61:
            log(ll_cmd) << "MENU_DISABLE\n";
            break;
        case 0x62:
            log(ll_cmd) << "MENU_ENABLE\n";
            break;
        case 0x63:
            log(ll_cmd) << "SAVE\n";
            break;
        case 0x64:
            log(ll_cmd) << "LOAD\n";
            break;
        case 0x65:
            {
                int arg1 = get_short();
                int arg2 = get_integer();
                int arg3 = get_integer();
                log(ll_cmd) << "RAND " << arg1 << ',' << arg2 << ',' << arg3 << '\n';
                break;
            }
        case 0x66:
        case 0x67:
        case 0x69:
            {
                int arg = get_integer();
                log(ll_debug) << "OP_" << hex(opcode) << arg << '\n';
                break;
            }
        case 0x68:
            {
                int id = get_integer();
                int count = get_integer();
                auto& offsets = m_arrays[id];
                offsets.clear();
                offsets.reserve (count);
                for (int i = 0; i < count; ++i)
                    offsets.push_back (get_dword());
                log(ll_cmd) << "SET_ARRAY " << id << ",[" << count << " entries]\n";
                break;
            }
        case 0x6A:
        case 0x6C:
            log(ll_debug) << "OP_" << hex(opcode) << '\n';
            break;
        case 0x6B:
            {
                int arg = get_integer();
                log(ll_debug) << "OP_6B " << arg << '\n';
                break;
            }
        case 0x80:
            {
                int arg1 = get_integer();
                int arg2 = get_integer();
                log(ll_cmd) << "SCN " << hex(arg1) << ',' << arg2 <<'\n';
                break;
            }
        case 0x81:
            {
                int arg1 = get_integer();
                int arg2 = get_integer();
                log(ll_cmd) << "HSCN " << arg1 << ',' << arg2 <<'\n';
                break;
            }
        case 0x82:
            {
                auto arg = get_integer();
                get_short();
                get_short();
                get_integer();
                get_integer();
                log(ll_cmd) << "CHAR " << hex(arg) << '\n';
                break;
            }
        case 0x83:
            log(ll_cmd) << "DELETECHAR\n";
            get_integer();
            get_integer();
            break;
        case 0x84:
            log(ll_cmd) << "COMPOUND\n";
            break;
        case 0x88:
            log(ll_cmd) << "FILLCOLOR\n";
            get_byte();
            get_byte();
            get_byte();
            get_integer();
            break;
        case 0x8A:
            log(ll_cmd) << "BRIGHTNESS\n";
            get_integer();
            break;
        case 0x90:
            log(ll_cmd) << "HANE\n";
            get_integer();
            break;
        case 0x91:
            log(ll_cmd) << "RIPPLE\n";
            get_integer();
            get_integer();
            break;
        case 0x92:
            log(ll_cmd) << "SHAKE\n";
            get_integer();
            get_integer();
            get_integer();
            break;
        case 0x93:
        case 0x96:
        case 0x97:
            {
                int arg1 = get_integer();
                int arg2 = get_integer();
                log(ll_cmd) << "EYECATCH " << arg1 << ',' << arg2 << '\n';
                break;
            }
        case 0x94:
            log(ll_cmd) << "X4SCROLL\n";
            get_integer();
            get_integer();
            get_integer();
            get_integer();
            get_integer();
            get_integer();
            break;
        case 0x95:
            log(ll_cmd) << "HCGWATCH\n";
            break;
        case 0xC0:
            {
                auto arg = get_integer();
                log(ll_cmd) << "MUS " << hex(arg) << '\n';
                break;
            }
        case 0xC1:
            log(ll_cmd) << "STOPMUSIC\n";
            break;
        case 0xC2:
            log(ll_cmd) << "FADEMUSIC\n";
            break;
        case 0xE0:
            if (m_version > 1)
            {
                int arg1 = get_integer();
                int arg2 = get_integer();
                int arg3 = get_byte();
                log(ll_cmd) << "SND " << hex(arg1) << ',' << arg2 << ',' << arg3 << '\n';
            }
            else
            {
                auto arg = get_integer();
                log(ll_cmd) << "SND " << hex(arg) << '\n';
            }
            break;
        case 0xE1:
            if (m_version > 1)
            {
                int arg = get_integer();
                log(ll_cmd) << "STOPSOUND " << arg << '\n';
            }
            else
            {
                log(ll_cmd) << "STOPSOUND\n";
            }
            break;
        case 0xE3:
            {
                int arg1 = get_integer();
                int arg2 = get_integer();
                log(ll_debug) << "OP_E3 " << arg1 << ',' << arg2 << '\n';
                break;
            }
        case 0xE4:
            {
                int arg1 = get_short();
                int arg2 = get_integer();
                log(ll_debug) << "OP_E4 " << arg1 << ',' << arg2 << '\n';
                break;
            }
        case 0xF5:
        case 0xF7:
        case 0xF8:
            {
                auto arg = get_integer();
                log(ll_debug) << "OP_" << hex(opcode) << ' ' << arg << '\n';
                break;
            }
        default:
            throw_error (pBytecode-1, opcode);
            break;
        }
    }
    return true;
}

void pias_reader::
decrypt ()
{
    if (m_decrypted)
        return;
    key_transform rnd (1);
    rnd.set_seed (get<uint32_t> (view.data()));
    auto data = const_cast<uint8_t*> (view.data());
    for (size_t i = 4; i < view.size(); i++)
    {
        data[i] ^= rnd.next();
    }
    m_decrypted = true;
}

unsigned pias_reader::
get_integer ()
{
    unsigned result = get_byte();
    unsigned code = result & 0xC0;
    if (0 == code)
        return result;
    result = (result & 0x3F) << 8 | get_byte();
    if (0x40 == code)
        return result;
    result = result << 8 | get_byte();
    if (0x80 == code)
        return result;
    return result << 8 | get_byte();
}

int pias_reader::
get_short ()
{
    int n = get_byte();
    if ((n & 0xC0) == 0xC0)
        return n << 8 | get_byte();
    return n;
}

void pias_reader::
get_string ()
{
    buffer.clear();
    size_t len = get_integer();
    for (size_t i = 0; i < len; ++i)
        buffer.push_back (get_word());
}

void pias_reader::
print_char (uint16_t w)
{
    uint16_t sjis = 0;
    if (w <= 0x1A28)
        sjis = pias_sjis_encoding[w];

    if (sjis)
        log(ll_text).put (static_cast<char> (sjis >> 8)).put (static_cast<char> (sjis));
    else
        log(ll_text) << "\\x" << std::setw(4) << std::hex << w;
}

void pias_reader::
print_text ()
{
    for (const auto& w : buffer)
    {
        print_char (w);
    }
    log(ll_text) << '\n';
}

int wmain (int argc, wchar_t* argv[])
{
    logging log_level = ll_cmd;
    int argn = 1;
    int last_arg = argc - 1;
    int version = 0;
    bool decrypt = false;
    while (argn < last_arg)
    {
        if (0 == std::wcscmp (argv[argn], L"-v"))
        {
            log_level = ll_debug;
            ++argn;
        }
        else if (0 == std::wcscmp (argv[argn], L"-d"))
        {
            decrypt = true;
        }
        else if (0 == std::wcscmp (argv[argn], L"-p"))
        {
            ++argn;
            if (argn >= last_arg)
            {
                argn = argc;
                break;
            }
            errno = 0;
            version = std::wcstol (argv[argn], nullptr, 10);
            if (errno)
            {
                std::fprintf (stderr, "invalid version specified\n");
                return 1;
            }
        }
        else
            break;
    }
    if (argn >= argc)
    {
        std::cout << "usage: depias [-v] text.dat\n";
                     "    -v  verbose output\n"
                     "    -d  decrypt script\n"
                     "    -p N specify version\n";
        return 0;
    }
    auto script_name = argv[argn];
    try
    {
        sys::mapping::readwrite in (script_name, sys::mapping::writecopy);
        pias_reader reader (in);
        reader.set_log_level (log_level);
        reader.ser_version (version);
        if (decrypt)
            reader.decrypt();
        return reader.run() ? 0 : 1;
    }
    catch (bytecode_error& X)
    {
        std::fprintf (stderr, "%S:%04X: %s\n", script_name, X.get_pos(), X.what());
        return 1;
    }
    catch (std::exception& X)
    {
        std::fprintf (stderr, "%S: %s\n", script_name, X.what());
        return 1;
    }
}
