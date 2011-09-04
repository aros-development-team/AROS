/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <proto/exec.h>

#include <aros/libcall.h>
#include <aros/altstack.h>

#define DEBUG 1
#include <aros/debug.h>

/* In order to avoid infinite recursive call of aros_set_relbase
   no shared library functions may be called from these functions.
   On some platforms they may call aros_set_relbase.
*/

void *aros_get_relbase(void)
{
    D(
        if (SysBase == NULL)
            bug("[aros_get_relbase]: Error! SysBase==NULL\n");
        else if (SysBase->ThisTask == NULL)
            bug("[aros_get_relbase]: Error! SysBase->ThisTask==NULL\n");
    )

    return (void *)aros_get_altstack(SysBase->ThisTask);
}

void *aros_set_relbase(void *libbase)
{
    D(
        if (SysBase == NULL)
            bug("[aros_get_relbase]: Error! SysBase==NULL\n");
        else if (SysBase->ThisTask == NULL)
            bug("[aros_get_relbase]: Error! SysBase->ThisTask==NULL\n");
    )

    return (void *)aros_set_altstack(SysBase->ThisTask,(IPTR)libbase);
}

void aros_push_relbase(void *libbase)
{
    D(
        if (SysBase == NULL)
            bug("[aros_get_relbase]: Error! SysBase==NULL\n");
        else if (SysBase->ThisTask == NULL)
            bug("[aros_get_relbase]: Error! SysBase->ThisTask==NULL\n");
    )

    aros_push_altstack(SysBase->ThisTask,(IPTR)libbase);
}

void aros_push2_relbase(void *libbase, void *ptr)
{
    D(
        if (SysBase == NULL)
            bug("[aros_get_relbase]: Error! SysBase==NULL\n");
        else if (SysBase->ThisTask == NULL)
            bug("[aros_get_relbase]: Error! SysBase->ThisTask==NULL\n");
    )

    aros_push_altstack(SysBase->ThisTask,(IPTR)ptr);
    aros_push_altstack(SysBase->ThisTask,(IPTR)libbase);
}

void *aros_pop_relbase(void)
{
    D(
        if (SysBase == NULL)
            bug("[aros_get_relbase]: Error! SysBase==NULL\n");
        else if (SysBase->ThisTask == NULL)
            bug("[aros_get_relbase]: Error! SysBase->ThisTask==NULL\n");
    )

    return (void *)aros_pop_altstack(SysBase->ThisTask);
}

void *aros_pop2_relbase(void)
{
    D(
        if (SysBase == NULL)
            bug("[aros_get_relbase]: Error! SysBase==NULL\n");
        else if (SysBase->ThisTask == NULL)
            bug("[aros_get_relbase]: Error! SysBase->ThisTask==NULL\n");
    )

    aros_pop_altstack(SysBase->ThisTask);
    return (void *)aros_pop_altstack(SysBase->ThisTask);
}
