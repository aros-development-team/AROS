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

/*****************************************************************************

    NAME */
        AROS_LH1(VOID, logLockEntries,

/*  SYNOPSIS */
                 AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 13, log)
/*  FUNCTION
        Acquire the resource's entry-list semaphore.

    INPUTS
        flags - LLF_READ or LLF_WRITE. Currently ignored.

    RESULT
        None. Returns after acquiring the semaphore.

    NOTES
        Call from task context where waiting is permitted. Both flag values
        acquire the same exclusive semaphore. Balance each acquisition with
        logUnlockEntries() in the same task.

        Hold the lock while traversing entries and inspecting borrowed
        attributes. This serializes cooperating users and the service task's
        insertion path; it does not establish complete entry-lifetime safety.

    BUGS
        logRemEntry() does not acquire this semaphore. The service task's
        removal and deallocation path does not acquire it either. A held
        lock alone cannot protect against every concurrent removal.

    SEE ALSO
        logUnlockEntries(), logNextEntry(), logGetEntryAttrs()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
    ObtainSemaphore(&LIBBASE->lrb_ReentrantLock);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(VOID, logUnlockEntries,

/*  SYNOPSIS */
                 AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 15, log)
/*  FUNCTION
        Release the semaphore acquired by logLockEntries().

    INPUTS
        flags - LLF_READ or LLF_WRITE. Currently ignored.

    RESULT
        None.

    NOTES
        The calling task must own a matching acquisition. Do not use this
        call to release another task's lock. Borrowed entry pointers are
        not reference-counted and must not be assumed valid after unlocking.

    SEE ALSO
        logLockEntries(), logNextEntry()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
    ReleaseSemaphore(&LIBBASE->lrb_ReentrantLock);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(APTR, logNextEntry,

/*  SYNOPSIS */
                 AROS_LHA(APTR *, entryHandle, A0),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 14, log)
/*  FUNCTION
        Advance a cursor through the global list of published log entries.

    INPUTS
        entryHandle - Pointer to a writable APTR cursor. Initialize it to
                      LOGEntry_First; subsequently retain each returned
                      entry or LOGEntry_Last. NULL is accepted.

    RESULT
        The next entry, also written to *entryHandle, or LOGEntry_Last
        for an empty list or end of traversal. Passing LOGEntry_Last leaves
        it unchanged; it does not return the final entry. A NULL cursor
        pointer returns NULL.

    NOTES
        Returned entries are borrowed. Hold logLockEntries() during
        traversal and coordinate removal separately as described there.
        Only pass live published entries from this list as cursors; a stale
        address is not made safe by the node-type check. This function does
        not acquire a lock or allocate storage.

    SEE ALSO
        logLockEntries(), logUnlockEntries(), logGetEntryAttrs()

*****************************************************************************/
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

/*****************************************************************************

    NAME */
        AROS_LH2(IPTR, logGetEntryAttrs,

/*  SYNOPSIS */
                 AROS_LHA(APTR, entryHandle, A0),
                 AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 9, log)
/*  FUNCTION
        Read selected attributes from a published log entry.

    INPUTS
        entryHandle - Live published entry handle, or NULL. Do not pass
                      LOGEntry_Last or a freed entry.
        tags - Tag list whose ti_Data fields point to writable destinations
               of the types listed below.

    TAGS
        LOGMA_Flags (ULONG *)
            Entry flags with the private high-byte flags removed.
        LOGMA_DateStamp (struct DateStamp **)
            Pointer to the entry's stored timestamp.
        LOGMA_EventID (ULONG *)
            Event identifier supplied by the producer.
        LOGMA_Origin (char **)
            Recorded originating task description.
        LOGMA_Component (char **)
            Producer name. Written only if a producer is associated.
        LOGMA_SubComponent (char **)
            Subcomponent string supplied when the entry was added.
        LOGMA_LogTag (char **)
            Source/tag string supplied when the entry was added.
        LOGMA_Entry (char **)
            Formatted message text.

    RESULT
        The number of attributes written. Zero is returned for NULL,
        a node not marked NT_LOGENTRY, or failure to open utility.library.
        Unsupported tags are ignored. Only the first occurrence of each
        supported tag is processed; skipped destinations are unchanged.

    NOTES
        Use normal tag-list termination and control tags. String and date
        results point into resource-owned storage; they are not copies.
        Do not modify or free them. Individual string results may be NULL.
        Hold the entry lock and coordinate entry removal before reading or
        copying these values. The function does not acquire the entry lock
        and may open utility.library; call from task context.

    BUGS
        LOGMA_Level and LOGMA_Type are not handled. Obtain their bits from
        LOGMA_Flags instead. The public LOGMA_Type macro is also malformed
        in this version of <resources/log.h>.

    SEE ALSO
        logNextEntry(), logLockEntries(), logRemEntry()

*****************************************************************************/
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

/*****************************************************************************

    NAME */
        AROS_LH1(void, logRemEntry,

/*  SYNOPSIS */
                 AROS_LHA(struct logEntry *, le, A0),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 11, log)
/*  FUNCTION
        Unlink a published entry and queue its disposal by the service task.

    INPUTS
        le - Live published entry from log.resource, removed at most once,
             or NULL. An entry still awaiting publication is not valid here.

    RESULT
        None. NULL has no effect.

    NOTES
        The function removes both global and producer linkage as applicable.
        If the service task is running, it queues EHMB_REMENTRY and returns
        without waiting for notification or deallocation. Treat the handle
        and all borrowed attributes as invalid after requesting removal.
        Do not free the entry yourself.

        List removal uses Forbid()/Permit(), not the entry semaphore.
        Callers must coordinate removal with readers; logLockEntries() alone
        does not exclude this path.

    BUGS
        No node-state or duplicate-removal check is made. If the service
        task has stopped, the entry is unlinked but not freed. The service
        task does not free the copied subcomponent string. Its removal
        notification contains a pointer that it then frees without waiting
        for listeners to consume the message.

    SEE ALSO
        logGetEntryAttrs(), logLockEntries(), logEventBroadcast()

*****************************************************************************/
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
