/*
    Copyright (C) 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    The shell program.
 */

/******************************************************************************

    NAME

	Shell

    SYNOPSIS

    LOCATION

	L:UserShell-Seg

    FUNCTION

	Start a shell (interactive or background).

    INPUTS

    RESULT

    HISTORY

    Aug 2011 - Use AOS startup packet mechanisms

    Sep 2010 - rewrite of the convertLine function.

    Jul 2010 - improved handling of $ and `: things like cd SYS:Olle/$pelle
	       work now. Non-alphanumerical var-names must be enclosed in
	       braces.

    Feb 2008 - initial support for .key/bra/ket/dot/dollar/default.

    EXAMPLE

	Resident Shell L:UserShell-Seg SYSTEM PURE ADD

	Configures the default User Shell to this shell

    BUGS

    SEE ALSO

    Execute, NewShell, Run

    INTERNALS

    The prompt support is not using SetCurrentDirName() as this function
    has improper limitations. More or less the same goes for GetProgramName().

******************************************************************************/

/* TODO:
   Alias [] support
   Break support (and +(0L) before execution) -- CreateNewProc()?
 */

#define DEBUG 0
#include <dos/dos.h>
#include <dos/stdio.h>
#include <dos/cliinit.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <libraries/expansionbase.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <ctype.h>

#include <aros/debug.h>

#include "Shell.h"

#define IS_SYSTEM ((ss->flags & (FNF_VALIDFLAGS | FNF_SYSTEM)) == (FNF_VALIDFLAGS | FNF_SYSTEM))
#define IS_SCRIPT (cli->cli_CurrentInput != cli->cli_StandardInput)

BOOL setInteractive(struct CommandLineInterface *cli, ShellState *ss)
{
    D(bug("Shell %d: Flags = 0x%x\n", ss->cliNumber, ss->flags));
    D(bug("Shell %d: cli_Interactive = %d\n", ss->cliNumber, cli->cli_Interactive));
    D(bug("Shell %d: cli_Background = %d\n", ss->cliNumber, cli->cli_Background));
    D(bug("Shell %d: cli_CurrentInput = %p\n", ss->cliNumber, cli->cli_CurrentInput));
    D(bug("Shell %d: cli_StandardInput = %p\n", ss->cliNumber, cli->cli_StandardInput));
    if (!cli->cli_Background && IS_SCRIPT)
    	cli->cli_Background = DOSTRUE;

    cli->cli_Interactive = (cli->cli_Background || IS_SCRIPT || IS_SYSTEM) ? DOSFALSE : DOSTRUE;
    D(bug("Shell %d: cli_Interactive => %d\n", ss->cliNumber, cli->cli_Interactive));
    D(bug("Shell %d: cli_Background => %d\n", ss->cliNumber, cli->cli_Background));

    return cli->cli_Interactive;
}

__startup AROS_CLI(ShellStart)
{
    LONG error;
    ShellState *ss;
    APTR DOSBase;
    struct Process *me = (struct Process *)FindTask(NULL);
    struct CommandLineInterface *cli;

    DOSBase = TaggedOpenLibrary(TAGGEDOPEN_DOS);
    if (!DOSBase)
        return RETURN_FAIL;

    D(bug("[Shell] executing\n"));
    ss = AllocMem(sizeof(ShellState), MEMF_CLEAR);
    if (!ss) {
    	SetIoErr(ERROR_NO_FREE_STORE);
    	CloseLibrary(DOSBase);
    	return RETURN_FAIL;
    }

    /* Cache the CLI flags, passed in by the AROS_CLI() macro */
    ss->flags = AROS_CLI_Flags;

    /* Select the input and output streams.
     * We don't use CurrentInput here, as it may
     * contain our script input.
     *
     * Note that if CurrentInput contained, for example:
     * ECHO ?
     * DIR
     *
     * The behaviour would be that ECHO would put its
     * ReadArgs query onto StandardOutput, and expects
     * its input on StandardInput, and then execute DIR.
     *
     * This is AOS compatible behavior. It would be
     * incorrect to assume that ECHO would print "DIR" on
     * StandardOutput
     */
    cli = Cli();

    SelectInput(cli->cli_StandardInput);
    SelectOutput(cli->cli_StandardOutput);
    setPath(BNULL, DOSBase);

    ss->cliNumber = me->pr_TaskNum;
    cliVarNum("process", ss->cliNumber, DOSBase);

    initDefaultInterpreterState(ss);

    if (AROS_CLI_Type == CLI_RUN) {
        FPrintf(cli->cli_StandardError, "[CLI %ld]\n", me->pr_TaskNum);
    }
    if (AROS_CLI_Type == CLI_NEWCLI) {
        FPrintf(cli->cli_StandardOutput, "New Shell process %ld\n", me->pr_TaskNum);
    }

    error = interact(ss, DOSBase);

    D(bug("Shell %d: exiting, error = %ld\n", ss->cliNumber, error));

    if (ss->arg_rd)
        FreeDosObject(DOS_RDARGS, ss->arg_rd);

    FreeMem(ss, sizeof(ShellState));

    /* Make sure Input(), Output() and pr_CES don't
     * point to any dangling files.
     */
    SelectInput(BNULL);
    SelectOutput(BNULL);
    me->pr_CES = BNULL;

    CloseLibrary(DOSBase);

    return error ? RETURN_FAIL : RETURN_OK;
}

