// -*- C++ -*-
//! \file       advb.cc
//! \date       2023 Oct 04
//! \brief      ADVB engine by Discovery
//

#include "bytecode.h"
#include <vector>
#include <cstdio>

class advb_reader : public bytecode_reader
{
    std::vector<char>       args;
    size_t                  arg_pos;
    bool                    m_did_unpack;

public:
    advb_reader (sys::mapping::map_base& in)
        : bytecode_reader (in)
        , m_did_unpack (false)
    {
        args.reserve (1024);
    }

    void unpack_script ();

    struct var_arg_printer
    {
        int arg;
        var_arg_printer (int a) : arg (a) { }
    };

protected:
    bool do_run () override;

private:
    const std::string& get_string ();
    template <typename T> T get_arg ();

    void skip_arg (size_t n) { arg_pos += n; }
    int get_var_arg ();

    void opcode_IF ();
    void opcode_arith (uint8_t code);

    static var_arg_printer print_arg (int arg) { return var_arg_printer (arg); }

    void lzss_unpack (sys::mapping::view<uint8_t>& output);
    static void setup_lzss_frame (std::vector<uint8_t>& frame);
};

inline std::ostream& operator<< (std::ostream& os, const advb_reader::var_arg_printer& p)
{
    if (p.arg < 0)
        return os << "VAR[" << (-p.arg - 1) << ']';
    else
        return os << static_cast<int16_t> (p.arg);
}

int advb_reader::
get_var_arg ()
{
    auto is_var = get_arg<uint8_t>();
    auto arg = get_arg<uint16_t>();
    // negative result indicates variable reference
    return is_var ? -arg-1 : arg;
}

void advb_reader::
unpack_script ()
{
    if (m_did_unpack)
        return;
    pBytecode = pBytecodeStart;
    size_t packed_size = get_word();
    get_word();
    size_t unpacked_size = get_word();
    get_word();
    sys::mapping::readwrite mem (sys::file_handle::invalid_handle(), sys::mapping::writeshare, unpacked_size);
    sys::mapping::view<uint8_t> unpacked (mem);
    lzss_unpack (unpacked);
    view.remap (mem, 0, unpacked_size);
    m_did_unpack = true;
    pBytecode = pBytecodeStart = view.data();
}

