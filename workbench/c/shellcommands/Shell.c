/*
    Copyright (C) 1995-2010, The AROS Development Team. All rights reserved.
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

  *  Alias [] support
  *  Break support (and +(0L) before execution) -- CreateNewProc()?

 */
#define  DEBUG  1
#define  DEBUG1 1
#include <aros/debug.h>

#include <exec/memory.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/var.h>
#include <dos/filesystem.h>
#include <dos/bptr.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <aros/asmcall.h>
#include <unistd.h>
#include <stdarg.h>

#include <aros/debug.h>

#define SH_GLOBAL_SYSBASE 1
#define SH_GLOBAL_DOSBASE 1
#include <aros/shcommands.h>

#include "Shell.h"
#include "Shell_inline.h" /* inline functions */

static void initDefaultInterpreterState(struct InterpreterState *is)
{
    int i;

    is->argcount = 0;

    for (i =  0; i < MAXARGS; ++i)
    {
        is->argnamelen[i] = 0;
        is->argname[i][0] = '\0';
        is->arg[i]        = (IPTR)NULL;
        is->arglen[i]     = 0;
        is->argdef[i]     = (IPTR)NULL;
        is->argdeflen[i]  = 0;
        is->argtype[i]    = NORMAL;
    }

    is->bra    = '<';
    is->ket    = '>';
    is->dollar = '$';
    is->dot    = '.';

    is->rdargs = NULL;
    is->stack  = NULL;
}

LONG pushInterpreterState(struct InterpreterState *is)
{
    struct InterpreterState *tmp_is;
    tmp_is = (struct InterpreterState *)AllocMem(sizeof(*is), MEMF_LOCAL);

    if (!tmp_is)
        return ERROR_NO_FREE_STORE;

    *tmp_is = *is;
    initDefaultInterpreterState(is);
    is->stack = tmp_is;
    return 0;
}

void popInterpreterState(struct InterpreterState *is)
{
    struct InterpreterState *tmp_is = is->stack;
    LONG i;

    for (i = 0; i < is->argcount; ++i)
        if (is->argdef[i])
            FreeMem((APTR)is->argdef[i], is->argdeflen[i] + 1);

    if (is->rdargs)
    {
        FreeDosObject(DOS_RDARGS, is->rdargs);
        is->rdargs = NULL;
    }

    if (tmp_is)
    {
        *is = *tmp_is;
        FreeMem(tmp_is, sizeof(*is));
    }
    else
        initDefaultInterpreterState(is);
}

AROS_SH1(Shell, 41.3,
         AROS_SHA(STRPTR, , COMMAND, / F, NULL))
{
    AROS_SHCOMMAND_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    /* STRPTR cmdline = SHArg(COMMAND); FIXME garbage here */
    STRPTR cmdline = SHArgLine();
    struct InterpreterState is;
    LONG error;

    D(bug("[Shell] executing shell\n"));
    cli = Cli();
    setPath(NULL);

    is.cliNumber = me->pr_TaskNum;

    if (strcmp(me->pr_Task.tc_Node.ln_Name, "Boot Shell") == 0)
        is.isBootShell = TRUE;
    else
        is.isBootShell = FALSE;

    initDefaultInterpreterState(&is);

    if (is.isBootShell)
        SetPrompt("%N> ");

    if (cmdline && cmdline[0] != '\0')
    {
        struct Redirection rd;
        struct CSource cs = { cmdline, strlen(cmdline), 0 };

        if (Redirection_init(&rd) == 0)
        {
            D(bug("[Shell] running command %s\n", cmdline));
            error = checkLine(&rd, &cs, &is);
            Redirection_release(&rd);
        }

        D(bug("[Shell] command done\n"));
    }
    else
        error = interact(&is);

    D(bug("Exiting shell\n"));
    return error;

    AROS_SHCOMMAND_EXIT
}

