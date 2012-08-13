/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <exec/lists.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <string.h>

#include "Shell.h"

#include <aros/debug.h>

static LONG convertLoop(LONG (*convertItem)(ShellState *, Buffer *, Buffer *, BOOL *),
                        LONG a, ShellState *ss, Buffer *in, Buffer *out)
{
    LONG c, p = 0, error, n = in->len;
    BOOL quoted = FALSE;

    bufferReset(out);

    for (; in->cur < n; p = c)
    {
        c = in->buf[in->cur];

        if (p == '*')
        {
            c = 0;
            bufferCopy(in, out, 1, SysBase);
        }
        else if (c == '"')
        {
            quoted = !quoted;
            bufferCopy(in, out, 1, SysBase);
        }
        else if (c == a)
        {
            if ((error = (*convertItem)(ss, in, out, &quoted)))
                return error;
        }
        else if (!quoted && c == ';' && c != ss->mchar0)
        {
             /* rest of line is comment, ignore it */
             break;
        }
        else
            bufferCopy(in, out, 1, SysBase);
    }

    in->cur = n;
    return 0;
}

static LONG convertLoopRedir(ShellState *ss, Buffer *in, Buffer *out)
{
    LONG c, p = 0, error, n = in->len;
    BOOL quoted = FALSE;

    bufferReset(out);

    for (; in->cur < n; p = c)
    {
        DB2(bug("[convertLoopRedir] cur %u (%c)\n", in->cur, in->buf[in->cur]));
        c = in->buf[in->cur];

        if (p == '*')
        {
            c = 0;
            bufferCopy(in, out, 1, SysBase);
        }
        else if (c == '"')
        {
            quoted = !quoted;
            bufferCopy(in, out, 1, SysBase);
        }
        else if (quoted)
            bufferCopy(in, out, 1, SysBase);
        else if (c == '<' || c == '>')
        {
            if ((error = convertRedir(ss, in, out)))
            {
                D(bug("[convertLoopRedir] convertRedir(%s) error %u\n", in->buf, error));
                return error;
            }
        }
        else
            bufferCopy(in, out, 1, SysBase);
    }

    in->cur = n;
    return 0;
}

static LONG readCommandR(ShellState *ss, Buffer *in, Buffer *out,
                         struct List *aliased)
{
    STRPTR command = ss->command + 2;
    TEXT buf[FILE_MAX];
    LONG i;

    i = bufferReadItem(command, FILE_MAX, in, DOSBase);
    D(bug("[readCommandR] Got item %d: %s\n", i, command));

    switch (i)
    {
    case ITEM_QUOTED: /* no alias expansion */
        if (in->cur < in->len)
            ++in->cur; /* skip separator */
        return bufferCopy(in, out, in->len - in->cur, SysBase);
    case ITEM_UNQUOTED:
        break;
    case ITEM_NOTHING:
        return 0;
    default:
        return ERROR_LINE_TOO_LONG; /* invalid argument */
    }

    /* Is this command an alias ? */
    if ((i = GetVar(command, buf, FILE_MAX, GVF_LOCAL_ONLY | LV_ALIAS)) > 0)
    {
        Buffer a = { buf, i, 0, 0 }, b = { 0 };
        struct Node anode, *n;
        TEXT cmd[FILE_MAX];
        LONG error;

        D(bug("Handling alias %s\n", command));
        switch (bufferReadItem(cmd, FILE_MAX, &a, DOSBase))
        {
        case ITEM_QUOTED:
        case ITEM_UNQUOTED:
            break;
        default:
            return ERROR_LINE_TOO_LONG; /* invalid argument line */
        }

        ForeachNode(aliased, n)
        {
            if (strcmp(cmd, n->ln_Name) == 0)
                return ERROR_LINE_TOO_LONG; /* alias loop */
        }

        D(bug("[Shell] found alias: %s\n", buf));
        anode.ln_Name = cmd;
        AddTail(aliased, &anode);
        a.cur = 0;

        /* vars substitution */
        if ((error = convertLoop(convertVar, '$', ss, &a, &b)))
            goto endReadAlias;

        /* alias foo bar1 [] bar2 */
        for (i = 0; i < b.len; ++i)
            if (b.buf[i] == '[' && b.buf[i + 1] == ']')
                break;

        bufferReset(&a);
        bufferCopy(&b, &a, i, SysBase);

        if (NULL == strchr(buf, ' '))
            /*
             * We need a separator here, between the command
             * and its first argument
             */
            bufferAppend(" ", 1, &a, SysBase);

        bufferCopy(in, &a, in->len - in->cur, SysBase);

        if (i < b.len)
        {
            b.cur += 2; /* skip [] */
            bufferCopy(&b, &a, b.len - b.cur, SysBase);
        }

        error = readCommandR(ss, &a, out, aliased);

endReadAlias:
        bufferFree(&a, SysBase);
        bufferFree(&b, SysBase);
        return error;
    }

