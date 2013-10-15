/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function system().
*/

#include "__posixc_intbase.h"

#include <dos/dos.h>
#include <proto/dos.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "__fdesc.h"
#include "__upath.h"

#define DEBUG 0
#include <aros/debug.h>

static int system_sh(const char *string);
static int system_no_sh(const char *string);

/*****************************************************************************

    NAME */
#include <stdlib.h>

	int system (

/*  SYNOPSIS */
	const char *string)

/*  FUNCTION
        Execute a command string. If string is NULL then 1 will be returned.

    INPUTS
        string - command to execute or NULL

    RESULT
        Return value of command executed. If value < 0 errno indicates error.
        1 is return if string is NULL.

    NOTES
        The system() version of posixc.library will translate UNIX<>Amiga
        if applicable as well as use a shell for executing text batch
        commands.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    BPTR lock;
    APTR old_proc_window;
    struct Process *me;

    if (!PosixCBase->doupath)
        return system_no_sh(string);
    
    if (string == NULL || string[0] == '\0')
    {
	D(bug("system(cmd=, args=)=1\n"));
	return 1;
    }

    me = (struct Process*) FindTask(NULL);
    old_proc_window = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR) -1;
    lock = Lock((STRPTR) "bin:sh", SHARED_LOCK);
    me->pr_WindowPtr = old_proc_window;
    
    if (lock)
    {
	UnLock(lock);
	return system_sh(string);
    }

    return system_no_sh(string);
} /* system */


static int system_sh(const char *string)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    pid_t pid = vfork();
    int status;

    D(bug("system_sh(%s)\n", string));
    
    if(pid > 0)
    {
	if(waitpid(pid, &status, 0) == -1)
	    return -1;
	return status;
    }
    else if(pid == 0)
    {
	execl((PosixCBase->doupath ? "/bin/sh" : "bin:sh"), "sh", "-c", string, (char *) NULL);
	_exit(127);
    }
    else
    {
        return -1;
    }
}
	
static int system_no_sh(const char *string)
{
    const char *apath;
    char *args, *cmd, *fullcmd;
    fdesc *in, *out, *err;
    BPTR infh, outfh, errfh;
    int ret;

    D(bug("system_no_sh(%s)\n", string));

    args = strdup(string);
    cmd = strsep(&args, " \t\n");
    if (!args)
        args = "";

    D(bug("system(cmd=%s, args=%s)\n", cmd, args));

    apath = __path_u2a(cmd);
    
    fullcmd = malloc(strlen(apath) + strlen(args) + 2);
    strcpy(fullcmd, apath);
    strcat(fullcmd, " ");
    strcat(fullcmd, args);

    free(cmd);
    
    in = __getfdesc(STDIN_FILENO);
    out = __getfdesc(STDOUT_FILENO);
    err = __getfdesc(STDERR_FILENO);

    D(bug("[system_no_sh]in: %p, in->fcb->fh: %p\n", in, BADDR(in->fcb->handle)));
    D(bug("[system_no_sh]out: %p, out->fcb->fh: %p\n", out, BADDR(out->fcb->handle)));
    D(bug("[system_no_sh]err: %p, err->fcb->fh: %p\n", err, BADDR(err->fcb->handle)));

    infh = in ? in->fcb->handle : BNULL;
    outfh = out ? out->fcb->handle : BNULL;
    errfh = err ? err->fcb->handle : BNULL;
    if (outfh == errfh)
        errfh = BNULL;

    D(bug("[system_no_sh]infh: %p, outfh: %p, errfh %p\n",
          BADDR(infh), BADDR(outfh), BADDR(errfh)
    ));

    ret = (int)SystemTags
    (
         fullcmd,
         SYS_Input, (IPTR)(infh),
         SYS_Output, (IPTR)(outfh),
         SYS_Error, (IPTR)(errfh),
         NULL
    );

    free(fullcmd);
    
    if (ret == -1)
        errno = __stdc_ioerr2errno(IoErr());

    return ret;
} /* system */

