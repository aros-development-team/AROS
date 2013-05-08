/*
    Copyright © 2007-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "uuid_private.h"
#include LC_LIBDEFS_FILE

#define CHECK(f1, f2) if (f1 != f2) return f1 < f2 ? -1 : 1;

/*****************************************************************************

    NAME */
        AROS_LH2(int, UUID_Compare,

/*  SYNOPSIS */
        AROS_LHA(const uuid_t *, u1, A0),
        AROS_LHA(const uuid_t *, u2, A1),

/*  LOCATION */
        struct uuid_base *, UUIDBase, 5, UUID)
        
/*  FUNCTION
        Compares between two UUIDs.

    INPUTS
        u1, u2 - UUIDs to be compared.

    RESULT
        <0 - if the u1 is lexically BEFORE u2
        =0 - if u1 equals u2
        >0 - if the u1 is lexically AFTER u2

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/        
{
    AROS_LIBFUNC_INIT

    int i;

    ASSERT(u1);
    ASSERT(u2);
    
    CHECK(u1->time_low, u2->time_low); 
    CHECK(u1->time_mid, u2->time_mid); 
    
    CHECK(u1->time_hi_and_version, u2->time_hi_and_version); CHECK(u1->clock_seq_hi_and_reserved, u2->clock_seq_hi_and_reserved); 
    CHECK(u1->clock_seq_low, u2->clock_seq_low); 

    for (i = 0; i < 6; i++)
    {
        if (u1->node[i] < u2->node[i])
            return -1;
        if (u1->node[i] > u2->node[i])
            return 1;
    }
    return 0;

    AROS_LIBFUNC_EXIT
}
