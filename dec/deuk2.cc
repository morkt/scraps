// -*- C++ -*-
//! \file       deuk2.cc
//! \date       2023 Oct 14
//! \brief      extract text from UK2 scripts.
//

#include "bytecode.h"
#include <vector>
#include <map>
#include <cstdio>
#include <unordered_set>

class uk2_arg
{
    enum arg_type
    {
        null,
        scalar,
        svar,
        gvar,
        lvar,
        str,
        var5,
        var8,
        var12,
        text,
        st,
    };

    arg_type    m_type;
    int         m_value;
    std::string m_text;

    uk2_arg () { }
    uk2_arg (arg_type t, int i) : m_type (t), m_value (i) { }

public:
    static uk2_arg SVar (int index) { return uk2_arg (svar, index); }
    static uk2_arg GVar (int index) { return uk2_arg (gvar, index); }
    static uk2_arg LocalVar (int index) { return uk2_arg (lvar, index); }
    static uk2_arg GVar5 () { return uk2_arg (var5, 0); }
    static uk2_arg GVar8 () { return uk2_arg (var8, 0); }
    static uk2_arg GVar12 () { return uk2_arg (var12, 0); }
    static uk2_arg Text (int index) { return uk2_arg (text, index); }
    static uk2_arg Static (int index) { return uk2_arg (st, index); }

    explicit uk2_arg (nullptr_t) : m_type (null) { }
    explicit uk2_arg (int v) : m_type(scalar), m_value (v) { }
    explicit uk2_arg (std::string&& s) : m_type (str), m_text (s) { }

    bool is_text () const { return m_type == text; }
    bool is_scalar () const { return m_type == scalar; }
    bool is_string () const { return m_type == str; }
    const std::string& get_text () const { return m_text; }

    std::ostream& put (std::ostream& out) const;
    static void escape_string (std::ostream& out, const std::string& text);
};

inline std::ostream& operator<< (std::ostream& out, const uk2_arg& arg)
{
    return arg.put (out);
}

class uk2_op
{
    std::vector<std::pair<char, uk2_arg> >  flow;

public:
    bool empty () const { return flow.empty(); }

    void add (char code, const uk2_arg& arg)
    {
        flow.push_back (std::make_pair (code, arg));
    }

    std::ostream& put (std::ostream& out) const;

    const uk2_arg& get_arg () const
    {
        if (!flow.empty())
            return flow.back().second;
        throw std::runtime_error ("access to empty opcode");
    }
};

inline std::ostream& operator<< (std::ostream& out, const uk2_op& op)
{
    return op.put (out);
}

class uk2_reader : public bytecode_reader
{
    std::string                     buffer;
    std::unordered_set<uint16_t>    jumps;
    std::vector<std::string>        locals;
    std::vector<std::string>        globals;

public:
    enum class version { pc98, win };
    const version default_version = version::win;

private:
    version         m_version;

public:
    uk2_reader () : m_version (default_version) { }

    uk2_reader (sys::mapping::map_base& in)
        : bytecode_reader (in)
        , m_version (default_version)
    {
        buffer.reserve (1024);
    }

    void set_version (version ver) { m_version = ver; }

protected:
    bool do_run () override;

    bool is_valid () const override
    {
        return view.size() > 0x18 && 0 == std::memcmp (view.data(), "<< UK2 TEXT Ver1.00 >>", 0x16);
    }

    const uint8_t* start () const override
    {
        return view.data() + 0x17;
    }

private:
    uk2_arg get_arg ();
    uk2_arg get_string_arg ();
    uk2_op get_op ();
    void get_ops (int count, std::ostream& log);
    void get_condition (std::ostream& log);

    const std::string& parse_text (const std::string& text);
    void set_table (const char* mnemonic, std::vector<std::string>& table);

    static const uint16_t sjis_table[];
};

