/*
    Copyright © 2012-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

#define __NOBLIBBASE__

#include <libraries/stdc.h>

/* We include the signal.h of posixc.library for getting min/max signal number
   TODO: Implement more elegant way to get maximum and minimum signal number
*/
#include <aros/posixc/signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "__stdc_intbase.h"
#include "__signal.h"
#include "__optionallibs.h"

#define DEBUG 0
#include <aros/debug.h>

struct signal_func_data *__sig_getfuncdata(int signum)
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();
    int i;

    if (signum < SIGHUP || signum > _SIGMAX)
    {
        errno = EINVAL;
        return NULL;
    }

    if (StdCBase->sigfunc_array == NULL)
    {
        StdCBase->sigfunc_array = malloc(_SIGMAX*sizeof(struct signal_func_data));

        if (!StdCBase->sigfunc_array)
        {
            errno = ENOMEM;
            return NULL;
        }

        for (i = 0; i < _SIGMAX; i++)
        {
            StdCBase->sigfunc_array[i].sigfunc = SIG_DFL;
            StdCBase->sigfunc_array[i].flags = 0;
        }
    }

    return &StdCBase->sigfunc_array[signum-1];
}

/* Handler for SIG_DFL */
/* TODO: Maybe a mechanism has to be implemented so that default signal handler
   can be overloaded by libraries. For example by stdcio.library or posixc.library
   so they can use stdio for presenting the caught signal
*/
void __sig_default(int signum)
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();
    char *taskname = FindTask(NULL)->tc_Node.ln_Name;
    char s[50];
 
    switch (signum)
    {
    case SIGABRT:
        sprintf(s, "Program '%s' aborted.", taskname);
        break;

    case SIGTERM:
        sprintf(s, "Program '%s' terminated.", taskname);
        break;

    default:
        sprintf(s, "Program '%s' caught signal %d\naborting...",
                taskname, signum
        );
        break;
    }

    /* Open requester if IntuitionBase is available otherwise use kprintf() */
    if (__intuition_available(StdCBase))
    {
        struct EasyStruct es =
            {
                sizeof(struct EasyStruct),
                0,
                "Caught Signal",
                "%s",
                "OK"
            };
        stdcEasyRequest(StdCBase->StdCIntuitionBase, NULL, &es, NULL, s);
    }
    else
        kprintf("%s\n", s);

    __stdc_jmp2exit(0, 20);

    assert(0); /* Should not be reached */
}
