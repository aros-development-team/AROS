/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Locale_RawDoFmt - locale.library's private replacement
          of exec.library/RawDoFmt function. IPrefs will install
          the patch.

    Lang: english
*/

#include <exec/rawfmt.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include "locale_intern.h"
#include <aros/asmcall.h>
#include <stdarg.h>


AROS_UFH3(VOID, LocRawDoFmtFormatStringFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct Locale *, locale, A2),
    AROS_UFHA(char, fill, A1))
{
    AROS_USERFUNC_INIT

#ifdef __mc68000__
    register char *pdata asm("a3") = hook->h_Data;
#else
    char *pdata = hook->h_Data;
#endif

    switch ((IPTR) hook->h_SubEntry)
    {
    case (IPTR) RAWFMTFUNC_STRING:
        /* Standard Array Function */
        *pdata++ = fill;
        break;
    case (IPTR) RAWFMTFUNC_SERIAL:
        /* Standard Serial Function */
        RawPutChar(fill);
        break;
    case (IPTR) RAWFMTFUNC_COUNT:
        /* Standard Count Function */
        (*((ULONG *) pdata))++;
        break;
    default:
        AROS_UFC3NR(void, hook->h_SubEntry,
            AROS_UFCA(char, fill, D0),
            AROS_UFCA(APTR, pdata, A3),
            AROS_UFCA(struct ExecBase *, SysBase, A6));
    }
    hook->h_Data = pdata;

    AROS_USERFUNC_EXIT
}

AROS_UFH3(VOID, LocRawDoFmtFormatStringFunc_SysV,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct Locale *, locale, A2), AROS_UFHA(char, fill, A1))
{
    AROS_USERFUNC_INIT

    APTR(*proc) (APTR, UBYTE) = (APTR) hook->h_SubEntry;
    char *pdata = hook->h_Data;

    switch ((IPTR) hook->h_SubEntry)
    {
    case (IPTR) RAWFMTFUNC_STRING:
        /* Standard Array Function */
        *pdata++ = fill;
        break;

    case (IPTR) RAWFMTFUNC_SERIAL:
        /* Standard Serial Function */
        RawPutChar(fill);
        break;

    case (IPTR) RAWFMTFUNC_COUNT:
        /* Standard Count Function */
        (*((ULONG *) pdata))++;
        break;

    default:
        pdata = proc(pdata, fill);
    }
    hook->h_Data = pdata;

    AROS_USERFUNC_EXIT
}

#undef LocaleBase

 /*****************************************************************************

    NAME */
#include <proto/locale.h>

        AROS_PLH4(APTR, LocRawDoFmt,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, FormatString, A0),
        AROS_LHA(APTR        , DataStream, A1),
        AROS_LHA(VOID_FUNC   , PutChProc, A2),
        AROS_LHA(APTR        , PutChData, A3),

/*  LOCATION */
        struct ExecBase *, SysBase, 31, Locale)

/*  FUNCTION
            See exec.library/RawDoFmt

    INPUTS
            See exec.library/RawDoFmt

    RESULT

    NOTES
            This function is not called by apps directly. Instead dos.library/DosGet-
        LocalizedString is patched to use this function. This means, that the
        LocaleBase parameter above actually points to SysBase, so we make use of
        the global LocaleBase variable. This function is marked as private,
        thus the headers generator won't mind the different basename in the header.

    EXAMPLE

    BUGS

    SEE ALSO
        RawDoFmt(), FormatString().

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Hook hook;
    APTR retval;

    hook.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(LocRawDoFmtFormatStringFunc);
    hook.h_SubEntry = (HOOKFUNC) PutChProc;
    hook.h_Data = PutChData;

    //kprintf("LocRawDoFmt: FormatString = \"%s\"\n", FormatString);

    REPLACEMENT_LOCK;

    retval = FormatString(&(IntLB(LocaleBase)->lb_CurrentLocale->il_Locale),
        (STRPTR) FormatString, DataStream, &hook);

    REPLACEMENT_UNLOCK;

    //kprintf("LocRawDoFmt: FormatString: returning %x\n", retval);

    return retval;

    AROS_LIBFUNC_EXIT

}

 /*****************************************************************************

    NAME */
#include <proto/locale.h>

        AROS_PLH4(APTR, LocVNewRawDoFmt,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, FormatString, A0),
        AROS_LHA(VOID_FUNC   , PutChProc, A2),
        AROS_LHA(APTR        , PutChData, A3),
        AROS_LHA(va_list     , DataStream, A1),

/*  LOCATION */
        struct ExecBase *, SysBase, 39, Locale)

/*  FUNCTION
            See exec.library/VNewRawDoFmt

    INPUTS
            See exec.library/VNewRawDoFmt

    RESULT

    NOTES
            This function is not called by apps directly. Instead exec.library/VNewRawDoFmt
        is patched to use this function. This means, that the library base parameter above
        actually points to SysBase, so we make use of the global LocaleBase variable.
        This function is marked as private, thus the headers generator won't mind the
        different basename in the header.

    EXAMPLE

    BUGS

    SEE ALSO
        RawDoFmt(), FormatString().

    INTERNALS

*****************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct Hook hook;

    hook.h_Entry =
        (HOOKFUNC) AROS_ASMSYMNAME(LocRawDoFmtFormatStringFunc_SysV);
    hook.h_SubEntry = (HOOKFUNC) PutChProc;
    hook.h_Data = PutChData;

    REPLACEMENT_LOCK;

    InternalFormatString(&(IntLB(LocaleBase)->lb_CurrentLocale->il_Locale),
        (STRPTR) FormatString, NULL, &hook, DataStream);

    REPLACEMENT_UNLOCK;

    return hook.h_Data;

    AROS_LIBFUNC_EXIT
}
