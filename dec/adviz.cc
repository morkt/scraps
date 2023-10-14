// -*- C++ -*-
//! \file       adviz.cc
//! \date       2023 Sep 30
//! \brief      extract text from ADVIZ engine scripts.
//

#include "bytecode.h"
#include <vector>
#include <map>
#include <cstdio>

class adviz_reader : public bytecode_reader
{
    std::vector<char>       buffer;

public:
    enum class version { pc98, w1, w2 };

private:
    struct argument
    {
        int     var_ref;
        int     value;

        argument (int v) : var_ref (0), value (v) { }
        argument (int n, int v) : var_ref (n), value (v) { }
    };

    std::vector<argument>       m_args;
    std::vector<std::string>    m_grp_table;
    std::vector<std::string>    m_anm_table;
    std::vector<std::string>    m_text_table;
    std::map<int, int>          m_vars;
    version                     m_version;
    bool                        m_did_unpack;

public:
    adviz_reader (sys::mapping::map_base& in)
        : bytecode_reader (in)
        , m_version (version::w1)
        , m_did_unpack (false)
    {
        buffer.reserve (1024);
    }

    void set_version (version ver) { m_version = ver; }

    void set_grp_table (const std::wstring& filename);
    void set_anm_table (const std::wstring& filename);
    void set_text_table (const std::wstring& filename);

protected:
    bool do_run () override;

private:
    void opcode_34 ();
    void opcode_4A ();
    void file_op (char bytecode);
    int get_arg ();
    void skip_args ();
    void check_args (size_t num) const
    {
        if (m_args.size() < num)
            throw_error (pBytecode, "not enough arguments");
    }
    int get_var (int var_ptr) const
    {
        auto it = m_vars.find (var_ptr);
        if (it != m_vars.end())
            return it->second;
        else
            return 0;
    }
    void parse_table (const std::wstring& filename, std::vector<std::string>& table);

    void put_string (uint8_t key);
    void process_text ();

    void unpack_script ();
    void prs_unpack (sys::mapping::view<uint8_t>& output);

    bool        dword_40D16C;
    int16_t     word_4141DC;
    int         dword_415368;
};

void adviz_reader::
unpack_script ()
{
    size_t unpacked_size = get_word();
    size_t packed_size = get_word();
    if (packed_size != view.size()-4)
        throw std::runtime_error ("invalid PRS script.");
    sys::mapping::readwrite mem (sys::file_handle::invalid_handle(), sys::mapping::writeshare, unpacked_size);
    sys::mapping::view<uint8_t> unpacked (mem);
    prs_unpack (unpacked);
    view.remap (mem, 0, unpacked_size);
    m_did_unpack = true;
    pBytecode = pBytecodeStart = view.data();
}

