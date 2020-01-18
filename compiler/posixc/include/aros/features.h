#ifndef _POSIXC_FEATURES_H
#define _POSIXC_FEATURES_H
/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * if we have been included without any version being set,
 * use the minimum supported to make sure definitions we want
 * are available ...
 */

#if !defined(_XOPEN_SOURCE)
#define         _XOPEN_SOURCE           500
#endif

/*
 * set features depending on what standards have been asked for...
 */

#ifdef _GNU_SOURCE
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

# undef         _BSD_SOURCE
# define        _BSD_SOURCE             1
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
#  ifdef _XOPEN_SOURCE_EXTENDED
#   define      __USE_XOPEN_EXTENDED    1
#  endif
# endif
#endif

/*
 * and now which features have been asked for...
 */

#ifdef _LARGEFILE_SOURCE
#define         __USE_LARGEFILE         1
#endif

#ifdef _LARGEFILE64_SOURCE
#define         __USE_LARGEFILE64       1
#endif

#if defined _FILE_OFFSET_BITS && _FILE_OFFSET_BITS == 64
# define        __USE_FILE_OFFSET64     1
#endif

#endif
