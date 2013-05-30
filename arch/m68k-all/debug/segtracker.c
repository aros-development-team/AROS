/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: m68k SegTracker emulation.
*/

#include <aros/config.h>
#include <aros/debug.h>
#include <libraries/debug.h>
#include <proto/exec.h>
#include <proto/debug.h>

#include "debug_intern.h"

struct SegSem
{
    struct SignalSemaphore seg_Semaphore;
    APTR seg_Find;
    /* Dummy list for programs that directly
     * read SegTracker segment list.
     */
    struct MinList seg_List;
};
#define SEG_SEM "SegTracker"

static AROS_UFH3(UBYTE*, SegTrack,
    AROS_UFHA(ULONG, Address, A0),
    AROS_UFHA(ULONG*, SegNum, A1),
    AROS_UFHA(ULONG*, Offset, A2))
{
    AROS_USERFUNC_INIT

    /* Bad code again but SegTrack() may be called in Supervisor context.. */
    struct DebugBase *DebugBase = (struct DebugBase*)FindName(&SysBase->LibList, "debug.library");
    if (DebugBase) {
        ULONG segNum2;
        BPTR segPtr, firstSeg;
        char *modName;
        if (DecodeLocation(Address, DL_SegmentNumber, &segNum2, DL_SegmentPointer, &segPtr,
            DL_FirstSegment, &firstSeg, DL_ModuleName, &modName, TAG_DONE)) {
            if (SegNum == Offset) {
                *SegNum = firstSeg;
            } else {
                *SegNum = segNum2;
                *Offset = Address - (ULONG)BADDR(segPtr);
            }
            return modName;
        }
    }

    return NULL;

    AROS_USERFUNC_EXIT
}

static int SegTracker_Init(struct DebugBase *DebugBase)
{
    struct SegSem *ss;

    ss = AllocMem(sizeof(struct SegSem), MEMF_CLEAR | MEMF_PUBLIC);
    if (ss) {
        ss->seg_Semaphore.ss_Link.ln_Name = SEG_SEM;
        ss->seg_Semaphore.ss_Link.ln_Pri = -127;
        ss->seg_Find = SegTrack;
        NEWLIST((struct List*)&ss->seg_List);
        AddSemaphore(&ss->seg_Semaphore);
    }
    return 1;
}

ADD2INITLIB(SegTracker_Init, -100)
