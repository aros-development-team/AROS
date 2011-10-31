/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    ROMTag scanner.
*/

#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_romtags.h>

struct ExecBase *krnInitResident(struct Resident *res, struct MemHeader *mh, struct TagItem *bootMsg)
{
    /* Magic. Described in rom/exec/exec_init.c. */
    return AROS_UFC3(struct ExecBase *, res->rt_Init,
                     AROS_UFCA(struct MemHeader *, mh, D0),
                     AROS_UFCA(struct TagItem *, bootMsg, A0),
 	             AROS_UFCA(struct ExecBase *, NULL, A6));
}
