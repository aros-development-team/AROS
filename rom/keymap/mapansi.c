/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "keymap_intern.h"

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <clib/keymap_protos.h>

	AROS_LH5(LONG, MapANSI,

/*  SYNOPSIS */
	AROS_LHA(STRPTR         , string, A0),
	AROS_LHA(LONG           , count, D0),
	AROS_LHA(STRPTR         , buffer, A1),
	AROS_LHA(LONG           , length, D1),
	AROS_LHA(struct KeyMap *, keyMap, A2),

/*  LOCATION */
	struct Library *, KeymapBase, 8, Keymap)

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

    D(bug("MapAnsi\n"));    
    return 0;

    AROS_LIBFUNC_EXIT
} /* MapANSI */
