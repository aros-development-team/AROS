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

	AROS_LH3(void, DoCDrawMethodTagList,

/*  SYNOPSIS */
	AROS_LHA(struct Hook     *, hook, A0),
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(struct TagItem  *, tags, A2),

/*  LOCATION */
	struct Library *, CyberGfxBase, 26, Cybergraphics)

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

    //aros_print_not_implemented ("DoCDrawMethodTagList");
    driver_DoCDrawMethodTagList(hook, rp, tags, CyberGfxBase);    

    AROS_LIBFUNC_EXIT
} /* DoCDrawMethodTagList */
