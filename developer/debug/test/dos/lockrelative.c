/*
    Copyright (C) 2021-2026, The AROS Development Team. All rights reserved.

    Desc: Exercise dosrelative.library: locate, create, open, rename and
          delete objects relative to a directory lock.
*/

#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/dosrelative.h>

struct Library *DOSRelativeBase;

static int failures;

static void report(const char *what, BOOL ok)
{
    if (ok)
        printf("  ok    %s\n", what);
    else
    {
        failures++;
        PrintFault(IoErr(), (STRPTR)what);
    }
}

static void selftest(void)
{
    char path[256];
    char buf[64];
    BPTR base, sub, fh, lock;
    LONG n;

    printf("self test in T:\n");

    base = Lock("T:", SHARED_LOCK);
    if (base == BNULL)
    {
        PrintFault(IoErr(), "lockrelative: T:");
        failures++;
        return;
    }

    DeleteFileRelative(base, "lockrelative-test/renamed");
    DeleteFileRelative(base, "lockrelative-test/link");
    DeleteFileRelative(base, "lockrelative-test/file");
    DeleteFileRelative(base, "lockrelative-test");

    sub = CreateDirRelative(base, "lockrelative-test");
    report("CreateDirRelative", sub != BNULL);
    if (sub == BNULL)
    {
        UnLock(base);
        return;
    }
    UnLock(sub);

    sub = LockRelative(base, "lockrelative-test", SHARED_LOCK);
    report("LockRelative (directory)", sub != BNULL);
    if (sub == BNULL)
    {
        UnLock(base);
        return;
    }

    if (NameFromLock(sub, path, sizeof(path)))
        printf("        %s\n", path);

    fh = OpenRelative(sub, "file", MODE_NEWFILE);
    report("OpenRelative MODE_NEWFILE", fh != BNULL);
    if (fh != BNULL)
    {
        n = Write(fh, "relative\n", 9);
        report("Write", n == 9);
        Close(fh);
    }

    fh = OpenRelative(sub, "file", MODE_OLDFILE);
    report("OpenRelative MODE_OLDFILE", fh != BNULL);
    if (fh != BNULL)
    {
        memset(buf, 0, sizeof(buf));
        n = Read(fh, buf, sizeof(buf) - 1);
        report("Read back", n == 9 && strcmp(buf, "relative\n") == 0);
        Close(fh);
    }

    lock = LockRelative(base, "lockrelative-test/file", SHARED_LOCK);
    report("LockRelative (path with directory)", lock != BNULL);
    if (lock != BNULL)
        UnLock(lock);

    /* Soft link following: the handler answers ERROR_IS_SOFT_LINK and the
     * library re-resolves the link target relative to the same lock. */
    if (!MakeLinkRelative(sub, "link", (SIPTR)"file", TRUE) &&
        (IoErr() == ERROR_NOT_IMPLEMENTED || IoErr() == ERROR_ACTION_NOT_KNOWN))
        printf("  skip  MakeLinkRelative (filesystem has no soft links)\n");
    else
    {
        report("MakeLinkRelative (soft)", TRUE);

        lock = LockRelative(sub, "link", SHARED_LOCK);
        report("LockRelative (through soft link)", lock != BNULL);
        if (lock != BNULL)
            UnLock(lock);

        fh = OpenRelative(sub, "link", MODE_OLDFILE);
        report("OpenRelative (through soft link)", fh != BNULL);
        if (fh != BNULL)
        {
            memset(buf, 0, sizeof(buf));
            n = Read(fh, buf, sizeof(buf) - 1);
            report("Read through soft link", n == 9 && strcmp(buf, "relative\n") == 0);
            Close(fh);
        }

        report("DeleteFileRelative (soft link)", DeleteFileRelative(sub, "link"));
    }

    lock = LockRelative(sub, "/", SHARED_LOCK);
    report("LockRelative (parent reference)", lock != BNULL && SameLock(lock, base) == LOCK_SAME);
    if (lock != BNULL)
        UnLock(lock);

    lock = LockRelative(sub, "T:lockrelative-test/file", SHARED_LOCK);
    report("LockRelative (absolute name ignores lock)", lock != BNULL);
    if (lock != BNULL)
        UnLock(lock);

    lock = LockRelative(sub, "does-not-exist", SHARED_LOCK);
    report("LockRelative (missing object fails)", lock == BNULL && IoErr() == ERROR_OBJECT_NOT_FOUND);
    if (lock != BNULL)
        UnLock(lock);

    report("IsFileSystemRelative", IsFileSystemRelative(sub, "file"));
    report("SetCommentRelative", SetCommentRelative(sub, "file", "relative comment"));
    report("SetProtectionRelative", SetProtectionRelative(sub, "file", FIBF_EXECUTE));
    report("RenameRelative (same directory)", RenameRelative(sub, "file", sub, "renamed"));
    report("RenameRelative (across locks)", RenameRelative(sub, "renamed", base, "lockrelative-test/file"));
    report("RenameRelative (relative to absolute)", RenameRelative(sub, "file", BNULL, "T:lockrelative-test/renamed"));

    {
        struct DevProc *dvp = GetDeviceProcRelative(sub, "renamed", NULL);
        report("GetDeviceProcRelative", dvp != NULL && dvp->dvp_Lock == sub);
        if (dvp != NULL)
        {
            dvp = GetDeviceProcRelative(sub, "renamed", dvp);
            report("GetDeviceProcRelative (no more entries)", dvp == NULL && IoErr() == ERROR_NO_MORE_ENTRIES);
            FreeDeviceProc(dvp);
        }
    }

    report("DeleteFileRelative (file)", DeleteFileRelative(sub, "renamed"));
    UnLock(sub);
    report("DeleteFileRelative (directory)", DeleteFileRelative(base, "lockrelative-test"));

    UnLock(base);
}

int main(int argc, char **argv)
{
    int rc = 0;

    DOSRelativeBase = OpenLibrary("dosrelative.library", 50);
    if (DOSRelativeBase == NULL)
    {
        printf("lockrelative: cannot open dosrelative.library\n");
        return 20;
    }

    if (argc < 3)
    {
        selftest();
        printf("%d failure(s)\n", failures);
        rc = failures ? 10 : 0;
    }
    else
    {
        BPTR baselock = Lock(argv[1], ACCESS_READ);
        if (baselock == BNULL)
        {
            PrintFault(IoErr(), "lockrelative: base lock");
            rc = 10;
        }
        else
        {
            BPTR filelock = LockRelative(baselock, argv[2], ACCESS_READ);
            if (filelock == BNULL)
            {
                PrintFault(IoErr(), "lockrelative: file lock");
                rc = 10;
            }
            else
            {
                char path[256];
                if (NameFromLock(filelock, path, sizeof(path)) == 0)
                {
                    PrintFault(IoErr(), "lockrelative: name from lock");
                    rc = 10;
                }
                else
                    printf("%s\n", path);
                UnLock(filelock);
            }
            UnLock(baselock);
        }
    }

    CloseLibrary(DOSRelativeBase);
    return rc;
}
