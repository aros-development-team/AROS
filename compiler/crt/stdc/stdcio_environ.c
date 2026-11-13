/*
    Copyright (C) 2012-2023, The AROS Development Team. All rights reserved.

    Desc: Code enabling environ emulation mode for user programs.
          This code is part of the static link library of stdcio.
          When programs do not access this variable environ emulation
          won't be enable.
          See __stdcio_set_environptr() autodoc for more information.
*/

#include <aros/symbolsets.h>

extern int __stdcio_set_environptr (char ***environptr);
char **environ;

static int __environ_init(struct ExecBase *SysBase)
{
    return __stdcio_set_environptr(&environ);
}

ADD2INIT(__environ_init, 0)

