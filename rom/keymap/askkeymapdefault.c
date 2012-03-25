/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: keymap.library function AskKeyMapDefault()
    Lang: english
*/
#include "keymap_intern.h"

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <clib/keymap_protos.h>

        AROS_LH0(struct KeyMap *, AskKeyMapDefault,

/*  SYNOPSIS */
        /* void */

/*  LOCATION */
        struct Library *, KeymapBase, 6, Keymap)

/*  FUNCTION
        Gives you a pointer to the current system default keymap.

    INPUTS

    RESULT
        Pointer to the system defaul keymap.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SetKeyMapDefault()

    INTERNALS

    HISTORY
        27-11-96    digulla automatically created from
                            keymap_lib.fd and clib/keymap_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    return (KMBase(KeymapBase)->DefaultKeymap);

    AROS_LIBFUNC_EXIT
} /* AskKeyMapDefault */
