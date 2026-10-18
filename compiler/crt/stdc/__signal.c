/*
    Copyright (C) 2012-2026, The AROS Development Team. All rights reserved.
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
#include <string.h>
#include <assert.h>

#include "__stdc_intbase.h"
#include "__signal.h"
#include "__optionallibs.h"

#include <aros/debug.h>

#include "debug.h"

/*****i**********************************************************************
    NAME
        struct signal_func_data * __stdc_sig_getfuncdata(

    SYNOPSIS
        int signum)

    FUNCTION
        Return the mutable signal record in the current stdc.library
        base, allocating the signal table on first use.

    INPUTS
        signum - Signal number from SIGHUP through _SIGMAX, inclusive
                 (1 through 64). Requires a valid current stdc.library
                 base with its memory pool initialized.

    RESULT
        A borrowed pointer to the record for signum, or NULL with
        errno set to EINVAL for an out-of-range number or ENOMEM
        if table allocation fails. Success does not clear errno.

    NOTES
        Internal signal support, not an application API. First use
        initializes every record with SIG_DFL, a NULL sigaction
        handler, and zeroed mask, action flags, and delivery flags.
        Subsequent calls preserve the records.

        The record is shared by signal(), sigaction(), and raise()
        using the same library base. Do not free it. Its storage is
        reclaimed when that base's memory pool is destroyed.

        Access is not synchronized. Serialize first use and record
        updates when sharing a base. This lookup accepts every
        number in range; it does not enforce sigaction() restrictions
        on SIGKILL and SIGSTOP or establish signal delivery support.

    SEE ALSO
        signal(), sigaction(), raise()

*****************************************************************************/
struct signal_func_data *__stdc_sig_getfuncdata(int signum)
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
            StdCBase->sigfunc_array[i].sigaction_func = NULL;
            memset(&StdCBase->sigfunc_array[i].sa_mask, 0,
                   sizeof(sigset_t));
            StdCBase->sigfunc_array[i].sa_flags = 0;
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
/*****i**********************************************************************
    NAME
        void __sig_default(

    SYNOPSIS
        int signum)

    FUNCTION
        Report a signal and leave the program through the abnormal
        exit path with return code 20.

    INPUTS
        signum - Signal number used in the diagnostic. SIGABRT and
                 SIGTERM select dedicated messages. Other values
                 are printed numerically.

        Requires initialized stdc startup exit state and a current
        task with a valid, null-terminated name.

    RESULT
        Does not return during normal operation. Registered atexit()
        functions are not called.

    NOTES
        Internal handler selected by raise() for SIG_DFL. All
        signals reaching this handler take the same termination
        path; signal-specific POSIX default actions are not provided.

        Displays an Intuition requester if intuition.library is
        available; otherwise writes through kprintf(). The requester
        may wait for user input. This is not an async-signal-safe
        termination path.

        Calling without initialized startup exit state invokes a
        fatal system alert.

    BUGS
        The diagnostic uses an unbounded task name in a 50-byte
        stack buffer. A long name can corrupt the stack before
        the program reaches the exit path.

    SEE ALSO
        raise(), signal(), __stdc_jmp2exit()

*****************************************************************************/
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
        kprintf("[%s] %s: %s\n", STDCNAME, __func__, s);

    __stdc_jmp2exit(0, 20);

    assert(0); /* Should not be reached */
}
