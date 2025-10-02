#ifndef _AROS_TYPES_WCHAR_T_H
#define _AROS_TYPES_WCHAR_T_H

/*
    Copyright © 2010-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: wchar_t definition for the AROS standard C library implementation.

    This header defines wchar_t as part of the AROS standard C runtime library.
    It provides a 32-bit unsigned integer type to represent wide characters,
    enabling full Unicode support (U+0000 to U+10FFFF) consistent with UCS-4.

    The AROS libc uses UTF-8 as the native multibyte encoding and relies on this
    wchar_t definition for all wide character and multibyte conversion functions.

    This definition is integral to the AROS standard C library and should be
    used consistently across all components relying on wide character support.

    NOTE:
        - wchar_t is defined as uint32_t to support the full Unicode range.
        - This definition assumes stateless UTF-8 encoding throughout the system.
        - In hosted environments where wchar_t is predefined by the compiler or
          system headers, this header may be conditionally excluded.
 */

#ifndef __cplusplus

#ifndef __WCHAR_TYPE__
# include <aros/types/int_t.h>
# if defined(__WCHAR_MAX__) && __WCHAR_MAX__ > 255
/* AROS policy: UCS-4 capable */
#  define __WCHAR_TYPE__ uint32_t
# else
#  define __WCHAR_TYPE__ char
# endif
#endif
typedef __WCHAR_TYPE__ wchar_t;

#if defined(__WCHAR_MAX__)
#if __WCHAR_MAX__ >= 0x10FFFF
#define MAX_UNICODE 0x10FFFF
#else
#define MAX_UNICODE __WCHAR_MAX__
#endif
#else
#define MAX_UNICODE 0xFF
#endif

#endif

#endif /* _AROS_TYPES_WCHAR_T_H */
