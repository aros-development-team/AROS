/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function system().
*/

#include "__arosc_privdata.h"

#include <dos/dos.h>
//#include <dos/filesystem.h>
#include <proto/dos.h>
//#include <utility/tagitem.h>
#include <unistd.h>
//#include <sys/types.h>
#include <sys/wait.h>

#include "__errno.h"
#include "__open.h"
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
    BPTR lock;
    APTR old_proc_window;
    struct Process *me;

    if (!__doupath)
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
	execl((__doupath ? "/bin/sh" : "bin:sh"), "sh", "-c", string, (char *) NULL);
	_exit(127);
    }
    else
    {
        return -1;
    }
}
	
static int system_no_sh(const char *string)
{
    struct arosc_privdata *pdata = __get_arosc_privdata();
    const char *apath;
    char *args, *cmd, *fullcmd;
    fdesc *in, *out, *err;
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
    
    in = pdata->acpd_fd_array[STDIN_FILENO];
    out = pdata->acpd_fd_array[STDOUT_FILENO];
    err = pdata->acpd_fd_array[STDERR_FILENO];
    
    ret = (int)SystemTags
    (
         fullcmd,
         SYS_Input, (IPTR)(in ? in->fcb->fh : NULL),
         SYS_Output, (IPTR)(out ? out->fcb->fh : NULL),
         SYS_Error, (IPTR)(err ? err->fcb->fh : NULL),
         NULL
    );

    free(fullcmd);
    
    if (ret == -1)
        errno = IoErr2errno(IoErr());

    return ret;
} /* system */

