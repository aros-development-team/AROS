/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* Run an unmodified command binary with a real pending Ctrl-C signal.
 * Usage: TypeRetryNative path/to/Type > unique-results-file
 * Fixtures are private to this process and existing names are refused.
 */
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <string.h>
#include <stdio.h>

static int write_fixture(CONST_STRPTR name)
{
    BPTR f = Open(name, MODE_NEWFILE);
    LONG n;
    if (!f) return 0;
    n = Write(f, "first\nsecond\n", 13);
    Close(f);
    return n == 13;
}

int main(int argc, char **argv)
{
    static const struct {
        CONST_STRPTR name, args, expected;
        BOOL signal;
        LONG rc;
    } cases[] = {
        {"single-break", "one.txt TO out\n", "first\n", TRUE, RETURN_ERROR},
        {"literal", "one.txt TO out\n", "first\nsecond\n", FALSE, RETURN_OK},
        {"wildcard", "#?.txt TO out\n", "first\nsecond\nfirst\nsecond\n", FALSE, RETURN_OK},
        {"multiple", "one.txt two.txt TO out\n", "first\nsecond\nfirst\nsecond\n", FALSE, RETURN_OK},
        {"missing", "missing TO out\n", "", FALSE, RETURN_ERROR}
    };
    BPTR segment, lock, olddir;
    char dirname[80];
    int failed = 0;
    if (argc != 2) {
        Printf("Usage: TypeRetryNative path/to/Type\n");
        return RETURN_FAIL;
    }
    segment = LoadSeg(argv[1]);
    if (!segment) { PrintFault(IoErr(), "LoadSeg"); return RETURN_FAIL; }
    {
        IPTR task = (IPTR)FindTask(NULL);
        unsigned pos = 14, shift;
        strcpy(dirname, "RAM:TypeRetry-");
        for (shift = sizeof(task) * 8; shift; shift -= 4)
            dirname[pos++] = "0123456789abcdef"[(task >> (shift - 4)) & 15];
        dirname[pos] = 0;
    }
    lock = Lock(dirname, ACCESS_READ);
    if (lock) {
        UnLock(lock); UnLoadSeg(segment);
        Printf("Refusing existing fixture directory\n");
        return RETURN_FAIL;
    }
    lock = CreateDir(dirname);
    if (!lock) { UnLoadSeg(segment); return RETURN_FAIL; }
    /* CreateDir returns an exclusive lock; wildcard matching needs to
     * duplicate the current directory lock, which must be shared.
     */
    UnLock(lock);
    lock = Lock(dirname, ACCESS_READ);
    if (!lock) {
        DeleteFile(dirname); UnLoadSeg(segment); return RETURN_FAIL;
    }
    olddir = CurrentDir(lock);
    if (!write_fixture("one.txt") || !write_fixture("two.txt")) {
        failed = 1;
        Printf("FAIL fixture creation\n");
    } else for (unsigned i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
        char output[128], command[128];
        LONG rc, error, n = -1;
        ULONG pending;
        BPTR f;
        snprintf(command, sizeof command, "%s", cases[i].args);
        SetSignal(0, SIGBREAKF_CTRL_C);
        if (cases[i].signal) Signal(FindTask(NULL), SIGBREAKF_CTRL_C);
        rc = RunCommand(segment, 65536, command, strlen(command));
        error = IoErr();
        pending = SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C;
        f = Open("out", MODE_OLDFILE);
        if (f) { n = Read(f, output, sizeof(output) - 1); Close(f); }
        if (n >= 0) output[n] = 0;
        if (rc != cases[i].rc || n < 0 || strcmp(output, cases[i].expected) || pending) {
            Printf("FAIL %s rc=%ld bytes=%ld pending=%lu ioerr=%ld\n", (IPTR)cases[i].name, rc, n, pending, error);
            ++failed;
        } else Printf("PASS %s rc=%ld bytes=%ld pending=%lu ioerr=%ld\n", (IPTR)cases[i].name, rc, n, pending, error);
        DeleteFile("out");
    }
    DeleteFile("one.txt"); DeleteFile("two.txt");
    UnLock(CurrentDir(olddir));
    if (!DeleteFile(dirname)) { Printf("FAIL fixture cleanup\n"); ++failed; }
    UnLoadSeg(segment);
    Printf("TypeRetryNative failures=%ld\n", (LONG)failed);
    return failed ? RETURN_ERROR : RETURN_OK;
}