bool adviz_reader::
do_run ()
{
    pBytecode = pBytecodeStart;
    if (m_version == version::pc98 && !m_did_unpack)
    {
        unpack_script();
    }
    while (pBytecode < view.end())
    {
        const auto currentPos = pBytecode;
        int8_t bytecode = get_byte();
        if (bytecode >= 0)
        {
            switch (bytecode)
            {
            case 0x10:
                {
                    int arg = get_arg();
                    int pos = get_word();
                    log(ll_trace) << put_offset(currentPos) << "JUMP_UNLESS " << arg << ", " << hex(pos+1) << '\n';
                    break;
                }
            case 0x11: // NOP
                log(ll_trace) << put_offset(currentPos) << "NOP\n";
                break;
            case 0x12:
            case 0x1A:
                {
                    uint16_t adr = get_arg();
                    log(ll_trace) << put_offset(currentPos) << "JUMP " << hex(adr) << '\n';
                    break;
                }
            case 0x14:
                {
                    int arg = get_arg();
                    int pos = get_word();
                    log(ll_trace) << put_offset(currentPos) << "JUMP_IF_NOT " << arg << ", " << hex(pos) << '\n';
                    break;
                }
            case 0x15:
                {
                    int arg = get_arg();
                    int pos = get_word();
                    log(ll_trace) << put_offset(currentPos) << "JUMP_IF " << arg << ", " << hex(pos) << '\n';
                    break;
                }
            case 0x1C:
                {
                    int arg = get_arg();
                    log(ll_cmd) << put_offset(currentPos) << "CALL " << hex(arg) << '\n';
                    break;
                }
            case 0x1E:
                {
                    log(ll_cmd) << put_offset(currentPos) << "RETURN\n";
                    break;
                }
            case 0x20:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x22:
                {
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x26:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x28:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x30:
                {
                    ++pBytecode;
                    int16_t word = get_word();
                    log(ll_trace) << "30 " << word << '\n';
                    break;
                }
            case 0x32:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x34:
                {
                    opcode_34();
                    break;
                }
            case 0x35:
                break;
            case 0x36:
                {
                    int16_t arg1 = get_arg();
                    int arg2 = get_arg();
                    word_4141DC = -1;
                    if (arg2)
                        word_4141DC = arg1;
                    break;
                }
            case 0x38:
                {
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x3A:
                {
                    dword_415368 = get_arg();
                    dword_40D16C = true;
                    break;
                }
            case 0x3C:
                {
                    get_arg();
                    break;
                }
            case 0x3E:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x40:
                {
                    int a1 = get_arg();
                    int a2 = get_arg();
                    log(ll_trace) << "VAR[" << a2 << "] := RAND(" << a1 << ")\n";
                    break;
                }
            case 0x41:
                {
                    ++pBytecode;
                    int var_ref = get<int16_t> (pBytecode);
                    pBytecode += 2;
                    log(ll_trace) << '[' << var_ref << "] := CURRENT_TIME\n";
                    break;
                }
            case 0x42:
                {
                    get_arg();
                    get_arg();
                    log(ll_trace) << "'B'\n";
                    break;
                }
            case 0x43:
                {
                    get_arg();
                    get_arg();
                    log(ll_trace) << "'C'\n";
                    break;
                }
            case 0x44:
                {
                    int a1 = get_arg();
                    int a2 = get_arg();
                    skip_args();
                    log(ll_trace) << "'D'\n";
                    break;
                }
            case 0x4A:
                opcode_4A();
                break;
            case 0x4B:
                {
                    int arg = get_arg();
                    log(ll_cmd) << "READ " << "SAVE" << arg << ".DAT\n";
                    break;
                }
            case 0x4C:
                {
                    log(ll_cmd) << "WRITE SYSSAVE.DAT\n";
                    break;
                }
            case 0x4D:
                {
                    log(ll_cmd) << "READ SYSSAVE.DAT\n";
                    break;
                }
            case 0x4E:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x4F:
                {
                    get_arg();
                    break;
                }
            case 0x50:
                {
                    get_arg();
                    break;
                }
            case 0x51:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x54:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x56:
                {
                    int arg = get_arg();
                    log(ll_cmd) << "SET_CURSOR " << arg << '\n';
                    break;
                }
            case 0x58:
                {
                    put_string (0x58);
                    break;
                }
            case 0x5A:
                {
                    int a1 = get_arg();
                    int a2 = get_arg();
                    int a3 = get_arg();
                    int a4 = get_arg();
                    int a5 = get_arg();
                    log(ll_trace) << put_offset(currentPos) << "5A " << a1 << ", " << a2 << ", " << a3 << ", " << a4 << ", " << a5 << '\n';
                    break;
                }
            case 0x5C:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x5D:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x5E:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x5F:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x68:
                {
                    int arg = get_arg();
                    log(ll_cmd) << "SELECT_PALETTE " << arg << '\n';
                    break;
                }
            case 0x6A:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    if (m_version != version::pc98)
                        get_arg();
                    break;
                }
            case 0x6E:
                {
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    get_arg();
                    break;
                }
            case 0x70:
            case 0x71:
            case 0x72:
            case 0x73:
            case 0x74:
                file_op (bytecode);
                break;

            default:
                throw_error (currentPos, bytecode);
            }
        }
        else if (-1 == bytecode)
        {
            log(ll_text) << put_offset(currentPos) << "__END__\n";
        }
        else
        {
            --pBytecode;
            get_arg();
        }
    }
    return true;
}

int adviz_reader::
get_arg ()
{
    m_args.clear();
    uint8_t argCode;
    const auto currentPos = pBytecode;
    do
    {
        argCode = get_byte();
        switch (argCode & 0xBF)
        {
        case 0x81:
            {
                auto word = get<int16_t> (pBytecode);
                if (word >= 0)
                {
                    log(ll_trace) << "STATIC_VAR_REF[" << word << "]\n";
                }
                else
                {
                    log(ll_trace) << "VAR_REF[" << word << "]\n";
                }
                pBytecode += 2;
                m_args.push_back (argument (word, 0));
                break;
            }
        case 0x84:
            {
                auto opCode = get_byte();
                switch (opCode)
                {
                case 1:
                    {
                        check_args(1);
                        auto& arg = m_args.back();
                        log(ll_trace) << "BOOL " << arg.value << "\n";
                        arg.value = !!arg.value;
                        break;
                    }
                case 3:
                    {
                        check_args(1);
                        auto& arg = m_args.back();
                        log(ll_trace) << "NEG " << arg.value << "\n";
                        arg.value = -arg.value;
                        break;
                    }
                case 4:
                    {
                        check_args(1);
                        int arg = m_args.back().value++;
                        log(ll_trace) << "INC " << arg << "\n";
                        int var_ref = m_args.back().var_ref;
                        if (var_ref)
                        {
                            m_vars[var_ref] = arg+1;
                            log(ll_trace) << "SET_VAR[" << var_ref << "] := " << (arg+1) << '\n';
                        }
                        break;
                    }
                case 5:
                    {
                        check_args(1);
                        int arg = m_args.back().value--;
                        log(ll_trace) << "DEC " << arg << "\n";
                        int var_ref = m_args.back().var_ref;
                        if (var_ref)
                        {
                            m_vars[var_ref] = arg-1;
                            log(ll_trace) << "SET_VAR[" << var_ref << "] := " << (arg-1) << '\n';
                        }
                        break;
                    }
                case 6: // *=
                    {
                        check_args(2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        m_args.back().value *= arg1;
                        m_args.back().var_ref = 0;
                        log(ll_trace) << "*= " << arg1 << '\n';
                        break;
                    }
                case 7: // /=
                    {
                        check_args(2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        if (arg1)
                            m_args.back().value /= arg1;
                        m_args.back().var_ref = 0;
                        log(ll_trace) << "/= " << arg1 << '\n';
                        break;
                    }
                case 8: // %=
                    {
                        check_args(2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        if (arg1)
                            m_args.back().value %= arg1;
                        m_args.back().var_ref = 0;
                        log(ll_trace) << "%= " << arg1 << '\n';
                        break;
                    }
                case 9: // +=
                    {
                        check_args(2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        m_args.back().value += arg1;
                        m_args.back().var_ref = 0;
                        log(ll_trace) << "+= " << arg1 << '\n';
                        break;
                    }
                case 0xA: // -=
                    {
                        check_args(2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        m_args.back().value -= arg1;
                        m_args.back().var_ref = 0;
                        log(ll_trace) << "-= " << arg1 << '\n';
                        break;
                    }
                case 0xB: // <
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "< (" << arg2 << ", " << arg1 << ")\n";
                        m_args.back().value = arg2 < arg1;
                        m_args.back().var_ref = 0;
                        break;
                    }
                case 0xC: // <=
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "> (" << arg2 << ", " << arg1 << ")\n";
                        m_args.back().value = arg2 <= arg1;
                        m_args.back().var_ref = 0;
                        break;
                    }
                case 0xD: // >
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "> (" << arg2 << ", " << arg1 << ")\n";
                        m_args.back().value = arg2 > arg1;
                        m_args.back().var_ref = 0;
                        break;
                    }
                case 0xE: // >=
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "< (" << arg2 << ", " << arg1 << ")\n";
                        m_args.back().value = arg2 >= arg1;
                        m_args.back().var_ref = 0;
                        break;
                    }
                case 0xF: // ==
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "EQ (" << arg1 << ", " << arg2 << ")\n";
                        m_args.back().value = arg1 == arg2;
                        m_args.back().var_ref = 0;
                        break;
                    }
                case 0x10: // !=
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "!= (" << arg1 << ", " << arg2 << ")\n";
                        m_args.back().value = arg1 != arg2;
                        m_args.back().var_ref = 0;
                        break;
                    }
                case 0x11: // &
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        m_args.back().value &= arg1;
                        m_args.back().var_ref = 0;
                        log(ll_trace) << "&= " << arg1 << '\n';
                        break;
                    }
                case 0x12: // ^
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        m_args.back().value ^= arg1;
                        m_args.back().var_ref = 0;
                        log(ll_trace) << "^= " << arg1 << '\n';
                        break;
                    }
                case 0x13:
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        m_args.back().value |= arg1;
                        m_args.back().var_ref = 0;
                        log(ll_trace) << "|= " << arg1 << '\n';
                        break;
                    }
                case 0x14: // &&
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "&& (" << arg1 << ", " << arg2 << ")\n";
                        m_args.back().value = arg1 && arg2;
                        m_args.back().var_ref = 0;
                        break;
                    }
                case 0x15: // ||
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "|| (" << arg1 << ", " << arg2 << ")\n";
                        m_args.back().value = arg1 || arg2;
                        m_args.back().var_ref = 0;
                        break;
                    }
                case 0x16:
                    {
                        check_args (2);
                        int value = m_args.back().value;
                        m_args.pop_back();
                        m_args.back().value = value;
                        auto var_ref = m_args.back().var_ref;
                        if (var_ref)
                        {
                            log(ll_cmd) << "SET_VAR[" << var_ref << "] := " << value << '\n';
                            m_vars[var_ref] = value;
                        }
                        break;
                    }
                case 0x17:
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "+ (" << arg1 << ", " << arg2 << ")\n";
                        int value = arg1 + arg2;
                        m_args.back().value = value;
                        auto var_ref = m_args.back().var_ref;
                        if (var_ref)
                        {
                            log(ll_trace) << "SET_VAR[" << m_args.back().var_ref << "] := " << value << '\n';
                            m_vars[var_ref] = value;
                        }
                        break;
                    }
                case 0x18:
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "- (" << arg1 << ", " << arg2 << ")\n";
                        int value = arg2 + arg1;
                        m_args.back().value = value;
                        auto var_ref = m_args.back().var_ref;
                        if (var_ref)
                        {
                            log(ll_trace) << "SET_VAR[" << m_args.back().var_ref << "] := " << value << '\n';
                            m_vars[var_ref] = value;
                        }
                        break;
                    }
                case 0x19:
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "* (" << arg1 << ", " << arg2 << ")\n";
                        int value = arg1 * arg2;
                        m_args.back().value = value;
                        auto var_ref = m_args.back().var_ref;
                        if (var_ref)
                        {
                            log(ll_trace) << "SET_VAR[" << m_args.back().var_ref << "] := " << value << '\n';
                            m_vars[var_ref] = value;
                        }
                        break;
                    }
                case 0x1A:
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "/ (" << arg2 << ", " << arg1 << ")\n";
                        int value = 0;
                        if (arg1)
                            value = arg2 / arg1;
                        m_args.back().value = value;
                        auto var_ref = m_args.back().var_ref;
                        if (var_ref)
                        {
                            log(ll_trace) << "SET_VAR[" << m_args.back().var_ref << "] := " << value << '\n';
                            m_vars[var_ref] = value;
                        }
                        break;
                    }
                case 0x1D:
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "| (" << arg1 << ", " << arg2 << ")\n";
                        int value = arg1 | arg2;
                        m_args.back().value = value;
                        auto var_ref = m_args.back().var_ref;
                        if (var_ref)
                        {
                            log(ll_trace) << "SET_VAR[" << m_args.back().var_ref << "] := " << value << '\n';
                            m_vars[var_ref] = value;
                        }
                        break;
                    }
                case 0x1C: // &
                    {
                        check_args (2);
                        int arg1 = m_args.back().value;
                        m_args.pop_back();
                        int arg2 = m_args.back().value;
                        log(ll_trace) << "& (" << arg1 << ", " << arg2 << ")\n";
                        int value = arg1 & arg2;
                        m_args.back().value = value;
                        auto var_ref = m_args.back().var_ref;
                        if (var_ref)
                        {
                            log(ll_trace) << "SET_VAR[" << m_args.back().var_ref << "] := " << value << '\n';
                            m_vars[var_ref] = value;
                        }
                        break;
                    }
                case 0x1B: // %
                case 0x1E: // ^
                    std::cerr << "operation " << hex(opCode) << " not implemented\n";
                    throw_error (pBytecode-1, "operation not implemented");
                default:
                    continue;
                }
                break;
            }
        case 0x90:
            {
                int var_ptr = get<int16_t> (pBytecode);
                pBytecode += 2;
                m_args.push_back (argument (var_ptr, get_var (var_ptr)));
                log(ll_trace) << "PUSH_VAR[" << var_ptr << "] -> " << m_args.back().value << '\n';
                break;
            }
        case 0xA0:
            {
                int var_ptr = get<int16_t> (pBytecode);
                pBytecode += 2;
                int var_ref = get_var (var_ptr);
                m_args.push_back (argument (var_ref, get_var (var_ref)));
                log(ll_trace) << "PUSH_VAR_REF[" << var_ptr << " -> " << var_ref << "] -> " << m_args.back().value << '\n';
                break;
            }
        default:
            {
                int arg;
                if (argCode & 2)
                {
                    arg = static_cast<int8_t> (get_byte());
                }
                else
                {
                    arg = static_cast<int16_t> (get_word());
                }
                m_args.push_back (argument (arg));
            }
            break;
        }
    }
    while (argCode & 0x40);
    if (m_args.empty())
        throw_error (currentPos, "argument list is empty");
    return m_args.back().value;
}

