/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <proto/exec.h>

#include <aros/libcall.h>
#include <aros/altstack.h>

#define DEBUG 0
#include <aros/debug.h>

/* In order to avoid infinite recursive call of aros_set_relbase
   no shared library functions may be called from these functions.
   On some platforms they may call aros_set_relbase.
*/

void *aros_get_relbase(void)
{
    void *ret;

    DB2(bug("aros_get_relbase()... "));
    D(
        if (SysBase->ThisTask == NULL)
            bug("[aros_get_relbase]: Error! SysBase->ThisTask==NULL\n");
    )

    ret = aros_get_altstack(SysBase->ThisTask);

    DB2(bug("0x%p\n", ret));
    return ret;
}

void *aros_set_relbase(void *libbase)
{
    D(
        if (SysBase->ThisTask == NULL)
            bug("[aros_get_relbase]: Error! SysBase->ThisTask==NULL\n");
    )

    return (void *)aros_set_altstack(SysBase->ThisTask,(IPTR)libbase);
}

void aros_push_relbase(void *libbase)
{
    D(
        if (SysBase->ThisTask == NULL)
            bug("[aros_get_relbase]: Error! SysBase->ThisTask==NULL\n");
    )

    aros_push_altstack(SysBase->ThisTask,(IPTR)libbase);
}

void aros_push2_relbase(void *libbase, void *ptr)
{
    DB2(bug("aros_push2_relbase(0x%p, 0x%p)\n", libbase, ptr));
    D(
        if (SysBase->ThisTask == NULL)
            bug("[aros_get_relbase]: Error! SysBase->ThisTask==NULL\n");
    )

    aros_push_altstack(SysBase->ThisTask,(IPTR)ptr);
    aros_push_altstack(SysBase->ThisTask,(IPTR)libbase);
}

void *aros_pop_relbase(void)
{
    D(
        if (SysBase->ThisTask == NULL)
            bug("[aros_get_relbase]: Error! SysBase->ThisTask==NULL\n");
    )

    return (void *)aros_pop_altstack(SysBase->ThisTask);
}

void *aros_pop2_relbase(void)
{
    void *ret;

    DB2(bug("aros_pop2_relbase()... "));
    D(
        if (SysBase->ThisTask == NULL)
            bug("[aros_get_relbase]: Error! SysBase->ThisTask==NULL\n");
    )

    aros_pop_altstack(SysBase->ThisTask);
    ret = aros_pop_altstack(SysBase->ThisTask);

    DB2(bug("0x%p\n", ret));
    return ret;
}