bool advb_reader::
do_run ()
{
    if (!m_did_unpack && pBytecode + 8 < view.end()
        && get<uint16_t> (pBytecode) == view.size()-8)
    {
        unpack_script();
    }
    pBytecode = pBytecodeStart;
    int last_op = -1;
    while (pBytecode + 3 < view.end())
    {
        const auto currentPos = pBytecode;
        auto bytecode = get_word() - 0xFC00;
        auto arg_len = get_word();
        if (pBytecode + arg_len > view.end())
            throw_error (currentPos, "invalid argument length");
        auto pArg = reinterpret_cast<const char*> (pBytecode);
        args.assign (pArg, pArg + arg_len);
        arg_pos = 0;
        if (0x0B == last_op || 0x21 == last_op)
            log(ll_cmd) << put_offset(currentPos) << '\n';
        switch (bytecode)
        {
        case 0x01:
            log(ll_cmd) << "IMAGE " << print_arg(get_var_arg()) << ", "
                        << get_string() << '\n';
            break;
        case 0x02:
            log(ll_cmd) << "ANIM " << get_string() << '\n';
            break;
        case 0x06:
            log(ll_text) << get_string() << '\n';
            break;    
        case 0x08:
            log(ll_cmd) << "SCRIPT " << get_string() << '\n';
            break;
        case 0x0A:
            log(ll_cmd) << "CALL " << hex(get_arg<uint16_t>()) << '\n';
            break;
        case 0x0B:
            log(ll_cmd) << "RET\n";
            break;
        case 0xC:
            log(ll_cmd) << "MUSIC " << get_string() << ", " << get_string() << '\n';
            break;
        case 0xD:
            log(ll_cmd) << "0D " << get_string() << ", " << get_string() << ", " << get_string() << '\n';
            break;
        case 0xE:
            {
                int n = get_var_arg();
                get_arg<uint16_t>();
                log(ll_cmd) << "OPTION " << print_arg(n) << ", " << get_string() << '\n';
                break;
            }
        case 0x16:
            log(ll_debug) << "WAIT\n";
            break;
        case 0x18:
            log(ll_cmd) << "CHOICE " << (arg_len == 1 ? "BEGIN" : "END") << '\n';
            break;
        case 0x19:
            log(ll_cmd) << "PROMPT\n";
            break;
        case 0x1A:
            log(ll_cmd) << "SWITCH BEGIN\n";
            break;
        case 0x1B:
            log(ll_cmd) << "SWITCH END\n";
            break;
        case 0x1C:
            log(ll_cmd) << "CASE " << print_arg(get_var_arg()) << '\n';
            break;
        case 0x1D:
            opcode_IF();
            break;
        case 0x1E:
            log(ll_cmd) << "ELSE\n";
            break;
        case 0x1F:
            log(ll_cmd) << "ENDIF\n";
            break;
        case 0x25:
            log(ll_cmd) << "VAR[" << get_arg<uint16_t>() << "]:=" << print_arg(get_var_arg()) << '\n';
            break;
        case 0x28:
        case 0x29:
        case 0x2A:
        case 0x2B:
        case 0x2E:
        case 0x2F:
        case 0x30:
            opcode_arith (bytecode);
            break;
        case 0x31:
            log(ll_cmd) << "PUSH " << print_arg(get_var_arg()) << '\n';
            break;
        case 0x32:
            {
                int var = get_arg<uint16_t>();
                log(ll_cmd) << "POP VAR[" << var << "]\n";
                break;
            }
        case 0x37:
            log(ll_cmd) << "SET_NAME " << print_arg(get_var_arg()) << ',' << get_string() << '\n';
            break;
        case 0x3C:
            log(ll_cmd) << "3C " << get_string() << '\n';
            break;
        case 0x43:
            log(ll_cmd) << "SWITCH BREAK\n";
            break;
        case 0x55:
            {
                int x = get_var_arg();
                int y = get_var_arg();
                int op = get_var_arg();
                log(ll_cmd) << "IMAGE_AT " << print_arg(x) << ',' << print_arg(y) << ',' << print_arg(op)
                    << ',' << get_string() << '\n';
                break;
            }
        case 0x59:
            log(ll_cmd) << "59 " << print_arg(get_var_arg()) << ',' << get_string() << '\n';
            break;
        case 0x73:
            log(ll_cmd) << "EFFECT " << get_string() << '\n';
            break;
        case 0x76:
            log(ll_cmd) << print_arg(get_var_arg()) << ", " << get_string() << '\n';
            break;
        default:
            log(ll_debug) << hex(bytecode) << " UNKNOWN";
            if (arg_len) log(ll_debug) << '[' << arg_len << ']';
            log(ll_debug) << '\n';
            break;
        }
        pBytecode += arg_len;
        last_op = bytecode;
    }
    return true;
}

template<typename T>
T advb_reader::get_arg ()
{
    if (arg_pos + sizeof(T) > args.size())
        throw_error (pBytecode, "not enough arguments");
    T val = *reinterpret_cast<const T*> (args.data() + arg_pos);
    arg_pos += sizeof(T);
    return val;
}

void advb_reader::
opcode_arith (uint8_t code)
{
    int var = get_arg<uint16_t>();
    int a1 = get_var_arg();
    int a2 = get_var_arg();
    char op = '?';
    switch (code)
    {
    case 0x28: op = '+'; break;
    case 0x29: op = '-'; break;
    case 0x2A: op = '*'; break;
    case 0x2B: op = '/'; break;
    case 0x2E: op = '&'; break;
    case 0x2F: op = '|'; break;
    case 0x30: op = '^'; break;
    }
    log(ll_cmd) << "VAR[" << var << "]:=" << print_arg(a1) << op << print_arg(a2) << '\n';
}