void adviz_reader::
skip_args ()
{
    uint8_t opCode;
    do
    {
        opCode = get<uint8_t> (pBytecode);
        pBytecode += 2;
        if ((opCode & 0xCF) != 0x84 && (opCode & 2) == 0)
            ++pBytecode;
    }
    while (opCode & 0x40);
}

void adviz_reader::
opcode_34 ()
{
    log(ll_trace) << put_offset(pBytecode-1) << "OPCODE_34\n";
    while (get<uint8_t> (pBytecode) != 0x35)
    {
        skip_args();
    }
}

void adviz_reader::
opcode_4A ()
{
    if (m_version != version::w2) // this is also for WADVIZ 1.XX
    {
        int arg = get_arg();
        log(ll_cmd) << "WRITE " << "SAVE" << arg << ".DAT\n";
    }
    else // XXX this is for WADVIZ 2.XX
    {
        buffer.clear();
        auto key = pBytecode[-1];
        uint8_t chr;
        do
        {
            chr = get_byte();
            chr ^= key;
            key += chr;
            if (chr)
                buffer.push_back (chr);
        }
        while (chr);
        log(ll_cmd) << "PROMPT ";
        log(ll_cmd).write (buffer.data(), buffer.size()).put ('\n');
    }
}

void adviz_reader::
file_op (char bytecode)
{
    const auto curPos = pBytecode-1;
    size_t arg1 = get_arg();
    int    arg2 = get_arg();
    switch (bytecode)
    {
    case 'p':
        log(ll_cmd) << "LOAD_SCRIPT ";
        if (arg1 < m_text_table.size())
            log(ll_cmd) << '"' << m_text_table[arg1] << '"';
        else
            log(ll_cmd) << arg1;
        log(ll_cmd) << '\n';
        break;
    case 'q':
        log(ll_cmd) << put_offset(curPos) << "LOAD_IMAGE ";
        if (arg1 && arg1 < m_grp_table.size())
            log(ll_cmd) << '"' << m_grp_table[arg1] << '"';
        else
            log(ll_cmd) << arg1;
        log(ll_cmd) << ", " << arg2 << '\n';
        break;
    case 'r':
        log(ll_cmd) << put_offset(curPos) << "LOAD_ANIM ";
        if (arg1 && arg1 < m_anm_table.size())
            log(ll_cmd) << '"' << m_anm_table[arg1] << '"';
        else
            log(ll_cmd) << arg1;
        log(ll_cmd) << ", " << arg2 << '\n';
        break;
        break;
    case 's':
        log(ll_cmd) << "LOAD_MUSIC" << ' ' << arg1 << ", " << arg2 << '\n';
        break;
    case 't':
        log(ll_cmd) << "LOAD_AUDIO" << ' ' << arg1 << '\n';
        break;
    }
}

