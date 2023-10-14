// -*- C++ -*-
//! \file       sysdef.h
//! \date       Mon Nov 01 01:12:02 2010
//! \brief      compiler-dependent macro definitions.
//

#ifndef SYSPP_DEF_H
#define SYSPP_DEF_H

#if defined(_WIN32)
#   define SYSPP_WIN32 1
#else
#   define SYSPP_WIN32 0
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
#   if defined(SYSPP_BUILD_DLL)
#	define SYSPP_DLLIMPORT		__declspec(dllexport)
#   elif defined(SYSPP_DLL)
#	define SYSPP_DLLIMPORT		__declspec(dllimport)
#   else
#	define SYSPP_DLLIMPORT
#   endif
#else
#   define SYSPP_DLLIMPORT
#endif

#if defined(__GNUC__) /* GNU C++ */
#   define SYSPP_GNUC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#   define SYSPP_GNUC 0
#endif

#if defined(_MSC_VER) /* Microsoft Visual C++ */
#   define SYSPP_MSC _MSC_VER
#else
#   define SYSPP_MSC 0
#endif

#if defined(__clang__) /* LLVM/Clang */
#   define SYSPP_CLANG (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#else
#   define SYSPP_CLANG 0
#endif

#if defined(__INTEL_COMPILER) /* Interl C/C++ */
#   define SYSPP_INTELC __INTEL_COMPILER
#else
#   define SYSPP_INTELC 0
#endif

#if defined(__xlC__) /* IBM XL C++ */
#   define SYSPP_IBMCPP __xlC__
#else
#   define SYSPP_IBMCPP 0
#endif

#if SYSPP_GNUC >= 40600 || SYSPP_IBMCPP >= 0x1201 || SYSPP_INTELC >= 1300 || SYSPP_CLANG >= 30100
#   define SYSPP_constexpr constexpr
#   define SYSPP_static_constexpr static constexpr
#   define SYSPP_noexcept noexcept
#else
#   define SYSPP_constexpr
#   define SYSPP_static_constexpr static const
#   define SYSPP_noexcept throw()
#endif

#if SYSPP_GNUC >= 40600 || SYSPP_MSC >= 1600 || SYSPP_INTELC >= 1210 || SYSPP_CLANG >= 20900
#   define SYSPP_nullptr nullptr
#else
#   define SYSPP_nullptr 0
#endif

#endif /* SYSPP_DEF_H */
