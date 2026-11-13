/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function _exit() with vfork() child interception.
*/

#include <setjmp.h>

#include "__posixc_intbase.h"
#include "__vfork.h"

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

        void _exit(

/*  SYNOPSIS */
        int code)

/*  FUNCTION
        Terminates the calling process immediately.

    INPUTS
        code - exit status.

    RESULT
        Does not return.

    NOTES
        stdc.library provides _exit() only as a weak alias of _Exit(), which
        longjmps to the exit jmp_buf registered in the CALLER'S per-opener
        StdCBase. A vfork() child runs as the parent task pretending to be the
        child: __vfork intercepts the exit jmp_buf in the StdCBase instance
        posixc.library holds, but a program's own _exit() call resolves through
        the PROGRAM'S StdCBase instance, sails past the interception, and
        executes the real program teardown in the borrowed context -
        destroying the parent. This strong implementation routes a
        pretend-child's _exit() to the vfork exit jmp_buf; outside vfork it is
        equivalent to _Exit().

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    if (PosixCBase->flags & PRETEND_CHILD)
    {
        struct vfork_data *udata = PosixCBase->vfork_data;

        D(bug("_exit(%d): PRETEND_CHILD, longjmp to vfork exit\n", code));

        udata->child_error = code;
        longjmp(udata->parent_newexitjmp, 1);
        /* not reached */
    }

    _Exit(code);

    /* not reached */
    for (;;)
        ;
}
