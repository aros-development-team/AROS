/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    spavnv() function, used to spawn new processes.
*/

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

#include "__errno.h"
#include "__upath.h"

typedef struct
{
    BPTR command;
    LONG returncode;
} childdata_t;

AROS_UFP3(static LONG, wait_entry,
AROS_UFPA(char *, argstr,A0),
AROS_UFPA(ULONG, argsize,D0),
AROS_UFPA(struct ExecBase *,SysBase,A6));

static BPTR DupFHFromfd(int fd, ULONG mode);
static char *join_args(char * const *argv);
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
    int ret = -1;

    if (path == NULL || argv == NULL || argv[0] == NULL)
    {
        errno = EFAULT;

	return -1;
    }

    apath = __path_u2a(path);
    if (!apath)
        return -1;

    errno = 0;

    switch (mode)
    {
        case P_WAIT:
	{
	    BPTR in, out, err;
	    childdata_t childdata;

	    struct TagItem tags[] =
	    {
	        { NP_Entry,       (IPTR)wait_entry },
		{ NP_Input,       0                }, /* 1 */
		{ NP_Output,      0                }, /* 2 */
		{ NP_Error,       0                }, /* 3 */
		{ NP_Arguments,   0                }, /* 4 */
		{ NP_CloseInput,  FALSE            },
		{ NP_CloseOutput, FALSE            },
		{ NP_CloseError,  FALSE            },
		{ NP_FreeSeglist, FALSE            },
		{ NP_Cli,         TRUE             },
		{ NP_Synchronous, TRUE             },
		{ NP_Name,        (IPTR)argv[0]    },
		{ NP_UserData,    (IPTR)&childdata },
		{ TAG_DONE,       0                }
	    };

	    /* The helper entry function takes the loadseg result in
	       the usedsata field of the task structure.  */
	    childdata.command = LoadSeg(apath);
	    if (!childdata.command)
	        goto err_wait;

	    D(bug("Command loaded = %s\n", apath));

	    tags[4].ti_Data = (IPTR)join_args(&argv[1]);
	    if (!tags[4].ti_Data)
	        goto err_wait;

	    D(bug("Args joined = %s\n", (char *)tags[4].ti_Data));

	    in  = DupFHFromfd(STDIN_FILENO,  FMF_READ);
	    out = DupFHFromfd(STDOUT_FILENO, FMF_WRITE);
	    err = DupFHFromfd(STDERR_FILENO, FMF_WRITE);

	    D(bug("in = %p - out = %p - err = %p\n", BADDR(in), BADDR(out), BADDR(err)))

	    if (in)  tags[1].ti_Data = (IPTR)in;
	    else     tags[1].ti_Tag  = TAG_IGNORE;
	    if (out) tags[2].ti_Data = (IPTR)out;
	    else     tags[2].ti_Tag  = TAG_IGNORE;
	    if (err) tags[3].ti_Data = (IPTR)err;
	    else     tags[3].ti_Tag  = TAG_IGNORE;


	    if (CreateNewProc(tags) != NULL)
	        ret = childdata.returncode;
	    else
	        ret = -1;

	    D(bug("Process created successfully: %s\n", ret == -1 ? "NO" : "YES"))

	    Close(in); Close(out); Close(err);

	    err_wait:
            UnLoadSeg(childdata.command);

	    D(bug("Command unloaded\n"));
            break;
	}

	default:
	    ret = -1;
	    errno = ENOSYS;
    }

    if (ret == -1 && errno == 0)
        errno = IoErr2errno(IoErr());

    return ret;
}


AROS_UFH3(static LONG, wait_entry,
AROS_UFHA(char *, argstr,A0),
AROS_UFHA(ULONG, argsize,D0),
AROS_UFHA(struct ExecBase *,SysBase,A6))
{
    struct DosLibrary *DOSBase;
    LONG rc = -1;
    childdata_t *childdata = (childdata_t *)FindTask(NULL)->tc_UserData;

    DOSBase = (struct DosLibrary *)OpenLibrary(DOSNAME, 39);

    if (DOSBase)
    {
        rc = RunCommand
	(
	    childdata->command, Cli()->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT, argstr, argsize
	);

	CloseLibrary((struct Library *)DOSBase);

    }

    childdata->returncode = rc;
    return rc;
}

#include "__open.h"
static BPTR DupFHFromfd(int fd, ULONG mode)
{
    fdesc *fdesc = __getfdesc(fd);
    BPTR ret = MKBADDR(NULL);

    if (fdesc != NULL && fdesc->fh != MKBADDR(NULL))
    {
        BPTR olddir = CurrentDir(fdesc->fh);
        ret = Open("", mode);
        CurrentDir(olddir);
    }

    return ret;
}

/* Join all elements of an argv array so to build one big string with
   all elements chained one after the other one.

   The resulting string's pointer is valid only until a subsequent call
   to this function.  */
static char *join_args(char * const *argv)
{
  char *last_arg_ptr, *args;
  size_t size = 0;
  int argc;

  if (!argv)
      return NULL;

  if (!argv[0])
      return "";

  if (!argv[1])
      return argv[0];


  for (argc = 0; argv[argc] != NULL; argc++)
      size += strlen(argv[argc]);

  #define __args (__get_arosc_privdata()->acpd_joined_args)
  args = __args = realloc_nocopy(__args, size+argc);
  if (!args)
      return NULL;

  last_arg_ptr = args;
  for (argc = 0; argv[argc] != NULL; argc++)
  {
      size = strlen(argv[argc]);

      memcpy(last_arg_ptr, argv[argc], size);

      last_arg_ptr += size + 1;

      last_arg_ptr[-1] = ' ';
  }

  last_arg_ptr[-1] = '\0';

  return args;
}