/* First we execute the script, then we interact with the user */
LONG interact(struct InterpreterState *is)
{
    LONG  error = 0;
    BOOL  moreLeft = FALSE;

    if (!cli->cli_Background)
    {
        SetVBuf(Output(), NULL, BUF_FULL, -1);
        if (is->isBootShell)
        {
            PutStr
            (
                "AROS - The AROS Research Operating System\n"
                "Copyright © 1995-2010, The AROS Development Team. All rights reserved.\n"
                "AROS is licensed under the terms of the AROS Public License (APL),\n"
                "a copy of which you should have received with this distribution.\n"
                "Visit http://www.aros.org/ for more information.\n"
            );
        }
        else
        {
            IPTR data[] = {(IPTR)is->cliNumber};

            VPrintf("New Shell process %ld\n", data);
        }
        SetVBuf(Output(), NULL, BUF_LINE, -1);
    }

    do
    {
        struct CSource cs = { NULL, 0, 0 };
        struct Redirection rd;

        if (Redirection_init(&rd) == 0)
        {
            if (cli->cli_Interactive)
                printPrompt(is);

            moreLeft = readLine(&cs, cli->cli_CurrentInput);
            if (cs.CS_Buffer)
            {
                error = checkLine(&rd, &cs, is);

                Redirection_release(&rd);
                FreeCSource(&cs);
            }
        }

        if (!moreLeft)
        {
            popInterpreterState(is);

            if (!cli->cli_Interactive)
            {
                Close(cli->cli_CurrentInput);

                if (AROS_BSTR_strlen(cli->cli_CommandFile))
                {
                    DeleteFile(AROS_BSTR_ADDR(cli->cli_CommandFile));
                    AROS_BSTR_setstrlen(cli->cli_CommandFile, 0);
                }

                if (!cli->cli_Background)
                {
                    cli->cli_CurrentInput = cli->cli_StandardInput;
                    cli->cli_Interactive = TRUE;
                    moreLeft = TRUE;
                    Flush(Output());
                    Flush(Error());
                }
            }
        }
    }
    while (moreLeft);

    if (cli->cli_Interactive)
        printFlush("Process %ld ending\n", is->cliNumber);

    return error;
}

/* Take care of one command line */
LONG checkLine(struct Redirection *rd, struct CSource *cs,
               struct InterpreterState *is)
{
    /* The allocation is taken care of by appendString */
    struct CSource   filtered = { NULL, 0, 0 };
    struct LocalVar *lv;
    LONG             result;

    if ((result = convertLine(&filtered, cs, rd, is)) == 0)
    {
        appendString(&filtered, "\n", 1);

        /* Only a comment ? */
        if (!rd->haveCommand)
        {
            result = 0;
            goto exit;
        }

        /* stegerg: Set redirection to default in/out handles */
        if (rd->haveOutRD || rd->haveAppRD)
        {
            LONG mode = rd->haveAppRD ? MODE_READWRITE : MODE_NEWFILE;
            rd->newOut = Open(rd->outFileName, mode);

            if (BADDR(rd->newOut) == NULL)
            {
                result = IoErr();
                PrintFault(result, rd->outFileName);
                goto exit;
            }

            if (rd->haveAppRD)
                Seek(rd->newOut, 0, OFFSET_END);

            rd->oldOut = SelectOutput(rd->newOut);
        }

        if (rd->haveInRD)
        {
            rd->newIn = Open(rd->inFileName, MODE_OLDFILE);

            if (BADDR(rd->newIn) == NULL)
            {
                result = IoErr();
                PrintFault(result, rd->inFileName);
                goto exit;
            }

            SelectInput(rd->newIn);
        }

        lv = FindVar("echo", LV_VAR);
        /* AmigaDOS's shell is content also with echo being set to anything
           that begins with "on" in order to trigger commands echoing on,
           it doesn't really have to be set to just "on". */
        /* Embedded command isn't echo'ed, but its result will be integrated
           in final command line, which will be echo'ed if the var is set. */
        if ((lv != NULL)                              &&
                (lv->lv_Len >= 2)                         &&
                (strncasecmp(lv->lv_Value, "on", 2) == 0) &&
                (!rd->embedded))
        {
            /* Ok, commands echoing is on. */
            /* If a redirection is present, echoing isn't expected to go to
               it. If a script is running, building commandLine allows us
               to show what the command line looks like after arguments
               substitution. */
            BPTR echoOut = ((rd->haveOutRD) || (rd->haveAppRD)) ? rd->oldOut : Output();
            if (cli->cli_Interactive)
                FPuts(echoOut, cs->CS_Buffer);
            else
            {
                STRPTR commandLine = AllocVec(1024, MEMF_ANY);
                snprintf(commandLine, 1024, "%s%s%s%s",
                         rd->commandStr,
                         rd->haveOutRD ? " >" : rd->haveAppRD ? " >>" : "",
                         rd->outFileName,
                         filtered.CS_Buffer);
                FPuts(echoOut, commandLine);
                FreeVec(commandLine);
            }
        }

        /* OK, we've got a command. Let's execute it! */
        result = executeLine(rd->commandStr, filtered.CS_Buffer, rd, is);

        SelectInput(cli->cli_StandardInput);
        SelectOutput(cli->cli_StandardOutput);
    }
    else
    {
        /* PutStr("Erroneous command line.\n"); */
        PrintFault(result, filtered.CS_Buffer);
    }

exit:
    FreeCSource(&filtered);