void adviz_reader::
parse_table (const std::wstring& filename, std::vector<std::string>& table)
{
    table.clear();
    sys::mapping::readonly file (filename);
    sys::mapping::const_view<char> tbl (file);
    auto ptr = tbl.begin();
    if (0x180 == *reinterpret_cast<const uint16_t*> (ptr))
        ptr += 2;
    std::string name;
    while (ptr + 12 <= tbl.end())
    {
        name.clear();
        int i;
        for (i = 0; i < 8; ++i)
        {
            if (' ' == ptr[i])
                break;
        }
        name.append (ptr, ptr + i);
        name.push_back ('.');
        for (i = 8; i < 11; ++i)
        {
            if (' ' == ptr[i])
                break;
        }
        name.append (ptr+8, ptr+i);
        ptr += 12;
        table.push_back (name);
    }
}

void adviz_reader::
set_text_table (const std::wstring& filename)
{
    parse_table (filename, m_text_table);
}

void adviz_reader::
set_grp_table (const std::wstring& filename)
{
    parse_table (filename, m_grp_table);
}

void adviz_reader::
set_anm_table (const std::wstring& filename)
{
    parse_table (filename, m_anm_table);
}

void adviz_reader::
put_string (uint8_t key)
{
    buffer.clear();
    uint8_t chr;
    do
    {
        chr = get_byte();
        if (m_version != version::pc98)
        {
            chr ^= key;
            key += chr;
        }
        if (chr)
            buffer.push_back (static_cast<char> (chr));
    }
    while (chr);
    process_text();
}