bool uk2_reader::
do_run ()
{
    jumps.clear();
    locals.clear();
    pBytecode = pBytecodeStart;
    while (pBytecode < view.end())
    {
        const auto currentPos = pBytecode;

        uint16_t offset = currentPos - view.data();
        if (jumps.count (offset))
            log(ll_cmd) << put_offset(currentPos) << '\n';

        uint8_t bytecode8 = get_byte();
        switch (bytecode8)
        {
        case 0:
            break;
        case 1:
            {
                auto arg = get_arg();
                auto op = get_op();
                const auto& s = op.get_arg();
                if (s.is_string() && arg.is_text())
                    log(ll_text) << parse_text (s.get_text()) << '\n';
                else
                    log(ll_cmd) << "OP_01 " << arg << op << '\n';
                break;
            }
        case 2:
            {
                auto w = get_word();
                log(ll_debug) << "OP_02 " << hex(w) << '\n';
                jumps.insert (w);
                break;
            }
        case 3:
            {
                auto arg = get_arg();
                log(ll_cmd) << "DEC " << arg << '\n';
                break;
            }
        case 4:
            {
                auto arg = get_arg();
                log(ll_cmd) << "INC " << arg << '\n';
                break;
            }
        case 0x52:
            {
                auto b = get_byte();
                log(ll_debug) << "RET " << (b - '0') << '\n';
                break;
            }
        default:
            {
                uint16_t byteCode16 = bytecode8 << 8 | get_byte();
                switch (byteCode16)
                {
                case 0x4130:
                    {
                        auto arg = get_arg();
                        log(ll_cmd) << "ANIM_INIT " << arg << '\n';
                        break;
                    }
                case 0x4131:
                    {
                        log(ll_cmd) << "ANIM_QUIT\n";
                        break;
                    }
                case 0x4132:
                    {
                        log(ll_debug) << "A2";
                        for (int i = 0; i < 6; ++i)
                        {
                            auto arg = get_arg();
                            log(ll_debug) << ' ' << arg;
                        }
                        log(ll_debug) << '\n';
                        break;
                    }
                case 0x4133:
                case 0x4134:
                    {
                        log(ll_debug) << 'A' << static_cast<char> (byteCode16);
                        get_ops (0x14, log(ll_debug));
                        log(ll_debug) << '\n';
                        break;
                    }
                case 0x4135:
                    {
                        auto arg = get_arg();
                        log(ll_debug) << "A5 " << arg << '\n';
                        break;
                    }
                case 0x4136:
                    {
                        log(ll_debug) << "A6\n";
                        break;
                    }
                case 0x4137:
                    {
                        auto arg = get_arg();
                        log(ll_debug) << "A7 " << arg << '\n';
                        break;
                    }
                case 0x4630:
                    {
                        auto arg = get_arg();
                        log(ll_cmd) << "F0 " << arg << '\n';
                        break;
                    }
                case 0x4635:
                    {
                        auto op = get_op();
                        log(ll_debug) << "F5" << op << '\n';
                        break;
                    }
                case 0x4641:
                    {
                        log(ll_debug) << "FA";
                        get_ops (5, log(ll_debug));
                        log(ll_debug) << '\n';
                        break;
                    }
                case 0x4648:
                    {
                        log(ll_debug) << "FH";
                        get_ops (2, log(ll_debug));
                        for (int i = 0; i < 5; ++i)
                        {
                            auto arg = get_arg();
                            log(ll_debug) << ',' << arg;
                        }
                        log(ll_debug) << '\n';
                        break;
                    }
                case 0x464B:
                    {
                        log(ll_debug) << "FK\n";
                        break;
                    }
                case 0x4930:
                    {
                        log(ll_cmd) << "WRITE_SAVE\n";
                        break;
                    }
                case 0x4931:
                    {
                        log(ll_cmd) << "READ_SAVE\n";
                        break;
                    }
                case 0x4932:
                    {
                        auto op = get_op();
                        log(ll_debug) << "I2" << op << '\n';
                        break;
                    }
                case 0x4A30:
                    {
                        auto pos = get_word();
                        log(ll_cmd) << "JUMP " << hex(pos) << '\n';
                        jumps.insert (pos);
                        break;
                    }
                case 0x4A31:
                    {
                        auto pos = get_word();
                        log(ll_debug) << "CALL " << hex(pos) << '\n';
                        jumps.insert (pos);
                        break;
                    }
                case 0x4A32:
                    {
                        auto arg = get_arg();
                        log(ll_text) << "J2 " << arg << '\n';
                        break;
                    }
                case 0x4C30:
                    {
                        log(ll_debug) << "JUMP_L0 {";
                        get_condition (log(ll_debug));
                        auto w = get_word();
                        log(ll_debug) << " } " << hex(w) << '\n';
                        jumps.insert (w);
                        break;
                    }
                case 0x4C31: // conditional jump
                    {
                        log(ll_cmd) << "JUMP_L1 {";
                        get_condition (log(ll_cmd));
                        auto w = get_word();
                        log(ll_cmd) << " } " << hex(w) << '\n';
                        jumps.insert (w);
                        break;
                    }
                case 0x4C32:
                    {
                        auto w = get_word();
                        log(ll_cmd) << "JUMP_L2 " << hex(w) << '\n';
                        jumps.insert (w);
                        break;
                    }
                case 0x4C33:
                    {
                        auto arg = get_arg();
                        auto op1 = get_op();
                        auto op2 = get_op();
                        auto op3 = get_op();
                        auto w = get_word();
                        log(ll_cmd) << "JUMP_L3 " << arg << op1 << op2 << op3 << " -> " << hex(w) << '\n';
                        jumps.insert(w);
                        break;
                    }
                case 0x4C34:
                    {
                        log(ll_cmd) << "JUMP_L4 {";
                        get_condition (log(ll_cmd));
                        auto w = get_word();
                        log(ll_cmd) << " } " << hex(w) << '\n';
                        jumps.insert (w);
                        break;
                    }
                case 0x4C35:
                    {
                        log(ll_cmd) << "JUMP_L5 {";
                        get_condition (log(ll_cmd));
                        auto w = get_word();
                        log(ll_cmd) << " } " << hex(w) << '\n';
                        jumps.insert (w);
                        break;
                    }
                case 0x4D30:
                    {
                        auto arg = get_arg();
                        log(ll_cmd) << "PLAY " << arg << '\n';
                        break;
                    }
                case 0x4D31:
                    {
                        log(ll_cmd) << "M1";
                        if (m_version == version::win)
                            get_ops (1, log(ll_cmd));
                        log(ll_cmd) << '\n';
                        break;
                    }
                case 0x4D35:
                    {
                        log(ll_cmd) << 'M' << static_cast<char> (byteCode16);
                        get_ops (1, log(ll_cmd));
                        log(ll_cmd) << '\n';
                        break;
                    }
                case 0x4D32:
                    {
                        log(ll_cmd) << "STOP_MUSIC\n";
                        break;
                    }
                case 0x4D34:
                    break;
                case 0x5430:
                    set_table ("SET_LOCAL_TABLE", locals);
                    break;
                case 0x5431:
                    set_table ("SET_NAME_TABLE", globals);
                    break;
                case 0x5535:
                    {
                        auto arg = get_arg();
                        auto op = get_op();
                        log(ll_cmd) << "RAND " << arg << op << '\n';
                        break;
                    }
                case 0x5549:
                    {
                        log(ll_debug) << "UI";
                        get_ops (6, log(ll_debug));
                        log(ll_debug) << '\n';
                        break;
                    }
                case 0x554B:
                case 0x5530:
                    {
                        auto op = get_op();
                        log(ll_debug) << 'U' << static_cast<char>(byteCode16) << op << '\n';
                        break;
                    }
                case 0x5531:
                    {
                        log(ll_debug) << "U1";
                        get_ops (2, log(ll_debug));
                        log(ll_debug) << '\n';
                        break;
                    }
                case 0x5532:
                    {
                        auto op = get_op();
                        log(ll_debug) << "U2" << op << '\n';
                        for (;;)
                        {
                            op = get_op();
                            if (op.empty())
                                break;
                            const auto& s = op.get_arg();
                            if (s.is_string())
                                log(ll_text) << parse_text (s.get_text()) << '\n';
                        }
                        break;
                    }
                case 0x5533:
                    {
                        log(ll_cmd) << "U3";
                        if (m_version == version::pc98)
                            get_ops (2, log(ll_cmd));
                        auto arg = get_arg();
                        log(ll_cmd) << ' ' << arg << '\n';
                        break;
                    }
                case 0x5536:
                    {
                        auto op1 = get_op();
                        auto op2 = get_op();
                        log(ll_debug) << "U6" << op1 << op2 << '\n';
                        break;
                    }
                case 0x5537:
                    {
                        auto op = get_op();
                        log(ll_debug) << "U7" << op << '\n';
                        break;
                    }
                case 0x5539:
                    {
                        auto arg = get_arg();
                        log(ll_debug) << "U9_KEYSTATE -> " << arg;
                        break;
                    }
                case 0x5542:
                    {
                        auto arg1 = get_arg();
                        auto arg2 = get_arg();
                        log(ll_cmd) << "UB " << arg1 << ',' << arg2 << '\n';
                        break;
                    }
                case 0x5543:
                    {
                        auto arg = get_arg();
                        log(ll_text) << "UC " << arg;
                        for (;;)
                        {
                            auto op = get_op();
                            if (op.empty())
                                break;
                            log(ll_text) << op;
                        }
                        log(ll_text) << '\n';
                        break;
                    }
                case 0x5544:
                    {
                        auto op = get_op();
                        auto arg = get_arg();
                        log(ll_cmd) << "UD" << op << ',' << arg << '\n';
                        break;
                    }
                case 0x5545:
                    {
                        auto op = get_op();
                        log(ll_cmd) << "UE" << op;
                        for (;;)
                        {
                            auto op = get_op();
                            if (op.empty())
                                break;
                            log (ll_cmd) << op;
                            get_ops (2, log(ll_cmd));
                        }
                        log(ll_cmd) << '\n';
                        break;
                    }
                case 0x5548:
                    {
                        auto op = get_op();
                        log(ll_debug) << "UH" << op << '\n';
                        break;
                    }
                case 0x554A:
                    {
                        auto op = get_op();
                        auto arg = get_arg();
                        log(ll_debug) << "UJ" << op << ',' << arg << '\n';
                        break;
                    }
                case 0x5731:
                    {
                        log(ll_debug) << "W1";
                        get_ops (7, log(ll_debug));
                        log(ll_debug) << '\n';
                        break;
                    }
                case 0x5732:
                    {
                        log(ll_debug) << "W2\n";
                        break;
                    }
                case 0x5734:
                    {
                        log(ll_debug) << "W4 {";
                        get_ops (2, log(ll_debug));
                        log(ll_debug) << "}\n";
                        break;
                    }
                case 0x5735:
                    {
                        auto arg = get_arg();
                        auto op = get_op();
                        log(ll_debug) << "W5 " << arg << op << '\n';
                        break;
                    }
                case 0x5736:
                    {
                        auto op1 = get_op();
                        auto arg = get_arg();
                        auto op2 = get_op();
                        log(ll_cmd) << "W6" << op1 << ',' << arg << ',' << op2 << '\n';
                        break;
                    }
                case 0x5738:
                    {
                        auto arg = get_arg();
                        auto op1 = get_op();
                        get_op();
                        get_op();
                        log(ll_debug) << "W8 " << '\n';
                        for (;;)
                        {
                            auto op = get_op();
                            if (op.empty())
                                break;
                            const auto& s = op.get_arg();
                            if (s.is_string())
                                log(ll_text) << parse_text (s.get_text()) << '\n';
                        }
                        break;
                    }
                case 0x5737:
                case 0x5741:
                case 0x5743:
                case 0x5744:
                case 0x5749:
                    {
                        auto op = get_op();
                        log(ll_debug) << 'W' << static_cast<char> (byteCode16) << op << '\n';
                        break;
                    }
                default:
                    throw_error (currentPos, byteCode16);
                }
            }
        }
    }
    return true;
}

