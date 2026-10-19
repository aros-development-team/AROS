/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
/* Exercise the shell and resident Run, with different outer/child streams. */
#include <aros/debug.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <stdio.h>
#include <string.h>

static ULONG failures;

static void check(BOOL okay, CONST_STRPTR label)
{
    if (!okay)
    {
        bug("RUN COMMAND REDIR FAIL: %s\n", label);
        failures++;
    }
}

static BOOL writeFile(CONST_STRPTR path, CONST_STRPTR text)
{
    BPTR file = Open(path, MODE_NEWFILE);
    LONG size = strlen(text), count;
    BOOL closed;
    if (!file) return FALSE;
    count = Write(file, text, size);
    closed = Close(file);
    return count == size && closed;
}

static BOOL matches(CONST_STRPTR path, CONST_STRPTR expected)
{
    char data[80];
    BPTR file = Open(path, MODE_OLDFILE);
    LONG count;
    if (!file) return FALSE;
    count = Read(file, data, sizeof data);
    Close(file);
    return count == strlen(expected) && !memcmp(data, expected, count);
}

static BOOL waitFor(CONST_STRPTR path, CONST_STRPTR expected)
{
    unsigned ticks;
    for (ticks = 0; ticks < 100; ticks++)
    {
        if (matches(path, expected)) return TRUE;
        Delay(1);
    }
    return FALSE;
}

static BOOL absent(CONST_STRPTR path)
{
    BPTR lock = Lock(path, SHARED_LOCK);
    if (!lock) return TRUE;
    UnLock(lock);
    return FALSE;
}

