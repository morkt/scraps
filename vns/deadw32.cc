// -*- C++ -*-
//! \file       deadw32.cc
//! \date       2022 Apr 10
//! \brief      extract script from ADVWin32 engine *.MES file
//

#include <cstdio>
#include <string>
#include "sysmemmap.h"

class adv_decompiler
{
    sys::mapping::const_view<uint8_t>   m_view;
    size_t      m_pos;
    size_t      m_end;
    FILE*       m_output;

public:
    adv_decompiler (sys::mapping::readonly& in, FILE* out) : m_view (in), m_output (out)
    {
        if (in.size() < 0x20)
            throw std::runtime_error ("invalid input format");

        if (0 != std::memcmp (m_view.data(), "ADVWin32 1.00", 13))
            throw std::runtime_error ("invalid input format");

        size_t file_size = *reinterpret_cast<const uint32_t*> (&m_view[0x14]);
        size_t data_pos  = *reinterpret_cast<const uint32_t*> (&m_view[0x18]);
        if (file_size != m_view.size() || data_pos < 0x20 || data_pos >= file_size)
            throw std::runtime_error ("invalid input format");

        m_pos = data_pos;
        m_end = file_size;
    }

    void run ()
    {
        while (m_pos < m_end)
        {
            auto op = get_byte();
            while (0x12 == op)
                op = get_byte();
            if ((op < 4 || op > 5) && (op < 0x15 || op > 0x72u))
            {
                switch (op)
                {
                case 0x03:
                case 0x13:
                case 0x14:
                    process_cmd (op);
                    break;
                case 0x11:
                    process_op_11();
                    break;
                default:
                    std::fprintf (m_output, "CMD_%02X\n", op);
                    break;
                }
                if (2 == op)
                    break;
            }
            else
            {
                std::fprintf (m_output, "CMD_%02X:\n", op);
//                if (op <= 5)
//                    --m_pos;
                parse_params();
            }
        }
    }

private:
    void parse_params ()
    {
        size_t off = 0;
        while (m_pos + off < m_end)
        {
            auto param_code = m_view[m_pos + off];
            if (param_code <= 3 || param_code >= 0x10 && param_code <= 0x72)
                break;

            switch (param_code)
            {
            case 0x04: case 0x05:
                process_param_04_05 (param_code, off);
                return;

            case 0x13:
                process_param_13 (off);
                break;

            case 0xE2: case 0xE3: case 0xE4:
                process_param_E2_E4 (off);
                break;

            case 0xE5:
                process_param_E5 (off);
                break;

            case 0xE6: case 0xE7: case 0xE8:
                process_param_E6_E8 (off);
                break;

            default:
                std::fprintf (m_output, "PARAM_%02X\n", param_code);
                off += 1;
//                throw std::runtime_error ("^invalid opcode");
            }
            auto next_op = m_view[m_pos + off];
            if (next_op > 3 && (next_op < 0x10 || next_op > 0x72) && next_op != 4 && next_op != 5)
            {
                // next_param = allocate another parameter
            }
            // current_param = next_param
        }
        m_pos += off;
    }

    void process_cmd (uint8_t op)
    {
        std::fprintf (m_output, "CMD_%02X: ", op);
        parse_params();
    }

    void process_op_11 ()
    {
        auto str = get_string();
//        std::fprintf (m_output, "TEXT %s\n", str.c_str());
        std::fprintf (m_output, "%s\n", str.c_str());
    }

    void process_param_04_05 (uint8_t code, size_t& off)
    {
        int type = code + 1;
        std::fprintf (m_output, "PARAM<%d>\n", type);
    }

    void process_param_13 (size_t& off)
    {
        throw std::runtime_error ("op_13 not implemented");
    }

    void process_param_E2_E4 (size_t& off)
    {
        auto op = m_view[m_pos + off];
        std::fputs ("PARAM_", m_output);
        if (0xE2 == op)
        {
            std::fprintf (m_output, "BYTE 0x%02X", m_view[m_pos + off + 1]);
            off += 2;
        }
        else if (0xE3 == op)
        {
            std::fprintf (m_output, "WORD 0x%04X", *reinterpret_cast<const uint16_t*> (&m_view[m_pos + off + 1]));
            off += 3;
        }
        else 
        {
            std::fprintf (m_output, "DWORD 0x%08X", *reinterpret_cast<const uint32_t*> (&m_view[m_pos + off + 1]));
            off += 5;
        }
        std::fputc ('\n', m_output);
    }

    void process_param_E5 (size_t& off)
    {
        size_t start = m_pos + off + 1;
        size_t pos = start;
        while (pos < m_end && m_view[pos])
            ++pos;
        std::fputs ("STR \"", m_output);
        std::fwrite (&m_view[start], 1, pos - start, m_output);
        std::fputs ("\"\n", m_output);
        off += pos - start + 2;
    }

    void process_param_E6_E8 (size_t& off)
    {
        auto op = m_view[m_pos + off];
        int type;
        switch (op)
        {
        case 0xE6:  type = 2; break;
        case 0xE7:  type = 1; break;
        default:    type = 8; break;
        }
        auto value = *reinterpret_cast<const uint16_t*> (&m_view[m_pos + off + 1]);
        off += 3;
        std::fprintf (m_output, "PARAM<%d> 0x%04X\n", type, value);
    }

    std::string get_string ()
    {
        size_t start = m_pos;
        while (m_pos < m_end && m_view[m_pos])
            m_pos++;
        std::string value (reinterpret_cast<const char*> (&m_view[start]), m_pos - start);
        ++m_pos;
        return value;
    }

    uint8_t get_byte ()
    {
        if (m_pos >= m_end)
            throw std::runtime_error ("premature end of file");
        return m_view[m_pos++];
    }

    uint16_t get_word ()
    {
        if (m_pos+1 >= m_end)
            throw std::runtime_error ("premature end of file");
        uint16_t val = *reinterpret_cast<const uint16_t*> (&m_view[m_pos]);
        m_pos += 2;
        return val;
    }

    uint32_t get_dword ()
    {
        if (m_pos+3 >= m_end)
            throw std::runtime_error ("premature end of file");
        uint32_t val = *reinterpret_cast<const uint32_t*> (&m_view[m_pos]);
        m_pos += 4;
        return val;
    }

    void report_error (const char* msg)
    {
        std::fprintf (stderr, ">%04X\n", m_pos);
        throw std::runtime_error (msg);
    }
};

int wmain (int argc, wchar_t* argv[])
try
{
    if (argc < 3)
    {
        std::puts ("usage: deadw32 INPUT OUTPUT");
        return 0;
    }
    sys::mapping::readonly in (argv[1]);
    FILE* out = _wfopen (argv[2], L"w");
    adv_decompiler dec (in, out);
    dec.run();
    std::fclose (out);

    return 0;
}
catch (std::exception& X)
{
    std::fprintf (stderr, "%S: %s\n", argv[1], X.what());
    return 1;
}
