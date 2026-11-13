/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Posix uselocale.
*/

#include <locale.h>

/****************************************************************************
    NAME
        locale_t uselocale(

    SYNOPSIS
        locale_t newloc)

    FUNCTION
        Return a locale handle without changing the active locale.

    INPUTS
        newloc - Value to return. The argument is not inspected,
                 dereferenced or retained.

    RESULT
        newloc unchanged. In particular, a zero argument returns
        zero and LC_GLOBAL_LOCALE returns LC_GLOBAL_LOCALE. The
        function does not change errno.

    NOTES
        Include <locale.h>. No storage is allocated or released.

    BUGS
        Per-thread locale selection is not implemented. This
        function neither installs newloc nor returns the previous
        locale. Passing zero does not query the active locale.
        Its return value cannot be used to save and restore locale
        state, and does not indicate that a locale was accepted.

    SEE ALSO
        newlocale(), freelocale(), setlocale()

*****************************************************************************/
locale_t uselocale(locale_t newloc)
{
	return newloc;
}