int main(int argc, char **argv)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    CONST_STRPTR payload = argc > 1 ? argv[1] : "PROGDIR:runcommandchild";
    char directory[100], input[140], childInput[140];
    char outer[140], inner[140], result[140], command[900];
    BPTR lock;
    unsigned attempt, mode;
    BOOL complete = TRUE;

    if (strlen(payload) > 240 || strchr(payload, '"') || strchr(payload, '*') ||
        strchr(payload, '\n') || strchr(payload, '\r'))
        return RETURN_FAIL;
    for (attempt = 0; attempt < 100; attempt++)
    {
        snprintf(directory, sizeof directory, "RAM:run-command-%lu-%u",
                 (unsigned long)me->pr_TaskNum, attempt);
        lock = CreateDir((CONST_STRPTR)directory);
        if (lock) { UnLock(lock); break; }
        if (IoErr() != ERROR_OBJECT_EXISTS) return RETURN_FAIL;
    }
    if (attempt == 100) return RETURN_FAIL;
    snprintf(input, sizeof input, "%s/outer-input", directory);
    snprintf(childInput, sizeof childInput, "%s/child-input", directory);
    if (!writeFile((CONST_STRPTR)input, "OUTER\n") ||
        !writeFile((CONST_STRPTR)childInput, "INNER\n"))
        return RETURN_FAIL;

    /* PLAIN is a diagnostic control for pre-existing Run behavior. */
    for (mode = argc > 2 && !strcmp(argv[2], "PLAIN") ? 3 : 0; mode < 4; mode++)
    {
        BPTR oldIn = Input(), oldOut = Output();
        LONG rc;
        BOOL ran;
        snprintf(outer, sizeof outer, "%s/outer-%u", directory, mode);
        snprintf(inner, sizeof inner, "%s/child-%u", directory, mode);
        snprintf(result, sizeof result, "%s/result-%u", directory, mode);
        if (mode == 0)
            snprintf(command, sizeof command,
                     "Run >NIL: <NIL: \"%s\" %s <NIL: >NIL:\n", payload, result);
        else if (mode == 1)
            snprintf(command, sizeof command,
                     "Run >%s <%s \"%s\" %s <%s >%s\n",
                     outer, input, payload, result, childInput, inner);
        else if (mode == 2)
            snprintf(command, sizeof command,
                     "rUn QUIET >%s <%s \"%s\" %s <%s >%s\n",
                     outer, input, payload, result, childInput, inner);
        else
            snprintf(command, sizeof command,
                     "Run \"%s\" %s <%s >%s\n", payload, result, childInput, inner);
        rc = SystemTags((CONST_STRPTR)command, SYS_Asynch, FALSE, TAG_DONE);
        ran = waitFor((CONST_STRPTR)result, mode ? "INNER\n" : "EMPTY\n");
        bug("RUN COMMAND REDIR CASE %lu: rc=%ld child=%ld\n",
            (unsigned long)mode, (long)rc, (long)ran);
        check(rc == RETURN_OK, "Run launch status");
        check(ran, mode ? "child reads its own input" : "RunBack NIL form executes child");
        check(Input() == oldIn && Output() == oldOut, "caller streams remain installed");
        if (mode)
            check(waitFor((CONST_STRPTR)inner, "INNER\n"), "child output reaches its own file");
        if (mode == 1 || mode == 2)
            check(matches((CONST_STRPTR)outer, ""), "outer output does not receive child bytes");
        if (!ran) { complete = FALSE; break; }
        /* The child may still be closing its redirected streams. */
        Delay(2);
        check(DeleteFile((CONST_STRPTR)result), "remove child completion file");
        if (mode) check(DeleteFile((CONST_STRPTR)inner), "remove child output");
        if (mode == 1 || mode == 2)
            check(DeleteFile((CONST_STRPTR)outer), "remove outer output");
    }
    if (complete)
    {
        BPTR oldIn = Input(), oldOut = Output();
        snprintf(outer, sizeof outer, "%s/failed-outer", directory);
        snprintf(inner, sizeof inner, "%s/should-not-exist", directory);
        snprintf(result, sizeof result, "%s/should-not-run", directory);
        snprintf(command, sizeof command,
                 "Run >%s <%s/missing \"%s\" %s <%s >%s\n",
                 outer, directory, payload, result, childInput, inner);
        SystemTags((CONST_STRPTR)command, SYS_Asynch, FALSE, TAG_DONE);
        Delay(10);
        check(absent((CONST_STRPTR)result), "failed outer redirect does not launch child");
        check(absent((CONST_STRPTR)inner), "failed outer redirect does not open child target");
        check(Input() == oldIn && Output() == oldOut, "failure preserves caller streams");
        check(DeleteFile((CONST_STRPTR)outer), "remove failed outer output");

        snprintf(command, sizeof command, "Echo BAD <%s <%s >%s\n",
                 input, childInput, inner);
        SystemTags((CONST_STRPTR)command, SYS_Asynch, FALSE, TAG_DONE);
        check(absent((CONST_STRPTR)inner), "ordinary duplicate input remains rejected");
        snprintf(command, sizeof command, "Echo BAD >%s >%s\n", outer, inner);
        SystemTags((CONST_STRPTR)command, SYS_Asynch, FALSE, TAG_DONE);
        check(matches((CONST_STRPTR)outer, "") && absent((CONST_STRPTR)inner),
              "ordinary duplicate output remains rejected");
        check(DeleteFile((CONST_STRPTR)outer), "remove rejected command output");

        snprintf(command, sizeof command, "Echo RESTORED >%s\n", result);
        check(SystemTags((CONST_STRPTR)command, SYS_Asynch, FALSE, TAG_DONE) == RETURN_OK &&
              matches((CONST_STRPTR)result, "RESTORED\n"),
              "shell remains usable after redirection errors");
        check(DeleteFile((CONST_STRPTR)result), "remove final probe output");
        check(DeleteFile((CONST_STRPTR)input), "remove outer input");
        check(DeleteFile((CONST_STRPTR)childInput), "remove child input");
        check(DeleteFile((CONST_STRPTR)directory), "remove test directory");
    }
    else
        bug("RUN COMMAND REDIR: evidence retained in %s\n", directory);
    bug("RUN COMMAND REDIR TEST: %lu failures\n", (unsigned long)failures);
    return failures ? RETURN_FAIL : RETURN_OK;
}
