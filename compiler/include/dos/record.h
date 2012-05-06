#ifndef DOS_RECORD_H
#define DOS_RECORD_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Record structures and definitions.
    Lang: english
*/

#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif


/* LockRecord() and LockRecords() locking modes. EXCLUSIVE modes mean that
   nobody else is allowed to lock a specific record, which is allowed, when
   locking with SHARED mode. When using IMMED modes, the timeout is ignored. */
#define REC_EXCLUSIVE       0
#define REC_EXCLUSIVE_IMMED 1
#define REC_SHARED          2
#define REC_SHARED_IMMED    3


/* Structure as passed to LockRecords() and UnLockRecords(). */
struct RecordLock64
{
    BPTR  rec_FH;     /* (struct FileHandle *) The file to get the current
                         record from. */
    UQUAD rec_Offset; /* The offset, the current record should start. */
    UQUAD rec_Length; /* The length of the current record. */
    ULONG rec_Mode;   /* The mode od locking (see above). */
};

struct RecordLock32
{
    BPTR  rec_FH;     /* (struct FileHandle *) The file to get the current
                         record from. */
    ULONG rec_Offset; /* The offset, the current record should start. */
    ULONG rec_Length; /* The length of the current record. */
    ULONG rec_Mode;   /* The mode od locking (see above). */
};

#if (__DOS64)
#define RecordLock RecordLock64 
#else
#define RecordLock RecordLock32
#endif

#endif /* DOS_RECORD_H */