/* First we execute the script, then we interact with the user */
LONG interact(ShellState *ss, APTR DOSBase)
{
    struct CommandLineInterface *cli = Cli();
    Buffer in = {0}, out = {0};
    BOOL moreLeft = FALSE;
    LONG error = 0;

    setInteractive(cli, ss);

    /* pre-allocate input buffer */
    if ((error = bufferAppend("?", 1, &in))) /* FIXME drop when readLine ok */
	return error;

    do {
	if ((error = Redirection_init(ss)) == 0)
	{
	    cliPrompt(ss, DOSBase);

	    bufferReset(&in); /* reuse allocated buffers */
	    bufferReset(&out);

	    D(bug("Shell %d: Reading in a line of input...\n", ss->cliNumber));
	    error = readLine(cli, &in, &moreLeft, DOSBase);
	    D(bug("Shell %d: moreLeft=%ld, error=%ld, Line is: %ld bytes (%s)\n", ss->cliNumber, moreLeft, error, in.len, in.buf));

	    if (error == 0 && in.len > 0)
		error = checkLine(ss, &in, &out, TRUE, DOSBase);

            /* The command may have modified cli_Background.
             * C:Execute does that.
             */
	    setInteractive(cli, ss);

	    /* As per AmigaMail Vol 2, "II-65: Writing a UserShell" */
	    if (IS_SYSTEM && !IS_SCRIPT)
	    	moreLeft = FALSE;

	    if (!cli->cli_Interactive)
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

	if (cli->cli_Interactive)
	{
	    Printf("Process %ld ending\n", ss->cliNumber);
	    Flush(Output());
	    break;
	}

	if (IS_SCRIPT) {
	    D(bug("Shell %d: Closing CLI input 0x%08lx\n", ss->cliNumber, cli->cli_CurrentInput));
	    Close(cli->cli_CurrentInput);

	    /* Now that we've closed CurrentInput, we can delete
	     * the CommandFile.
	     */
	    if (AROS_BSTR_strlen(cli->cli_CommandFile))
	    {
		DeleteFile(AROS_BSTR_ADDR(cli->cli_CommandFile));
		AROS_BSTR_setstrlen(cli->cli_CommandFile, 0);
	    }

	    cli->cli_CurrentInput = cli->cli_StandardInput;
	    cli->cli_Background = IsInteractive(cli->cli_CurrentInput) ? DOSFALSE : DOSTRUE;

	    setInteractive(cli, ss);
	}


	/* As per AmigaMail Vol 2, "II-65: Writing a UserShell",
	 * if we were running a SYSTEM command, we're done now.
	 */
	moreLeft = cli->cli_Interactive;

        if (cli->cli_Interactive) {
            D(bug("Shell %d: Flushing output 0x%lx, error 0x%lx\n", ss->cliNumber, Output(), ErrorOutput()));
            Flush(Output());
            Flush(ErrorOutput());
        }
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
        D(bug("convertLine: haveCommand = %ld, out->buf=%s\n", haveCommand, out->buf));
	/* Only a comment or dot command ? */
	if (haveCommand == FALSE)
	    goto exit;

	if (echo)
	    cliEcho(ss, out->buf, DOSBase);

	/* OK, we've got a command. Let's execute it! */
	result = executeLine(ss, out->buf, DOSBase);

	/* If the command changed the cli's definition
	 * of cli_StandardInput/StandardOutput, let's
	 * reflect that.
	 */
	SelectInput(cli->cli_StandardInput);
	SelectOutput(cli->cli_StandardOutput);
    }
    else
    {
        D(bug("convertLine: error = %ld\n", result));
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
	Flush(ErrorOutput());
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

    SetProgramName("");

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

    cli->cli_Module = BNULL;
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

	D(bug("[Shell] command loaded: len=%ld, args=%s\n", len, cmd));
	SetIoErr(0); /* Clear error before we execute this command */
	SetSignal(0, SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);

	cli->cli_Module = seglist;
	pr->pr_Task.tc_Node.ln_Name = command;

	mem_before = FindVar("__debug_mem", LV_VAR) ? AvailMem(MEMF_ANY) : 0;
	cli->cli_ReturnCode = RunCommand(seglist, defaultStack, cmd, len);

	/* Update the state of the cli_Interactive field */
	setInteractive(cli, ss);

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
		    Printf("*** '%s' returned with freed signal 0x%lx\n", command, sigmask);
		    AllocSignal(sigbit);
		}
		else if (!(sig_before & sigmask) && (sig_after & sigmask))
		{
		    /* Command has set signal => free it */
		    Printf("*** '%s' returned with unfreed signal 0x%lx\n", command, sigmask);
		    FreeSignal(sigbit);
		}
	    }
	}

	if (mem_before)
	{
	    ULONG mem_after = AvailMem(MEMF_ANY);
	    Printf("Memory leak of %lu bytes\n", mem_before - mem_after);
	}

	D(bug("[Shell] returned %ld: %s\n", cli->cli_ReturnCode, command));
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
