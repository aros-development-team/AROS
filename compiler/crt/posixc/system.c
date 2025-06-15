/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function system().
*/

#include <aros/debug.h>

#include <dos/dos.h>
#include <proto/dos.h>

#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "__posixc_intbase.h"

#include "__fdesc.h"
#include "__upath.h"

static int system_sh(const char *string);
static int system_no_sh(const char *string);

/*****************************************************************************

    NAME */
#include <stdlib.h>

        int system (

/*  SYNOPSIS */
        const char *string)

/*  FUNCTION
        Executes a command specified by the string argument.

        If the string is `NULL`, the function returns 1 to indicate that
        a command processor is available.

        If the string is non-NULL, the command is passed to a shell (if
        available) or executed directly if no shell is present.

    INPUTS
        string - Command string to execute, or NULL to check for shell support.

    RESULT
        On success, returns the command's exit status.
        On error, returns -1 and sets `errno` appropriately.
        If `string` is NULL, returns 1 (indicating a shell is available).

    NOTES
        - On AROS, if the environment supports it, the command will be
          executed through `/bin/sh` (or `bin:sh`).
        - If no shell is found, the command is executed directly using the
          AROS `SystemTags()` interface.
        - The implementation ensures standard input/output/error are passed
          through to the new process.
        - Command paths and arguments are translated from UNIX-style to
          Amiga-style if necessary.

    EXAMPLE
        system("ls -l /");

    BUGS
        - Argument splitting and quoting are simplistic; edge cases may break.
        - Behavior depends on the shell being available in `bin:sh`.

    SEE ALSO
        exec(), popen(), fork(), execl(), waitpid()

    INTERNALS
        - Checks if the shell is available by trying to lock `bin:sh`.
        - Uses `vfork()` and `execl()` when a shell is available.
        - Falls back to `SystemTags()` for direct command execution.
        - Accesses standard file descriptors using internal `fdesc` structs.

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

