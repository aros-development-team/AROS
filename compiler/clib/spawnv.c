/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    spavnv() function, used to spawn new processes.
*/

#include <proto/dos.h>
#include <errno.h>
#include <aros/debug.h>

#include "__upath.h"
#include "__spawnv.h"

/*****************************************************************************

    NAME */
#include <process.h>

        int spawnv(

/*  SYNOPSIS */
         int         mode,
	 const char *path,
         char *const argv[])

/*  FUNCTION
	Spawn a child process, given a vector of arguments

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

	path - the full path name of the executable.

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
    const char *apath;

    if (path == NULL || argv == NULL || argv[0] == NULL)
    {
        errno = EFAULT;

	return -1;
    }

    apath = __path_u2a(path);
    if (!apath)
        return -1;

    return __spawnv(mode, LoadSeg(apath), argv);
}
