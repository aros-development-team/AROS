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
        AROS_LH1I(void, UUID_Clear,

/*  SYNOPSIS */
        AROS_LHA(uuid_t *, uuid, A0),

/*  LOCATION */
        struct uuid_base *, UUIDBase, 11, UUID)

/*  FUNCTION
        Clears the specified uuid.

    INPUTS
        uuid - UUID to be cleared.

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

    int i;
    
    ASSERT(uuid);
    
    uuid->time_low = uuid->time_mid = uuid->time_hi_and_version = 0;
    uuid->clock_seq_hi_and_reserved = uuid->clock_seq_low = 0;
    for (i=0; i < 6; i++)
        uuid->node[i] = 0;
    
    AROS_LIBFUNC_EXIT
}
