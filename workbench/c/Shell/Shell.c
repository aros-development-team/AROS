/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    The shell program.
 */

/******************************************************************************

    NAME

	Shell

    SYNOPSIS

	COMMAND/K/F,FROM

    LOCATION

	C:

    FUNCTION

	Start a shell (interactive or background).

    INPUTS

	COMMAND  --  command line to execute

	FROM     --  script to invoke before user interaction


    RESULT

    HISTORY

    Sep 2010 - rewrite of the convertLine function.

    Jul 2010 - improved handling of $ and `: things like cd SYS:Olle/$pelle
	       work now. Non-alphanumerical var-names must be enclosed in
	       braces.

    Feb 2008 - initial support for .key/bra/ket/dot/dollar/default.

    EXAMPLE

	Shell FROM S:Startup-Sequence

	Starts a shell and executes the startup script.

    BUGS

    SEE ALSO

    Execute, NewShell

    INTERNALS

    The prompt support is not using SetCurrentDirName() as this function
    has improper limitations. More or less the same goes for GetProgramName().

******************************************************************************/

/* TODO:
   Alias [] support
   Break support (and +(0L) before execution) -- CreateNewProc()?
 */

#include <dos/dos.h>
#include <dos/stdio.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <libraries/expansionbase.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <ctype.h>

#define DEBUG 0
#include <aros/debug.h>

#include "Shell.h"

static void PrintBanner(struct DosLibrary *DOSBase)
{
    PutStr
    (
	"AROS - The AROS Research Operating System\n"
	"Copyright © 1995-2011, The AROS Development Team. "
	"All rights reserved.\n"
	"AROS is licensed under the terms of the "
	"AROS Public License (APL),\n"
	"a copy of which you should have received "
	"with this distribution.\n"
	"Visit http://www.aros.org/ for more information.\n"
    );
}

BOOL isInteractive(struct CommandLineInterface *cli)
{
    return !cli->cli_Background && cli->cli_CurrentInput == cli->cli_StandardInput;
}

AROS_ENTRY(__startup ULONG, ShellStart,
	   AROS_UFHA(char *, argstr, A0),
	   AROS_UFHA(ULONG, argsize, D0),
	   struct ExecBase *, SysBase)
{
    AROS_USERFUNC_INIT

    ShellState *ss;
    LONG error;
    BOOL isBootShell;
    BOOL isBannerDone;
    APTR DOSBase;
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR *segArray, mySeg;

    D(bug("[Shell] executing\n"));

    segArray = BADDR(me->pr_SegList);
    mySeg = segArray[3];
    segArray[4] = segArray[3];
    segArray[3] = BNULL;
    segArray[0] = (BPTR)3;

    DOSBase = TaggedOpenLibrary(TAGGEDOPEN_DOS);

    ss = AllocMem(sizeof(ShellState), MEMF_CLEAR);
    if (!ss) {
    	SetIoErr(ERROR_NO_FREE_STORE);
    	CloseLibrary(DOSBase);
    	return RETURN_FAIL;
    }

    setPath(BNULL, DOSBase);

    ss->cliNumber = me->pr_TaskNum;
    cliVarNum("process", ss->cliNumber, DOSBase);

    isBootShell = (strcmp(me->pr_Task.tc_Node.ln_Name, "Boot Shell") == 0) && ss->cliNumber == 1;
    isBannerDone = FALSE;

    initDefaultInterpreterState(ss);

    if (isBootShell) {
    	struct ExpansionBase *ExpansionBase = (struct ExpansionBase*)TaggedOpenLibrary(TAGGEDOPEN_EXPANSION);
        SetPrompt("%N> ");
	if (ExpansionBase && !(ExpansionBase->Flags & EBF_SILENTSTART)) {
	    PrintBanner(DOSBase);
	    isBannerDone = TRUE;
	}
	CloseLibrary((struct Library *)ExpansionBase);
    }

