/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: EfsTest - exercise a mounted Envoy export (or any volume) through
          dos.library and report each step.

    EfsTest DEVICE/A
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/exall.h>
#include <dos/notify.h>
#include <exec/memory.h>
#include <stdio.h>
#include <string.h>

static int fails;

static void Report(const char *what, BOOL ok)
{
    LONG err = IoErr();             /* before printf, which may change it */
    printf("  %-44s %s\n", what, ok ? "ok" : "FAILED");
    if (!ok)
    {
        printf("    IoErr %ld\n", (long)err);
        fails++;
    }
    fflush(stdout);
}

static void Path(char *dst, const char *dev, const char *rest)
{
    strcpy(dst, dev);
    strcat(dst, rest);
}

int main(void)
{
    IPTR args[1] = { 0 };
    struct RDArgs *rda;
    const char *dev;
    char path[256], path2[256];
    BPTR lock, lock2, fh;
    struct FileInfoBlock *fib;
    UBYTE *big, *back;
    LONG n, i;

    if (!(rda = ReadArgs("DEVICE/A", args, NULL)))
    {
        PrintFault(IoErr(), "EfsTest");
        return RETURN_FAIL;
    }
    dev = (const char *)args[0];
    fib = AllocDosObject(DOS_FIB, NULL);
    big = AllocVec(40000, MEMF_ANY);
    back = AllocVec(65536, MEMF_ANY);
    if (!fib || !big || !back)
        return RETURN_FAIL;
    for (i = 0; i < 40000; i++)
        big[i] = (UBYTE)(i * 7 + (i >> 8));

    printf("EfsTest on %s\n", dev);

    /* root */
    lock = Lock((STRPTR)dev, SHARED_LOCK);
    Report("Lock root", lock != BNULL);
    if (lock)
    {
        BOOL ok = Examine(lock, fib);
        Report("Examine root", ok && fib->fib_DirEntryType > 0);
        if (ok)
            printf("    root name '%s', type %ld, protection %08lx\n", fib->fib_FileName, (long)fib->fib_DirEntryType, (unsigned long)fib->fib_Protection);
        n = 0;
        while (ExNext(lock, fib))
        {
            printf("    %-24s %8ld  %s\n", fib->fib_FileName, (long)fib->fib_Size, fib->fib_DirEntryType > 0 ? "(dir)" : "");
            n++;
        }
        Report("ExNext listing ends with no more entries", IoErr() == ERROR_NO_MORE_ENTRIES && n > 0);
        printf("    %ld entries\n", (long)n);

        /* ExAll */
        {
            struct ExAllControl *eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
            UBYTE *buf = AllocVec(4096, MEMF_ANY);
            LONG total = 0, exerr = 0;
            BOOL more;
            if (eac && buf)
            {
                eac->eac_LastKey = 0;
                eac->eac_MatchString = NULL;
                eac->eac_MatchFunc = NULL;
                do
                {
                    struct ExAllData *ed;
                    more = ExAll(lock, (struct ExAllData *)buf, 4096, ED_COMMENT, eac);
                    exerr = more ? 0 : IoErr();
                    for (ed = (struct ExAllData *)buf; eac->eac_Entries && ed; ed = ed->ed_Next)
                    {
                        printf("    exall: %-20s type %ld size %lu prot %08lx comment '%s'\n", ed->ed_Name, (long)ed->ed_Type, (unsigned long)ed->ed_Size, (unsigned long)ed->ed_Prot, ed->ed_Comment ? (char *)ed->ed_Comment : "");
                        total++;
                    }
                } while (more);
                printf("    ExAll ended with %ld\n", (long)exerr);
                Report("ExAll(ED_COMMENT) returns the entries", total == n && exerr == ERROR_NO_MORE_ENTRIES);
            }
            if (eac) FreeDosObject(DOS_EXALLCONTROL, eac);
            if (buf) FreeVec(buf);
        }
        lock2 = DupLock(lock);
        Report("DupLock root", lock2 != BNULL);
        if (lock2)
        {
            Report("SameLock(root, dup) == LOCK_SAME", SameLock(lock, lock2) == LOCK_SAME);
            UnLock(lock2);
        }
        UnLock(lock);
    }

    /* write, read back, seek */
    Path(path, dev, "big.bin");
    fh = Open(path, MODE_NEWFILE);
    Report("Open big.bin MODE_NEWFILE", fh != BNULL);
    if (fh)
    {
        n = Write(fh, big, 40000);
        Report("Write 40000 bytes", n == 40000);
        Report("Close", Close(fh));
    }
    fh = Open(path, MODE_OLDFILE);
    Report("Open big.bin MODE_OLDFILE", fh != BNULL);
    if (fh)
    {
        n = Read(fh, back, 65536);
        printf("    Read returned %ld\n", (long)n);
        Report("Read returns 40000 (buffer 65536)", n == 40000);
        Report("Read data equals written data", n == 40000 && memcmp(big, back, 40000) == 0);
        Report("Seek(-100, OFFSET_END) returns 40000", Seek(fh, -100, OFFSET_END) == 40000);
        n = Read(fh, back, 1000);
        Report("Read at end gives the last 100 bytes", n == 100 && memcmp(big + 39900, back, 100) == 0);
        Report("Seek(10, OFFSET_BEGINNING)", Seek(fh, 10, OFFSET_BEGINNING) == 40000);
        n = Read(fh, back, 16);
        Report("Read 16 bytes at offset 10", n == 16 && memcmp(big + 10, back, 16) == 0);
        Report("ExamineFH size 40000", ExamineFH(fh, fib) && fib->fib_Size == 40000);
        Close(fh);
    }
    Path(path, dev, "hello.txt");
    fh = Open(path, MODE_OLDFILE);
    Report("Open hello.txt", fh != BNULL);
    if (fh)
    {
        n = Read(fh, back, 200);
        back[n > 0 ? n : 0] = '\0';
        Report("Read hello.txt", n > 0);
        printf("    '%s'\n", back);
        Close(fh);
    }

    /* directory, rename, comment, protect, date, delete */
    Path(path, dev, "NewDir");
    lock = CreateDir(path);
    Report("CreateDir NewDir", lock != BNULL);
    if (lock)
    {
        Report("Examine NewDir is a directory", Examine(lock, fib) && fib->fib_DirEntryType > 0);
        lock2 = ParentDir(lock);
        Report("ParentDir(NewDir) is the root", lock2 != BNULL && Examine(lock2, fib) && fib->fib_DirEntryType > 0);
        if (lock2) UnLock(lock2);
        UnLock(lock);
    }
    Path(path, dev, "big.bin");
    Path(path2, dev, "NewDir/big2.bin");
    Report("Rename big.bin -> NewDir/big2.bin", Rename(path, path2));
    Report("SetComment", SetComment(path2, "a comment"));
    Report("SetProtection(FIBF_DELETE)", SetProtection(path2, FIBF_DELETE));
    {
        struct DateStamp ds = { 17809, 1066, 148 };
        Report("SetFileDate", SetFileDate(path2, &ds));
    }
    lock = Lock(path2, SHARED_LOCK);
    Report("Lock NewDir/big2.bin", lock != BNULL);
    if (lock)
    {
        BOOL ok = Examine(lock, fib);
        Report("Examine: comment and protection as set", ok && !strcmp((char *)fib->fib_Comment, "a comment") && (fib->fib_Protection & FIBF_DELETE));
        Report("Examine: date as set", ok && fib->fib_Date.ds_Days == 17809 && fib->fib_Date.ds_Minute == 1066);
        printf("    size %ld prot %08lx date %ld/%ld/%ld comment '%s'\n", (long)fib->fib_Size, (unsigned long)fib->fib_Protection, (long)fib->fib_Date.ds_Days, (long)fib->fib_Date.ds_Minute, (long)fib->fib_Date.ds_Tick, fib->fib_Comment);
        fh = OpenFromLock(lock);
        Report("OpenFromLock", fh != BNULL);
        if (fh)
        {
            n = Read(fh, back, 100);
            Report("Read through OpenFromLock", n == 100 && memcmp(big, back, 100) == 0);
            Close(fh);
        }
        else
            UnLock(lock);
    }
    Report("SetProtection(0)", SetProtection(path2, 0));
    Report("DeleteFile NewDir/big2.bin", DeleteFile(path2));
    Path(path, dev, "NewDir");
    Report("DeleteFile NewDir", DeleteFile(path));

    /* notification */
    {
        struct NotifyRequest *nr = AllocVec(sizeof(struct NotifyRequest), MEMF_PUBLIC | MEMF_CLEAR);
        LONG sig = AllocSignal(-1);
        Path(path, dev, "notify.txt");
        if (nr && sig >= 0)
        {
            nr->nr_Name = path;
            nr->nr_Flags = NRF_SEND_SIGNAL;
            nr->nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
            nr->nr_stuff.nr_Signal.nr_SignalNum = sig;
            SetSignal(0, 1UL << sig);
            Report("StartNotify notify.txt", StartNotify(nr));
            fh = Open(path, MODE_NEWFILE);
            if (fh)
            {
                Write(fh, "x", 1);
                Close(fh);
            }
            for (i = 0; i < 30 && !(SetSignal(0, 0) & (1UL << sig)); i++)
                Delay(10);
            Report("notification signal arrived", (SetSignal(0, 1UL << sig) & (1UL << sig)) != 0);
            EndNotify(nr);
            DeleteFile(path);
        }
        if (sig >= 0) FreeSignal(sig);
        if (nr) FreeVec(nr);
    }

    /* info */
    lock = Lock((STRPTR)dev, SHARED_LOCK);
    if (lock)
    {
        struct InfoData *id = AllocVec(sizeof(struct InfoData), MEMF_PUBLIC | MEMF_CLEAR);
        if (id)
        {
            BOOL ok = Info(lock, id);
            Report("Info", ok && id->id_NumBlocks > 0);
            if (ok)
                printf("    blocks %lu used %lu bpb %ld state %ld type %08lx\n", (unsigned long)id->id_NumBlocks, (unsigned long)id->id_NumBlocksUsed, (long)id->id_BytesPerBlock, (long)id->id_DiskState, (unsigned long)id->id_DiskType);
            FreeVec(id);
        }
        Report("IsFileSystem", IsFileSystem((STRPTR)dev));
        UnLock(lock);
    }

    /* failures */
    Path(path, dev, "no/such/file.txt");
    fh = Open(path, MODE_OLDFILE);
    Report("Open of a missing file fails with 205", fh == BNULL && IoErr() == ERROR_OBJECT_NOT_FOUND);
    if (fh) Close(fh);
    Path(path, dev, "nosuch");
    Report("DeleteFile of a missing object fails", !DeleteFile(path));
    lock = Lock(path, SHARED_LOCK);
    Report("Lock of a missing object fails with 205", lock == BNULL && IoErr() == ERROR_OBJECT_NOT_FOUND);
    if (lock) UnLock(lock);

    printf("%d failures\n", fails);
    FreeVec(big);
    FreeVec(back);
    FreeDosObject(DOS_FIB, fib);
    FreeArgs(rda);
    return fails ? RETURN_WARN : RETURN_OK;
}