void uk2_reader::
set_table (const char* mnemonic, std::vector<std::string>& table)
{
    log(ll_cmd) << mnemonic << " {\n";
    table.clear();
    for (;;)
    {
        auto op = get_op();
        if (op.empty())
            break;
        const auto& s = op.get_arg();
        if (s.is_string())
        {
            table.push_back (s.get_text());
            log(ll_cmd) << parse_text (s.get_text()) << '\n';
        }
        else
            log(ll_cmd) << op << '\n';
    }
    log(ll_cmd) << "}\n";
}

std::ostream& uk2_arg::
put (std::ostream& out) const
{
    switch (m_type)
    {
    case null:      out << "NULL"; break;
    case scalar:    out << m_value; break;
    case svar:      out << "Str[" << m_value << ']'; break;
    case gvar:      out << "Global[" << m_value << ']'; break;
    case lvar:      out << "Local[" << m_value << ']'; break;
//    case str:       escape_string (out, m_text); break;
    case str:       out << '"' << m_text << '"'; break;
    case var5:      out << "GVar5"; break;
    case var8:      out << "GVar8"; break;
    case var12:     out << "GVar12"; break;
    case text:      out << "Text[" << m_value << ']'; break;
    case st:        out << "Static[" << m_value << ']'; break;
    default: std::runtime_error ("invalid argument type");
    }
    return out;
}

