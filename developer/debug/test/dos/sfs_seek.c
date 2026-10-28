/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
/* Run on a disposable SFS volume with at least 2200000000 free bytes. */
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

static int failures;
static const char *name = "SFST:audit-seek-regression.tmp";

static void check(const char *label, LONG offset, LONG mode, int success)
{
    BPTR f = Open((CONST_STRPTR)name, MODE_OLDFILE);
    LONG result, error;
    if (!f) { PrintFault(IoErr(), (CONST_STRPTR)"reopen"); failures++; return; }
    SetIoErr(0);
    result = Seek(f, offset, mode);
    error = IoErr();
    /* Each handle starts at zero, so a successful seek must return zero. */
    if (success ? result != 0 : (result != -1 || error != ERROR_SEEK_ERROR))
        failures++;
    Printf("%s offset=%ld mode=%ld result=%ld error=%ld expected=%ld\n",
           label, offset, mode, result, error, success ? 0L : -1L);
    if (success && result == 0 && offset == 1 && mode == OFFSET_BEGINNING) {
        UBYTE byte = 1;
        if (Read(f, &byte, 1) != 1 || byte != 0) failures++;
        if (Seek(f, -2, OFFSET_CURRENT) != 2) failures++;
        if (Seek(f, 0, OFFSET_CURRENT) != 0) failures++;
    }
    Close(f);
    Flush(Output());
}

int main(void)
{
    struct InfoData *info;
    BPTR lock, file;
    APTR buffer;
    LONG i;
    PutStr("SFS seek regression entered\n"); Flush(Output());
    lock = Lock((CONST_STRPTR)name, ACCESS_READ);
    if (lock) { UnLock(lock); PutStr("Refusing existing test file\n"); return 20; }
    if (IoErr() != ERROR_OBJECT_NOT_FOUND) { PrintFault(IoErr(), "test path"); return 20; }
    info = AllocVec(sizeof(*info), MEMF_PUBLIC | MEMF_CLEAR);
    lock = Lock("SFST:", ACCESS_READ);
    if (!info || !lock) { PrintFault(IoErr(), "setup"); if (info) FreeVec(info); if(lock) UnLock(lock); return 20; }
    if (!Info(lock, info)) { PrintFault(IoErr(), "Info"); UnLock(lock); FreeVec(info); return 20; }
    Printf("filesystem=%lx freeblocks=%ld bytesperblock=%ld\n", info->id_DiskType,
           info->id_NumBlocks-info->id_NumBlocksUsed, info->id_BytesPerBlock);
    i = info->id_DiskType == 0x53465300 &&
        (unsigned long long)(info->id_NumBlocks-info->id_NumBlocksUsed)*info->id_BytesPerBlock >= 2200000000ULL;
    UnLock(lock); FreeVec(info);
    if (!i) { PutStr("Requires SFS and 2200000000 free bytes\n"); return 20; }
    buffer = AllocVec(1048576, MEMF_PUBLIC | MEMF_CLEAR);
    if (!buffer) { PutStr("buffer allocation failed\n"); return 20; }
    file = Open((CONST_STRPTR)name, MODE_NEWFILE);
    if (!file) { PrintFault(IoErr(), "create"); FreeVec(buffer); return 20; }
    i = Write(file, buffer, 16); Close(file);
    if (i != 16) { FreeVec(buffer); DeleteFile((CONST_STRPTR)name); return 20; }
    check("small-zero", 0, OFFSET_BEGINNING, 1);
    check("small-read-relative", 1, OFFSET_BEGINNING, 1);
    check("small-eof", 16, OFFSET_BEGINNING, 1);
    check("small-past-eof", 17, OFFSET_BEGINNING, 0);
    check("small-negative", -1, OFFSET_BEGINNING, 0);
    check("small-end", -1, OFFSET_END, 1);
    check("small-before-start", -17, OFFSET_END, 0);
    file = Open((CONST_STRPTR)name, MODE_NEWFILE);
    if (!file) { PrintFault(IoErr(), "create large"); FreeVec(buffer); DeleteFile((CONST_STRPTR)name); return 20; }
    for (i=0; i<2049; i++) {
        if (Write(file, buffer, 1048576) != 1048576) { PrintFault(IoErr(), "write"); break; }
    }
    Close(file); FreeVec(buffer);
    Printf("written_MiB=%ld\n", i); Flush(Output());
    if(i != 2049) { DeleteFile((CONST_STRPTR)name); return 20; }
    check("large-zero", 0, OFFSET_BEGINNING, 1);
    check("large-read-relative", 1, OFFSET_BEGINNING, 1);
    check("large-LONG_MAX", 2147483647L, OFFSET_BEGINNING, 1);
    check("large-negative", -1, OFFSET_BEGINNING, 0);
    check("large-current-zero", 0, OFFSET_CURRENT, 1);
    check("large-current-negative", -1, OFFSET_CURRENT, 0);
    check("large-end-in-range", -2147483647L, OFFSET_END, 1);
    /* The existing signed destination limit remains unchanged. */
    check("large-end-out-of-range", 0, OFFSET_END, 0);
    if (!DeleteFile((CONST_STRPTR)name)) { PrintFault(IoErr(), "cleanup"); return 20; }
    Printf("temporary file deleted; failures=%ld\n", (LONG)failures);
    return failures ? 10 : 0;
}
