/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: OS specific functions for signal blocking
    Lang: english
*/

#include <signal.h>
#include <stdio.h>

/* The real functions are written in assembler as stubs which call these */
void _os_enable (void)
{
    sigset_t set;

    /* if (supervisor)
    {
        fprintf (stderr, "Enable() called in supervisor mode\n");
    } */

    sigfillset (&set);

    sigprocmask (SIG_UNBLOCK, &set, NULL);
} /* _os_enable */

void _os_disable (void)
{
    sigset_t set;

    sigfillset (&set);

    sigprocmask (SIG_BLOCK, &set, NULL);
} /* _os_disable */
