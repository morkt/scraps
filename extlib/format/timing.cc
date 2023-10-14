/* -*- C++ -*-
 * File        : timing.cc
 * Created     : Mon Jul 16 16:48:20 2007
 * Description : timing benchmark of format implementation.
 *
 * $Id$
 */

#define EXT_USE_STD_STRING
#include "format.hpp"
#include "timer.hpp"
#include <boost/timer/timer.hpp>
#include <boost/cstdint.hpp>
#include <boost/format.hpp>
#include <limits>
#include <iostream>
#include <iomanip>

#ifdef _MSC_VER
#define COMPILER_NAME	"Microsoft C/C++ Compiler"
#define COMPILER_VER	_MSC_VER
#elif defined(__GNUC__)
#define COMPILER_NAME	"GNU C/C++ Compiler"
#define COMPILER_VER	__VERSION__
#else
#define COMPILER_NAME	"Unknown C/C++ Compiler"
#define COMPILER_VER	"(unknown version)"
#endif

const int DEFAULT_ITERATIONS = 70000;

class nullbuf : public std::streambuf
{
    typedef char				char_type;
    typedef std::char_traits<char_type>		traits_type;
    typedef traits_type::int_type 		int_type;

    virtual int_type overflow (int_type c) { return c; }
    virtual std::streamsize xsputn (const char_type*, std::streamsize size)
	{ return size; }
};

struct stream_redirect
{
    stream_redirect (std::ios& stream, std::streambuf* target)
       	: m_ios (stream), m_rdbuf (m_ios.rdbuf (target))
	{ }
    ~stream_redirect ()
	{ m_ios.rdbuf (m_rdbuf); }

private:
    std::ios&		m_ios;
    std::streambuf*	m_rdbuf;
};

typedef boost::timer::cpu_times times_type;
//typedef double times_type;

template <typename Func>
times_type test_format (Func test_func)
{
    nullbuf null;
    stream_redirect redir (std::cout, &null);

//    sys::timer t;
    boost::timer::cpu_timer t;
    for (int i = 0; i < DEFAULT_ITERATIONS; ++i)
	test_func();

    return t.elapsed();
}

template <typename Func>
times_type test_printf (Func test_func)
{
    freopen ("NUL", "wb", stdout);

//    sys::timer t;
    boost::timer::cpu_timer t;
    for (int i = 0; i < DEFAULT_ITERATIONS; ++i)
	test_func();

    return t.elapsed();
}

const char desc[] = "Description text";
double x = 1.23456, y = 23.45678, z = 345.6789;

void test_long_stream ()
{
    std::cout
       	<< std::setiosflags(std::ios::left) << std::setw(20) << desc
	<< std::resetiosflags(std::ios::left)
	<< ' ' << std::setprecision(5) << std::setw(10) << x
	<< ' ' << std::setw(10) << y
	<< ' ' << std::setw(10) << z << '\n';
}

void test_long_boost ()
{
    std::cout << boost::format("%-20s %10.5g %10.5g %10.5g\n") %desc %x %y %z;
}

void test_long_format ()
{
    std::cout << ext::format("%-20s %10.5g %10.5g %10.5g\n") %desc %x %y %z;
}

void test_long_printf ()
{
    fprintf (stdout, "%-20s %10.5g %10.5g %10.5g\n", desc, x, y, z);
}

void test_dummy_format ()
{
    ext::format f("%-20s %10.5g %10.5g %10.5g\n");
    f %desc %x %y %z;
    f.str();
}

std::string base_name (std::string name)
{
    size_t pos = name.find_last_of ("/\\");
    if (pos != std::string::npos)
        name.erase (0, pos+1);
    pos = name.find_last_of ('.');
    if (pos != std::string::npos)
        name.erase (pos);
    return name;
}

double convert_times (double x) { return x; }
double convert_times (const boost::timer::cpu_times& t)
{
    double total = t.user + t.system;
    total /= 1e9;
    return total;
}

int main (int argc, char* argv[])
try
{
    std::clog << base_name (argv[0]) << ": " << COMPILER_NAME << " version " << COMPILER_VER << '\n';

//    double t2 = test_format (&test_dummy_format);
//    double t4 = convert_times (test_format (&test_long_stream));
    double t2 = convert_times (test_format (&test_long_format));
//    double t3 = convert_times (test_format (&test_long_boost));
//    double t1 = convert_times (test_printf (&test_long_printf));
//    std::clog << ext::format("%-20s %.3g s\n") %"test_long_printf" %t1;
//    std::clog << ext::format("%-20s %.3g s\n") %"test_long_stream" %t4;
    std::clog << ext::format("%-20s %.3g s\n") %"test_long_format" %t2;
//    std::clog << ext::format("%-20s %.3g s\n") %"test_long_boost"  %t3;
    return EXIT_SUCCESS;
}
catch (std::exception& X)
{
    std::cerr << '(' << typeid(X).name() << "): " << X.what() << '\n';
    return EXIT_FAILURE;
}
