/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include "diskfont_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/diskfont.h>

	AROS_LH1(VOID, DisposeFontContents,

/*  SYNOPSIS */
	AROS_LHA(struct FontContentsHeader *, fontContentsHeader, A1),

/*  LOCATION */
	struct Library *, DiskfontBase, 8, Diskfont)

/*  FUNCTION

    Free a FontContents array obtained from NewFontContents().

    INPUTS

    fontContentsHeader  --  Pointer to a struct FontContentsHeader got from
                            NewFontContents().

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    NewFontContents()

    INTERNALS

    HISTORY

    5.8.1999  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    (void)DiskfontBase;

    FreeVec(fontContentsHeader);

    AROS_LIBFUNC_EXIT
    
} /* DisposeFontContents */
