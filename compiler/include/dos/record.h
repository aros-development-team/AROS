#ifndef DOS_RECORD_H
#define DOS_RECORD_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Record structures
    Lang: english
*/

#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

struct RecordLock
{
    BPTR  rec_FH;
    ULONG rec_Offset;
    ULONG rec_Length;
    ULONG rec_Mode;
};

/* LockRecord() and LockRecords() modes */
#define REC_EXCLUSIVE       0
#define REC_EXCLUSIVE_IMMED 1
#define REC_SHARED          2
#define REC_SHARED_IMMED    3

#endif /* DOS_RECORD_H */
