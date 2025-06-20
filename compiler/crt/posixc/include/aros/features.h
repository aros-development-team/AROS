#ifndef _POSIXC_FEATURES_H
#define _POSIXC_FEATURES_H
/*
    Copyright © 2020-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * POSIXC features configuration header.
 * Largely modeled after glibc's features.h behavior.
 */

 /* --- Feature Cleanup --- */

#ifdef __USE_ISOC99
#undef __USE_ISOC99
#endif
#ifdef __USE_XOPEN
#undef __USE_XOPEN
#endif
#ifdef __USE_XOPEN_EXTENDED
#undef __USE_XOPEN_EXTENDED
#endif
#ifdef __USE_UNIX98
#undef __USE_UNIX98
#endif
#ifdef __USE_XOPEN2K
#undef __USE_XOPEN2K
#endif
#ifdef __USE_XOPEN2K8
#undef __USE_XOPEN2K8
#endif
#ifdef __USE_LARGEFILE
#undef __USE_LARGEFILE
#endif
#ifdef __USE_LARGEFILE64
#undef __USE_LARGEFILE64
#endif
#ifdef __USE_FILE_OFFSET64
#undef __USE_FILE_OFFSET64
#endif
#ifdef __USE_SYSV
#undef __USE_SYSV
#endif
#ifdef __USE_BSD
#undef __USE_BSD
#endif
#ifdef __USE_NIXCOMMON
#undef __USE_NIXCOMMON
#endif
#ifdef __USE_MISC
#undef __USE_MISC
#endif

/*
 Supported feature-test macros and their effects:

 __STRICT_ANSI__              Enable strict ISO C compliance (disables all extensions).

 _ISOC99_SOURCE               Enable ISO C99 extensions to ISO C89.

 _POSIX_SOURCE                Enable IEEE Std 1003.1 (POSIX.1-1990).
 _POSIX_C_SOURCE              Set POSIX version level:
                                1           = POSIX.1-1990
                                2           = POSIX.2-1992
                                199309L     = POSIX.1b-1993 (realtime extensions)
                                199506L     = POSIX.1c-1995 (pthreads)
                                200112L     = POSIX.1-2001 (includes .1b and .1c)
                                200809L     = POSIX.1-2008

 _XOPEN_SOURCE                Enable X/Open and POSIX features:
                                500         = SUSv2 (POSIX.1-1996)
                                600         = SUSv3 (POSIX.1-2001)
                                700         = SUSv4 (POSIX.1-2008)
 _XOPEN_SOURCE_EXTENDED       Further enable X/Open extensions beyond _XOPEN_SOURCE.

 _SVID_SOURCE                 Enable System V (SVID) extensions.
 _BSD_SOURCE                  Enable BSD 4.3 extensions.
                              (Deprecated: use _DEFAULT_SOURCE instead.)

 _LARGEFILE_SOURCE            Enable support for large files (e.g., `fopen64()`).
 _LARGEFILE64_SOURCE          Expose 64-bit file offset functions explicitly (e.g., `fseeko64()`).
 _FILE_OFFSET_BITS=N          Set default file offset size:
                                32 or 64     (e.g., 64 makes `off_t` 64-bit by default)

 _GNU_SOURCE                  Enable all of the above features, plus GNU extensions.
*/

/* __BSD_VISIBLE as alias for _BSD_SOURCE */
#if defined(__BSD_VISIBLE)
# define _BSD_SOURCE 1
#endif

/* Handle _GNU_SOURCE implying all */
#if defined(_GNU_SOURCE)
# undef _POSIX_SOURCE
# define _POSIX_SOURCE 1
# undef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200809L
# undef _XOPEN_SOURCE
# define _XOPEN_SOURCE 700
# undef _XOPEN_SOURCE_EXTENDED
# define _XOPEN_SOURCE_EXTENDED 1
# undef _LARGEFILE64_SOURCE
# define _LARGEFILE64_SOURCE 1
# undef _ISOC99_SOURCE
# define _ISOC99_SOURCE 1
# undef _SVID_SOURCE
# define _SVID_SOURCE 1
# undef _BSD_SOURCE
# define _BSD_SOURCE 1
# undef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE 1
#endif

