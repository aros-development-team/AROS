/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: AROS specific function for environ emulation handling
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/stdcio.h>

#include "__env.h"

char ***__posixc_get_environptr (void)
{
    return __stdcio_get_environptr();
}

int __posixc_set_envlistptr (char ***envlistptr)
{
    return __stdcio_set_envlistptr(envlistptr);
}

char ***__posixc_get_envlistptr (void)
{
    return __stdcio_get_envlistptr();
}

__env_item *__posixc_env_getvar(const char *varname, int valuesize)
{
    return (__env_item *)__stdcio_env_getvar(varname, valuesize);
}