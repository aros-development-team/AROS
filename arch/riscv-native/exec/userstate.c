/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: UserState() - Return to normal mode after changing things.
*/

#include <proto/exec.h>

/* See rom/exec/userstate.c for documentation */

AROS_LH1(void, UserState,
    AROS_LHA(APTR, superSP, D0),
    struct ExecBase *, SysBase, 26, Exec)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
} /* UserState() */