    if (cli->cli_Interactive)
    {
        Flush(Output());
        Flush(Error());
    }

    return result;
}

BOOL doCommand(struct CSource *cs, struct CSource *filtered,
               CONST_STRPTR cmd1, LONG l1,
               CONST_STRPTR cmd2, LONG l2,
               TEXT *res, LONG *error,
               struct InterpreterState *is)
{
    CONST_STRPTR s = cs->CS_Buffer + cs->CS_CurChr + 1;
    CONST_STRPTR start = s;
    LONG len = l1;
    BOOL match = FALSE;

    if (strncasecmp(s, cmd1, l1) == 0)
        match = TRUE;
    else if (cmd2 && strncasecmp(s, cmd2, l2) == 0)
    {
        match = TRUE;
        len = l2;
    }

    if (match)
    {
        s += len;
        while (s[0] == ' ' || s[0] == '\t')
            ++s;

        if (s[0] == '\n' || s[0] == '\0')
        {
            appendString(filtered, &is->dot, 1);
            appendString(filtered, start, len);
            *error = ERROR_REQUIRED_ARG_MISSING;
            return TRUE;
        }
        else
        {
            *res = s[0];
            cs->CS_CurChr += s - start + 2;
        }

        return TRUE;
    }

    return FALSE;
}

/* Currently, no error checking is involved */
BOOL copyEmbedResult(struct CSource *filtered, struct Redirection *embedRd)
{
    char a = 0;

    Seek(embedRd->newOut, 0, OFFSET_BEGINNING);

    while (((a = FGetC(embedRd->newOut)) != '\n') && (a != EOF))
        appendString(filtered, &a, 1);

    return TRUE;
}

BOOL readLine(struct CSource *cs, BPTR inputStream)
{
    BOOL comment = FALSE, quoted = FALSE;
    LONG letter = EOF, prev = EOF;
    TEXT c;

    while (inputStream)
    {
        letter = FGetC(inputStream);

        if (letter == '\n' || letter == EOF)
            break;

        /* command line preprocessing */
        switch (letter)
        {
        case '"':
            if (quoted && prev == '*')
                break;
            else
                quoted = !quoted;
            break;
        case ';':
            comment = !quoted;
            break;
        default:
            if (isspace(letter))
            {
                if (cs->CS_Length == 0) /* ignore leading spaces */
                    continue;
                if (quoted)
                    break;
                if (isspace(prev)) /* ignore multiple spaces */
                    continue;
                else
                    letter = ' ';
            }
        }

        prev = letter;

        if (comment) /* ignore */
            continue;

        c = (TEXT)letter;
        appendString(cs, &c, 1);
    }

    D(if (cs->CS_Buffer) bug("[Shell] readLine: %s\n", cs->CS_Buffer));
    return letter == EOF ? FALSE : TRUE;
}

void unloadCommand(BPTR commandSeg, struct ShellState *ss)
{
#if SET_HOMEDIR
    if (ss->homeDirChanged)
    {
        UnLock(SetProgramDir(ss->oldHomeDir));
        ss->homeDirChanged = FALSE;
    }
#endif

    if (!cli->cli_Module) return;

    if (ss->residentCommand)
    {
        struct Segment *residentSeg = (struct Segment *)BADDR(commandSeg);

        Forbid();

        /* Decrease usecount */
        if (residentSeg->seg_UC > 0)
            residentSeg->seg_UC--;

        Permit();

        ss->residentCommand = FALSE;
    }
    else
        UnLoadSeg(commandSeg);
}


