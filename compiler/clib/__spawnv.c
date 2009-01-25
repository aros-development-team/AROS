/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    spavnv() function, used to spawn new processes.
*/
#define DEBUG 0
#include "__arosc_privdata.h"

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <aros/debug.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/syscall.h>
#include <sys/wait.h>

/*****************************************************************************

    NAME */
#include <process.h>

        int __spawnv(

/*  SYNOPSIS */
         int         mode,
	 const char *filename,
         int         searchpath,
         char *const argv[])

/*  FUNCTION
	Spawn a child process, given a vector of arguments and an already LoadSeg()'d executable.

    INPUTS
	mode - the way the child process has to be loaded, and how the parent has to behave
	       after the child process is initiated. Specify one of the following values:

	       P_WAIT    - the child program is loaded into memory, then it's executed while
	                   the parent process waits for it to terminate, at which point the
			   patent process resumes execution.

	       P_NOWAIT  - the parent program is executed concurrently with the new child process.

	       P_OVERLAY - teplace the parent program with the child program in memory and then
	                   execute the child. The parent program will never be resumed. This
			   mode is equivalent to calling one of the exec*() functions.

	filename  - command to execute

	searchpath - boolean to indicate if path should be searched for command

	argv - a pointer to a NULL terminated array of strings representing arguments to pass
	       to the child process. The first entry in the array is conventionally the name of
	       the program to spawn, but in any case it must _never_ be NULL, and the argv
	       pointer itself must never be NULL either.

    RESULT

	If P_WAIT is specified, then the return code of the child program is returned.
	If instead P_NOWAIT is used, then the pid of the newly created process is returned.
	Finally, if P_OVERLAY is used, the function doesn't return unless an error has occurred,
	in which case -1 is returned also for the other modes and the global errno variable will
	hold the proper error code.

    NOTES

        The way the child process behaves regarding parent's file descriptors, signal handlers
	and so on is the same way it would behave with one of the exec*(3) functions.
	This, for one, means that all filedescriptors are inherited except the ones which have
	the close-on-exec flag set.

    EXAMPLE

    BUGS

    SEE ALSO
	execl(), execle(), execlp(), execlpe(), execv(), execve(), execvp(), execvpe(), getenv(),
	putenv(), setenv(), spawn(), spawnl(), spawnle(), spawnlp(), spawnlpe(), spawnp(),
	spawnve(), spawnvp(), spawnvpe(), wait(), waitpid()

    INTERNALS

        For now only the stdin, stout and stderr file descriptors are inherited, and signals
	are not handled yet.

******************************************************************************/
{
    switch (mode)
    {
        case P_NOWAIT:
        case P_WAIT:
        {
            pid_t pid;
            int ret = 0;

            pid = vfork();
            
            D(bug("__spawnv: vfork pid = %d\n", pid));
            
            if (pid > 0)
            {
                if (ret != 0)
                {
                    D(bug("__spawnv: From parent child returned %d from execv,\n"
                          "          errno = %d\n", ret, errno));
                    /* Child exec did return => error */
                    return -1;
                }
                
                if (mode == P_WAIT)
                {
                    int status;
                    
                    waitpid(pid, &status, 0);
                    
                    if (WIFEXITED(status))
                        return WEXITSTATUS(status);
                    else
                        /* FIXME: Is this the right thing to do ? */
                        return -1;
                }
                else /* mode == P_NOWAIT */
                    return pid;
            }
            else if (pid == 0)
            {
                if (searchpath)
                    ret = execvp(filename, argv);
                else
                    ret = execv(filename, argv);

                D(bug("__spawnv: Child exec returned %d\n", ret));
                
                _exit(0);
            }
            else /* Error in vfork */
                return -1;
        }
        break;
        
        case P_OVERLAY:
        {
            if (searchpath)
                return execvp(filename, argv);
            else
                return execv(filename, argv);
            
            assert(0);
        }
        break;
        
        default:
        errno = EINVAL;
        return -1;
    }

    assert(0); /* Should not be reached */
    errno = ENOSYS;
    return -1;
}