    D(bug("[readCommandR] Copying buffer %s, len %d, pos %d\n", in->buf, in->len, in->cur));

    return bufferCopy(in, out, in->len - in->cur, SysBase);
}

static LONG readCommand(ShellState *ss, Buffer *in, Buffer *out)
{
    struct List aliased;

    NewList(&aliased);
    bufferReset(out);

    return readCommandR(ss, in, out, &aliased);
}

/* The shell has the following semantics when it comes to command lines:
   Redirection (<,>,>>) may be written anywhere (except before the command
   itself); the following item (as defined by ReadItem() is the redirection
   file. The first item of the command line is the command to be executed.
   This may be an alias, that is there is a Local LV_ALIAS variable that
   should be substituted for the command text. Aliasing only applies to
   commands and not to options, for instance. Variables (set by SetEnv or Set)
   may be referenced by prepending a '$' to the variable name. */
LONG convertLine(ShellState *ss, Buffer *in, Buffer *out, BOOL *haveCommand)
{
    LONG c = in->buf[in->cur], error;

    if (c == ';' && c != ss->mchar0) /* skip comment */
        return 0;

    if (c == ss->dot) /* .dot command at start of line */
        return convertLineDot(ss, in);

    /* Vars and BackTicks can't be properly handled by using FindItem() as
       it wouldn't find them when they aren't surrounded with blank spaces,
       so we handle them ourselves here. Environment variables are always
       referenced by prepending a '$' to their name, it's only scripts
       argument variables that can be referenced by prepending a modified
       (.dollar) sign. Environment variable names containing non-alpha-
       numerical characters must be surrounded with braces ( ${_} ).
       CLI number substitution <$$> handles .dollar and .bra and .ket
       signs subtitution.
       <$$> and Variables and BackTicks need to be handled only once per
       line, but in right order: Commodore's DPAT script builds an
       environment variable per script used, by including the current CLi's
       number in the variable name: $qw{$$} so we must first handle CLI
       number substitution, then extract variables, and then handle
       BackTicks nested commands.
     */

    /* PASS 1: `backticks` substitution */
    D(bug("[convertLine] Pass 1: on (%s)\n", in->buf));
    if ((error = convertLoop(convertBackTicks, '`', ss, in, out)))
    {
        D(bug("[convertLine] Pass 1: Error %lu parsing backticks\n", error));
        return error;
    }

    /* PASS 2: <args> substitution & CLI# <$$>*/
    D(bug("[convertLine] Pass 2: on (%s)\n", out->buf));
    if ((error = convertLoop(convertArg, ss->bra, ss, out, in)))
    {
        D(bug("[convertLine] Pass 2: Error %lu parsing <arguments> substitution and <$$> CLI#\n", error));
        return error;
    }

    /* PASS 3: ${vars} substitution */
    D(bug("[convertLine] Pass 3: on (%s)\n", in->buf));
    if ((error = convertLoop(convertVar, '$', ss, in, out)))
    {
        D(bug("[convertLine] Pass 3: Error %lu parsing variables\n", error));
        return error;
    }

    /* PASS 4: command & aliases */
    D(bug("[convertLine] Pass 4: on (%s)\n", out->buf));
    if ((error = readCommand(ss, out, in)))
    {
        D(bug("[convertLine] Pass 4: Error %lu parsing aliases\n", error));
        return error;
    }

    *haveCommand = TRUE;

    /* PASS 5: redirections */
    D(bug("[convertLine] Pass 5: cur %d len %d (%s)\n", in->cur, in->len, in->buf));
    error = convertLoopRedir(ss, in, out);
    if (error)
    {
        D(bug("[convertLine] Pass 5: Error %lu parsing redirect\n", error));
        return error;
    }

    if (out->len == 0 || out->buf[out->len-1] != '\n')
    {
        /*
         * Make sure that the output buffer (command arguments) ends with a newline.
         * This is OS 3.1-compatible behavior. RunCommand() injects the supplied line
         * into command's Input(), but doesn't append a newline. And without a newline,
         * ReadArgs() will halt, waiting for it.
         */
        D(bug("[convertLine] Appending a newline\n"));
        error = bufferAppend("\n", 1, out, SysBase);
    }

    D(bug("[convertLine] Result: cur %d len %d (%s)\n", out->cur, out->len, out->buf));

    return error;
}
