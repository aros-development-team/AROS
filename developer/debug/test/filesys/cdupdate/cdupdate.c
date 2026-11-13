/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include <aros/debug.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <string.h>

static ULONG failures;
static void check(BOOL okay, CONST_STRPTR message)
{
    if (!okay) { bug("CDUPDATE FAIL: %s\n", message); failures++; }
}

int main(void)
{
    CONST_STRPTR file_name = "CD0:intro.dat";
    CONST_STRPTR missing_name = "CD0:__aros_cdupdate_missing__";
    struct InfoData info;
    BPTR lock = BNULL, file = BNULL;
    UBYTE reference[64], actual[64];
    LONG got, written, error;

    lock = Lock("CD0:", ACCESS_READ);
    if (!lock || !Info(lock, &info) || info.id_DiskState != ID_WRITE_PROTECTED)
    {
        bug("CDUPDATE FAIL: test requires a write-protected CD0: volume\n");
        failures++;
        goto end;
    }
    UnLock(lock); lock = BNULL;
    file = Open(file_name, MODE_OLDFILE);
    if (!file) { check(FALSE, "open reference file"); goto end; }
    got = Read(file, reference, sizeof(reference));
    check(got == sizeof(reference), "read reference bytes");
    written = Write(file, reference, 1); error = IoErr();
    check(written == -1 && error == ERROR_DISK_WRITE_PROTECTED,
          "write through a read handle reports write-protected failure");
    Close(file); file = BNULL;
    if (got != sizeof(reference)) goto end;

    file = Open(file_name, MODE_READWRITE);
    check(file != BNULL, "update-open existing CD file succeeds");
    if (file)
    {
        check(Read(file, actual, sizeof(actual)) == sizeof(actual) &&
              memcmp(reference, actual, sizeof(actual)) == 0,
              "update handle reads original data");
        check(Seek(file, 0, OFFSET_BEGINNING) == sizeof(actual), "seek update handle");
        got = Write(file, reference, 1); error = IoErr();
        check(got == -1 && error == ERROR_DISK_WRITE_PROTECTED,
              "write fails with -1 and write-protected error");
        check(Seek(file, 0, OFFSET_BEGINNING) >= 0, "seek after rejected write");
        check(Read(file, actual, sizeof(actual)) == sizeof(actual) &&
              memcmp(reference, actual, sizeof(actual)) == 0,
              "rejected write leaves original data intact");
        Close(file); file = BNULL;
    }

    file = Open(file_name, MODE_NEWFILE); error = IoErr();
    check(file == BNULL && error == ERROR_DISK_WRITE_PROTECTED,
          "new-file open cannot truncate an existing CD file");
    if (file) { Close(file); file = BNULL; }
    file = Open(missing_name, MODE_READWRITE); error = IoErr();
    check(file == BNULL && error == ERROR_DISK_WRITE_PROTECTED,
          "update-open cannot create a missing CD file");
    if (file) { Close(file); file = BNULL; }
    lock = Lock(missing_name, ACCESS_READ);
    check(lock == BNULL, "missing update target was not created");
    if (lock) { UnLock(lock); lock = BNULL; }
    file = Open("CD0:", MODE_READWRITE); error = IoErr();
    check(file == BNULL && error == ERROR_OBJECT_WRONG_TYPE,
          "update-open rejects directories");
    if (file) { Close(file); file = BNULL; }

    file = Open(file_name, MODE_OLDFILE);
    check(file && Read(file, actual, sizeof(actual)) == sizeof(actual) &&
          memcmp(reference, actual, sizeof(actual)) == 0,
          "failed create and truncate operations preserve the file");
end:
    if (file) Close(file);
    if (lock) UnLock(lock);
    bug("CDUPDATE TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
