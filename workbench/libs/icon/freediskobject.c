/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/arossupport.h>
#include "icon_intern.h"

extern const IPTR IconDesc[];

/*****************************************************************************

    NAME */
#include <clib/icon_protos.h>

	AROS_LH1(void, FreeDiskObject,

/*  SYNOPSIS */
	AROS_LHA(struct DiskObject *, diskobj, A0),

/*  LOCATION */
	struct Library *, IconBase, 15, Icon)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)
    
    void **mem = (void**)diskobj;
    mem--;
    
    /* It's enough to free our pool */
    DeletePool(mem[0]);

    AROS_LIBFUNC_EXIT
} /* FreeDiskObject */
