/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    keymap_lib.fd and clib/keymap_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,KeymapBase)

    D(bug("AskKeymapDefault\n"));    
    
    return (NULL);

    AROS_LIBFUNC_EXIT
} /* AskKeyMapDefault */
