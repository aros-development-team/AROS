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
	AROS_LHA(APTR             , srcRect, A0),
	AROS_LHA(UWORD            , SrcW, D0),
	AROS_LHA(UWORD            , SrcH, D1),
	AROS_LHA(UWORD            , SrcMod, D2),
	AROS_LHA(struct RastPort *, RastPort, A1),
	AROS_LHA(UWORD            , DestX, D3),
	AROS_LHA(UWORD            , DestY, D4),
	AROS_LHA(UWORD            , DestW, D5),
	AROS_LHA(UWORD            , DestH, D6),
	AROS_LHA(UBYTE            , SrcFormat, D7),

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

#if 0
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("ScalePixelArray");
#endif

    return 0;
    
    AROS_LIBFUNC_EXIT
} /* ScalePixelArray */
