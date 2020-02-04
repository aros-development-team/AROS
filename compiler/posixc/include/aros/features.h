#ifndef _POSIXC_FEATURES_H
#define _POSIXC_FEATURES_H
/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * Set features depending on which standards have been requested.
 * posixc headers use mostly the same flags as glibc for compatibility */

/* __USE_ISOC99
 * expose definitions and prototypes for ISO C99. */
#ifdef __USE_ISOC99
#undef __USE_ISOC99
#endif
/* __USE_XOPEN
 * expose definitions and prototypes for X/Open Portability Guide. */
#ifdef __USE_XOPEN
#undef __USE_XOPEN
#endif
/* __USE_XOPEN_EXTENDED
 * expose definitions and prototypes for X/Open Unix. */
#ifdef __USE_XOPEN_EXTENDED
#undef __USE_XOPEN_EXTENDED
#endif
/* __USE_UNIX98
 * expose definitions and prototypes for Single Unix V2. */
#ifdef __USE_UNIX98
#undef __USE_UNIX98
#endif
/* __USE_XOPEN2K
 *  expose definitions and prototypes for X/Open Portability Guide 6. */
#ifdef __USE_XOPEN2K
#undef __USE_XOPEN2K
#endif
/* __USE_XOPEN2K8
 * expose definitions and prototypes for X/Open Portability Guide 7. */
#ifdef __USE_XOPEN2K8
#undef __USE_XOPEN2K8
#endif
/* __USE_LARGEFILE
 * expose definitions and prototypes for for large file acces. */
#ifdef __USE_LARGEFILE
#undef __USE_LARGEFILE
#endif
/* __USE_LARGEFILE64
 * expose definitions and prototypes for large file access using 64bit names. */
#ifdef __USE_LARGEFILE64
#undef __USE_LARGEFILE64
#endif
/* __USE_FILE_OFFSET64
 * expose definitions and prototypes to use 64bit io interfaces as default. */
#ifdef __USE_FILE_OFFSET64
#undef __USE_FILE_OFFSET64
#endif
/* __USE_SYSV
 * expose definitions and prototypes for System V Unix */
#ifdef __USE_SYSV
#undef __USE_SYSV
#endif
/* __USE_BSD
 * expose definitions and prototypes for BSD-4.3 */
#ifdef __USE_BSD
#undef __USE_BSD
#endif
/* __USE_NIXCOMMON
 * expose definitions and prototypes common to BSD and System V Unix. */
#ifdef __USE_NIXCOMMON
#undef __USE_NIXCOMMON
#endif

/*
 included support if for the following compiler/user options -:

 __STRICT_ANSI__                ISO Standard C
 _ISOC99_SOURCE                 ISO C99 Extensions

 _POSIX_SOURCE                  IEEE Std 1003.1
 _POSIX_C_SOURCE                1 = IEEE Std 1003.1
                                >= 2, include IEEE Std 1003.2
                                >= 199309L, include IEEE Std 1003.1b-1993
                                >= 199506L, include IEEE Std 1003.1c-1995
                                >= 200112L, include all of IEEE 1003.1-2004
                                >= 200809L, include all of IEEE 1003.1-2008
 _XOPEN_SOURCE                  500 = Single Unix conformance
                                600 = Sixth revision
                                700 = Seventh revision
 _XOPEN_SOURCE_EXTENDED         X/Open Portability Guide, and X/Open Unix extensions

 _SVID_SOURCE                   ISO C, POSIX C, and System V Unix
 _BSD_SOURCE                    ISO C, POSIX C, and BSD-4.3

 _LARGEFILE_SOURCE              use the large file access by default
 _LARGEFILE64_SOURCE            expose the 64bit large file support
 _FILE_OFFSET_BITS=N            determine the IO interface to use (32/64bit)

 _GNU_SOURCE                    enables all of the C/POSIX standards and extensions
                                and includes GNU extensions
*/

/* treat __BSD_VISIBLE the same as _BSD_SOURCE */
#if defined(__BSD_VISIBLE)
# define        _BSD_SOURCE             __BSD_VISIBLE
#endif

#if defined(_GNU_SOURCE)
# undef         _POSIX_SOURCE
# define        _POSIX_SOURCE           1
# undef         _POSIX_C_SOURCE
# define        _POSIX_C_SOURCE         200809L

# undef         _XOPEN_SOURCE
# define        _XOPEN_SOURCE           700
# undef         _XOPEN_SOURCE_EXTENDED
# define        _XOPEN_SOURCE_EXTENDED  1

# undef         _LARGEFILE64_SOURCE
# define        _LARGEFILE64_SOURCE     1

# undef         _ISOC99_SOURCE
# define        _ISOC99_SOURCE          1

# undef         _SVID_SOURCE
# define        _SVID_SOURCE            1

# undef         _BSD_SOURCE
# define        _BSD_SOURCE             1
#endif

/* define _BSD_SOURCE/_XOPEN_SOURCE=500
 * if nothing else has been defined  */
#if (!defined(__STRICT_ANSI__) && \
     !defined(_ISOC99_SOURCE) && \
     !defined(_POSIX_SOURCE) && \
     !defined(_POSIX_C_SOURCE) && \
     !defined(_XOPEN_SOURCE) && \
     !defined(_XOPEN_SOURCE_EXTENDED) && \
     !defined(_BSD_SOURCE))
#define         _XOPEN_SOURCE           500
# define        _BSD_SOURCE             1
#endif

#if defined (_POSIX_C_SOURCE)
#if (_POSIX_C_SOURCE - 0) >= 200112L
# define        __USE_XOPEN2K           1
# undef         __USE_ISOC99
# define        __USE_ISOC99            1
#endif

#if (_POSIX_C_SOURCE - 0) >= 200809L
# define        __USE_XOPEN2K8          1
#endif
#endif

#ifdef _XOPEN_SOURCE
# define        __USE_XOPEN             1
# if (_XOPEN_SOURCE - 0) >= 500
#  if (_XOPEN_SOURCE - 0) >= 600
#   if (_XOPEN_SOURCE - 0) >= 700
#    define     __USE_XOPEN2K8          1
#   endif
#   define      __USE_XOPEN2K           1
#   undef       __USE_ISOC99
#   define      __USE_ISOC99            1
#  endif
#  define       __USE_XOPEN_EXTENDED    1
#  define       __USE_UNIX98            1
#  undef        _LARGEFILE_SOURCE
#  define       _LARGEFILE_SOURCE       1
# else
#  if defined(_XOPEN_SOURCE_EXTENDED)
#   define      __USE_XOPEN_EXTENDED    1
#  endif
# endif
#endif

#if defined(_BSD_SOURCE) || defined(_SVID_SOURCE)
# define        __USE_NIXCOMMON         1
#endif

#if defined(_BSD_SOURCE)
# define        __USE_BSD               1
#endif

#if defined(_SVID_SOURCE)
# define        __USE_SYSV              1
#endif

/*
 * and now which features have been asked for...
 */

#if defined(_LARGEFILE_SOURCE)
#define         __USE_LARGEFILE         1
#endif

#if defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64
# define        __USE_FILE_OFFSET64     1
#endif

// On AROS, __USE_FILES_OFFSET64 implies __USE_LARGEFILE64
#if defined(__USE_FILE_OFFSET64) || defined(_LARGEFILE64_SOURCE)
#define         __USE_LARGEFILE64       1
#endif

#endif
