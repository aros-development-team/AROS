/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "diskfont_intern.h"

/*****************************************************************************

    NAME */
#include <clib/diskfont_protos.h>

	AROS_LH2(struct FontContentsHeader *, NewFontContents,

/*  SYNOPSIS */
	AROS_LHA(BPTR  , fontsLock, A0),
	AROS_LHA(STRPTR, fontName, A1),

/*  LOCATION */
	struct Library *, DiskfontBase, 7, Diskfont)

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
			    diskfont_lib.fd and clib/diskfont_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,DiskfontBase)
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("NewFontContents");
	return (0L);
    AROS_LIBFUNC_EXIT
} /* NewFontContents */
