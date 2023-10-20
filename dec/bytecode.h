// -*- C++ -*-
//! \file       bytecode.h
//! \date       2023 Sep 29
//! \brief      base class for bytecode decompilers.
//

#ifndef BYTECODE_H
#define BYTECODE_H

#include <sstream>
#include <iomanip>
#include <iostream>
#include "sysmemmap.h"

namespace detail {
    class bytecode_offset_printer
    {
        size_t  m_offset;
        int     m_width;

    public:
        explicit bytecode_offset_printer (size_t offset, int width = 8)
            : m_offset (offset), m_width (width) { }

        std::ostream& put (std::ostream& out) const
        {
            return out << std::hex << std::uppercase << std::setw(m_width) << std::setfill('0') << m_offset << std::dec << ':';
        }
    };

    class hex_number_printer
    {
        unsigned    num;

    public:
        explicit hex_number_printer (unsigned n) : num (n) { }

        std::ostream& put (std::ostream& out) const
        {
            out << std::hex << std::setfill('0') << std::uppercase;
            if (num > 0xFFFF)
                out << std::setw(8);
            else if (num > 0xFF)
                out << std::setw(4);
            else
                out << std::setw(2);
            return out << num << std::dec;
        }
    };

    inline std::ostream& operator<< (std::ostream& out, const bytecode_offset_printer& b)
    {
        return b.put (out);
    }

    inline std::ostream& operator<< (std::ostream& out, const hex_number_printer& h)
    {
        return h.put (out);
    }

    class null_stream : public std::ostream
    {
        struct null_buffer : std::streambuf
        {
            int overflow (int c) override { return c; }
        };

        null_buffer m_sb;

        null_stream () : std::ostream (&m_sb) { }

    public:
        static std::ostream& instance ()
        {
            static null_stream s_null;
            return s_null;
        }
    };
}

class bytecode_error : public std::runtime_error
{
    size_t  pos;

public:
    bytecode_error (size_t pos, const char* message) : std::runtime_error (message), pos (pos)
    {
    }

    bytecode_error (size_t pos, const std::string& message) : std::runtime_error (message), pos (pos)
    {
    }

    bytecode_error (size_t pos, uint16_t bytecode) : std::runtime_error (format_message (bytecode)), pos (pos)
    {
    }

    size_t get_pos () const { return pos; }

private:
    static std::string format_message (uint16_t bytecode)
    {
        std::ostringstream err;
        err << "unknown bytecode " << detail::hex_number_printer(bytecode);
        return err.str();
    }
};

enum logging
{
    ll_trace,
    ll_debug,
    ll_cmd,
    ll_text,
    ll_alert,
    ll_none
};

class bytecode_reader
{
protected:
    sys::mapping::const_view<uint8_t> view;
    const uint8_t*              pBytecodeStart;
    const uint8_t*              pBytecode;
    logging                     m_log_level;

protected:
    bytecode_reader ()
        : pBytecodeStart (nullptr)
        , m_log_level (ll_debug)
    { }

    bytecode_reader (sys::mapping::map_base& in)
        : view (in)
        , pBytecodeStart (view.data())
        , m_log_level (ll_debug)
    { }

public:
    bool run ()
    {
        pBytecode = pBytecodeStart = init();
        return do_run();
    }

    void init (sys::mapping::map_base& in)
    {
        view.remap (in);
    }

    logging set_log_level (logging level)
    {
        auto prior = m_log_level;
        m_log_level = level;
        return prior;
    }

    virtual bool is_valid () const
    {
        return view.size();
    }

protected:
    virtual bool do_run () = 0;

    virtual const uint8_t* init ()
    {
        if (!is_valid())
            throw std::runtime_error ("no valid bytecode");
        return start();
    }

    virtual const uint8_t* start () const
    {
        return view.data();
    }

    std::ostream& log (logging level) const
    {
        if (ll_alert == level)
            return std::cerr;
        if (level >= m_log_level)
            return std::cout;
        else
            return detail::null_stream::instance();
    }

    template <typename T>
    T get (const uint8_t* ptr) const
    {
        if (ptr + sizeof(T) > view.end())
            throw bytecode_error (pBytecode - view.data(), "Failed attempt to access data out of script bounds");
        return *reinterpret_cast<const T*> (ptr);
    }

    static detail::hex_number_printer hex (unsigned n) { return detail::hex_number_printer (n); }

    detail::bytecode_offset_printer put_offset (const uint8_t* ptr, int width = 4) const
    {
        return detail::bytecode_offset_printer (ptr - view.data(), width);
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

    void throw_error (const uint8_t* pos, const char* message) const
    {
        throw bytecode_error (pos - view.data(), message);
    }

    void throw_error (const uint8_t* pos, uint16_t bytecode) const
    {
        throw bytecode_error (pos - view.data(), bytecode);
    }
};

#endif /* BYTECODE_H */
