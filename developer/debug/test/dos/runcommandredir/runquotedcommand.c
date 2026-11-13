/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
/* Launch real children whose executable and argument names require quoting. */
#include <aros/debug.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <stdio.h>
#include <string.h>

static BOOL copyFile(CONST_STRPTR from, CONST_STRPTR to)
{
    char data[512];
    BPTR in = Open(from, MODE_OLDFILE), out;
    LONG n;
    BOOL okay = TRUE;
    if (!in) return FALSE;
    out = Open(to, MODE_NEWFILE);
    if (!out) { Close(in); return FALSE; }
    while ((n = Read(in, data, sizeof data)) > 0)
        if (Write(out, data, n) != n) { okay = FALSE; break; }
    if (n < 0) okay = FALSE;
    if (!Close(out)) okay = FALSE;
    Close(in);
    return okay;
}

static BOOL matches(CONST_STRPTR path)
{
    char data[8];
    BPTR file = Open(path, MODE_OLDFILE);
    LONG n;
    if (!file) return FALSE;
    n = Read(file, data, sizeof data);
    Close(file);
    return n == 6 && !memcmp(data, "INNER\n", 6);
}

static BOOL waitFor(CONST_STRPTR path)
{
    unsigned ticks;
    for (ticks = 0; ticks < 100; ticks++)
    {
        if (matches(path)) return TRUE;
        Delay(1);
    }
    return FALSE;
}

/* Quote one AmigaDOS token, retaining literal stars and double quotes. */
static void quote(char *out, CONST_STRPTR in)
{
    *out++ = '"';
    while (*in)
    {
        if (*in == '*' || *in == '"') *out++ = '*';
        *out++ = *in++;
    }
    *out++ = '"';
    *out = 0;
}

int main(void)
{
    static CONST_STRPTR prefixes[] = {
        "Run", "rUn qUiEt", "Run COMMAND", "Run QUIET COMMAND=",
        "Run COMMAND =", "Run >NIL: <NIL:", "Run QUIET >NIL: <NIL:"
    };
    static CONST_STRPTR names[] = { "child with spaces", "child *\"quoted\"" };
    struct Process *me = (struct Process *)FindTask(NULL);
    char directory[100], child[140], input[140], output[140], result[140];
    char qchild[300], qinput[300], qoutput[300], qresult[300], command[1400];
    BPTR lock, file;
    unsigned attempt, name, mode, failures = 0;
    BOOL complete = TRUE;

    for (attempt = 0; attempt < 100; attempt++)
    {
        snprintf(directory, sizeof directory, "RAM:run-quoted-%lu-%u",
                 (unsigned long)me->pr_TaskNum, attempt);
        lock = CreateDir(directory);
        if (lock) { UnLock(lock); break; }
        if (IoErr() != ERROR_OBJECT_EXISTS) return RETURN_FAIL;
    }
    if (attempt == 100) return RETURN_FAIL;
    snprintf(input, sizeof input, "%s/input file", directory);
    file = Open(input, MODE_NEWFILE);
    if (!file) return RETURN_FAIL;
    if (Write(file, "INNER\n", 6) != 6) { Close(file); return RETURN_FAIL; }
    if (!Close(file)) return RETURN_FAIL;
    quote(qinput, input);

    for (name = 0; name < sizeof names / sizeof names[0]; name++)
    {
        snprintf(child, sizeof child, "%s/%s", directory, names[name]);
        if (!copyFile("PROGDIR:runcommandchild", child)) return RETURN_FAIL;
        quote(qchild, child);
        for (mode = 0; mode < sizeof prefixes / sizeof prefixes[0]; mode++)
        {
            LONG rc;
            BOOL ran, wrote;
            snprintf(output, sizeof output, "%s/output %u-%u", directory, name, mode);
            snprintf(result, sizeof result, "%s/result %u-%u", directory, name, mode);
            quote(qoutput, output);
            quote(qresult, result);
            snprintf(command, sizeof command, "%s %s %s <%s >%s\n",
                     prefixes[mode], qchild, qresult, qinput, qoutput);
            rc = SystemTagList(command, NULL);
            ran = waitFor(result);
            wrote = waitFor(output);
            bug("RUN QUOTED CASE %u/%u: rc=%ld child=%ld output=%ld\n",
                name, mode, rc, (LONG)ran, (LONG)wrote);
            if (rc || !ran || !wrote)
            {
                failures++;
                complete = FALSE;
                break;
            }
            Delay(2);
            if (!DeleteFile(result) || !DeleteFile(output)) failures++;
        }
        if (!complete) break;
        if (!DeleteFile(child)) failures++;
    }
    if (complete)
    {
        char prompt[140], noise[140];
        BPTR in, out;
        LONG rc;
        BOOL ran;

        /* Keep Run's existing ReadArgs '?' prompt working. Its replacement
         * command comes from Input(), not the original argument string. */
        snprintf(prompt, sizeof prompt, "%s/prompt", directory);
        snprintf(noise, sizeof noise, "%s/help", directory);
        snprintf(output, sizeof output, "%s/prompt output", directory);
        snprintf(result, sizeof result, "%s/prompt result", directory);
        quote(qoutput, output);
        quote(qresult, result);
        snprintf(command, sizeof command, "PROGDIR:runcommandchild %s <%s >%s\n",
                 qresult, qinput, qoutput);
        file = Open(prompt, MODE_NEWFILE);
        if (!file) return RETURN_FAIL;
        if (Write(file, command, strlen(command)) != strlen(command))
        { Close(file); return RETURN_FAIL; }
        if (!Close(file)) return RETURN_FAIL;
        in = Open(prompt, MODE_OLDFILE);
        out = Open(noise, MODE_NEWFILE);
        if (!in || !out) return RETURN_FAIL;
        rc = SystemTags("Run ?\n", SYS_Input, in, SYS_Output, out, TAG_DONE);
        ran = waitFor(result) && waitFor(output);
        bug("RUN QUOTED HELP: rc=%ld child=%ld\n", rc, (LONG)ran);
        if (rc || !ran) failures++;
        Delay(2);
        Close(in);
        Close(out);
        if (!DeleteFile(prompt) || !DeleteFile(noise) ||
            !DeleteFile(result) || !DeleteFile(output)) failures++;
        if (!DeleteFile(input) || !DeleteFile(directory)) failures++;
    }
    else
        bug("RUN QUOTED: evidence retained in %s\n", directory);
    bug("RUN QUOTED TEST: %u failures\n", failures);
    return failures ? RETURN_FAIL : RETURN_OK;
}