void adviz_reader::
process_text ()
{
    log(ll_text).write (buffer.data(), buffer.size()).put ('\n');
}

void adviz_reader::
prs_unpack (sys::mapping::view<uint8_t>& output)
{
    size_t dst = 0;
    uint16_t mask = 0;
    uint16_t ctl = 0;
    while (pBytecode < view.end() && dst < output.size())
    {
        mask >>= 1;
        if (!mask)
        {
            ctl = get_word();
            mask = 0x8000;
        }
        if (!(ctl & mask))
        {
            output[dst++] = get_byte();
        }
        else
        {
            uint8_t byte = get_byte();
            size_t count, off;
            if (byte & 1)
            {
                byte >>= 1;
                count = (byte >> 4) + 2;
                off = (byte & 0xF) + 1;
            }
            else
            {
                size_t ax = get_byte() << 7 | byte >> 1;
                count = ((ax >> 10) & 0x3F) + 2;
                off = (ax & 0x3FF) + 1;
            }
            if (off >= dst)
                throw std::runtime_error ("invalid compressed file");
            count = std::min (count, output.size() - dst);
            while (count --> 0)
            {
                output[dst] = output[dst-off];
                ++dst;
            }
        }
    }
}

int wmain (int argc, wchar_t* argv[])
{
    logging log_level = ll_cmd;
    int argn = 1;
    int last_arg = argc - 1;
    std::wstring version;
    std::wstring anm_tbl, grp_tbl, text_tbl;
    while (argn < last_arg)
    {
        if (0 == std::wcscmp (argv[argn], L"-v"))
        {
            log_level = ll_trace;
        }
        else if (0 == std::wcscmp (argv[argn], L"-p"))
        {
            ++argn;
            if (argn >= last_arg)
            {
                argn = argc;
                break;
            }
            version = argv[argn];
        }
        else if (0 == std::wcscmp (argv[argn], L"-a"))
        {
            ++argn;
            if (argn >= last_arg)
            {
                argn = argc;
                break;
            }
            anm_tbl = argv[argn];
        }
        else if (0 == std::wcscmp (argv[argn], L"-g"))
        {
            ++argn;
            if (argn >= last_arg)
            {
                argn = argc;
                break;
            }
            grp_tbl = argv[argn];
        }
        else if (0 == std::wcscmp (argv[argn], L"-t"))
        {
            ++argn;
            if (argn >= last_arg)
            {
                argn = argc;
                break;
            }
            text_tbl = argv[argn];
        }
        else
            break;
        ++argn;
    }
    if (argn >= argc)
    {
        std::cout << "usage: adviz [-v] ... SCRIPT.ADV\n"
                     "    -v            verbose output\n"
                     "    -p p|w|w2     set interpreter version (PC-98/Wadviz/Wadviz2)\n"
                     "                  default is Wadviz\n"
                     "  also, optional tables may be specified:\n"
                     // scripts have to be properly executed to really make use of these
                     "    -a ANM_TBL.SYS\n";
                     "    -g GRP_TBL.SYS\n";
                     "    -t TEXT_TBL.SYS\n";
        return 0;
    }
    try
    {
        sys::mapping::readonly in (argv[argn]);
        adviz_reader reader (in);
        if (!grp_tbl.empty())
            reader.set_grp_table (grp_tbl);
        if (!text_tbl.empty())
            reader.set_text_table (text_tbl);
        if (!anm_tbl.empty())
            reader.set_anm_table (anm_tbl);
        if (!version.empty())
        {
            switch (std::toupper (version[0]))
            {
            case 'P':
                reader.set_version (adviz_reader::version::pc98);
                break;
            case 'W':
                if (version.size() == 1 || version[1] == '1')
                {
                    reader.set_version (adviz_reader::version::w1);
                    break;
                }
                else if (version.size() > 1 && version[1] == '2')
                {
                    reader.set_version (adviz_reader::version::w2);
                    break;
                }
                /* FALL THROUGH */
            default:
                std::fprintf (stderr, "%S: unknown script version specified\n", version.c_str());
                return 1;
            }
        }
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