BPTR loadCommand(STRPTR commandName, struct ShellState *ss)
{
    BPTR   oldCurDir;
    BPTR   commandSeg = NULL;
    BPTR  *paths;
    struct Segment *residentSeg;
    BOOL   absolutePath = strpbrk(commandName, "/:") != NULL;
    BPTR   file;

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

                ss->residentCommand = TRUE;
                Permit();
                return MKBADDR(residentSeg);
            }
        }

        Permit();
    }

    ss->residentCommand = FALSE;

    D(bug("[Shell] trying to load command: %s\n", commandName));

    oldCurDir = CurrentDir(NULL);
    CurrentDir(oldCurDir);

    file = Open(commandName, MODE_OLDFILE);

    if (!file)
    {
        if
        (
            absolutePath ||                 /* If this was an absolute path, we don't check the paths set by
	                                       'path' or the C: multiassign */
            IoErr() == ERROR_OBJECT_IN_USE  /* The object might be exclusively locked */
        )
            return NULL;

        /* Search the command in the path */

        for
        (
            paths = (BPTR *)BADDR(cli->cli_CommandDir);
            file == NULL && paths != NULL;
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

#if SET_HOMEDIR
        if (commandSeg)
        {
            BPTR lock = ParentOfFH(file);

            if (lock)
            {
                ss->oldHomeDir = SetProgramDir(lock);
                ss->homeDirChanged = TRUE;
            }
        }
        else
#endif
        {
            struct FileInfoBlock fib;
            if (Examine(file, &fib) && fib.fib_Protection & FIBF_SCRIPT)
            {
                commandSeg = LoadSeg("C:Execute");
                if (commandSeg)
                {
                    ss->script = TRUE;
                    ss->scriptLock = Lock(commandName, SHARED_LOCK);
                }
            }
            else
                SetIoErr(ERROR_FILE_NOT_OBJECT);
        }

        Close(file);
    }

    CurrentDir(oldCurDir);

    return commandSeg;
}


/* Execute one command */
LONG executeLine(STRPTR command, STRPTR commandArgs, struct Redirection *rd,
                 struct InterpreterState *is)
{
    BPTR              module;
    LONG              error = 0;
    struct ShellState ss = {FALSE};
    TEXT cmd[4096];

    ss.script = FALSE;

    /* FIXME comment is lie and is initialised just 5 lines above ! drop that !
      if ss->residentCommand isn't initialized as FALSE, it's value is rather
      random ( loadCommand doesn't change it ) so unloadCommand almost always
      thinks that last Command was resident, and doesn't do an UnloadSeg...
    */

    D(bug("[Shell] executeLine: %s %s\n", command, commandArgs));
    module = loadCommand(command, &ss);

    /* Set command name even if we couldn't load the command to be able to
       report errors correctly */
    SetProgramName(command);