    if (argsize > 0)
    {
	Buffer in = { argstr, argsize, 0, 0 };
	Buffer out = {0};

	if ((error = Redirection_init(ss)) == 0)
	{
	    D(bug("[Shell] running command: %s\n", in->buf));
	    error = checkLine(ss, &in, &out, TRUE, DOSBase);
	    Redirection_release(ss, DOSBase);

	    bufferFree(&in);
	    bufferFree(&out);
	}
    } else
	error = interact(ss, isBootShell, isBannerDone, DOSBase);

    D(bug("[Shell] exiting, error = %d\n", error));
    
    CloseLibrary(DOSBase);

    FreeMem(ss, sizeof(ShellState));

    segArray[3] = mySeg;

    return error ? RETURN_FAIL : RETURN_OK;

    AROS_USERFUNC_EXIT
}

/* First we execute the script, then we interact with the user */
LONG interact(ShellState *ss, BOOL isBootShell, BOOL isBannerDone, APTR DOSBase)
{
    struct CommandLineInterface *cli = Cli();
    Buffer in = {0}, out = {0};
    BOOL moreLeft = FALSE;
    LONG error = 0;

    if (!cli->cli_Background)
    {
	SetVBuf(Output(), NULL, BUF_FULL, -1);
	if (isBootShell)
	{
	    if (!isBannerDone)
	        PrintBanner(DOSBase);
	}
	else
	    Printf("New Shell process %ld\n", ss->cliNumber);

	SetVBuf(Output(), NULL, BUF_LINE, -1);
    }

    /* pre-allocate input buffer */
    if ((error = bufferAppend("?", 1, &in))) /* FIXME drop when readLine ok */
	return error;

    do {
	if ((error = Redirection_init(ss)) == 0)
	{
	    cliPrompt(ss, DOSBase);

	    bufferReset(&in); /* reuse allocated buffers */
	    bufferReset(&out);

	    error = readLine(cli, &in, &moreLeft, DOSBase);

	    if (error == 0 && in.len > 0)
		error = checkLine(ss, &in, &out, TRUE, DOSBase);

	    if (!isInteractive(cli)) /* stop script ? */
	    {
		if (error || (cli->cli_ReturnCode >= cli->cli_FailLevel))
		    moreLeft = FALSE;

		if (CheckSignal(SIGBREAKF_CTRL_D))
		{
		    PrintFault(ERROR_BREAK, "Shell");
		    moreLeft = FALSE;
		}
	    }

	    Redirection_release(ss, DOSBase);
	}

	if (moreLeft)
	    continue;

	popInterpreterState(ss);

	if (isInteractive(cli))
	{
	    Printf("Process %ld ending\n", ss->cliNumber);
	    Flush(Output());
	    break;
	}

	if (AROS_BSTR_strlen(cli->cli_CommandFile))
	{
	    DeleteFile(AROS_BSTR_ADDR(cli->cli_CommandFile));
	    AROS_BSTR_setstrlen(cli->cli_CommandFile, 0);
	}

	if (cli->cli_Background)
	    break;

	D(bug("[Shell] Closing CLI input 0x%p\n", cli->cli_CurrentInput));
	Close(cli->cli_CurrentInput);

	cli->cli_CurrentInput = cli->cli_StandardInput;
	cli->cli_Interactive = TRUE;
	moreLeft = TRUE;

	D(bug("[Shell] Flushing output 0x%p, error 0x%p\n", Output(), ErrorOutput()));
	Flush(Output());
	Flush(ErrorOutput());
    } while (moreLeft);

    bufferFree(&in);
    bufferFree(&out);

    return error;
}

