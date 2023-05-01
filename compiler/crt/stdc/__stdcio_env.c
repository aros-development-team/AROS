/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>

#include "__stdcio_intbase.h"
#include "__env.h"

#include <stdio.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

        int __stdcio_set_envlistptr (

/*  SYNOPSIS */
        char ***envlistptr)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
        Private - do not use!

    EXAMPLE

    BUGS

    SEE ALSO
        __stdcio_get_envlistptr()

    INTERNALS

******************************************************************************/
{
    struct StdCIOIntBase *StdCIOBase =
        (struct StdCIOIntBase *)__aros_getbase_StdCIOBase();

    D(bug("[stdcio] %s(0x%p)\n", __func__, envlistptr);)

    StdCIOBase->env_list = (struct __env_item *)envlistptr;

    return (1);
}

/*****************************************************************************

    NAME */
#include <stdlib.h>

        char ***__stdcio_get_envlistptr (

/*  SYNOPSIS */
        void)

/*  FUNCTION

    INPUTS
        -

    RESULT

    NOTES
        Private - do not use!

    EXAMPLE

    BUGS

    SEE ALSO
        __stdcio_set_envlistptr()

    INTERNALS

******************************************************************************/
{
    struct StdCIOIntBase *StdCIOBase =
        (struct StdCIOIntBase *)__aros_getbase_StdCIOBase();

    return (char ***)StdCIOBase->env_list;
}
