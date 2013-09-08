/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

#include "__arosc_privdata.h"
#include "__signal.h"

struct signal_func_data *__sig_getfuncdata(int signum)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    int i;

    if (signum < SIGHUP || signum > _SIGMAX)
    {
        errno = EINVAL;
        return NULL;
    }

    if (aroscbase->acb_sigfunc_array == NULL)
    {
        aroscbase->acb_sigfunc_array =
            AllocPooled(aroscbase->acb_internalpool,
                        _SIGMAX*sizeof(struct signal_func_data)
            );

        if (!aroscbase->acb_sigfunc_array)
        {
            errno = ENOMEM;
            return NULL;
        }

        for (i = 0; i < _SIGMAX; i++)
        {
            aroscbase->acb_sigfunc_array[i].sigfunc = SIG_DFL;
            aroscbase->acb_sigfunc_array[i].flags = 0;
        }
    }

    return &aroscbase->acb_sigfunc_array[signum-1];
}

/* Handler for SIG_DFL */
void __sig_default(int signum)
{
    switch (signum)
    {
    case SIGABRT:
        fprintf(stderr, "Program aborted.\n");
        break;

    case SIGTERM:
        fprintf(stderr, "Program terminated.\n");
        break;

    default:
        fprintf(stderr, "Caught signal %d; aborting...\n", signum);
        break;
    }

    __arosc_jmp2exit(0, 20);

    assert(0); /* Should not be reached */
}
