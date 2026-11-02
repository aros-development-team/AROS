/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Test ExNext() after the directory changes between Examine() and the first
    ExNext(), and ExNext() after a finished scan.

    Usage: exnextdelete [device:]

    The default device is RAM:. Without the ram-handler fix the "later free"
    case crashes the handler, so a cold boot is needed after a failed run.
*/

#include <proto/dos.h>

#include <dos/dos.h>
#include <dos/exall.h>

#include <stdio.h>
#include <string.h>

static char root[64];

static int make_dir(const char *name)
{
    char path[128];
    BPTR lock;

    snprintf(path, sizeof(path), "%s/%s", root, name);
    lock = CreateDir(path);
    if (!lock)
        return 0;
    UnLock(lock);
    return 1;
}

static int make_file(const char *name)
{
    char path[128];
    BPTR file;

    snprintf(path, sizeof(path), "%s/%s", root, name);
    file = Open(path, MODE_NEWFILE);
    if (!file)
        return 0;
    Close(file);
    return 1;
}

static int delete_name(const char *name)
{
    char path[128];

    snprintf(path, sizeof(path), "%s/%s", root, name);
    return DeleteFile(path) != 0;
}

static int scan_start(const char *name, BPTR *lock, struct FileInfoBlock *fib)
{
    char path[128];

    snprintf(path, sizeof(path), "%s/%s", root, name);
    *lock = Lock(path, SHARED_LOCK);
    return *lock && Examine(*lock, fib);
}

static int scan_check(const char *label, BPTR lock, struct FileInfoBlock *fib,
    const char *expected)
{
    char seen[128] = "";
    int ok;

    while (ExNext(lock, fib))
    {
        strncat(seen, fib->fib_FileName, sizeof(seen) - strlen(seen) - 1);
        strncat(seen, " ", sizeof(seen) - strlen(seen) - 1);
    }
    ok = IoErr() == ERROR_NO_MORE_ENTRIES && !strcmp(seen, expected);
    printf("%s: [%s] %s\n", label, seen, ok ? "ok" : "FAILED");
    fflush(stdout);
    return ok;
}

int main(int argc, char **argv)
{
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    BPTR lock;
    int ok;

    snprintf(root, sizeof(root), "%sexnextdelete", argc > 1 ? argv[1] : "RAM:");
    lock = CreateDir(root);
    if (!fib || !lock)
        return RETURN_ERROR;
    UnLock(lock);
    ok = make_dir("a") && make_dir("b") && make_dir("c") && make_dir("d")
        && make_dir("e");

    /* The next entry of a running scan is deleted. */
    ok &= make_file("a/1") && make_file("a/2") && make_file("a/3");
    ok &= scan_start("a", &lock, fib) && ExNext(lock, fib);
    ok &= delete_name("a/2");
    ok &= scan_check("next entry deleted", lock, fib, "3 ");
    UnLock(lock);

    /* The freed entry's memory is reused by a file in another directory. */
    ok &= make_file("b/1") && make_file("b/2");
    ok &= scan_start("b", &lock, fib);
    ok &= delete_name("b/1");
    ok &= make_file("c/other");
    ok &= scan_check("slot reused", lock, fib, "2 ");
    UnLock(lock);

    /* A finished scan stays finished. */
    ok &= scan_start("b", &lock, fib);
    ok &= scan_check("full scan", lock, fib, "2 ");
    ok &= scan_check("after the end", lock, fib, "");
    UnLock(lock);

    /* Another block of the same size is freed after the entry. "kept" stays
       between the two so that they do not merge. */
    ok &= make_file("d/spare") && make_file("d/kept");
    ok &= make_file("e/1") && make_file("e/2");
    ok &= scan_start("e", &lock, fib);
    ok &= delete_name("e/1") && delete_name("d/spare");
    ok &= scan_check("later free", lock, fib, "2 ");
    UnLock(lock);

    FreeDosObject(DOS_FIB, fib);
    printf("exnextdelete: %s\n", ok ? "passed" : "FAILED");
    return ok ? RETURN_OK : RETURN_FAIL;
}
