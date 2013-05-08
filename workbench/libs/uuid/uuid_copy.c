/*
    Copyright © 2007-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "uuid_private.h"
#include LC_LIBDEFS_FILE

/*****************************************************************************

    NAME */
        AROS_LH2I(void, UUID_Copy,

/*  SYNOPSIS */
        AROS_LHA(const uuid_t *, src, A0),
        AROS_LHA(uuid_t *, dst, A1),
        
/*  LOCATION */
        struct uuid_base *, UUIDBase, 12, UUID)

/*  FUNCTION
        Copies the UUID's.

    INPUTS
        src - the source UUID.
        dst - the desitation UUID.

    RESULT
        This function always succeeds.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/       
{
    AROS_LIBFUNC_INIT

    ASSERT(dst);
    ASSERT(src);
    
    *dst = *src;
    
    AROS_LIBFUNC_EXIT
}
