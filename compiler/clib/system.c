/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function system().
*/

#include "__arosc_privdata.h"

#include <dos/dos.h>
#include <proto/dos.h>
//#include <utility/tagitem.h>
#include <unistd.h>
#include <errno.h>
//#include <sys/types.h>
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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    BPTR lock;
    APTR old_proc_window;
    struct Process *me;

    if (!aroscbase->acb_doupath)
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
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
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
	execl((aroscbase->acb_doupath ? "/bin/sh" : "bin:sh"), "sh", "-c", string, (char *) NULL);
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

    D(bug("[system_no_sh]in: %p, in->fcb->fh: %p\n", in, BADDR(in->fcb->fh)));
    D(bug("[system_no_sh]out: %p, out->fcb->fh: %p\n", out, BADDR(out->fcb->fh)));
    D(bug("[system_no_sh]err: %p, err->fcb->fh: %p\n", err, BADDR(err->fcb->fh)));

    infh = in ? in->fcb->fh : BNULL;
    outfh = out ? out->fcb->fh : BNULL;
    errfh = err ? err->fcb->fh : BNULL;
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
        errno = __arosc_ioerr2errno(IoErr());

    return ret;
} /* system */

