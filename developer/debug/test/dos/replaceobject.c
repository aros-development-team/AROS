/*
    Copyright (C) 2026 John KNIPPER <code@jkn.me>.
    Distributed under the AROS Public License, version 1.1.

    Exercise ReplaceObject() on a real filesystem. Usage:

        replaceobject [DIR]

    DIR defaults to RAM:. The test works in DIR/replaceobject.tmp, which
    must not exist, and removes it afterwards. It returns WARN when the
    handler answers ERROR_ACTION_NOT_KNOWN and FAIL on any wrong result.
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

static char work[256], path_a[300], path_b[300], path_c[300];
static int failed;

static void check(BOOL condition, const char *what)
{
    if (!condition)
    {
        printf("FAIL: %s (IoErr %ld)\n", what, (long)IoErr());
        failed++;
    }
}

static BOOL put(const char *path, const char *text)
{
    BPTR fh = Open(path, MODE_NEWFILE);
    LONG length = strlen(text);
    BOOL ok;

    if (fh == BNULL)
        return FALSE;
    ok = Write(fh, text, length) == length;
    return Close(fh) && ok;
}

/* The file's contents, or "" when it cannot be read */
static const char *get(const char *path)
{
    static char buffer[32];
    BPTR fh = Open(path, MODE_OLDFILE);
    LONG length = -1;

    if (fh != BNULL)
    {
        length = Read(fh, buffer, sizeof(buffer) - 1);
        Close(fh);
    }
    buffer[length > 0 ? length : 0] = '\0';
    return buffer;
}

static BOOL missing(const char *path)
{
    BPTR lock = Lock(path, SHARED_LOCK);

    if (lock != BNULL)
    {
        UnLock(lock);
        return FALSE;
    }
    return IoErr() == ERROR_OBJECT_NOT_FOUND;
}

static void refused(const char *source, const char *target, LONG error,
    const char *what)
{
    BOOL result = ReplaceObject(source, target);

    check(!result && IoErr() == error, what);
}

static void name(char *buffer, const char *leaf)
{
    strcpy(buffer, work);
    AddPart(buffer, leaf, sizeof(path_a));
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "RAM:";
    BPTR lock;

    strcpy(work, dir);
    if (!AddPart(work, "replaceobject.tmp", sizeof(work)) || !missing(work))
    {
        printf("%s must not exist\n", work);
        return RETURN_FAIL;
    }
    lock = CreateDir(work);
    if (lock == BNULL)
    {
        printf("Cannot create %s\n", work);
        return RETURN_FAIL;
    }
    UnLock(lock);

    name(path_a, "target");
    name(path_b, "new");
    name(path_c, "drawer");
    check(put(path_a, "old") && put(path_b, "new"), "write files");

    /* The basic replacement */
    if (!ReplaceObject(path_b, path_a) && IoErr() == ERROR_ACTION_NOT_KNOWN)
    {
        printf("%s does not support ACTION_REPLACE_OBJECT\n", dir);
        DeleteFile(path_b);
        DeleteFile(path_a);
        DeleteFile(work);
        return RETURN_WARN;
    }
    check(!strcmp(get(path_a), "new"), "target holds the new data");
    check(missing(path_b), "source name is gone");

    /* No target: a rename */
    check(ReplaceObject(path_a, path_b), "replace onto a missing name");
    check(!strcmp(get(path_b), "new") && missing(path_a), "renamed");
    check(put(path_a, "old"), "write target again");

    /* The same name twice */
    check(ReplaceObject(path_a, path_a), "replace a file by itself");
    check(!strcmp(get(path_a), "old"), "self replacement keeps the data");

    /* Refusals leave both files */
    lock = Lock(path_a, SHARED_LOCK);
    check(lock != BNULL, "lock target");
    refused(path_b, path_a, ERROR_OBJECT_IN_USE, "locked target refused");
    UnLock(lock);

    /* FAT marks a file read-only only when both bits forbid access */
    check(SetProtection(path_a, FIBF_WRITE | FIBF_DELETE), "protect target");
    refused(path_b, path_a, ERROR_DELETE_PROTECTED,
        "delete-protected target refused");
    check(SetProtection(path_a, 0), "unprotect target");

    lock = CreateDir(path_c);
    check(lock != BNULL, "create drawer");
    if (lock != BNULL)
        UnLock(lock);
    refused(path_b, path_c, ERROR_OBJECT_WRONG_TYPE, "drawer target refused");
    refused(path_c, path_a, ERROR_OBJECT_WRONG_TYPE, "drawer source refused");

    check(!strcmp(get(path_a), "old") && !strcmp(get(path_b), "new"),
        "refusals kept both files");

    DeleteFile(path_c);
    DeleteFile(path_b);
    DeleteFile(path_a);
    check(DeleteFile(work), "clean up");

    printf("%s: %s\n", dir, failed ? "FAIL" : "PASS");
    return failed ? RETURN_FAIL : RETURN_OK;
}
