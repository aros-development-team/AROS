/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/timer.h>

#include <proto/log.h>
#include <proto/alib.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <resources/log.h>

#include <stddef.h>
#include <string.h>

#include "log_intern.h"

#include LC_LIBDEFS_FILE

extern BOOL GM_UNIQUENAME(HaveDOS)(LIBBASETYPEPTR LIBBASE);
extern BOOL GM_UNIQUENAME(OpenUtility)(LIBBASETYPEPTR LIBBASE);
extern BOOL GM_UNIQUENAME(HaveTimer)(LIBBASETYPEPTR LIBBASE);

#define DOSBase LIBBASE->lrb_DosBase
#define TimerBase LIBBASE->lrb_TimerIOReq.tr_node.io_Device
#define UtilityBase LIBBASE->lrb_UtilityBase
#define KernelBase LIBBASE->lrb_KernelBase

AROS_LH1(VOID, logLockEntries,
         AROS_LHA(ULONG, flags, D0),
         LIBBASETYPEPTR, LIBBASE, 13, log)
{
    AROS_LIBFUNC_INIT
    struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
    ObtainSemaphore(&LIBBASE->lrb_ReentrantLock);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(VOID, logUnlockEntries,
         AROS_LHA(ULONG, flags, D0),
         LIBBASETYPEPTR, LIBBASE, 15, log)
{
    AROS_LIBFUNC_INIT
    struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
    ReleaseSemaphore(&LIBBASE->lrb_ReentrantLock);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(APTR, logNextEntry,
         AROS_LHA(APTR *, entryHandle, A0),
         LIBBASETYPEPTR, LIBBASE, 14, log)
{
    AROS_LIBFUNC_INIT

    struct logEntryPrivate *leP;
    struct logEntry *le;

    if (entryHandle) {
        if (*entryHandle == (APTR)LOGEntry_First) {
            leP = (struct logEntryPrivate *)GetHead(&LIBBASE->lrb_LRProvider.lrh_Entries);
            if (leP)
                *entryHandle = &leP->le_Node;
            else
                *entryHandle = (APTR)LOGEntry_Last;
        }
        else if (*entryHandle != (APTR)LOGEntry_Last) {
            le = *entryHandle;
            if (le->le_Node.ln_Type == NT_LOGENTRY) {
                leP = (struct logEntryPrivate *)((IPTR)le - offsetof(struct logEntryPrivate, le_Node));
                leP = (struct logEntryPrivate *)GetSucc(leP);
                if (leP)
                    *entryHandle = &leP->le_Node;
            else
                *entryHandle = (APTR)LOGEntry_Last;
            }
            else
                *entryHandle = (APTR)LOGEntry_Last;
        } else {
            // Return the last entry if called with LOGEntry_Last
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

    if (le && (le->le_Node.ln_Type == NT_LOGENTRY) && (GM_UNIQUENAME(OpenUtility)(LIBBASE))) {
        struct logEntryPrivate *leP;
        struct TagItem *ti;
        leP = (struct logEntryPrivate *)((IPTR)le - offsetof(struct logEntryPrivate, le_Node));

        if((ti = FindTagItem(LOGMA_Flags, tags))) {
            *((ULONG *) ti->ti_Data) = (leP->lectx_Flags & ~LOGM_Flag_PrivateMask);
            count++;
        }
        if((ti = FindTagItem(LOGMA_DateStamp, tags))) {
            *((struct DateStamp **) ti->ti_Data) = &leP->le_DateStamp;
            count++;
        }
        if((ti = FindTagItem(LOGMA_EventID, tags))) {
            *((ULONG *) ti->ti_Data) = leP->le_eid;
            count++;
        }
        if((ti = FindTagItem(LOGMA_Origin, tags))) {
            *((char**) ti->ti_Data) = leP->lectx_Originator;
            count++;
        }
        if((ti = FindTagItem(LOGMA_Component, tags))) {
            if (leP->lep_Producer) {
                struct LogResHandle *lrHandle = (struct LogResHandle *)leP->lep_Producer;
                *((char**) ti->ti_Data) = lrHandle->lrh_Node.ln_Name;
                count++;
            }
        }
        if((ti = FindTagItem(LOGMA_SubComponent, tags))) {
            *((char**) ti->ti_Data) = leP->lep_Node.ln_Name;
            count++;
        }
        if((ti = FindTagItem(LOGMA_LogTag, tags))) {
            *((char**) ti->ti_Data) = leP->le_Node.ln_Name;
            count++;
        }
        if((ti = FindTagItem(LOGMA_Entry, tags))) {
            *((char**) ti->ti_Data) = leP->le_Entry;
            count++;
        }
    }

    return count;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH7(struct logEntry *, logAddEntryA,

/*  SYNOPSIS */
         AROS_LHA(ULONG, flags, D0),
         AROS_LHA(APTR, loghandle, A0),
         AROS_LHA(STRPTR, sub, A1),
         AROS_LHA(STRPTR, src, A2),
         AROS_LHA(ULONG, eventid, D1),
         AROS_LHA(STRPTR, fmtstr, A3),
         AROS_LHA(RAWARG, fmtdata, A4),

/*  LOCATION */
         LIBBASETYPEPTR, LIBBASE, 10, log)

/*  FUNCTION
        Creates and queues a new log entry associated with the given
        logging handle. The entry includes timestamp, event ID,
        optional subsystem/source strings, originator information,
        and a formatted message string built from 'fmtstr' and 'fmtdata'.

    INPUTS
        flags     - Logging flags and priority level (LOGM_*).
        loghandle - Handle to an open log resource (struct LogResHandle *).
        sub       - Optional subsystem string, may be NULL.
        src       - Optional source string, may be NULL.
        eventid   - User-supplied event identifier.
        fmtstr    - Format string for the log message.
        fmtdata   - Argument data matching the format string.

    RESULT
        Pointer to a logEntry structure if successful,
        or NULL if the entry could not be created.

    NOTES
        The returned logEntry is owned by log.resource and should not
        be modified or freed by the caller. The timestamp source depends
        on availability of DOS, timer.device, or kernel facilities.

    EXAMPLE
        struct logEntry *e = logAddEntryA(LOGM_Flag_Info, myLogHandle,
            "filesystem", "disk.device", 42,
            "Mounted volume %s", (RAWARG)"DH0:");

    BUGS
        Kernel timestamp support is incomplete (marked TODO in source).

    SEE ALSO

    INTERNALS
        Allocates a logEntryPrivate from the handle’s pool.
        Uses logCopyStrFmtA() to duplicate the formatted message.
        Links the entry into the handle’s list unless producer is provider.
        Posts an EHMB_ADDENTRY message to the log resource service port.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct LogResHandle *lrHandle = (struct LogResHandle *)loghandle;
    if ((LIBBASE->lrb_Task) && (lrHandle)) {
        struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
        struct logEntryPrivate *leP;
        if ((leP = AllocVecPooled(lrHandle->lrh_Pool, sizeof(struct logEntryPrivate)))) {
            memset(leP, 0, sizeof(struct logEntryPrivate));
            leP->lep_Node.ln_Pri = (flags & LOGM_Flag_LevelMask);
            leP->lectx_Level = flags;
            leP->lep_Producer = loghandle;

            if((leP->le_Entry = logCopyStrFmtA(fmtstr, fmtdata))) {
                struct Task *thisTask = FindTask(NULL);

                if (sub)
                    leP->lep_Node.ln_Name = logCopyStr(sub);

                if (src)
                    leP->le_Node.ln_Name = logCopyStr(src);

                leP->lectx_Originator = logCopyStrFmt("0x%p %s", thisTask, ((struct Node *)thisTask)->ln_Name);
                leP->le_eid = eventid;

                if(GM_UNIQUENAME(HaveDOS)(LIBBASE)) {
                    DateStamp(&leP->le_DateStamp);
                    leP->lectx_Flags &= ~(LOGF_Flag_Private_STMPTimer|LOGF_Flag_Private_STMPKrn);
                } else if (GM_UNIQUENAME(HaveTimer)(LIBBASE)) {
                    struct timerequest tr;
                    CopyMem(&LIBBASE->lrb_TimerIOReq, &tr, sizeof(struct timerequest));
                    tr.tr_node.io_Command = TR_GETSYSTIME;
                    DoIO((struct IORequest *) &tr);
                    leP->le_DateStamp.ds_Days = tr.tr_time.tv_secs / (24*60*60);
                    leP->le_DateStamp.ds_Minute = (tr.tr_time.tv_secs / 60) % 60;
                    leP->le_DateStamp.ds_Tick = (tr.tr_time.tv_secs % 60) * 50;
                    leP->lectx_Flags |= LOGF_Flag_Private_STMPTimer;
                } else {
                    leP->lectx_Flags |= LOGF_Flag_Private_STMPKrn;
                    if (KernelBase) {
                        UQUAD stamp = KrnTimeStamp();
                        if (stamp != 0) {
                            // TODO:
                            
                        }
                    }
                }

                Forbid();
                if (leP->lep_Producer != &LIBBASE->lrb_LRProvider)
                    AddTail(&lrHandle->lrh_Entries, &leP->le_Node);
                Permit();

                leP->le_Node.ln_Type = EHMB_ADDENTRY;
                PutMsg(LIBBASE->lrb_ServicePort, (struct Message *)leP);

                return((struct logEntry *)&leP->le_Node);
            }
            FreeVecPooled(lrHandle->lrh_Pool, leP);
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

    if (le) {
        struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
        struct logEntryPrivate *leP;
        leP = (struct logEntryPrivate *)((IPTR)le - offsetof(struct logEntryPrivate, le_Node));

        Forbid();
        Remove(&leP->lep_Node);
        if (leP->lep_Producer != &LIBBASE->lrb_LRProvider)
            Remove(&leP->le_Node);
        Permit();

        if (LIBBASE->lrb_Task) {
            leP->le_Node.ln_Type = EHMB_REMENTRY;
            PutMsg(LIBBASE->lrb_ServicePort, (struct Message *)leP);
        }
    }
    AROS_LIBFUNC_EXIT
}
