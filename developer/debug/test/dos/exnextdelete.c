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

static int dos_error(const char *operation, const char *path)
{
    LONG error = IoErr();

    fprintf(stderr, "exnextdelete: %s %s failed (IoErr %ld)\n",
        operation, path, (long)error);
    return 0;
}

static int cleanup(void)
{
    static const char *names[] = {
        "a/1", "a/2", "a/3", "b/1", "b/2", "c/other",
        "d/spare", "d/kept", "e/1", "e/2", "a", "b", "c", "d", "e"
    };
    char path[128];
    unsigned int i;
    int ok = 1;

    /* Only remove known test names inside the root created by this run. */
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)
    {
        snprintf(path, sizeof(path), "%s/%s", root, names[i]);
        if (!DeleteFile(path) && IoErr() != ERROR_OBJECT_NOT_FOUND)
            ok = dos_error("DeleteFile", path);
    }
    if (!DeleteFile(root))
        ok = dos_error("DeleteFile", root);
    return ok;
}

static int make_dir(const char *name)
{
    char path[128];
    BPTR lock;

    snprintf(path, sizeof(path), "%s/%s", root, name);
    lock = CreateDir(path);
    if (!lock)
        return dos_error("CreateDir", path);
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
        return dos_error("Open", path);
    if (!Close(file))
        return dos_error("Close", path);
    return 1;
}

static int delete_name(const char *name)
{
    char path[128];

    snprintf(path, sizeof(path), "%s/%s", root, name);
    return DeleteFile(path) != 0 || dos_error("DeleteFile", path);
}

static int scan_start(const char *name, BPTR *lock, struct FileInfoBlock *fib)
{
    char path[128];

    snprintf(path, sizeof(path), "%s/%s", root, name);
    *lock = Lock(path, SHARED_LOCK);
    if (!*lock)
        return dos_error("Lock", path);
    return Examine(*lock, fib) || dos_error("Examine", path);
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
    BPTR lock = BNULL;
    int ok = 1;
    int length;

    length = snprintf(root, sizeof(root), "%sexnextdelete", argc > 1 ? argv[1] : "RAM:");
    if (!fib)
    {
        dos_error("AllocDosObject", "DOS_FIB");
        return RETURN_ERROR;
    }
    if (length < 0 || (size_t)length >= sizeof(root))
    {
        fprintf(stderr, "exnextdelete: test directory path is too long\n");
        FreeDosObject(DOS_FIB, fib);
        return RETURN_ERROR;
    }
    lock = CreateDir(root);
    if (!lock)
    {
        dos_error("CreateDir", root);
        FreeDosObject(DOS_FIB, fib);
        return RETURN_ERROR;
    }
    UnLock(lock);
    lock = BNULL;
#define REQUIRE(expr) do { if (!(expr)) { ok = 0; goto out; } } while (0)
    REQUIRE(make_dir("a") && make_dir("b") && make_dir("c") && make_dir("d")
        && make_dir("e"));

    /* The next entry of a running scan is deleted. */
    REQUIRE(make_file("a/1") && make_file("a/2") && make_file("a/3"));
    REQUIRE(scan_start("a", &lock, fib) && ExNext(lock, fib));
    REQUIRE(delete_name("a/2"));
    ok &= scan_check("next entry deleted", lock, fib, "3 ");
    UnLock(lock);
    lock = BNULL;

    /* The freed entry's memory is reused by a file in another directory. */
    REQUIRE(make_file("b/1") && make_file("b/2"));
    REQUIRE(scan_start("b", &lock, fib));
    REQUIRE(delete_name("b/1"));
    REQUIRE(make_file("c/other"));
    ok &= scan_check("slot reused", lock, fib, "2 ");
    UnLock(lock);
    lock = BNULL;

    /* A finished scan stays finished. */
    REQUIRE(scan_start("b", &lock, fib));
    ok &= scan_check("full scan", lock, fib, "2 ");
    ok &= scan_check("after the end", lock, fib, "");
    UnLock(lock);
    lock = BNULL;

    /* Another block of the same size is freed after the entry. "kept" stays
       between the two so that they do not merge. */
    REQUIRE(make_file("d/spare") && make_file("d/kept"));
    REQUIRE(make_file("e/1") && make_file("e/2"));
    REQUIRE(scan_start("e", &lock, fib));
    REQUIRE(delete_name("e/1") && delete_name("d/spare"));
    ok &= scan_check("later free", lock, fib, "2 ");
out:
    if (lock)
        UnLock(lock);
    ok &= cleanup();

    FreeDosObject(DOS_FIB, fib);
    printf("exnextdelete: %s\n", ok ? "passed" : "FAILED");
    return ok ? RETURN_OK : RETURN_FAIL;
}
