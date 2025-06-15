#ifndef _STDC_WCHAR_CHAR_H
#define _STDC_WCHAR_CHAR_H
/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#if !defined(_STDC_NOINLINE) && !defined(_STDC_NOINLINE_STDLIB)

#include <stddef.h>
#include <wchar.h>
#include <errno.h>

#include <aros/types/mbstate_t.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WCHAR_FNAM_PREFIX)
# define WCHAR_FNAM_CONCAT(a, b) a##b
# define WCHARFUNC(name) WCHAR_FNAM_CONCAT(WCHAR_FNAM_PREFIX, name)
#else
# define WCHARFUNC(name) name
#endif

/* inline code */
/* The multi-byte character functions are implemented inline so that they adapt to the
   size of wchar_t used by the compiler. This would not be possible if the code would
   be compiled in the shared library or even the static link library.
*/

#define _isascii(c) (((c) & ~0x7F) == 0)

#if !defined(_STDC_NOINLINE_MBTOWC)
static __inline__
int WCHARFUNC(mbtowc)(wchar_t * restrict pwc, const char * restrict s, size_t n)
{
    if (s == NULL)
        /* No state-dependent multi-byte character encoding */
        return 0;
    else
    {
        if (_isascii(*s))
        {
            if (pwc)
                *pwc = (wchar_t)*s;
            if (*s == 0)
                return 0;
            else
                return 1;
        }
        else
            return -1;
    }
}
#endif /* !_STDC_NOINLINE_MBTOWC */


#if !defined(_STDC_NOINLINE_WCTOMB)
static __inline__
int WCHARFUNC(wctomb)(char *s, wchar_t wchar)
{
    if (s == NULL)
        /* No state dependent encodings */
        return 0;
    else if (_isascii((int)wchar))
    {
        *s = (char)wchar;
        if (wchar == 0)
            return 0;
        else
            return 1;
    }
    else
        return -1;
}
#endif /* !_STDC_NOINLINE_WCTOMB */


#if !defined(_STDC_NOINLINE_MBSTOWCS)
static __inline__
size_t WCHARFUNC(mbstowcs)(wchar_t * restrict pwcs, const char * restrict s, size_t n)
{
    size_t l;

    for(l = 0; n > 0; s++, pwcs++, n--)
    {
        *pwcs = (wchar_t)*s;

        if (*s == '\0')
            break;

        l++;
    }

    return l;
}
#endif /* !_STDC_NOINLINE_MBSTOWCS */


#if !defined(_STDC_NOINLINE_WCSTOMBS)
static __inline__
size_t WCHARFUNC(wcstombs)(char * restrict s, const wchar_t * restrict pwcs, size_t n)
{
    size_t l;

    for(l = 0; n > 0; s++, pwcs++, n--)
    {
        if (!_isascii((int)*pwcs))
            return (size_t)-1;

        *s = (char)*pwcs;

        if (*s == '\0')
            break;

        l++;
    }

    return l;
}
#endif /* !_STDC_NOINLINE_WCSTOMBS */

#ifdef __cplusplus
}
#endif

#endif /* !_STDC_NOINLINE && !_STDC_NOINLINE_STDLIB */

#endif /* _STDC_WCHAR_CHAR_H */
