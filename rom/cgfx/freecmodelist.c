/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH1(void, FreeCModeList,

/*  SYNOPSIS */
	AROS_LHA(struct List *, , A0),

/*  LOCATION */
	struct Library *, CyberGfxBase, 13, Cybergraphics)

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

    aros_print_not_implemented ("FreeCModeList");

    AROS_LIBFUNC_EXIT
} /* FreeCModeList */
