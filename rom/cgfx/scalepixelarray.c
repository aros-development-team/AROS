/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH10(LONG, ScalePixelArray,

/*  SYNOPSIS */
	AROS_LHA(APTR             , , A0),
	AROS_LHA(UWORD            , , D0),
	AROS_LHA(UWORD            , , D1),
	AROS_LHA(UWORD            , , D2),
	AROS_LHA(struct RastPort *, , A1),
	AROS_LHA(UWORD            , , D3),
	AROS_LHA(UWORD            , , D4),
	AROS_LHA(UWORD            , , D5),
	AROS_LHA(UWORD            , , D6),
	AROS_LHA(UBYTE            , , D7),

/*  LOCATION */
	struct Library *, CyberGfxBase, 15, Cybergraphics)

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
			    cybergraphics_lib.fd and clib/cybergraphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,CyberGfxBase)
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("ScalePixelArray");

    AROS_LIBFUNC_EXIT
} /* ScalePixelArray */