/* Fallback defaults */
#if (!defined(__STRICT_ANSI__) && \
     !defined(_ISOC99_SOURCE) && \
     !defined(_POSIX_SOURCE) && \
     !defined(_POSIX_C_SOURCE) && \
     !defined(_XOPEN_SOURCE) && \
     !defined(_XOPEN_SOURCE_EXTENDED) && \
     !defined(_BSD_SOURCE) && \
     !defined(_DEFAULT_SOURCE))
# define _XOPEN_SOURCE 500
# define _BSD_SOURCE 1
#endif

/* Enable features based on _POSIX_C_SOURCE level */
#if defined(_POSIX_C_SOURCE)
# if (_POSIX_C_SOURCE >= 200112L)
#  define __USE_XOPEN2K 1
#  define __USE_ISOC99 1
# endif
# if (_POSIX_C_SOURCE >= 200809L)
#  define __USE_XOPEN2K8 1
# endif
#endif

/* Enable XOPEN support */
#ifdef _XOPEN_SOURCE
# define __USE_XOPEN 1
# if (_XOPEN_SOURCE >= 500)
#  define __USE_XOPEN_EXTENDED 1
#  define __USE_UNIX98 1
#  define _LARGEFILE_SOURCE 1
#  if (_XOPEN_SOURCE >= 600)
#   define __USE_XOPEN2K 1
#   define __USE_ISOC99 1
#   if (_XOPEN_SOURCE >= 700)
#    define __USE_XOPEN2K8 1
#   endif
#  endif
# else
#  if defined(_XOPEN_SOURCE_EXTENDED)
#   define __USE_XOPEN_EXTENDED 1
#  endif
# endif
#endif

/* SVID and BSD extensions */
#if defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || defined(_DEFAULT_SOURCE)
# define __USE_NIXCOMMON 1
# define __USE_MISC      1
#endif

#if defined(_BSD_SOURCE)
# define __USE_BSD 1
#endif

#if defined(_SVID_SOURCE)
# define __USE_SYSV 1
#endif

/* Large file interface flags */
#if defined(_LARGEFILE_SOURCE)
# define __USE_LARGEFILE 1
#endif

#if defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64
# define __USE_FILE_OFFSET64 1
#endif

#if defined(__USE_FILE_OFFSET64) || defined(_LARGEFILE64_SOURCE)
# define __USE_LARGEFILE64 1
#endif

#if (!defined(_XOPEN_SOURCE) && !defined(_POSIX_SOURCE) && !defined(_BSD_SOURCE))
# define NO_POSIX_WRAPPERS
#endif

/* --- Function Mangling Support --- */
#if (1)
// We explicitly enable this for now, to prevent symbol collisions that happen
// on nightly builds between posixc and stdcio
#define POSIXC_MANGLE_FUNCS
#endif

#ifndef POSIXC_FN_PREFIX
#  define POSIXC_FN_PREFIX __posixc_
#endif

#define POSIXC_FNAM_CONCAT(a, b) a##b
#define POSIXC_FNAM_EVAL(a, b) POSIXC_FNAM_CONCAT(a, b)

// Stringify helper
#include <aros/strmacro.h>

#if defined(AROS_POSIXC_BUILD) || !defined(POSIXC_MANGLE_FUNCS)
  // No mangling
#  define POSIXCFUNC(ret_type, name, args) ret_type name args
#elif defined(__GNUC__) || defined(__clang__)
  // Mangled symbol name
#  define POSIXCFUNC(ret_type, name, args) \
     ret_type name args __asm__(__AROS_STR(POSIXC_FNAM_EVAL(POSIXC_FN_PREFIX, name)))
#else
#  error "POSIXC_MANGLE_FUNCS requires compiler support for __asm__ symbol renaming"
#endif

#endif /* _POSIXC_FEATURES_H */
