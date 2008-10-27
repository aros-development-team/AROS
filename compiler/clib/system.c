/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function system().
*/

#include "__arosc_privdata.h"

#include <dos/dos.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "__errno.h"
#include "__open.h"
#include "__spawnv.h"
#include "__upath.h"
#include <process.h>

#define DEBUG 0
#include <aros/debug.h>

/* defined in __spawnv.c */
AROS_UFP3(extern LONG, wait_entry, 
AROS_UFPA(char *, argstr,A0),
AROS_UFPA(ULONG, argsize,D0),
AROS_UFPA(struct ExecBase *,SysBase,A6));

int system_sh(const char *string);
int system_no_sh(const char *string);

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
    
    if(lock)
    {
	UnLock(lock);
	return system_sh(string);
    }
    else
    {
	return system_no_sh(string);
    }
} /* system */

int system_sh(const char *string)
{
    pid_t pid = vfork();
    int status;

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
	
int system_no_sh(const char *string)
{
    CONST_STRPTR apath;
    char *args, *cmd;
    BPTR seg;
    int ret;

    cmd = strdup(string);
    args = cmd;

    while (++args)
    {
	switch (args[0])
	{
	case ' ':
	case '\t':
	case '\n':
	    args[0] = '\0';
	    break;
	case '\0':
	    args = NULL;
	    break;
	}

	if (!args)
	    break;

	if (args[0] == '\0')
	{
	    ++args;
	    break;
	}
    }

    D(bug("system(cmd=%s, args=%s)\n", cmd, args ? args : ""));

    apath = (STRPTR) __path_u2a(cmd);
    seg = LoadSeg(apath);
    if (seg == MKBADDR(NULL))
    {
	struct CommandLineInterface *cli = Cli();
	BPTR oldCurDir = CurrentDir(NULL);
	BPTR *paths = cli ? BADDR(cli->cli_CommandDir) : NULL;

	for (; seg == MKBADDR(NULL) && paths; paths = BADDR(paths[0]))
	{
	    CurrentDir(paths[1]); 
	    seg = LoadSeg(apath);
	}

	if (seg == MKBADDR(NULL))
	{
	    errno = IoErr2errno(IoErr());
	    D(bug("system(cmd=%s, args=%s)=-1, errno=%d\n",
		  cmd, args ? args : "", errno));
	    CurrentDir(oldCurDir);
	    free(cmd);
	    return -1;
	}
	else
	    errno = 0;

	CurrentDir(oldCurDir);
    }
    else
	errno = 0;

    {
	childdata_t childdata;

	struct TagItem tags[] =
	{
	    { NP_Entry,       (IPTR)wait_entry },
	    { NP_Arguments,   (IPTR)args       },
	    { NP_CloseInput,  FALSE            },
	    { NP_CloseOutput, FALSE            },
	    { NP_CloseError,  FALSE            },
	    { NP_FreeSeglist, FALSE            },
	    { NP_Cli,         TRUE             },
	    { NP_Synchronous, TRUE             },
	    { NP_Name,        (IPTR)cmd        },
	    { NP_UserData,    (IPTR)&childdata },
	    { TAG_DONE,       0                }
	};

	childdata.command = seg;
	childdata.ppriv = __get_arosc_privdata();

	if (CreateNewProc(tags) != NULL)
	    ret = childdata.returncode;
	else
	{
	    ret = -1;
	    errno = IoErr2errno(IoErr());
	}

	UnLoadSeg(seg);
    }

    D(bug("system(cmd=%s, args=%s)=%d, errno=%d\n",
	  cmd, args ? args : "", ret, errno));
    free(cmd);
    return ret;
} /* system */

