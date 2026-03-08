/*
    Copyright (C) 2012-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <aros/types/__sighandler_t.h>
#include <aros/types/sigset_t.h>

struct signal_func_data
{
    __sighandler_t *sigfunc;
    void (*sigaction_func)(int, void *, void *);
    sigset_t sa_mask;
    int sa_flags;
    UBYTE flags;
};

#define __SIG_RUNNING 1 /* Signal handler is running */
#define __SIG_PENDING 2 /* Signal handler is pending */

struct signal_func_data *__stdc_sig_getfuncdata(int signum);
__sighandler_t __sig_default;
