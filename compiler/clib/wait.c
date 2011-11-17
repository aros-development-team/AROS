/*
    Copyright © 2004-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <proto/exec.h>
#include <aros/startup.h>
#include <aros/debug.h>

#include "__arosc_privdata.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/*****************************************************************************

    NAME */

        pid_t wait(

/*  SYNOPSIS */
        int *status)

/*  FUNCTION
        Waits for child process to change state. State change is one of the
        following events: child has exited, child was terminated by a signal,
        child was stopped by a signal, child was resumed by a signal.
        
        The function stores status of the process that changed state in the
        pointer given as status argument.

        The following macros can be used to extract information from the
        status value:

        WIFEXITED(status)    - true if the process has exited
        WEXITSTATUS(status)  - exit status of the exited process
        WIFSIGNALED(status)  - true if the child process was terminated by a
                               signal
        WTERMSIG(status)     - number of the signal that caused process
                               termination
        WIFSTOPPED(status)   - true if the child process was stopped by a
                               signal
        WSTOPSIG(status)     - number of the signal that caused child process
                               stop
        WIFCONTINUED(status) - true if the child process was resumed by the
                               SIGCONT signal.
        
        Parent process will be suspended until a child changes state. If a
        child process has already changed state, function returns immediately.

    INPUTS
        status - Pointer to int where child return status will be stored or
        NULL if you don't want to store status.

    RESULT
        Process id of the child process on success or -1 on error. If an error
        occurred, the global variable errno is set.

    NOTES
        This function will work only for child processeses notifying parent
        process of their death, for example processes created by vfork() call.
        If you want to use it for other processes, remember to set the
        NP_NotifyOnDeath tag value to TRUE during child process creation.

    EXAMPLE

    BUGS

    SEE ALSO
        waitpid()

    INTERNALS
        Since POSIX signals are not yet implemented, WIFSIGNALED, WIFSTOPPED
        and WIFCONTINUED macros always return 0. WIFEXITED always returns 1.

        The et_UniqueID field of the ETask structure is used as process id.

******************************************************************************/
{
    pid_t ret = -1;
    struct ETask *et;

    D(bug("wait()\n"));

    et = GetETask(FindTask(NULL));
    if(!et)
    {
        /* only ETasks are fertile */
        errno = ECHILD;
        return -1;
    }

    et = (struct ETask *)ChildWait(0);
    if (et != (struct ETask *)CHILD_NOTNEW)
    {
        if(status)
        {
            *status = et->et_Result1;
        }
        ret = et->et_UniqueID;
        ChildFree(et->et_UniqueID);
    }
    else
        errno = ECHILD;

    return ret;
}
