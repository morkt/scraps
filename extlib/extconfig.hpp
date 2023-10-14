/* -*- C++ -*-
 * File:        extconfig.hpp
 * Created:     Sat Jul 21 02:42:27 2007
 * Description: compile-time configuration for the Ext library
 *
 * $Id$
 */

#ifndef EXTCONFIG_HPP
#define EXTCONFIG_HPP

//#if defined _MT || defined _MULTI_THREAD
//#define EXT_MT
//#endif

#if !defined _MSC_VER || defined _NATIVE_WCHAR_T_DEFINED
#define EXT_NATIVE_WCHAR_T 1
#endif

#if   defined __GNUC__
#define EXT_TLS		__thread
#elif defined _MSC_VER
#define EXT_TLS		declspec(thread)
#else
#define EXT_TLS
#endif

#endif /* EXTCONFIG_HPP */