void uk2_arg::
escape_string (std::ostream& out, const std::string& text)
{
    out.put ('"');
    for (auto c : text)
    {
        if (!(c >= 0x20 && c < 0x7F))
            out << "\\x" << detail::hex_number_printer(static_cast<uint8_t> (c));
        else
            out.put (c);
    }
    out.put ('"');
}

void uk2_reader::
get_ops (int count, std::ostream& out)
{
    while (count --> 0)
    {
        auto op = get_op();
        if (op.empty())
            break;
        out << op;
    }
}

const std::string& uk2_reader::
parse_text (const std::string& text)
{
    bool kana = false;
    bool number = false;
    buffer.clear();
    for (auto it = text.begin(); it != text.end(); ++it)
    {
        uint8_t c = *it;
        if (6 == c || 5 == c)
        {
            // this is a hack to allow substution done in get_string_arg
            const char* glob = 6 == c ? "\\NAME" : "\\T";
            buffer.append (glob);
            while (++it != text.end())
            {
                buffer.push_back (*it);
                if (*it == ']')
                    break;
            }
            continue;
        }
        else if ('\\' == c)
        {
            if (++it == text.end())
            {
                buffer.push_back ('\\');
                break;
            }
            c = *it;
            switch (c)
            {
            case 'C':
                buffer.append ("\\C");
                if (std::next(it) != text.end())
                    buffer.push_back (*++it);
                continue;
            case 'K':
                kana = false;
                continue;
            case 'k':
                kana = true;
                continue;
            case 'N':
                number = true;
                continue;
            case 'n':
                number = false;
                continue;
            }
            buffer.push_back ('\\');
            buffer.push_back (c);
            continue;
        }
        if (c >= '0' && c <= '9' && number)
        {
            // full-width numbers
            buffer.push_back (0x82);
            buffer.push_back (0x4F + c - '0');
            continue;
        }
        else if (c < 0x80 || c > 0x9F && c < 0xE0)
        {
            if (!kana)
            {
                uint16_t sjis = sjis_table[c];
                buffer.push_back (sjis & 0xFF);
                c = sjis >> 8;
            }
        }
        else
        {
            buffer.push_back (c);
            if (++it == text.end())
                break;
            c = *it;
        }
        buffer.push_back (c);
    }
    return buffer;
}

