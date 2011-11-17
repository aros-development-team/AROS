/*
    Copyright © 2004-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <proto/exec.h>
#include <exec/lists.h>
#include <aros/startup.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "__arosc_privdata.h"

/*****************************************************************************

    NAME */

        pid_t waitpid(

/*  SYNOPSIS */
        pid_t pid,
        int *status,
        int options)

/*  FUNCTION
        Waits for child process with given process id to change state. State
        change is one of the following events: child has exited, child was
        terminated by a signal, child was stopped by a signal, child was
        resumed by a signal.
        
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

        Unless WNOHANG option is set, parent process will be suspended until a
        child changes state. If a child process has already changed state,
        function returns immediately.

    INPUTS
        pid - Process id of the process you want to wait for or -1 to wait for
                any child process
        status - Pointer to int where child status will be stored or NULL if
                you don't want to store status.
        options - ORed zero or more of the following constants:
        
            WNOHANG - return immediately if no child process changed state

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
        wait()

    INTERNALS
        Since POSIX signals are not yet implemented, WIFSIGNALED, WIFSTOPPED
        and WIFCONTINUED macros always return 0. WIFEXITED always returns 1.

        The et_UniqueID field of the ETask structure is used as process id.

******************************************************************************/
{
    struct ETask *et;
    ULONG tid = pid;
    pid_t ret = -1;
    int exchildno;

    D(bug("waitpid(%d, %p, %d)\n", pid, status, options));

    et = GetETask(FindTask(NULL));
    if(!et)
    {
        /* only ETasks are fertile */
        errno = ECHILD;
        return -1;
    }
    
    if (pid < -1 || pid == 0)
    {
        /* process groups not yet supported */
        errno = EINVAL;
        return -1;
    }

    if (pid == -1)
        tid = 0;

    if(tid != 0 && ChildStatus(tid) == CHILD_NOTFOUND)
    {
        /* error if there's no such child */
        errno = ECHILD;
        return -1;
    }

    if (options & ~WNOHANG)
    {
        /* option not yet supported */
        errno = EINVAL;
        return -1;
    }
    
    /* not very pretty, perhaps we need a function for counting dead 
       children? */
    ListLength(&et->et_TaskMsgPort.mp_MsgList, exchildno);
    if ((options & WNOHANG) && (
        (tid != 0 && ChildStatus(tid) != CHILD_EXITED) ||
        (tid == 0 && exchildno == 0)
    ))
    {
        D(bug("waitpid: no dead children\n"));
        /* no dead children atm */
        return 0;
    }

    et = (struct ETask *)ChildWait(tid);
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
    
    D(bug("waitpid: leaving (%d)\n", ret));

    return ret;
}
