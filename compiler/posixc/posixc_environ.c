/*
    Copyright © 2012-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code enabling environ emulation mode for user programs.
          This code is part of the static link library of arosc.
          When programs do not access this variable environ emulation
          won't be enable.
          See __posixc_set_environptr() autodoc for more information.
*/

#include <aros/symbolsets.h>
#include <unistd.h>

char **environ;

static int __environ_init(struct ExecBase *SysBase)
{
    return __posixc_set_environptr(&environ);
}

ADD2INIT(__environ_init, 0)