uk2_arg uk2_reader::
get_arg ()
{
    const auto curPos = pBytecode;
    auto code = get_byte();
    log(ll_trace)<<"[arg:"<<hex(code)<<']';
    uint8_t lo = code & 0x1F;
    switch (code >> 5)
    {
    case 0:
        if (lo >= 0x1F)
            return uk2_arg (get_word());
        else
            return uk2_arg::GVar (lo);
        break;
    case 1: return uk2_arg::SVar (lo);
    case 2: return uk2_arg::LocalVar (lo);
    case 3: return get_string_arg();
    case 5: return uk2_arg::Static (get_byte());
    case 6: return uk2_arg::Text (lo);
    case 4:
        {
            auto op = get_op();
            switch (lo)
            {
            case 5:     return uk2_arg::GVar5();
            case 8:     return uk2_arg::GVar8();
            case 0x12:  return uk2_arg::GVar12();
            }
        }
    }
    return uk2_arg(nullptr);
}

uk2_arg uk2_reader::
get_string_arg ()
{
    std::ostringstream os;
    std::string str;
    char c;
    while (c = get_byte())
    {
        if (5 == c)
        {
            auto n = get_byte() - 1;
            if (n < locals.size())
                str.append (locals[n]);
            else
            {
                os.str (std::string());
                os << "\x05[" << n << ']';
                str.append (os.str());
            }
        }
        else if (6 == c)
        {
            auto n = get_byte() - 1;
            if (n < globals.size())
                str.append (globals[n]);
            else
            {
                os.str (std::string());
                os << "\x06[" << n << ']';
                str.append (os.str());
            }
        }
        else
            str.push_back (c);
    }
    return uk2_arg (std::move (str));
}