/* Take care of one command line */
LONG checkLine(ShellState *ss, Buffer *in, Buffer *out, BOOL echo, APTR DOSBase)
{
    struct CommandLineInterface *cli = Cli();
    BOOL haveCommand = FALSE;
    LONG result;

    if ((result = convertLine(ss, in, out, &haveCommand, DOSBase)) == 0)
    {
	/* Only a comment or dot command ? */
	if (haveCommand == FALSE)
	    goto exit;

	if (echo)
	    cliEcho(ss, out->buf, DOSBase);

	/* Add terminating linefeed */
	bufferAppend("\n", 1, out);

	/* OK, we've got a command. Let's execute it! */
	result = executeLine(ss, out->buf, DOSBase);

	SelectInput(cli->cli_StandardInput);
	SelectOutput(cli->cli_StandardOutput);
    }
    else
    {
	PrintFault(result, haveCommand ? ss->command + 2 : NULL);
	cli->cli_ReturnCode = RETURN_ERROR;
	cli->cli_Result2 = result;
    }

exit:
    /* FIXME error handling is bullshit */

    cliVarNum("RC", cli->cli_ReturnCode, DOSBase);
    cliVarNum("Result2", cli->cli_Result2, DOSBase);

    if (cli->cli_Interactive)
    {
    	struct Process *me = (struct Process *)FindTask(NULL);

	Flush(me->pr_CES);
	Flush(Output());
    }

    return result;
}

/* Function: unloadCommand
 *
 * Action:   Free the resources held by a (loaded) command.
 *
 * Input:    ShellState	   *ss              --  this state
 *           BPTR	    commandSeg      --  segment of the program to unload
 *           BOOL	    homeDirChanged  --  home changed flag
 *
 * Output:   --
 */
static void unloadCommand(ShellState *ss, BPTR commandSeg,
			  BOOL homeDirChanged, BOOL residentCommand,
			  APTR DOSBase)
{
    struct CommandLineInterface *cli = Cli();

    if (homeDirChanged)
	UnLock(SetProgramDir(ss->oldHomeDir));

    if (!cli->cli_Module)
	return;

    if (residentCommand)
    {
	struct Segment *residentSeg = (struct Segment *)BADDR(commandSeg);

	Forbid();

	/* Decrease usecount */
	if (residentSeg->seg_UC > 0)
	    residentSeg->seg_UC--;

	Permit();
    }
    else
	UnLoadSeg(commandSeg);
}

/* Function: loadCommand
 *
 * Action:   Load a command, searching the paths, C: and the resident lists.
 *
 * Input:    ShellState    *ss              --  this state
 *           STRPTR	    commandName     --  the command to load
 *           BOOL	   *homeDirChanged  --  home changed result
 *           BPTR	   *scriptLock	    --	lock of script if one
 *
 * Output:   BPTR  --  segment of the loaded command or NULL if there was an
 *                     error
 */
