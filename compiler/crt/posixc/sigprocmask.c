/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function sigprocmask().
*/

#include "__posixc_intbase.h"

#include <aros/debug.h>
#include <string.h>
#include <errno.h>

/*****************************************************************************

    NAME */

#include <signal.h>

        int sigprocmask (

/*  SYNOPSIS */
        int  how,
        const  sigset_t *set,
        sigset_t *oldset)

/*  FUNCTION
        Examine or change the signal mask of the calling thread.

    INPUTS
        how    - how to change the mask: SIG_BLOCK, SIG_UNBLOCK, or
                 SIG_SETMASK
        set    - if non-NULL, the set of signals to apply according to 'how'
        oldset - if non-NULL, receives the previous signal mask

    RESULT
        0 on success, -1 on error with errno set.

    NOTES
        Since AROS does not deliver asynchronous signals the mask only
        affects what raise() would do if checked.

    EXAMPLE

    BUGS

    SEE ALSO
        sigaction(), raise()

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    unsigned int i;

    if (!PosixCBase)
    {
        errno = EINVAL;
        return -1;
    }

    /* Return old mask if requested */
    if (oldset)
        memcpy(oldset, &PosixCBase->sigmask, sizeof(sigset_t));

    if (set)
    {
        switch (how)
        {
        case SIG_BLOCK:
            /* OR in the new set */
            for (i = 0; i < _SIG_WORDS; i++)
                PosixCBase->sigmask.__val[i] |= set->__val[i];
            break;

        case SIG_UNBLOCK:
            /* AND out the new set */
            for (i = 0; i < _SIG_WORDS; i++)
                PosixCBase->sigmask.__val[i] &= ~set->__val[i];
            break;

        case SIG_SETMASK:
            /* Replace entirely */
            memcpy(&PosixCBase->sigmask, set, sizeof(sigset_t));
            break;

        default:
            errno = EINVAL;
            return -1;
        }
    }

    return 0;
} /* sigprocmask */
