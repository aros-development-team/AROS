/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
/* Run an explicit standalone Run executable; never replace resident Run. */
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dostags.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    char directory[120], path[2][160], command[500], data[128];
    static const char overwrite[] = "RUN_REDIRECT_OK\n";
    static const char append[] = "PREFIX\nRUN_REDIRECT_OK\n";
    const char *expected[2] = {overwrite, append};
    const size_t expected_length[2] = {sizeof overwrite - 1, sizeof append - 1};
    BPTR file;
    unsigned attempt, round, ticks;
    size_t argument_length = 0;
    LONG count;
    int all_ok = 1;

    /* Validate the bounded command argument before formatting it as a string. */
    if (argc == 2)
    {
        while (argument_length < 241 && argv[1][argument_length])
        {
            char c = argv[1][argument_length];
            if (c == '"' || c == '*' || c == '\r' || c == '\n')
                break;
            ++argument_length;
        }
    }
    if (argc != 2 || argument_length > 240 || argv[1][argument_length])
    {
        puts("Usage: run_redirection <explicit standalone Run executable>");
        return RETURN_FAIL;
    }
    /* CreateDir is the ownership boundary. Never truncate a pre-existing file. */
    for (attempt = 0; attempt < 100; ++attempt)
    {
        BPTR lock;
        snprintf(directory, sizeof directory, "RAM:run-redirection-%lu-%u",
                 (unsigned long)me->pr_TaskNum, attempt);
        lock = CreateDir((CONST_STRPTR)directory);
        if (lock) { UnLock(lock); break; }
        if (IoErr() != ERROR_OBJECT_EXISTS) return RETURN_FAIL;
    }
    if (attempt == 100) return RETURN_FAIL;
    snprintf(path[0], sizeof path[0], "%s/overwrite", directory);
    snprintf(path[1], sizeof path[1], "%s/append", directory);

    for (round = 0; round < 2; ++round)
    {
        LONG result;
        int matched = 0;
        if (round)
        {
            file = Open((CONST_STRPTR)path[round], MODE_NEWFILE);
            if (!file) { all_ok = 0; break; }
            count = Write(file, "PREFIX\n", 7);
            if (!Close(file) || count != 7) { all_ok = 0; break; }
        }
        snprintf(command, sizeof command, "\"%s\" %s%s Echo RUN_REDIRECT_OK",
                 argv[1], round ? ">>" : ">", path[round]);
        result = SystemTags((CONST_STRPTR)command, SYS_Asynch, FALSE, TAG_DONE);
        for (ticks = 0; result == RETURN_OK && ticks < 250; ++ticks)
        {
            file = Open((CONST_STRPTR)path[round], MODE_OLDFILE);
            if (file)
            {
                count = Read(file, data, sizeof data - 1);
                Close(file);
                if (count >= 0)
                {
                    data[count] = 0;
                    if ((size_t)count == expected_length[round] &&
                        !memcmp(data, expected[round], (size_t)count))
                    { matched = 1; break; }
                }
            }
            Delay(1);
        }
        printf("RUN %s %s: launch=%ld exact_output=%d\n",
               round ? "APPEND" : "REDIRECT", matched ? "PASS" : "FAIL",
               (long)result, matched);
        if (!matched) all_ok = 0;
    }
    if (all_ok)
    {
        /* A background Echo may still be closing its output after the read. */
        for (round = 0; round < 2; ++round)
        {
            for (ticks = 0; ticks < 250 && !DeleteFile((CONST_STRPTR)path[round]); ++ticks) Delay(1);
            if (ticks == 250) all_ok = 0;
        }
        if (all_ok && !DeleteFile((CONST_STRPTR)directory)) all_ok = 0;
    }
    if (!all_ok) printf("Test files retained: %s\n", directory);
    puts(all_ok ? "RUN REDIRECTION PASS: launch status, exact bytes, cleanup" : "RUN REDIRECTION FAIL");
    return all_ok ? RETURN_OK : RETURN_FAIL;
}