void advb_reader::
opcode_IF ()
{
    log(ll_cmd) << "IF";
    int clause_count = 0;
    while (get_arg<int16_t>() != -1)
    {
        arg_pos -= 2;
        int a1 = get_var_arg();
        int op = get_arg<uint8_t>();
        int a2 = get_var_arg();
        const char* op_str;
        switch (op)
        {
        case 0: op_str = "=="; break;
        case 1: op_str = ">";  break;
        case 2: op_str = "<";  break;
        case 3: op_str = ">="; break;
        case 4: op_str = "<="; break;
        case 5: op_str = "!="; break;
        default: op_str = "??"; break;
        }
        if (clause_count)
            log(ll_cmd) << " AND";
        log(ll_cmd) << ' ' << print_arg(a1) << op_str << print_arg(a2);
        ++clause_count;
    }
    if (!clause_count)
        log(ll_cmd) << " TRUE";
    log(ll_cmd) << '\n';
}

const std::string& advb_reader::
get_string ()
{
    static std::string arg;
    arg.clear();
    if (arg_pos >= args.size())
    {
        log(ll_debug) << put_offset(pBytecode) << " not enough arguments\n";
        return arg;
    }
    auto zero_pos = std::find (args.begin() + arg_pos, args.end(), '\0');
    arg.assign (args.begin() + arg_pos, zero_pos);
    arg_pos = zero_pos - args.begin() + 1;
    return arg;
}

void advb_reader::
setup_lzss_frame (std::vector<uint8_t>& frame)
{
    frame.resize (0x1000);
    size_t dst = 0;
    for (int i = 0; i < 0x100; ++i)
    {
        for (int j = 0; j < 13; ++j)
            frame[dst++] = i;
    }
    // 0xD00
    for (int i = 0; i < 0x100; ++i)
        frame[dst++] = i;
    // 0xE00
    for (int i = 0xFF; i >= 0; --i)
        frame[dst++] = i;
    // 0xF00
    for (int i = 0; i < 0x80; ++i)
        frame[dst++] = 0;
    // 0xF80
    for (int i = 0; i < 0x6E; ++i)
        frame[dst++] = 0x20;
}

void advb_reader::
lzss_unpack (sys::mapping::view<uint8_t>& output)
{
    std::vector<uint8_t> frame;
    setup_lzss_frame (frame);
    int frame_pos = 0xFEE;
    const int frame_mask = 0xFFF;
    size_t dst = 0;
    uint8_t mask = 0;
    uint8_t ctl;
    while (pBytecode < view.end() && dst < output.size())
    {
        mask <<= 1;
        if (!mask)
        {
            ctl = get_byte();
            mask = 1;
        }
        if (ctl & mask)
        {
            output[dst++] = frame[frame_pos++ & frame_mask] = get_byte();
        }
        else
        {
            unsigned lo = get_byte();
            unsigned hi = get_byte();
            unsigned offset = (hi & 0xF0) << 4 | lo;
            int count = std::min ((hi & 0xF) + 3, output.size() - dst);
            while (count --> 0)
            {
                auto b = frame[offset++ & frame_mask];
                output[dst++] = frame[frame_pos++ & frame_mask] = b;
            }
        }
    }
}

int wmain (int argc, wchar_t* argv[])
{
    logging log_level = ll_cmd;
    bool force_unpack = false;
    int argn = 1;
    int last_arg = argc - 1;
    while (argn < last_arg)
    {
        if (0 == std::wcscmp (argv[argn], L"-v"))
        {
            log_level = ll_debug;
            ++argn;
        }
        else if (0 == std::wcscmp (argv[argn], L"-u"))
        {
            force_unpack = true;
            ++argn;
        }
        else
            break;
    }
    if (argn >= argc)
    {
        std::cout << "usage: advb [-v] [-u] INPUT\n"
                     "    -v  verbose output\n"
                     "    -u  unpack script\n";
        return 0;
    }
    auto script_name = argv[argn];
    try
    {
        sys::mapping::readonly in (script_name);
        advb_reader reader (in);
        reader.set_log_level (log_level);
        if (force_unpack)
            reader.unpack_script();
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
