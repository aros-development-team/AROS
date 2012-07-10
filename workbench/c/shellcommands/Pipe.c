/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */
/******************************************************************************

    NAME

        Pipe <command>

    SYNOPSIS

        COMMAND/F

    LOCATION

       C:

    FUNCTION

        Uses the _pchar and _mchar environment variables to split
        the COMMAND into fragments.

        Where _pchar is seen, the commands on either side are connected
        with a PIPE: from the left side's Output() to the right side's Input().

        Where _mchar is seen, the commands are executed in sequence, with
        no PIPE: between them, and Input() and Output() comes from the
        terminal.

    INPUTS

        COMMAND -- the command to execute

    RESULT

    NOTES

        The "_pchar" and "_mchar" environment variables are used to determine
        where to split the command, and what action to perform.

    EXAMPLE

        > set _pchar "|"
        > set _mchar ";"
        > echo Hello ; echo World
        Hello
        World
        > Type S:Startup-Sequence | Sort
        
    BUGS

        Note that _pchar and _mchar are limited to 2 characters - any
        additional characters will be silently ignored.

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include <aros/shcommands.h>

static inline STRPTR NextToken(STRPTR *pstr, CONST_STRPTR tok)
{
    STRPTR str, ostr = *pstr;
    int toklen = strlen(tok);

    if (*ostr == 0)
        return NULL;

    for (str = ostr; *str; str++) {
        if (strncmp(str, tok, toklen) == 0) {
            *str = 0;
            str += strlen(tok);
            *pstr = str;
            return ostr;
        }
    }

    /* Token not found? */
    *pstr = ostr + strlen(ostr);
    return ostr;
}

static BOOL Pipe(struct Library *DOSBase, BPTR pipe[2])
{
    pipe[1]=Open("PIPE:*", MODE_NEWFILE);
    if (pipe[1]) {
        TEXT buff[64];
        if (NameFromFH(pipe[1], buff, sizeof(buff)) != 0) {
            pipe[0] = Open(buff, MODE_OLDFILE);
            if (pipe[0])
                return TRUE;
        }
        Close(pipe[1]);
    }
    return FALSE;
}

static LONG pcharExecute(struct Library *DOSBase, STRPTR cmd, CONST_STRPTR pchar, BPTR in, BPTR out)
{
    STRPTR leftcmd, rightcmd;
    BPTR pipe[2];
    TEXT buff[64];
    LONG ret = RETURN_FAIL;

    leftcmd = NextToken(&cmd, pchar);
    if (!leftcmd)
        return RETURN_OK;

    rightcmd = cmd;
    if (*rightcmd == 0) {
        /* No pipe character? */
        return SystemTags(leftcmd, SYS_Input, in, SYS_Output, out, TAG_END);
    }

    if (Pipe(DOSBase, pipe)) {
        BPTR cis = OpenFromLock(DupLockFromFH(in));

        struct TagItem tags[] = {
            { SYS_Input,        (IPTR)cis },
            { SYS_Output,       (IPTR)pipe[1] },
            { SYS_Asynch,       TRUE },
            { TAG_DONE,         0 }
        };

        if (SystemTagList(leftcmd, tags) == -1) {
            PrintFault(IoErr(), leftcmd);
            Close(cis);
            Close(pipe[0]);
            Close(pipe[1]);
            return RETURN_FAIL;
        }

        ret = pcharExecute(DOSBase, rightcmd, pchar, pipe[0], out);
        /* Flush pipe */
        while (FGets(pipe[0], buff, sizeof buff) != NULL);
        Close(pipe[0]);
    }

    return ret;
}


AROS_SH1(Pipe, 41.1,
AROS_SHA(STRPTR, , , /F,   ""))
{
    AROS_SHCOMMAND_INIT
    CONST_STRPTR cmd = SHArg( );
    struct CommandLineInterface *cli = Cli();

    TEXT pchar[3], mchar[3];
    LONG len, ret = RETURN_FAIL;
    STRPTR tcmd, subcmd, cp;

    len = GetVar("_pchar", pchar, sizeof pchar, GVF_LOCAL_ONLY | LV_VAR);
    if (len <= 0)
        pchar[0] = 0;
    pchar[2] = 0;

    len = GetVar("_mchar", mchar, sizeof mchar, GVF_LOCAL_ONLY | LV_VAR);
    if (len <= 0)
        mchar[0] = 0;
    mchar[2] = 0;

    /* Tokenize by mchar, if needed */
    len = strlen(cmd) + 1;
    if ((tcmd = AllocVec(len, MEMF_ANY))) {
        CopyMem(cmd, tcmd, len);
        cp = tcmd;

        for (subcmd = NextToken(&cp, mchar); subcmd; subcmd = NextToken(&cp, mchar)) {
            ret = pcharExecute((struct Library *)DOSBase, subcmd, pchar, Input(), Output());
            if (ret >= cli->cli_FailLevel)
                break;
        }

        FreeVec(tcmd);
    }

    return ret;
    AROS_SHCOMMAND_EXIT
}