uk2_op uk2_reader::
get_op ()
{
    uk2_op op;
    int8_t code;
    do
    {
        const auto curPos = pBytecode;
        code = get_byte();
        if (!(code & 0x7F))
            break;
        log(ll_trace) << "[op:" << hex(static_cast<uint8_t>(code)) <<']';
        auto arg = get_arg();
        switch (code & 0x7F)
        {
        case 1: op.add ('+', arg); break;
        case 2: op.add ('-', arg); break;
        case 3: op.add ('*', arg); break;
        case 4: op.add ('/', arg); break;
        case 5: op.add ('%', arg); break;
        case 7: op.add ('=', arg); break;
        default: throw_error (curPos, "invalid operation code");
        }
    }
    while (code > 0);
    return op;
}

void uk2_reader::
get_condition (std::ostream& out)
{
    uint8_t state = 3;
    do
    {
        buffer.clear();
        auto op1 = get_op();
        auto code = get_byte();
        log(ll_trace)<<"[cond:" <<hex(code)<<']';
        auto op2 = get_op();
        switch (code)
        {
        case 1: buffer.append (" '<'"); break;
        case 2: buffer.append (" '>'"); break;
        case 3: buffer.append (" '!='"); break;
        case 4: buffer.append (" '=='"); break;
        }
        if (1 == state)
            out << " && ";
        else if (2 == state)
            out << " || ";
        out << op1 << buffer << op2;
        state = get_byte();
    }
    while (state);
}

std::ostream& uk2_op::
put (std::ostream& out) const
{
    for (const auto& op : flow)
    {
        out << ' ' << op.first << op.second;
    }
    return out;
}

