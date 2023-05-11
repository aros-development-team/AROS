/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#define __NOLIBBASE__
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/timer.h>

#include <proto/log.h>
#include <proto/alib.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <resources/log.h>

#include "log_intern.h"

#include LC_LIBDEFS_FILE

extern BOOL GM_UNIQUENAME(OpenDOS)(LIBBASETYPEPTR LIBBASE);
extern BOOL GM_UNIQUENAME(OpenUtility)(LIBBASETYPEPTR LIBBASE);

#define DOSBase LIBBASE->lrb_DosBase
#define TimerBase LIBBASE->lrb_TimerIOReq.tr_node.io_Device
#define UtilityBase LIBBASE->lrb_UtilityBase

AROS_LH1(VOID, logLockEntries,
         AROS_LHA(ULONG, flags, D0),
         LIBBASETYPEPTR, LIBBASE, 13, log)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}

AROS_LH1(VOID, logUnlockEntries,
         AROS_LHA(ULONG, flags, D0),
         LIBBASETYPEPTR, LIBBASE, 15, log)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}

AROS_LH1(APTR, logNextEntry,
         AROS_LHA(APTR *, entryHandle, A0),
         LIBBASETYPEPTR, LIBBASE, 14, log)
{
    AROS_LIBFUNC_INIT

    struct logEntryPrivate *leP;
    struct logEntry *le;

    if (entryHandle)
    {
        if (*entryHandle == (APTR)LOGEntry_First)
        {
            leP = (struct logEntryPrivate *)GetHead(&LIBBASE->lrb_LRProvider.lrh_Entries);
            if (leP)
                *entryHandle = &leP->le_Node;
            else
                *entryHandle = (APTR)LOGEntry_Last;
        }
        else if (*entryHandle != (APTR)LOGEntry_Last)
        {
            le = *entryHandle;
            leP = (struct logEntryPrivate *)((IPTR)le - ((IPTR)&leP->le_Node - (IPTR)leP));
            leP = (struct logEntryPrivate *)GetSucc(leP);
            if (leP)
                *entryHandle = &leP->le_Node;
            else
                *entryHandle = (APTR)LOGEntry_Last;
        }
        return *entryHandle;
    }
    return NULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(IPTR, logGetEntryAttrs,
         AROS_LHA(APTR, entryHandle, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, LIBBASE, 9, log)
{
    AROS_LIBFUNC_INIT

    struct logEntry *le = (struct logEntry *)entryHandle;
    IPTR count = 0;

    if (le && (GM_UNIQUENAME(OpenUtility)(LIBBASE)))
    {
        struct logEntryPrivate *leP;
        struct TagItem *ti;
        leP = (struct logEntryPrivate *)((IPTR)le - ((IPTR)&leP->le_Node - (IPTR)leP));

        if((ti = FindTagItem(LOGMA_Flags, tags)))
        {
            *((ULONG *) ti->ti_Data) = (leP->lectx_Flags & ~LOGM_Flag_PrivateMask);
            count++;
        }
        if((ti = FindTagItem(LOGMA_DateStamp, tags)))
        {
            *((struct DateStamp **) ti->ti_Data) = &leP->le_DateStamp;
            count++;
        }
        if((ti = FindTagItem(LOGMA_EventID, tags)))
        {
            *((ULONG *) ti->ti_Data) = leP->le_eid;
            count++;
        }
        if((ti = FindTagItem(LOGMA_Origin, tags)))
        {
            *((char**) ti->ti_Data) = leP->lectx_Originator;
            count++;
        }
        if((ti = FindTagItem(LOGMA_Component, tags)))
        {
            if (leP->lep_Producer)
            {
                struct LogResHandle *lrHandle = (struct LogResHandle *)leP->lep_Producer;
                *((char**) ti->ti_Data) = lrHandle->lrh_Node.ln_Name;
                count++;
            }
        }
        if((ti = FindTagItem(LOGMA_SubComponent, tags)))
        {
            *((char**) ti->ti_Data) = leP->lep_Node.ln_Name;
            count++;
        }
        if((ti = FindTagItem(LOGMA_LogTag, tags)))
        {
            *((char**) ti->ti_Data) = leP->le_Node.ln_Name;
            count++;
        }
        if((ti = FindTagItem(LOGMA_Entry, tags)))
        {
            *((char**) ti->ti_Data) = leP->le_Entry;
            count++;
        }
    }

    return count;

    AROS_LIBFUNC_EXIT
}

AROS_LH7(struct logEntry *, logAddEntryA,
         AROS_LHA(ULONG, flags, D0),
         AROS_LHA(APTR, loghandle, A0),
         AROS_LHA(STRPTR, sub, A1),
         AROS_LHA(STRPTR, src, A2),
         AROS_LHA(ULONG, eventid, D1),
         AROS_LHA(STRPTR, fmtstr, A3),
         AROS_LHA(RAWARG, fmtdata, A4),
         LIBBASETYPEPTR, LIBBASE, 10, log)
{
    AROS_LIBFUNC_INIT

    struct LogResHandle *lrHandle = (struct LogResHandle *)loghandle;
    if (lrHandle)
    {
        struct logEntryPrivate *leP;
        if((leP = AllocVecPooled(lrHandle->lrh_Pool, sizeof(struct logEntryPrivate))))
        {
            leP->lep_Node.ln_Pri = (flags & LOGM_Flag_LevelMask);
            leP->lep_Node.ln_Succ = NULL;
            leP->lep_Node.ln_Pred = NULL;
            leP->le_Node.ln_Succ = NULL;
            leP->le_Node.ln_Pred = NULL;
            leP->lectx_Level = flags;
            leP->lep_Producer = loghandle;

            if((leP->le_Entry = logCopyStrFmtA(fmtstr, fmtdata)))
            {
                struct Task *thisTask = FindTask(NULL);
                if (sub)
                    leP->lep_Node.ln_Name = logCopyStr(sub);
                else leP->lep_Node.ln_Name = NULL;
                if (src)
                    leP->le_Node.ln_Name = logCopyStr(src);
                else leP->le_Node.ln_Name = NULL;

                leP->lectx_Originator = logCopyStrFmt("0x%p %s", thisTask, ((struct Node *)thisTask)->ln_Name);
                leP->le_eid = eventid;

                if(GM_UNIQUENAME(OpenDOS)(LIBBASE))
                {
                    DateStamp(&leP->le_DateStamp);
                } else {
                    struct timerequest tr;
                    CopyMem(&LIBBASE->lrb_TimerIOReq, &tr, sizeof(struct timerequest));
                    tr.tr_node.io_Command = TR_GETSYSTIME;
                    DoIO((struct IORequest *) &tr);
                    leP->le_DateStamp.ds_Days = tr.tr_time.tv_secs / (24*60*60);
                    leP->le_DateStamp.ds_Minute = (tr.tr_time.tv_secs / 60) % 60;
                    leP->le_DateStamp.ds_Tick = (tr.tr_time.tv_secs % 60) * 50;
                }
                
                Forbid();
                AddTail(&LIBBASE->lrb_LRProvider.lrh_Entries, &leP->lep_Node);
                if (leP->lep_Producer != &LIBBASE->lrb_LRProvider)
                    AddTail(&lrHandle->lrh_Entries, &leP->le_Node);
                Permit();

#if (0)
//                TODO
//                PutMsg(LIBBASE->lrb_ServicePort, (struct Message *)leP);
                Signal(LIBBASE->lrb_Task, SIGF_SINGLE);
#endif
                return((struct logEntry *)&leP->le_Node);
            }
            FreeVec(leP);
        }
    }

    return(NULL);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, logRemEntry,
         AROS_LHA(struct logEntry *, le, A0),
         LIBBASETYPEPTR, LIBBASE, 11, log)
{
    AROS_LIBFUNC_INIT

    struct logEntryPrivate *leP;
    leP = (struct logEntryPrivate *)((IPTR)le - ((IPTR)&leP->le_Node - (IPTR)leP));

    Forbid();
    Remove(&leP->lep_Node);
    if (leP->lep_Producer != &LIBBASE->lrb_LRProvider)
        Remove(&leP->le_Node);
    Permit();

#if (0)
// TODO
//    PutMsg(LIBBASE->lrb_ServicePort, (struct Message *)leP);
#endif
    Signal(LIBBASE->lrb_Task, SIGF_SINGLE);

    FreeVec(le->lectx_Originator);
    FreeVec(le->le_Entry);
    FreeVec(le);

    AROS_LIBFUNC_EXIT
}
