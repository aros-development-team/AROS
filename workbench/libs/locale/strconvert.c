/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: StrConvert - Stub for the Locale StrConvert function.
    Lang: english
*/
#include <exec/types.h>
#include <proto/exec.h>
#include "locale_intern.h"
#include <aros/asmcall.h>

#define        DEBUG_STRCONVERT(x)        ;

/*****************************************************************************

    NAME */
#include <proto/locale.h>

        AROS_LH5(ULONG, StrConvert,

/*  SYNOPSIS */
        AROS_LHA(const struct Locale *, locale, A0),
        AROS_LHA(CONST_STRPTR   , string, A1),
        AROS_LHA(APTR           , buffer, A2),
        AROS_LHA(ULONG          , bufferSize, D0),
        AROS_LHA(ULONG          , type, D1),

/*  LOCATION */
        struct LocaleBase *, LocaleBase, 29, Locale)

/*  FUNCTION
        This function will transform the string given and place the
        result in the supplied buffers, copying at most bufferSize
        bytes.

        The transformation is such that if the C strcmp() function
        was called on two strings transformed by this function then
        the result will be the same as calling the Locale StrnCmp()
        function on the two strings.

    INPUTS
        locale      -   the Locale to use for the transformation.
        string      -   the string to be transformed
        buffer      -   the destination for the transformed string.
                        This buffer may need to be larger than the
                        untransformed string.
        bufferSize  -   the maximum number of bytes to place in
                        buffer.
        type        -   how to transform the string. See the
                        StrnCmp() function for possible values.

    RESULT
        Length of the number of BYTES placed in the buffer by
        the transformation process minus 1 (for NULL termination).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        StrnCmp()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG result;

    DEBUG_STRCONVERT(dprintf
        ("StrConvert: locale 0x%lx <%s> buffer 0x%lx size %ld type 0x%lx\n",
            locale, string, buffer, bufferSize, type));

    DEBUG_STRCONVERT(dprintf("StrConvert: Function 0x%lx\n",
            IntL(locale)->il_LanguageFunctions[15]));

#ifdef AROS_CALL4
    result = AROS_CALL4(ULONG, IntL(locale)->il_LanguageFunctions[15],
        AROS_LCA(CONST_STRPTR, string, A1),
        AROS_LCA(APTR, buffer, A2),
        AROS_LCA(ULONG, bufferSize, D0),
        AROS_LCA(ULONG, type, D1), struct LocaleBase *, LocaleBase);
#else
    result = AROS_UFC4(ULONG, IntL(locale)->il_LanguageFunctions[15],
        AROS_UFCA(CONST_STRPTR, string, A1),
        AROS_UFCA(APTR, buffer, A2),
        AROS_UFCA(ULONG, bufferSize, D0), AROS_UFCA(ULONG, type, D1));
#endif

    DEBUG_STRCONVERT(dprintf("StrConvert: retval %lu\n", result));

    return result;

    AROS_LIBFUNC_EXIT
}
