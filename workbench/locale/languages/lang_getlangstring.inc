/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: common code for language description files
*/

/* ----------------------------------------------------------------------- */

/* Bit masks for locale .language functions. Only implement GetString() */
#define LF_GetLangStr       (1L << 3)

/* ------------------------------------------------------------------------
   Language specific functions
 ------------------------------------------------------------------------ */


/* ULONG LanguageMask():
    This function is to inform locale.library what functions it should
    use from this library. This is done by returning a bitmask containing
    1's for functions to use, and 0's for functions to ignore.

    Unused bits MUST be 0 for future compatibility.
*/
AROS_LH0(ULONG, mask, struct Language *, language, 5, language)
{
    AROS_LIBFUNC_INIT

    return ( LF_GetLangStr );

    AROS_LIBFUNC_EXIT
}

/* STRPTR GetLangString(ULONG num): Language function 3
    This function is called by GetLocaleStr() and should return
    the string matching the string id passed in as num.
*/
AROS_LH1(STRPTR, getlangstring,
    AROS_LHA(ULONG, id, D0),
    struct LocaleBase *, LocaleBase, 9, language)
{
    AROS_LIBFUNC_INIT

    if(id < MAXSTRMSG)
        return ___strings[id];
    else
        return NULL;

    AROS_LIBFUNC_EXIT
}