    if (module != NULL)
    {
        struct Task *me = FindTask(NULL);
        STRPTR oldtaskname = me->tc_Node.ln_Name;
        BOOL  __debug_mem;
        LONG mem_before;
        ULONG sig_before = ((struct Process *)me)->pr_Task.tc_SigAlloc;
        ULONG sig_after;
        BYTE sigbit;
        ULONG sigmask;

        BPTR seglist = ss.residentCommand ? ((struct Segment *)BADDR(module))->seg_Seg : module;

        STRPTR dst = cmd, src;
        LONG len = 0;

        if (ss.script)
        {
            *dst++ = '"';
            NameFromLock(ss.scriptLock, dst, sizeof(cmd));
            while (*dst != '\0')
            {
                ++dst;
                ++len;
            }
            *dst++ = '"';
            *dst++ = ' ';
            UnLock(ss.scriptLock);
            len += 2;
        }

        src = commandArgs;
        for (; src && *src != '\0'; ++dst, ++src, ++len)
            *dst = *src;
        *dst = '\0';

        D(bug("[Shell] command loaded: len=%d, args=%s\n", len, cmd));

        SetIoErr(0); /* Clear error before we execute this command */
        SetSignal(0, SIGBREAKF_CTRL_C);

        cli->cli_Module = seglist;
        me->tc_Node.ln_Name = command;

        __debug_mem = FindVar("__debug_mem", LV_VAR) != NULL;

        if (__debug_mem)
        {
            FreeVec(AllocVec((ULONG)(~0ul / 2), MEMF_ANY)); /* Flush memory */
            mem_before = AvailMem(MEMF_ANY);
            Printf("Available total memory before command execution: %10ld\n", mem_before);
        }

        cli->cli_ReturnCode = RunCommand(seglist, cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT,
                                         cmd, len);

        /*
            Check if running the command has changed signal bits of the Shell process.
            If there is a difference the signals will be set or freed to avoid that
            the Shell runs out of free signals.
        */
        sig_after = ((struct Process *)me)->pr_Task.tc_SigAlloc;
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

        if (__debug_mem)
        {
            LONG mem_after;
            FreeVec(AllocVec((ULONG)(~0ul / 2), MEMF_ANY)); /* Flush memory */

            mem_after = AvailMem(MEMF_ANY);
            Printf("Available total memory after command execution:  %10ld\n", mem_after);
            Printf("Memory difference (before - after):              %10ld\n", mem_before - mem_after);
        }

        me->tc_Node.ln_Name = oldtaskname;

        D(bug("[Shell] returned from command: %s\n", command));
        unloadCommand(module, &ss);

        cli->cli_Result2 = IoErr();
    }
    else
    {
        /* Implicit cd? */
        /* SFS returns ERROR_INVALID_COMPONENT_NAME if you try to open "" */
        if (!(rd->haveInRD || rd->haveOutRD || rd->haveAppRD) &&
                (IoErr() == ERROR_OBJECT_WRONG_TYPE || IoErr() == ERROR_OBJECT_NOT_FOUND || IoErr() == ERROR_INVALID_COMPONENT_NAME))
        {
            BPTR lock = Lock(command, SHARED_LOCK);

            if (lock != NULL)
            {
                struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);

                if (fib != NULL)
                {
                    if (Examine(lock, fib))
                    {
                        if (fib->fib_DirEntryType > 0)
                        {
                            setPath(lock);
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

        if (IoErr())
        {
            cli->cli_Result2 = IoErr();
            PrintFault(IoErr(), command);
        }
    }

    D(bug("[Shell] done with the command...\n"));
    return error;
}


LONG Redirection_init(struct Redirection *rd)
{
    bzero(rd, sizeof(struct Redirection));

    rd->commandStr  = AllocVec(COMMANDSTR_LEN, MEMF_CLEAR);

    if (!rd->commandStr)
        return ERROR_NO_FREE_STORE;

    /* Preset the first bytes to "C:" to handle C: multiassigns */
    *rd->commandStr++ = 'C';
    *rd->commandStr++ = ':';

    return 0;
}


void Redirection_release(struct Redirection *rd)
{
    /* -2 as we set pointer 2 bytes ahead to be able to use C: as a multi-
       assign in a smooth way */
    FreeVec(rd->commandStr - 2);

    /* Close redirection files and install regular input and output streams */
    if (rd->newIn)
        Close(rd->newIn);
    rd->newIn = NULL;

    if (rd->newOut)
        Close(rd->newOut);
    rd->newOut = NULL;
}

static void printPath(void)
{
    STRPTR  buf;
    ULONG   i;

    for (i = 256; ; i += 256)
    {
        buf = AllocVec(i, MEMF_ANY);

        if (buf == NULL)
            break;

        if (GetCurrentDirName(buf, i) == DOSTRUE)
        {
            FPuts(Output(), buf);
            FreeVec(buf);
            break;
        }

        FreeVec(buf);

        if (IoErr() != ERROR_OBJECT_TOO_LARGE)
            break;
    }
}


static void setPath(BPTR lock)
{
    BPTR    dir;
    STRPTR  buf;
    ULONG   i;

    if (lock == NULL)
        dir = CurrentDir(NULL);
    else
        dir = lock;

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

    if (lock == NULL)
        CurrentDir(dir);
}

static void printPrompt(struct InterpreterState *is)
{
    BSTR prompt = cli->cli_Prompt;
    LONG length = AROS_BSTR_strlen(prompt);
    ULONG i;

    for (i = 0; i < length; i++)
    {
        if (AROS_BSTR_getchar(prompt, i) == '%')
        {
            i++;

            if (i == length)
                break;

            switch (AROS_BSTR_getchar(prompt, i))
            {
            case 'N':
            case 'n':
                Printf("%ld", is->cliNumber);
                break;
            case 'R':
            case 'r':
                Printf("%ld", cli->cli_ReturnCode);
                break;
            case 'S':
            case 's':
                printPath();
                break;
            default:
                FPutC(Output(), '%');
                FPutC(Output(), AROS_BSTR_getchar(prompt, i));
                break;
            }
        }
        else
            FPutC(Output(), AROS_BSTR_getchar(prompt, i));
    }

    Flush(Output());
}

