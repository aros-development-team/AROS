/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "diskfont_intern.h"

/*****************************************************************************

    NAME */
#include <clib/diskfont_protos.h>

	AROS_LH2(struct DiskFont *, NewScaledDiskFont,

/*  SYNOPSIS */
	AROS_LHA(struct TextFont *, sourceFont, A0),
	AROS_LHA(struct TextAttr *, destTextAttr, A1),

/*  LOCATION */
	struct Library *, DiskfontBase, 9, Diskfont)

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

    (void)sourceFont;
    (void)destTextAttr;
    (void)DiskfontBase;

    return (0L);

    AROS_LIBFUNC_EXIT
} /* NewScaledDiskFont */
