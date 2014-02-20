/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang:
*/

/******************************************************************************

    NAME

        Execute <script> [{<arguments>}]

    SYNOPSIS

        FILE/A

    LOCATION

        C:

    FUNCTION

        Executes a script with DOS commands.

    INPUTS

        FILE -- file to execute

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/bptr.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <string.h>

#define DEBUG 0
#include <aros/debug.h>

#ifndef USE_EMBEDDED_COMMANDS
#define SH_GLOBAL_SYSBASE       1       /* For __sprintf */
#endif
#include <aros/shcommands.h>

AROS_SH2(Execute, 41.1,
AROS_SHA(STRPTR, ,NAME     , /A, NULL),
AROS_SHA(STRPTR, ,ARGUMENTS, /F, NULL))
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();
    STRPTR arguments = SHArg(ARGUMENTS), s;
    STRPTR extraargs = NULL;
    BPTR from;
    LONG len;
    BPTR ces;
    struct Process *me = (struct Process *)FindTask(NULL);

    if (!cli)
        return RETURN_ERROR;

    if(! (ces=me->pr_CES))
        ces=me->pr_COS;

    if (! arguments)
        arguments = "";

    /* See if we have extra arguments buffered in Input()
     */
    if (Input() != BNULL) {
        struct FileHandle *fh = BADDR(Input());

        if (fh->fh_Pos > 0 && fh->fh_End > 0) {
            LONG extraargsize = fh->fh_End - fh->fh_Pos;

            if (extraargsize > 0) {
                LONG argsize = strlen(arguments);
                extraargs = AllocVec(argsize + 1 + extraargsize + 1, MEMF_ANY);
                if (extraargs) {
                    CopyMem(arguments, extraargs, argsize);
                    extraargs[argsize++] = '\n';
                    CopyMem(BADDR(fh->fh_Buf)+fh->fh_Pos, &extraargs[argsize], extraargsize);
                    argsize += extraargsize;
                    extraargs[argsize++] = 0;
                    arguments = extraargs;
                }
            }
        }
    }

    from = Open(SHArg(NAME), MODE_OLDFILE);

    if (!from)
    {
        IPTR data[] = { (IPTR)SHArg(NAME) };
        VFPrintf(me->pr_CES, "EXECUTE: can't open %s\n", data);
        PrintFault(IoErr(), NULL);
        if (extraargs)
            FreeVec(extraargs);
        return RETURN_FAIL;
    }

    if (cli->cli_StandardInput == cli->cli_CurrentInput)
    {
        cli->cli_CurrentInput = from;
    }
    else
    {
        struct DateStamp ds;
        BYTE tmpname[256];
        BPTR tmpfile = BNULL;
        ULONG count = 0;
        BYTE tmpdir[4];
        BPTR tmplock;
        struct Window *win;
        struct Process *proc = (struct Process*)FindTask(0);

        DateStamp(&ds);
        
        win = proc->pr_WindowPtr;
        proc->pr_WindowPtr = (struct Window *)-1;
        tmplock = Lock("T:", SHARED_LOCK);
        proc->pr_WindowPtr = win;
        if (tmplock) {
            strcpy(tmpdir, "T:");
            UnLock(tmplock);
        } else {
            strcpy(tmpdir, ":T/");
        }

        do {
            count++;
            __sprintf(tmpname, "%sTmp%lu%lu%lu%lu%lu", tmpdir,
                      ((struct Process *)FindTask(NULL))->pr_TaskNum,
                      ds.ds_Days, ds.ds_Minute, ds.ds_Tick, count);
            tmpfile = Open(tmpname, MODE_NEWFILE);
        } while (tmpfile == BNULL && IoErr() == ERROR_OBJECT_IN_USE);

        if (tmpfile)
        {
            LONG c;

            //if (FPuts(tmpfile, ".pushis\n") != -1)
                while((c = FGetC(from)) != -1 && FPutC(tmpfile, c) != -1);

            c = IoErr();
            Close(from);

            if (c)
            {
                FPuts(me->pr_CES,
                      "EXECUTE: error while creating temporary file\n");
                PrintFault(c, NULL);
                Close(tmpfile);
                DeleteFile(tmpname);
                if (extraargs)
                    FreeVec(extraargs);

                return RETURN_FAIL;
            }

            c = '\n';
            FPutC(tmpfile, c);

            //FPuts(tmpfile, ".popis\n");

            while((c = FGetC(cli->cli_CurrentInput)) != -1 && FPutC(tmpfile, c) != -1);

            c = IoErr();

            if (c)
            {
                FPuts(me->pr_CES, "EXECUTE: error while creating temporary file\n");
                PrintFault(c, NULL);
                Close(tmpfile);
                DeleteFile(tmpname);
                if (extraargs)
                    FreeVec(extraargs);

                return RETURN_FAIL;
            }

            Close(cli->cli_CurrentInput);
            if (AROS_BSTR_strlen(cli->cli_CommandFile))
                DeleteFile(AROS_BSTR_ADDR(cli->cli_CommandFile));

            {
                LONG len = strlen(tmpname);
                CopyMem(tmpname, AROS_BSTR_ADDR(cli->cli_CommandFile), len);
                AROS_BSTR_setstrlen(cli->cli_CommandFile, len);
            }

            cli->cli_CurrentInput = tmpfile;
            Seek(tmpfile, 0, OFFSET_BEGINNING);
        }
        else
        {
            /*
              we should try to open ":T", but since ":"
              is not handled correctly yet, we just give up
            */
            LONG c = IoErr();
            FPuts(me->pr_CES, "EXECUTE: error while creating temporary file\n");
            PrintFault(c, NULL);
            Close(from);
            if (extraargs)
                FreeVec(extraargs);

            return RETURN_FAIL;
        }
    }

    /* Update cli_CommandName to be the name of the script */
    len = strlen(SHArg(NAME));
    s = AROS_BSTR_ADDR(cli->cli_CommandName);
    AROS_BSTR_setstrlen(cli->cli_CommandName, len);
    CopyMem(SHArg(NAME), s, len);

    if (arguments && strlen(arguments)) {
        struct FileHandle *fh;
        TEXT *fh_buff;

        len = strlen(arguments);

        /* Inject the command args into cli->cli_StandardInput 
         *
         * It would be nice to have a standard DOS LVO that
         * could do this for us.
         */
        Flush(cli->cli_StandardInput);
        if (SetVBuf(cli->cli_StandardInput, NULL, BUF_LINE, len + 1) == 0) {
            fh = BADDR(cli->cli_StandardInput);
            fh->fh_Pos = 0;
            fh->fh_End = len + 1;
            fh_buff = BADDR(fh->fh_Buf);
            CopyMem(arguments, fh_buff, len);
            fh_buff[len] = '\n';
            /* Prevent RunCommand() from flushing cli_StandardInput */
            SelectInput(BNULL);
        } else {
            VFPrintf(me->pr_CES, "EXECUTE: Can't inject command line\n", NULL);
            return RETURN_FAIL;
        }
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