const uint16_t uk2_reader::sjis_table[] = {
//      00      01      02      03      04      05      06      07      08      09      0A      0B      0C      0D      0E      0F      
    0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, // 00
    0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, // 10
    0x4F82, 0x5082, 0x5182, 0x5282, 0x5382, 0x2A2A, 0x5482, 0x5582, 0x5682, 0x5782, 0x5882, 0xA082, 0xA282, 0xA482, 0xA682, 0xA882, // 20
    0xA982, 0xAB82, 0xAD82, 0xAF82, 0xB182, 0xB382, 0xB582, 0xB782, 0xB982, 0xBB82, 0xBD82, 0xBF82, 0xC282, 0xC482, 0xC682, 0xC882, // 30
    0xC982, 0xCA82, 0xCB82, 0xCC82, 0xCD82, 0xD082, 0xD382, 0xD682, 0xD982, 0xDC82, 0xDD82, 0xDE82, 0xDF82, 0xE082, 0xE282, 0xE482, // 40
    0xE682, 0xE782, 0xE882, 0xE982, 0xEA82, 0xEB82, 0xED82, 0xF082, 0xF182, 0xAA82, 0xAC82, 0xAE82, 0x2A2A, 0xB082, 0xB282, 0x6481, // 50
    0xB682, 0xB882, 0xBA82, 0xBC82, 0xBE82, 0xC082, 0xCF82, 0xC582, 0xC782, 0xCE82, 0xD182, 0xD482, 0xD782, 0xDA82, 0x9F82, 0xA182, // 60
    0xA382, 0xA582, 0xA782, 0xE182, 0xE382, 0xE582, 0x4183, 0x4383, 0x4583, 0x4783, 0x4983, 0x4A83, 0x4C83, 0x4E83, 0x5083, 0x5283, // 70
    0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, // 80
    0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, 0x2A2A, // 90
    0x5483, 0x5683, 0x5883, 0x5A83, 0x5C83, 0x5E83, 0x6083, 0x6383, 0x6583, 0x6783, 0x6983, 0x6A83, 0x6B83, 0x6C83, 0x6D83, 0x6E83, // A0
    0x7183, 0x7483, 0x7783, 0x7A83, 0x7D83, 0x7E83, 0x4281, 0x8183, 0x8283, 0x8483, 0x8683, 0x8883, 0x8983, 0x8A83, 0x8B83, 0x8C83, // B0
    0x8D83, 0x8F83, 0x6081, 0x9383, 0x4B83, 0x4D83, 0x4F83, 0x4981, 0x5383, 0x5583, 0x5783, 0x5983, 0x5B83, 0x5B81, 0x5F83, 0x6183, // C0
    0x4181, 0x6683, 0x6883, 0x6F83, 0x7283, 0x7583, 0x7883, 0x7B83, 0x4083, 0x4283, 0x4881, 0x4683, 0x4883, 0x8383, 0x8583, 0x8783, // D0
    0, 0,
};

int wmain (int argc, wchar_t* argv[])
{
    logging log_level = ll_cmd;
    uk2_reader::version version = uk2_reader::version::win;
    int argn = 1;
    int last_arg = argc - 1;
    while (argn < last_arg)
    {
        if (0 == std::wcscmp (argv[argn], L"-v"))
        {
            log_level = ll_debug;
        }
        else if (0 == std::wcscmp (argv[argn], L"-p"))
        {
            version = uk2_reader::version::pc98;
        }
        else
            break;
        ++argn;
    }
    if (argn >= argc)
    {
        std::cout << "usage: deuk2 [-v][-p] [START.MES] SCRIPT.MES\n";
        return 0;
    }
    try
    {
        uk2_reader reader;
        reader.set_version (version);
        reader.set_log_level (ll_none);
        int last_arg = argc - 1;
        if (argn < last_arg && 0 != std::wcscmp (argv[argn], argv[argn+1]))
        {
            sys::mapping::readonly start (argv[argn]);
            reader.init (start);
            if (!reader.run())
            {
                std::fprintf (stderr, "%S: startup parse failed\n", argv[argn]);
                return 1;
            }
            ++argn;
        }
        sys::mapping::readonly in (argv[argn]);
        reader.init (in);
        reader.set_log_level (log_level);
        return reader.run() ? 0 : 1;
    }
    catch (bytecode_error& X)
    {
        std::fprintf (stderr, "%S:%04X: %s\n", argv[argn], X.get_pos(), X.what());
        return 1;
    }
    catch (std::exception& X)
    {
        std::fprintf (stderr, "%S: %s\n", argv[argn], X.what());
        return 1;
    }
}
