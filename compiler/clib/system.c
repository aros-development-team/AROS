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

extern BPTR DupFHFromfd(int fd, ULONG mode);

static void syncFilePos(int from_fd, BPTR to_fh)
{
   fdesc *desc;
   BPTR fh;
   LONG offset;

   desc = __getfdesc(from_fd);
   if (desc)
   {
	fh = (BPTR)(desc->fh);
	Flush(fh);
	offset = Seek(fh, 0, OFFSET_CURRENT);

	if (offset > 0)
	    if (Seek(to_fh, offset, OFFSET_BEGINNING) < 0)
		D(bug("system: Seek error: %d\n", IoErr()));
   }
}

static void syncFilePosBack(BPTR from_fh, int to_fd)
{
   fdesc *desc;
   BPTR fh;
   LONG offset;

   desc = __getfdesc(to_fd);
   if (desc)
   {
	fh = (BPTR)(desc->fh);
	Flush(fh);
	offset = Seek(from_fh, 0, OFFSET_CURRENT);

	if (offset > 0)
	    if (Seek(fh, offset, OFFSET_BEGINNING) < 0)
		D(bug("system: Seek error: %d\n", IoErr()));
   }
}

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
    const char *apath;
    char *args, *cmd;
    BPTR seg;
    int ret;

    if (string == NULL || string[0] == '\0')
    {
	D(bug("system(cmd=, args=)=1\n"));
	return 1;
    }

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

    apath = __path_u2a(cmd);
    seg = LoadSeg(apath);
    if (seg == MKBADDR(NULL))
    {
	struct CommandLineInterface *cli = Cli();
	BPTR oldCurDir = CurrentDir(NULL);
	BPTR *paths = BADDR(cli->cli_CommandDir);

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
	    /* UnLock(lockCurDir); */
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
	BPTR in, out, err;
	childdata_t childdata;

	struct TagItem tags[] =
	{
	    { NP_Entry,       (IPTR)wait_entry },
	    { NP_Input,       0                }, /* 1 */
	    { NP_Output,      0                }, /* 2 */
	    { NP_Error,       0                }, /* 3 */
	    { NP_Arguments,   (IPTR)args       }, /* 4 */
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

	in  = DupFHFromfd(STDIN_FILENO,  FMF_READ);
	out = DupFHFromfd(STDOUT_FILENO, FMF_WRITE);
	err = DupFHFromfd(STDERR_FILENO, FMF_WRITE);

	/* isnt the responsability of DupFH/DupLock/Open(""/Lock(""
           to keep files pointers in sync ??? 
           or better, the filesystem ??? */
	if (in)
	{
	    tags[1].ti_Data = (IPTR)in;
	    syncFilePos(STDIN_FILENO, in);
	}
	else
	    tags[1].ti_Tag  = TAG_IGNORE;

	if (in)
	{
	    tags[2].ti_Data = (IPTR)out;
	    syncFilePos(STDOUT_FILENO, out);
	}
	else
	    tags[2].ti_Tag  = TAG_IGNORE;

	if (in)
	{
	    tags[3].ti_Data = (IPTR)err;
	    syncFilePos(STDERR_FILENO, err);
	}
	else
	    tags[3].ti_Tag  = TAG_IGNORE;

	childdata.parent_does_upath = __doupath;

	if (CreateNewProc(tags) != NULL)
	    ret = childdata.returncode;
	else
	{
	    ret = -1;
	    errno = IoErr2errno(IoErr());
	}

	UnLoadSeg(seg);

	if (in)
	{
	    syncFilePosBack(in, STDIN_FILENO);
	    Close(in);
	}

	if (out)
	{
	    syncFilePosBack(out, STDOUT_FILENO);
	    Close(out);
	}

	if (err)
	{
	    syncFilePosBack(err, STDERR_FILENO);
	    Close(err);
	}
    }

    D(bug("system(cmd=%s, args=%s)=%d, errno=%d\n",
	  cmd, args ? args : "", ret, errno));
    free(cmd);
    return ret;
} /* system */