static BPTR loadCommand(ShellState *ss, STRPTR commandName, BPTR *scriptLock,
			BOOL *homeDirChanged, BOOL *residentCommand,
			APTR DOSBase)
{
    struct CommandLineInterface *cli = Cli();
    BPTR oldCurDir;
    BPTR commandSeg = BNULL;
    BPTR  *paths;
    struct Segment *residentSeg;
    BOOL absolutePath = strpbrk(commandName, "/:") != NULL;
    BPTR file;
    LONG err = 0;

    /* We check the resident lists only if we do not have an absolute path */
    if (!absolutePath)
    {
	Forbid();

	/* Check regular list first... */
	residentSeg = FindSegment(commandName, NULL, FALSE);

	if (residentSeg == NULL)
	{
	    /* ... then the system list */
	    residentSeg = FindSegment(commandName, NULL, TRUE);
	}

	if (residentSeg != NULL)
	{
	    /* Can we use this command? */
	    if (residentSeg->seg_UC == CMD_INTERNAL || residentSeg->seg_UC >= 0)
	    {
		if (residentSeg->seg_UC >= 0)
		    residentSeg->seg_UC++;

		Permit();
		*residentCommand = TRUE;
		return MKBADDR(residentSeg);
	    }
	}

	Permit();
    }

    oldCurDir = CurrentDir(BNULL);
    CurrentDir(oldCurDir);

    file = Open(commandName, MODE_OLDFILE);
    err = IoErr();

    if (!file)
    {
	if
	(
	    absolutePath ||                 /* If this was an absolute path, we don't check the paths set by
					       'path' or the C: multiassign */
	    err == ERROR_OBJECT_IN_USE  /* The object might be exclusively locked */
	)
	    return BNULL;

	/* Search the command in the path */
	for
	(
	    paths = (BPTR *)BADDR(cli->cli_CommandDir);
	    file == BNULL && paths != NULL;
	    paths = (BPTR *)BADDR(paths[0])    /* Go on with the next path */
	)
	{
	    CurrentDir(paths[1]);
	    file = Open(commandName, MODE_OLDFILE);
	}

	/* The last resort -- the C: multiassign */
	if (!file)
	{
	    commandName -= 2;
	    file = Open(commandName, MODE_OLDFILE);
	}
    }

    if (file)
    {
	commandSeg = LoadSeg(commandName);
	err = IoErr();

	if (commandSeg)
	{
	    BPTR lock = ParentOfFH(file);

	    if (lock)
	    {
		ss->oldHomeDir = SetProgramDir(lock);
		*homeDirChanged = TRUE; /* TODO merge */
	    }
	}
	/* Do not attempt to execute corrupted executables (BAD_HUNK)
	 * Do not swallow original error code */
	if (!commandSeg && err == ERROR_NOT_EXECUTABLE) {
	    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
	    if (fib && ExamineFH(file, fib) && (fib->fib_Protection & FIBF_SCRIPT))
	    {
		commandSeg = LoadSeg("C:Execute");
		if (commandSeg) {
		    *scriptLock = Lock(commandName, SHARED_LOCK);
		    if (*scriptLock == BNULL) {
		    	UnLoadSeg(commandSeg);
		    	commandSeg = BNULL;
		    }
		}
	    }
	    else
		err = ERROR_FILE_NOT_OBJECT;
	    FreeDosObject(DOS_FIB, fib);
	}

	Close(file);
    } else
    	err = IoErr();

    CurrentDir(oldCurDir);
    SetIoErr(err);

    return commandSeg;
}

