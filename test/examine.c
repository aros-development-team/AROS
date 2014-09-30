/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>
#include <dos/datetime.h>
#include <proto/dos.h>
#include <stdio.h>

#define ENTRYTYPESTR(e) ( \
    e == ST_PIPEFILE ? "ST_PIPEFILE" : \
    e == ST_LINKFILE ? "ST_LINKFILE" : \
    e == ST_FILE     ? "ST_FILE" :     \
    e == ST_ROOT     ? "ST_ROOT" :     \
    e == ST_USERDIR  ? "ST_USERDIR" :  \
    e == ST_SOFTLINK ? "ST_SOFTLINK" : \
    e == ST_LINKDIR  ? "ST_LINKDIR" :  \
                       "unknown"       \
)

int main (int argc, char **argv) {
    BPTR lock;
    struct FileInfoBlock *fib;
    struct DateTime dt;
    char date[32], time[32];

    if (argc == 0) {
        printf("usage: %s file\n", argv[0]);
        return 1;
    }

    lock = Lock(argv[1], SHARED_LOCK);
    if (lock == BNULL) {
        printf("couldn't open file [%ld]\n", (long)IoErr());
        return 1;
    }

    fib = (struct FileInfoBlock *) AllocDosObject(DOS_FIB, NULL);
    if (fib == NULL) {
        printf("couldn't allocate FileInfoBlock structure\n");
        UnLock(lock);
        return 1;
    }

    if (! Examine(lock, fib)) {
        printf("Examine() failed [%ld]\n", (long)IoErr());
        FreeDosObject(DOS_FIB, fib);
        UnLock(lock);
        return 1;
    }

    printf("fib_DiskKey     : 0x%lx\n",   fib->fib_DiskKey);
    printf("fib_DirEntryType: %d [%s]\n", (int)fib->fib_DirEntryType, ENTRYTYPESTR(fib->fib_DirEntryType));
    printf("fib_FileName    : %s\n",      fib->fib_FileName);
    printf("fib_Protection  : 0x%04x\n",  (unsigned)fib->fib_Protection);
    printf("fib_EntryType   : %d [%s]\n", (int)fib->fib_EntryType, ENTRYTYPESTR(fib->fib_EntryType));
    printf("fib_Size        : %d\n",      (int)fib->fib_Size);
    printf("fib_NumBlocks   : %d\n",      (int)fib->fib_NumBlocks);

    dt.dat_Stamp = fib->fib_Date;
    dt.dat_Format = FORMAT_DOS;
    dt.dat_Flags = 0;
    dt.dat_StrDay = NULL;
    dt.dat_StrDate = date;
    dt.dat_StrTime = time;
    DateToStr(&dt);
    
    printf("fib_Date        : %s %s\n", date, time);

    printf("fib_Comment     : %s\n", fib->fib_Comment);
    printf("fib_OwnerUID    : %d\n", fib->fib_OwnerUID);
    printf("fib_OwnerGID    : %d\n", fib->fib_OwnerGID);

    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);

    return 0;
}