/* Execute one command */
LONG executeLine(ShellState *ss, STRPTR commandArgs, APTR DOSBase)
{
    struct CommandLineInterface *cli = Cli();
    STRPTR command = ss->command + 2;
    BOOL homeDirChanged = FALSE, residentCommand = FALSE;
    BPTR module, scriptLock = BNULL;
    LONG error = 0;
    TEXT *cmd;

    D(bug("[Shell] executeLine: %s %s\n", command, commandArgs));

    cmd = AllocVec(4096 * sizeof(TEXT), MEMF_ANY);
    if (!cmd)
    	return ERROR_NO_FREE_STORE;

    module = loadCommand(ss, command, &scriptLock,
			 &homeDirChanged, &residentCommand, DOSBase);

    /* Set command name even if we couldn't load the command to be able to
       report errors correctly */
    SetProgramName(command);

    if (module)
    {
	struct Process *pr = (struct Process *) FindTask(NULL);
	ULONG defaultStack = cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT;
	STRPTR oldtaskname = pr->pr_Task.tc_Node.ln_Name;
	ULONG mem_before = 0;
	ULONG sig_before = pr->pr_Task.tc_SigAlloc;
	ULONG sig_after;
	ULONG sigmask;
	BYTE sigbit;

	BPTR seglist = residentCommand ? ((struct Segment *)BADDR(module))->seg_Seg : module;

	STRPTR dst = cmd, src;
	LONG len = 0;

	if (scriptLock)
	{
	    *dst++ = '"';
	    if (NameFromLock(scriptLock, dst, FILE_MAX) == 0) {
	    	error = IoErr(); /* bad FS handler ? */
	    	goto errexit;
	    }
	    while (*++dst != '\0');

	    *dst++ = '"';
	    *dst++ = ' ';
	    UnLock(scriptLock);
	    len = dst - cmd;
	}

	src = commandArgs;
	for (; src && *src != '\0'; ++dst, ++src, ++len)
	    *dst = *src;
	*dst = '\0';

	D(bug("[Shell] command loaded: len=%d, args=%s\n", len, cmd));
	SetIoErr(0); /* Clear error before we execute this command */
	SetSignal(0, SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);

	cli->cli_Module = seglist;
	pr->pr_Task.tc_Node.ln_Name = command;

	mem_before = FindVar("__debug_mem", LV_VAR) ? AvailMem(MEMF_ANY) : 0;
	cli->cli_ReturnCode = RunCommand(seglist, defaultStack, cmd, len);

	/*
	    Check if running the command has changed signal bits of the Shell
	    process. If there is a difference the signals will be set or freed
	    to avoid that the Shell runs out of free signals.
	 */
	sig_after = pr->pr_Task.tc_SigAlloc;
	if (sig_before != sig_after)
	{
	    for (sigbit = 0; sigbit < 32; sigbit++)
	    {
		sigmask = 1L << sigbit;
		if ((sig_before & sigmask) && !(sig_after & sigmask))
		{
		    /* Command has deleted signal => set it */
		    Printf("*** Command returned with freed signal 0x%lx\n", sigmask);
		    AllocSignal(sigbit);
		}
		else if (!(sig_before & sigmask) && (sig_after & sigmask))
		{
		    /* Command has set signal => free it */
		    Printf("*** Command returned with unfreed signal 0x%lx\n", sigmask);
		    FreeSignal(sigbit);
		}
	    }
	}

	if (mem_before)
	{
	    ULONG mem_after = AvailMem(MEMF_ANY);
	    Printf("Memory leak of %lu bytes\n", mem_before - mem_after);
	}

	D(bug("[Shell] returned %d: %s\n", cli->cli_ReturnCode, command));
	pr->pr_Task.tc_Node.ln_Name = oldtaskname;
	unloadCommand(ss, module, homeDirChanged, residentCommand, DOSBase);

	cli->cli_Result2 = IoErr();
    }
    else
    {
	/* Implicit CD ? */
	if (ss->newIn || ss->newOut) {
	    error = ERROR_TOO_MANY_ARGS;
	    goto errexit;
	}

	/* SFS returns ERROR_INVALID_COMPONENT_NAME if you try to open "" */
	error = IoErr();

	if (error == ERROR_OBJECT_WRONG_TYPE ||
	    error == ERROR_OBJECT_NOT_FOUND ||
	    error == ERROR_INVALID_COMPONENT_NAME)
	{
	    BPTR lock = Lock(command, SHARED_LOCK);

	    if (lock)
	    {
		struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);

		if (fib)
		{
		    if (Examine(lock, fib))
		    {
			if (fib->fib_DirEntryType > 0)
			{
			    setPath(lock, DOSBase);
			    lock = CurrentDir(lock);
			}
			else
			    SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		    }

		    FreeDosObject(DOS_FIB, fib);
		}

		/* UnLock the old currentdir */
		UnLock(lock);
	    }
	}

	if ((error = IoErr()))
	{
	    cli->cli_Result2 = error;
	    PrintFault(error, command);
	}
    }
errexit:
    FreeVec(cmd);
    return error;
}

void setPath(BPTR lock, APTR DOSBase)
{
    BPTR dir;
    STRPTR buf;
    ULONG i;

    if (lock)
	dir = lock;
    else
	dir = CurrentDir(BNULL);

    for (i = 256; ; i += 256)
    {
	buf = AllocVec(i, MEMF_ANY);

	if (buf == NULL)
	    break;

	if (NameFromLock(dir, buf, i))
	{
	    SetCurrentDirName(buf);
	    FreeVec(buf);
	    break;
	}

	FreeVec(buf);
    }

    if (lock == BNULL)
	CurrentDir(dir);
}
