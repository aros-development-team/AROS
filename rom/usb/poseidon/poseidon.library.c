/****************************************************************************

                 __   __                                    V/\V.       /\
                |" | |" |                                   mMMnw,      || []
                |  | |  |                                  (o  o)W   () || ||
                |__|_|_"|                                  | /  |Mw  || ||//
                ("  "  \|                                  \ -'_/mw   \\||/
                 \______)                                   ~%%/WM"    \||
 _____    ___     ______  _____  __  _____     ___  __  __/~~__ ~~\    _||
|"("  \()/\" \ ()/"_    )|"(___) ) )|"("  \ ()/\" \(__)/" ) /" ) " \  /_)O
|  )   )/" \  \ (_/"\__/ |  )_  ( ( |  )_  ) /" \  \  /  /|/  / ·\  \/ ,|O
| (___/(  (_\__) _\  \_  | (__)  ) )| (__) |(  (_\__)/  /"/  /   |\   '_|O
|  |  _ \  /  / /" \_/ ) | ")__ ( ( |  )"  ) \  /  //  /|/  / . .|/\__/ ||
|__| (_) \/__/ (______/  |_(___) )_)|_(___/ . \/__/(__/ (__/ .:.:|      ||
                 _____
                |" __ \  Poseidon -- The divine USB stack for Amiga computers
                | (__) )
                |  __ (  Designed and written by
                |"(__) )   Chris Hodges <chrisly@platon42.de>
                |_____/  Copyright ©2002-2009 Chris Hodges. All rights reserved.

 ****************************************************************************/

/*
 *----------------------------------------------------------------------------
 *                          Poseidon main library
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "poseidon.library.h"

#include "numtostr.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/usbclass.h>
#include <proto/timer.h>

#ifdef __AROS__
#include <aros/bootloader.h>
#include <proto/bootloader.h>
#endif

#define NewList(list) NEWLIST(list)

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))

extern const struct PsdWStringMap usbclasscodestr[];
extern const struct PsdULStringMap usbcomboclasscodestr[];
extern const struct PsdULStringMap usbdesctypestr[];
extern const struct PsdWStringMap usbhwioerrstr[];
extern const struct PsdUWStringMap usblangids[];
extern const struct PsdUWStringMap usbvendorids[];

extern struct ExecBase *SysBase;

/* Static data */
const char GM_UNIQUENAME(libname)[]     = MOD_NAME_STRING;

#define UsbClsBase puc->puc_ClassBase
#define DOSBase ps->ps_DosBase
#define TimerBase ps->ps_TimerIOReq.tr_node.io_Device

/* LibInit */
static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR ps)
{
    KPRINTF(10, ("libInit ps: 0x%p SysBase: 0x%p\n",
                 ps, SysBase));

    ps->ps_StackInit = FALSE;
    ps->ps_UtilityBase = (struct UtilityBase *) OpenLibrary("utility.library", 39);

#define UtilityBase ps->ps_UtilityBase

    if (UtilityBase)
    {
#ifdef __AROS__
    	APTR BootLoaderBase = OpenResource("bootloader.resource");

    	if (BootLoaderBase)
    	{
            struct List *args = GetBootInfo(BL_Args);

	    if (args)
            {
            	struct Node *node;

            	for (node = args->lh_Head; node->ln_Succ; node = node->ln_Succ)
            	{
                    if (stricmp(node->ln_Name, "usbdebug") == 0)
                    {
                    	ps->ps_Flags = PSF_KLOG;
                    	break;
                    }
                }
            }
        }
#endif

        NewList(&ps->ps_Hardware);
        NewList(&ps->ps_Classes);
        NewList(&ps->ps_ErrorMsgs);
        NewList(&ps->ps_EventHooks);
        ps->ps_EventReplyPort.mp_Flags = PA_IGNORE;
        NewList(&ps->ps_EventReplyPort.mp_MsgList);
        NewList(&ps->ps_ConfigRoot);
        NewList(&ps->ps_AlienConfigs);

        NewList(&ps->ps_DeadlockDebug);

        InitSemaphore(&ps->ps_ReentrantLock);
        InitSemaphore(&ps->ps_PoPoLock);

        if((ps->ps_MemPool = CreatePool(MEMF_CLEAR|MEMF_PUBLIC|MEMF_SEM_PROTECTED, 16384, 1024)))
        {
            if((ps->ps_SemaMemPool = CreatePool(MEMF_CLEAR|MEMF_PUBLIC, 16*sizeof(struct PsdReadLock), sizeof(struct PsdBorrowLock))))
            {
                pInitSem(ps, &ps->ps_Lock, "PBase");
                pInitSem(ps, &ps->ps_ConfigLock, "ConfigLock");
                KPRINTF(20, ("libInit: Done!\n"));
                return TRUE;
            }
            DeletePool(ps->ps_MemPool);
        } else {
            KPRINTF(20, ("libInit: CreatePool() failed!\n"));
        }
        CloseLibrary((struct Library *) UtilityBase);
    } else {
        KPRINTF(20, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
    }
    return FALSE;
}

/* LibOpen */
static int GM_UNIQUENAME(libOpen)(LIBBASETYPEPTR ps)
{
    struct PsdIFFContext *pic;

    KPRINTF(10, ("libOpen ps: 0x%p\n", ps));
    ObtainSemaphore(&ps->ps_ReentrantLock);
    if(!ps->ps_StackInit)
    {
        ps->ps_TimerIOReq.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
        ps->ps_TimerIOReq.tr_node.io_Message.mn_ReplyPort    = NULL;
        ps->ps_TimerIOReq.tr_node.io_Message.mn_Length       = sizeof(struct timerequest);
        if(!OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) &ps->ps_TimerIOReq, 0))
        {
            ps->ps_TimerIOReq.tr_node.io_Message.mn_Node.ln_Name = "Poseidon";
            ps->ps_TimerIOReq.tr_node.io_Command = TR_ADDREQUEST;

            ps->ps_ReleaseVersion = RELEASEVERSION;
            ps->ps_OSVersion = MAKE_ID('A','R','O','S');

            pic = pAllocForm(ps, NULL, IFFFORM_PSDCFG);
            if((ps->ps_GlobalCfg = psdAllocVec(sizeof(struct PsdGlobalCfg))))
            {
                ps->ps_GlobalCfg->pgc_ChunkID = AROS_LONG2BE(IFFCHNK_GLOBALCFG);
                ps->ps_GlobalCfg->pgc_Length = AROS_LONG2BE(sizeof(struct PsdGlobalCfg)-8);
                ps->ps_GlobalCfg->pgc_LogInfo = TRUE;
                ps->ps_GlobalCfg->pgc_LogWarning = TRUE;
                ps->ps_GlobalCfg->pgc_LogError = TRUE;
                ps->ps_GlobalCfg->pgc_LogFailure = TRUE;
                ps->ps_GlobalCfg->pgc_BootDelay = 2;
                ps->ps_GlobalCfg->pgc_SubTaskPri = 5;
                ps->ps_GlobalCfg->pgc_PopupDeviceNew = PGCP_ISNEW;
                ps->ps_GlobalCfg->pgc_PopupDeviceGone = TRUE;
                ps->ps_GlobalCfg->pgc_PopupDeviceDeath = TRUE;
                ps->ps_GlobalCfg->pgc_PopupCloseDelay = 5;
                ps->ps_GlobalCfg->pgc_PopupActivateWin = FALSE;
                ps->ps_GlobalCfg->pgc_PopupWinToFront = TRUE;
                ps->ps_GlobalCfg->pgc_AutoDisableLP = FALSE;
                ps->ps_GlobalCfg->pgc_AutoDisableDead = FALSE;
                ps->ps_GlobalCfg->pgc_AutoRestartDead = TRUE;
                ps->ps_GlobalCfg->pgc_PowerSaving = FALSE;
                ps->ps_GlobalCfg->pgc_ForceSuspend = FALSE;
                ps->ps_GlobalCfg->pgc_SuspendTimeout = 30;

                ps->ps_GlobalCfg->pgc_PrefsVersion = 0; // is updated on writing
                ps->ps_ConfigRead = FALSE;
                if(pic)
                {
                    pic = pAllocForm(ps, pic, IFFFORM_STACKCFG);
                    if(pic)
                    {
                        pAddCfgChunk(ps, pic, ps->ps_GlobalCfg);
                    }
                }
                ps->ps_PoPo.po_InsertSndFile = psdCopyStr("SYS:Prefs/Presets/Poseidon/Connect.iff");
                ps->ps_PoPo.po_RemoveSndFile = psdCopyStr("SYS:Prefs/Presets/Poseidon/Disconnect.iff");

                {
                    STRPTR tmpstr;
                    tmpstr = psdCopyStr((STRPTR) VERSION_STRING);
                    if(tmpstr)
                    {
                        tmpstr[strlen(tmpstr)-2] = 0;
                        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Welcome to %s (%p)!", tmpstr, ps->ps_ReleaseVersion);
                        psdFreeVec(tmpstr);
                    } else {
                        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Welcome to %s", VERSION_STRING);
                    }
                }

                psdAddErrorMsg0(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "This is the AROS port.");

                KPRINTF(10, ("libOpen: Ok\n"));
                ps->ps_StackInit = TRUE;
                ReleaseSemaphore(&ps->ps_ReentrantLock);
                pStartEventHandler(ps);

                return TRUE;
            } else {
                KPRINTF(20, ("libOpen: No memory for cfg!\n"));
            }
        } else {
            KPRINTF(20, ("libOpen: OpenDevice(timer.device) failed!\n"));
        }
        ReleaseSemaphore(&ps->ps_ReentrantLock);
        return FALSE;
    }
    ReleaseSemaphore(&ps->ps_ReentrantLock);
    KPRINTF(5, ("libOpen: openCnt = %ld\n", ps->ps_Library.lib_OpenCnt));
    return TRUE;
}

int GM_UNIQUENAME(libExpunge)(LIBBASETYPEPTR ps)
{
    struct PsdHardware *phw = (struct PsdHardware *) ps->ps_Hardware.lh_Head;
    struct PsdUsbClass *puc = (struct PsdUsbClass *) ps->ps_Classes.lh_Head;
    struct PsdErrorMsg *pem = (struct PsdErrorMsg *) ps->ps_ErrorMsgs.lh_Head;
    struct PsdIFFContext *pic = (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head;
    KPRINTF(10, ("libExpunge ps: 0x%p\n", ps));
    while(phw->phw_Node.ln_Succ)
    {
        psdRemHardware(phw);
        phw = (struct PsdHardware *) ps->ps_Hardware.lh_Head;
    }
    while(puc->puc_Node.ln_Succ)
    {
        psdRemClass(puc);
        puc = (struct PsdUsbClass *) ps->ps_Classes.lh_Head;
    }
    while(pem->pem_Node.ln_Succ)
    {
        psdRemErrorMsg(pem);
        pem = (struct PsdErrorMsg *) ps->ps_ErrorMsgs.lh_Head;
    }

    while(pic->pic_Node.ln_Succ)
    {
        pFreeForm(ps, pic);
        pic = (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head;
    }

    if(ps->ps_PoPo.po_Task)
    {
        ps->ps_PoPo.po_ReadySignal = SIGB_SINGLE;
        ps->ps_PoPo.po_ReadySigTask = FindTask(NULL);
        Signal(ps->ps_PoPo.po_Task, SIGBREAKF_CTRL_C);
        while(ps->ps_PoPo.po_Task)
        {
            Wait(1L<<ps->ps_PoPo.po_ReadySignal);
        }
        ps->ps_PoPo.po_ReadySigTask = NULL;
        //FreeSignal(ps->ps_PoPo.po_ReadySignal);
    }
    if(ps->ps_EventHandler.ph_Task)
    {
        ps->ps_EventHandler.ph_ReadySignal = SIGB_SINGLE;
        ps->ps_EventHandler.ph_ReadySigTask = FindTask(NULL);
        Signal(ps->ps_EventHandler.ph_Task, SIGBREAKF_CTRL_C);
        while(ps->ps_EventHandler.ph_Task)
        {
            Wait(1L<<ps->ps_EventHandler.ph_ReadySignal);
        }
        ps->ps_EventHandler.ph_ReadySigTask = NULL;
        //FreeSignal(ps->ps_EventHandler.ph_ReadySignal);
    }
    psdFreeVec(ps->ps_PoPo.po_InsertSndFile);
    psdFreeVec(ps->ps_PoPo.po_RemoveSndFile);
    pGarbageCollectEvents(ps);

    CloseDevice((struct IORequest *) &ps->ps_TimerIOReq);
    DeletePool(ps->ps_SemaMemPool);
    DeletePool(ps->ps_MemPool);

    KPRINTF(1, ("libExpunge: closelibrary utilitybase 0x%p\n",
                UtilityBase));
    CloseLibrary((struct Library *) UtilityBase);

    CloseLibrary(DOSBase);

    KPRINTF(1, ("libExpunge: removing library node 0x%p\n",
                &ps->ps_Library.lib_Node));
    Remove(&ps->ps_Library.lib_Node);

    return TRUE;
}
/* \\\ */

ADD2INITLIB(GM_UNIQUENAME(libInit), 0)
ADD2OPENLIB(GM_UNIQUENAME(libOpen), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(libExpunge), 0);

/*
 * ***********************************************************************
 * * Library functions                                                   *
 * ***********************************************************************
 */

static const ULONG *PsdPTArray[PGA_LAST+1];

/* *** Memory *** */

/* /// "psdAllocVec()" */
AROS_LH1(APTR, psdAllocVec,
         AROS_LHA(ULONG, size, D0),
         LIBBASETYPEPTR, ps, 5, psd)
{
    AROS_LIBFUNC_INIT
    ULONG *pmem;
    KPRINTF(1, ("psdAllocVec(%ld)\n", size));
#ifndef MEMDEBUG
    if((pmem = AllocPooled(ps->ps_MemPool, size + (1*sizeof(ULONG)))))
    {
#else
    if((pmem = AllocPooled(ps->ps_MemPool, size + (1*sizeof(ULONG)) + 1024)))
    {
        ULONG upos = size + (1*sizeof(ULONG));
        UWORD unum = 1024;
        UBYTE *dbptr = (UBYTE *) pmem;
        while(unum--)
        {
            dbptr[upos] = upos;
            upos++;
        }
#endif
        *pmem++ = size;
        ps->ps_MemAllocated += size;
        return((APTR) pmem);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdFreeVec()" */
AROS_LH1(void, psdFreeVec,
         AROS_LHA(APTR, pmem, A1),
         LIBBASETYPEPTR, ps, 6, psd)
{
    AROS_LIBFUNC_INIT
    ULONG size;

    KPRINTF(1, ("psdFreeVec(%p)\n", pmem));
    if(pmem)
    {
        size = ((ULONG *) pmem)[-1];
        ps->ps_MemAllocated -= size;

#ifdef MEMDEBUG
        FreePooled(ps->ps_MemPool, &((ULONG *) pmem)[-1], size + (1*sizeof(ULONG)) + 1024);
#else
        FreePooled(ps->ps_MemPool, &((ULONG *) pmem)[-1], size + (1*sizeof(ULONG)));
#endif
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** PBase *** */

/* /// "pDebugSemaInfo()" */
void pDebugSemaInfo(LIBBASETYPEPTR ps, struct PsdSemaInfo *psi)
{
    struct PsdReadLock *prl;
    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "Semaphore %p %s (Excl/SharedLockCount %ld/%ld) (Owner: %s):",
                   psi->psi_LockSem,
                   psi->psi_LockSem->pls_Node.ln_Name,
                   psi->psi_LockSem->pls_ExclLockCount,
                   psi->psi_LockSem->pls_SharedLockCount,
                   psi->psi_LockSem->pls_Owner ? psi->psi_LockSem->pls_Owner->tc_Node.ln_Name : "None");

    prl = (struct PsdReadLock *) psi->psi_LockSem->pls_WaitQueue.lh_Head;
    while(prl->prl_Node.ln_Succ)
    {
        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                           "  Waiting Task: %p (%s) %s",
                           prl->prl_Task, prl->prl_Task->tc_Node.ln_Name,
                           prl->prl_IsExcl ? "Excl" : "Shared");
        prl = (struct PsdReadLock *) prl->prl_Node.ln_Succ;
    }
    prl = (struct PsdReadLock *) psi->psi_LockSem->pls_ReadLocks.lh_Head;
    while(prl->prl_Node.ln_Succ)
    {
        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "  Readlock Task: %p (%s), Count %ld",
                       prl->prl_Task, prl->prl_Task->tc_Node.ln_Name,
                       prl->prl_Count);
        prl = (struct PsdReadLock *) prl->prl_Node.ln_Succ;
    }
}
/* \\\ */

/* /// "pInitSem()" */
void pInitSem(LIBBASETYPEPTR ps, struct PsdLockSem *pls, STRPTR name)
{
    struct PsdSemaInfo *psi = NULL;
    NewList(&pls->pls_WaitQueue);
    NewList(&pls->pls_ReadLocks);
    pls->pls_Node.ln_Name = name;
    // struct should be nulled anyway
    pls->pls_Owner = NULL;
    pls->pls_ExclLockCount = 0;
    pls->pls_SharedLockCount = 0;
    pls->pls_Dead = FALSE;

    Forbid();
    psi = (struct PsdSemaInfo *) AllocPooled(ps->ps_SemaMemPool, sizeof(struct PsdSemaInfo));
    if(!psi)
    {
        Permit();
        return;
    }
    psi->psi_LockSem = pls;
    AddTail(&ps->ps_DeadlockDebug, &psi->psi_Node);
    Permit();
}
/* \\\ */

/* /// "pDeleteSem()" */
void pDeleteSem(LIBBASETYPEPTR ps, struct PsdLockSem *pls)
{
    struct PsdSemaInfo *psi;
    Forbid();
    pls->pls_Dead = TRUE;
    psi = (struct PsdSemaInfo *) ps->ps_DeadlockDebug.lh_Head;
    while(psi->psi_Node.ln_Succ)
    {
        if(psi->psi_LockSem == pls)
        {
            if(pls->pls_SharedLockCount + pls->pls_ExclLockCount)
            {
                psdAddErrorMsg0(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Semaphore still locked when attempting to delete it!\n");
                pDebugSemaInfo(ps, psi);
            } else {
                Remove(&psi->psi_Node);
                FreePooled(ps->ps_SemaMemPool, psi, sizeof(struct PsdSemaInfo));
            }
            break;
        }
        psi = (struct PsdSemaInfo *) psi->psi_Node.ln_Succ;
    }
    Permit();
}
/* \\\ */

/* /// "pLockSemExcl()" */
void pLockSemExcl(LIBBASETYPEPTR ps, struct PsdLockSem *pls)
{
    struct PsdReadLock waitprl;
    struct Task *thistask = FindTask(NULL);

    waitprl.prl_Task = thistask;
    waitprl.prl_IsExcl = TRUE;

    Forbid();
    do
    {
        // it's already mine!!
        if(thistask == pls->pls_Owner)
        {
            break;
        }
        if(!pls->pls_ExclLockCount)
        {
            // easy case: no shared locks, no exclusive locker
            if(!pls->pls_SharedLockCount)
            {
                break;
            }
            // sole readlock promotion case
            if((pls->pls_SharedLockCount == 1) && ((struct PsdReadLock *) pls->pls_ReadLocks.lh_Head)->prl_Task == thistask)
            {
                KPRINTF(1, ("Promoting read lock (%p) to write lock!\n", thistask));
                break;
            }
        }

        // okay, bad luck, we've got to wait somehow
        AddHead(&pls->pls_WaitQueue, &waitprl.prl_Node);
        thistask->tc_SigRecvd &= ~SIGF_SINGLE;

        Wait(SIGF_SINGLE);

        Remove(&waitprl.prl_Node);
    } while(TRUE);
    pls->pls_Owner = thistask;
    pls->pls_ExclLockCount++;
    Permit();
}
/* \\\ */

/* /// "pLockSemShared()" */
void pLockSemShared(LIBBASETYPEPTR ps, struct PsdLockSem *pls)
{
    struct PsdReadLock *prl;
    struct Task *thistask = FindTask(NULL);

    Forbid();
    // is this already locked exclusively by me?
    if(thistask == pls->pls_Owner)
    {
        // yes? then just increase exclusive lock count
        pls->pls_ExclLockCount++;
        Permit();
        return;
    }

    // find existing readlock
    prl = (struct PsdReadLock *) pls->pls_ReadLocks.lh_Head;
    while(prl->prl_Node.ln_Succ)
    {
        if(prl->prl_Task == thistask)
        {
            KPRINTF(1, ("Increasing ReadLock (%p) count to %ld\n", thistask, prl->prl_Count));
            prl->prl_Count++;
            Permit();
            return;
        }
        prl = (struct PsdReadLock *) prl->prl_Node.ln_Succ;
    }

    // this is a new readlock, generate context
    if(!(prl = (struct PsdReadLock *) AllocPooled(ps->ps_SemaMemPool, sizeof(struct PsdReadLock))))
    {
        KPRINTF(20, ("No mem for shared lock! context (%p) on %p\n", thistask, pls));
        // try exclusive lock as fallback (needs no memory)
        Permit();
        pLockSemExcl(ps, pls);
        return;
    }

    KPRINTF(1, ("New ReadLockShared context (%p) on %p\n", thistask, pls));
    prl->prl_Task = thistask;
    prl->prl_Count = 0;
    prl->prl_IsExcl = FALSE;

    // if it's exclusively locked, wait for this lock to vanish
    while(pls->pls_Owner)
    {
        AddTail(&pls->pls_WaitQueue, &prl->prl_Node);
        thistask->tc_SigRecvd &= ~SIGF_SINGLE;

        Wait(SIGF_SINGLE);

        Remove(&prl->prl_Node);
    }

    if(prl->prl_IsExcl)
    {
        // we got promoted by BorrowLocks during the process! So we don't need the shared stuff anymore
        FreePooled(ps->ps_SemaMemPool, prl, sizeof(struct PsdReadLock));
        pls->pls_Owner = thistask;
        pls->pls_ExclLockCount++;
    } else {
        // got the lock!
        AddHead(&pls->pls_ReadLocks, &prl->prl_Node);
        prl->prl_Count++;
        pls->pls_SharedLockCount++;
    }
    Permit();
    return;
}
/* \\\ */

/* /// "pUnlockSem()" */
void pUnlockSem(LIBBASETYPEPTR ps, struct PsdLockSem *pls)
{
    struct PsdReadLock *prl;
    struct Task *thistask = FindTask(NULL);
    BOOL gotit = FALSE;

    Forbid();
    if(pls->pls_Owner)
    {
        // exclusively locked, this means unlocking task must be owner
        if(pls->pls_Owner != thistask)
        {
            Permit();
            psdDebugSemaphores();
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Attempt to unlock exclusive semaphore %p not owned by task %s!",
                           pls, thistask->tc_Node.ln_Name);
            return;

        }
        if(--pls->pls_ExclLockCount)
        {
            // still locked
            Permit();
            return;
        }
        pls->pls_Owner = NULL;
        // otherwise drop through and notify
    } else {
        if(!pls->pls_SharedLockCount)
        {
            Permit();
            psdDebugSemaphores();
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Attempt to unlock (free) semaphore %p once too often by task %s!",
                           pls, thistask->tc_Node.ln_Name);
            return;
        }
        // find readlock
        prl = (struct PsdReadLock *) pls->pls_ReadLocks.lh_Head;
        while(prl->prl_Node.ln_Succ)
        {
            if(prl->prl_Task == thistask)
            {
                if(--prl->prl_Count)
                {
                    // can't be the last lock, so just reduce count and return
                    Permit();
                    return;
                }
                // remove read lock, it's no longer needed
                KPRINTF(1, ("Removing read lock context (%p) on %p!\n", thistask, pls));
                Remove(&prl->prl_Node);
                FreePooled(ps->ps_SemaMemPool, prl, sizeof(struct PsdReadLock));
                gotit = TRUE;
                // losing a designated lock
                pls->pls_SharedLockCount--;
                break;
            }
            prl = (struct PsdReadLock *) prl->prl_Node.ln_Succ;
        }
        if(!gotit)
        {
            Permit();
            psdDebugSemaphores();
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Attempt to unlock (shared) semaphore %p once too often by task %s!",
                           pls, thistask->tc_Node.ln_Name);
            return;
        }

        // we need to notify anyway, because the waiter could already have a shared lock
        // on the same semaphore, and if we only notified on LockCount reaching zero,
        // the locker would wait forever.
    }

    // notify waiting tasks
    prl = (struct PsdReadLock *) pls->pls_WaitQueue.lh_Head;
    while(prl->prl_Node.ln_Succ)
    {
        Signal(prl->prl_Task, SIGF_SINGLE);
        prl = (struct PsdReadLock *) prl->prl_Node.ln_Succ;
    }
    Permit();
}
/* \\\ */

/* /// "psdDebugSemaphores()" */
AROS_LH0(void, psdDebugSemaphores,
         LIBBASETYPEPTR, ps, 81, psd)
{
    AROS_LIBFUNC_INIT
    struct Task *thistask = FindTask(NULL);
    struct PsdSemaInfo *psi;

    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "Debug Semaphores (%p)", thistask);

    Forbid();
    // search for context
    psi = (struct PsdSemaInfo *) ps->ps_DeadlockDebug.lh_Head;
    while(psi->psi_Node.ln_Succ)
    {
        pDebugSemaInfo(ps, psi);
        psi = (struct PsdSemaInfo *) psi->psi_Node.ln_Succ;
    }
    Permit();
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdLockReadPBase()" */
AROS_LH0(void, psdLockReadPBase,
         LIBBASETYPEPTR, ps, 8, psd)
{
    AROS_LIBFUNC_INIT
    KPRINTF(2, ("psdLockReadPBase(%p)\n", FindTask(NULL)));
    pLockSemShared(ps, &ps->ps_Lock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdLockWritePBase()" */
AROS_LH0(void, psdLockWritePBase,
         LIBBASETYPEPTR, ps, 7, psd)
{
    AROS_LIBFUNC_INIT
    KPRINTF(2, ("psdLockWritePBase(%p)\n", FindTask(NULL)));
    pLockSemExcl(ps, &ps->ps_Lock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdUnlockPBase()" */
AROS_LH0(void, psdUnlockPBase,
         LIBBASETYPEPTR, ps, 9, psd)
{
    AROS_LIBFUNC_INIT
    KPRINTF(2, ("psdUnlockPBase(%p)\n", FindTask(NULL)));
    pUnlockSem(ps, &ps->ps_Lock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdBorrowLocksWait()" */
AROS_LH2(ULONG, psdBorrowLocksWait,
         AROS_LHA(struct Task *, task, A1),
         AROS_LHA(ULONG, signals, D0),
         LIBBASETYPEPTR, ps, 97, psd)
{
    AROS_LIBFUNC_INIT
    struct Task *thistask = FindTask(NULL);
    ULONG cnt = 0;
    ULONG sigmask;
    struct PsdSemaInfo *psi;
    struct PsdLockSem *pls;
    struct PsdReadLock *prl;
    struct PsdBorrowLock *pbl;
    struct List borrows;
    struct List reclaims;
    BOOL moveowner;

    XPRINTF(10, ("Borrowing locks from %p (%s) to %p (%s)!\n",
                  thistask, thistask->tc_Node.ln_Name, task, task->tc_Node.ln_Name));

    Forbid();
    psi = (struct PsdSemaInfo *) ps->ps_DeadlockDebug.lh_Head;
    while(psi->psi_Node.ln_Succ)
    {
        pls = psi->psi_LockSem;
        if(pls->pls_Owner == thistask)
        {
            cnt++;
        }
        if(pls->pls_SharedLockCount)
        {
            struct PsdReadLock *prl = (struct PsdReadLock *) pls->pls_ReadLocks.lh_Head;
            do
            {
                if(prl->prl_Task == thistask)
                {
                    cnt++;
                    break;
                }
            } while((prl = (struct PsdReadLock *) prl->prl_Node.ln_Succ)->prl_Node.ln_Succ);
        }
        psi = (struct PsdSemaInfo *) psi->psi_Node.ln_Succ;
    }
    if(!cnt)
    {
        Permit();
        XPRINTF(10, ("Nothing to borrow!\n"));
        return(Wait(signals));
    }

    NewList(&borrows);
    NewList(&reclaims);
    XPRINTF(10, ("Borrowing %ld locks\n", cnt));

    psi = (struct PsdSemaInfo *) ps->ps_DeadlockDebug.lh_Head;
    while(psi->psi_Node.ln_Succ)
    {
        moveowner = TRUE;
        pls = psi->psi_LockSem;
        if(pls->pls_Owner == thistask)
        {
            // check if the target task is already waiting for that lock
            // in this case, we simply remove our exclusive lock and let
            // the other task catch it
            prl = (struct PsdReadLock *) pls->pls_WaitQueue.lh_Head;
            while(prl->prl_Node.ln_Succ)
            {
                if(prl->prl_Task == task)
                {
                    if(!prl->prl_IsExcl)
                    {
                        // if we hand over the excl lock, we have to make sure that the exclusiveness is kept
                        // and no other thread may catch it while it is shared.
                        // hence we will need set this lock exclusive aswell
                        // this no optimal solution, but it guarantees the same
                        // behaviour with pending lock and no pending lock
                        prl->prl_IsExcl = TRUE;
                        XPRINTF(10, ("Promo waiting lock to excl\n"));
                    }
                    // move shared lock to top of the list
                    Remove(&prl->prl_Node);
                    AddHead(&pls->pls_WaitQueue, &prl->prl_Node);
                    if((pbl = (struct PsdBorrowLock *) AllocPooled(ps->ps_SemaMemPool, sizeof(struct PsdBorrowLock))))
                    {
                        pbl->pbl_LockSem = pls;
                        pbl->pbl_ExclLockCount = pls->pls_ExclLockCount;
                        AddTail(&reclaims, &pbl->pbl_Node);

                        // unlock exclusive lock
                        pls->pls_ExclLockCount = 0;
                        pls->pls_Owner = NULL;
                        Signal(task, SIGF_SINGLE);
                        XPRINTF(10, ("Waiting lock %p transfer\n", pls));
                    }
                    moveowner = FALSE;
                    break;
                }
                prl = (struct PsdReadLock *) prl->prl_Node.ln_Succ;
            }
            if(moveowner)
            {
                if((pbl = (struct PsdBorrowLock *) AllocPooled(ps->ps_SemaMemPool, sizeof(struct PsdBorrowLock))))
                {
                    pbl->pbl_LockSem = pls;
                    pbl->pbl_ExclLockCount = pls->pls_ExclLockCount;
                    AddTail(&borrows, &pbl->pbl_Node);
                    pls->pls_Owner = task;
                    XPRINTF(10, ("Lock %p transfer\n", pls));
                }
            }
        }
        if(pls->pls_SharedLockCount)
        {
            prl = (struct PsdReadLock *) pls->pls_ReadLocks.lh_Head;
            do
            {
                if(prl->prl_Task == thistask)
                {
                    // check if target task is waiting for this task
                    struct PsdReadLock *prl2 = (struct PsdReadLock *) pls->pls_WaitQueue.lh_Head;
                    while(prl2->prl_Node.ln_Succ)
                    {
                        if(prl2->prl_Task == task)
                        {
                            // move lock to top of the list
                            Remove(&prl2->prl_Node);
                            AddHead(&pls->pls_WaitQueue, &prl2->prl_Node);
                            if((pbl = (struct PsdBorrowLock *) AllocPooled(ps->ps_SemaMemPool, sizeof(struct PsdBorrowLock))))
                            {
                                pbl->pbl_LockSem = pls;
                                pbl->pbl_ReadLock = prl;
                                pbl->pbl_Count = prl->prl_Count;
                                AddHead(&reclaims, &pbl->pbl_Node);

                                // unlock shared lock
                                Remove(&prl->prl_Node);
                                FreePooled(ps->ps_SemaMemPool, prl, sizeof(struct PsdReadLock));
                                pls->pls_SharedLockCount--;
                                Signal(task, SIGF_SINGLE);
                            }
                            moveowner = FALSE;
                            XPRINTF(10, ("Waiting shared lock %p transfer\n", pls));
                            break;
                        }
                        prl2 = (struct PsdReadLock *) prl2->prl_Node.ln_Succ;
                    }
                    if(moveowner)
                    {
                        // check if target task already has a shared lock on this
                        prl2 = (struct PsdReadLock *) pls->pls_ReadLocks.lh_Head;
                        do
                        {
                            if(prl2->prl_Task == task)
                            {
                                if((pbl = (struct PsdBorrowLock *) AllocPooled(ps->ps_SemaMemPool, sizeof(struct PsdBorrowLock))))
                                {
                                    // we redirect to this other lock
                                    pbl->pbl_LockSem = pls;
                                    pbl->pbl_ReadLock = prl2;
                                    pbl->pbl_Count = prl->prl_Count; // save the old lock count
                                    AddTail(&borrows, &pbl->pbl_Node);

                                    // unlock shared lock
                                    Remove(&prl->prl_Node);
                                    FreePooled(ps->ps_SemaMemPool, prl, sizeof(struct PsdReadLock));
                                    pls->pls_SharedLockCount--;
                                    // just increase lockcount, so a split occurs automatically
                                    prl2->prl_Count += pbl->pbl_Count;
                                }
                                XPRINTF(10, ("Already locked %p transfer\n", pls));
                                moveowner = FALSE;
                                break;
                            }
                        } while((prl2 = (struct PsdReadLock *) prl2->prl_Node.ln_Succ)->prl_Node.ln_Succ);
                    }
                    if(moveowner)
                    {
                        if((pbl = (struct PsdBorrowLock *) AllocPooled(ps->ps_SemaMemPool, sizeof(struct PsdBorrowLock))))
                        {
                            pbl->pbl_LockSem = pls;
                            pbl->pbl_ReadLock = prl;
                            pbl->pbl_Count = prl->prl_Count;
                            AddTail(&borrows, &pbl->pbl_Node);
                            prl->prl_Task = task;
                            XPRINTF(10, ("Std lock %p transfer\n", pls));
                        }
                    }
                    break;
                }
            } while((prl = (struct PsdReadLock *) prl->prl_Node.ln_Succ)->prl_Node.ln_Succ);
        }
        psi = (struct PsdSemaInfo *) psi->psi_Node.ln_Succ;
    }

    sigmask = Wait(signals);

    // try to get moved locks back first
    pbl = (struct PsdBorrowLock *) borrows.lh_Head;
    while(pbl->pbl_Node.ln_Succ)
    {
        Remove(&pbl->pbl_Node);
        pls = pbl->pbl_LockSem;
        if(pbl->pbl_ExclLockCount)
        {
            if(pbl->pbl_ExclLockCount == pls->pls_ExclLockCount)
            {
                // all fine, other task didn't use the locks or returned them already
                pls->pls_Owner = thistask;
                FreePooled(ps->ps_SemaMemPool, pbl, sizeof(struct PsdBorrowLock));
            } else {
                // okay, bad thing, release lock and try to obtain it again -- eventually the other task should free the lock again
                pls->pls_ExclLockCount -= pbl->pbl_ExclLockCount;
                AddTail(&reclaims, &pbl->pbl_Node);
            }
        } else {
            if(pls->pls_Owner == task)
            {
                // oh, damn. The other task converted our shared lock into an exclusive lock --
                // we cannot claim this back right now. This gets tricky now.
                if(pbl->pbl_Count == pbl->pbl_ReadLock->prl_Count)
                {
                    // luckily, the count didn't change, so we just release the shared lock and requeue us into the reclaim list
                    Remove(&pbl->pbl_ReadLock->prl_Node);
                    FreePooled(ps->ps_SemaMemPool, pbl->pbl_ReadLock, sizeof(struct PsdReadLock));
                    pbl->pbl_ReadLock = NULL;
                    pls->pls_SharedLockCount--; // should turn to 0
                } else {
                    // can it get worse? obviously, the alien task also has added some read locks
                    // this means we need to split up!
                    // therefore we leave a few lock counts and requeue
                    pbl->pbl_ReadLock->prl_Count -= pbl->pbl_Count;
                    pbl->pbl_ReadLock = NULL;
                }
                AddHead(&reclaims, &pbl->pbl_Node);
            } else {
                if(pbl->pbl_Count == pbl->pbl_ReadLock->prl_Count)
                {
                    // the count didn't change, just so just change owner
                    pbl->pbl_ReadLock->prl_Task = thistask;
                    FreePooled(ps->ps_SemaMemPool, pbl, sizeof(struct PsdBorrowLock));
                } else {
                    // the alien task still has some read locks
                    // this means we need to split up!
                    // therefore we leave a few lock counts and requeue
                    pbl->pbl_ReadLock->prl_Count -= pbl->pbl_Count;
                    pbl->pbl_ReadLock = NULL;
                    AddHead(&reclaims, &pbl->pbl_Node);
                }
            }
        }
        pbl = (struct PsdBorrowLock *) borrows.lh_Head;
    }

    // try to reclaim released locks
    pbl = (struct PsdBorrowLock *) reclaims.lh_Head;
    while(pbl->pbl_Node.ln_Succ)
    {
        Remove(&pbl->pbl_Node);
        pls = pbl->pbl_LockSem;
        while(pbl->pbl_Count)
        {
            pLockSemShared(ps, pls);
            --pbl->pbl_Count;
        }
        while(pbl->pbl_ExclLockCount)
        {
            pLockSemExcl(ps, pls);
            --pbl->pbl_ExclLockCount;
        }
        FreePooled(ps->ps_SemaMemPool, pbl, sizeof(struct PsdBorrowLock));
        pbl = (struct PsdBorrowLock *) reclaims.lh_Head;
    }
    Permit();

    return(sigmask);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Support *** */

/* /// "psdCopyStr()" */
AROS_LH1(STRPTR, psdCopyStr,
         AROS_LHA(CONST_STRPTR, name, A0),
         LIBBASETYPEPTR, ps, 10, psd)
{
    AROS_LIBFUNC_INIT
    STRPTR rs = psdAllocVec((ULONG) strlen(name)+1);
    KPRINTF(1, ("psdCopyStr(%s)\n", name));
    if(rs)
    {
        strcpy(rs, name);
    }
    return(rs);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdSafeRawDoFmtA()" */
AROS_LH4(void, psdSafeRawDoFmtA,
         AROS_LHA(STRPTR, buf, A0),
         AROS_LHA(ULONG, len, D0),
         AROS_LHA(CONST_STRPTR, fmtstr, A1),
         AROS_LHA(IPTR *, fmtdata, A2),
         LIBBASETYPEPTR, ps, 42, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdRawDoFmt rdf;

    if(len > 0)
    {
        rdf.rdf_Len = len;
        rdf.rdf_Buf = buf;
        RawDoFmt(fmtstr, fmtdata, (void (*)()) pPutChar, &rdf);
        buf[len-1] = 0;
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "pPutChar()" */
AROS_UFH2(void, pPutChar,
                   AROS_UFHA(char, ch, D0),
                   AROS_UFHA(struct PsdRawDoFmt *, rdf, A3))
{
    AROS_USERFUNC_INIT
    if(rdf->rdf_Len)
    {
        rdf->rdf_Len--;
        *rdf->rdf_Buf++ = ch;
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "psdCopyStrFmtA()" */
AROS_LH2(STRPTR, psdCopyStrFmtA,
         AROS_LHA(CONST_STRPTR, fmtstr, A0),
         AROS_LHA(IPTR *, fmtdata, A1),
         LIBBASETYPEPTR, ps, 68, psd)
{
    AROS_LIBFUNC_INIT
    ULONG len = 0;
    STRPTR buf;

    RawDoFmt(fmtstr, fmtdata, (void (*)()) pRawFmtLength, &len);
    buf = psdAllocVec(len+1);
    if(buf)
    {
        psdSafeRawDoFmtA(buf, len+1, fmtstr, fmtdata);
    }
    return(buf);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "pRawFmtLength()" */
AROS_UFH2(void, pRawFmtLength,
          AROS_UFHA(char, ch, D0),
          AROS_UFHA(ULONG *, len, A3))
{
    AROS_USERFUNC_INIT
    (*len)++;
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "psdDelayMS()" */
AROS_LH1(void, psdDelayMS,
         AROS_LHA(ULONG, milli, D0),
         LIBBASETYPEPTR, ps, 11, psd)
{
    AROS_LIBFUNC_INIT
    struct MsgPort mp;
    struct timerequest tr;

    KPRINTF(1, ("psdDelayMS(%ld)\n", milli));
    mp.mp_Flags = PA_SIGNAL;
    mp.mp_SigBit = SIGB_SINGLE;
    mp.mp_SigTask = FindTask(NULL);
    NewList(&mp.mp_MsgList);
    CopyMem(&ps->ps_TimerIOReq, &tr, sizeof(tr));
    tr.tr_node.io_Message.mn_ReplyPort = &mp;
    tr.tr_time.tv_secs  = 0;
    tr.tr_time.tv_micro = milli * 1000;
    DoIO((struct IORequest *) &tr);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdGetAttrsA()" */
AROS_LH3(LONG, psdGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, psdstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, ps, 22, psd)
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti;
    ULONG count = 0;
    ULONG *packtab = NULL;

    KPRINTF(1, ("psdGetAttrsA(%ld, %p, %p)\n", type, psdstruct, tags));

    if(type <= PGA_LAST)
    {
        packtab = (ULONG *) PsdPTArray[type];
    }

    switch(type)
    {
        case PGA_STACK:
            psdstruct = ps;
            if((ti = FindTagItem(PA_HardwareList, tags)))
            {
                *((struct List **) ti->ti_Data) = &ps->ps_Hardware;
                count++;
            }
            if((ti = FindTagItem(PA_ClassList, tags)))
            {
                *((struct List **) ti->ti_Data) = &ps->ps_Classes;
                count++;
            }
            if((ti = FindTagItem(PA_ErrorMsgList, tags)))
            {
                *((struct List **) ti->ti_Data) = &ps->ps_ErrorMsgs;
                count++;
            }
            break;

        case PGA_HARDWARE:
            if((ti = FindTagItem(HA_DeviceList, tags)))
            {
                *((struct List **) ti->ti_Data) = &(((struct PsdHardware *) psdstruct)->phw_Devices);
                count++;
            }
            break;

        case PGA_DEVICE:
            if((ti = FindTagItem(DA_ConfigList, tags)))
            {
                *((struct List **) ti->ti_Data) = &(((struct PsdDevice *) psdstruct)->pd_Configs);
                count++;
            }
            if((ti = FindTagItem(DA_DescriptorList, tags)))
            {
                *((struct List **) ti->ti_Data) = &(((struct PsdDevice *) psdstruct)->pd_Descriptors);
                count++;
            }
            break;

        case PGA_CONFIG:
            if((ti = FindTagItem(CA_InterfaceList, tags)))
            {
                *((struct List **) ti->ti_Data) = &(((struct PsdConfig *) psdstruct)->pc_Interfaces);
                count++;
            }
            break;

        case PGA_INTERFACE:
            if((ti = FindTagItem(IFA_EndpointList, tags)))
            {
                *((struct List **) ti->ti_Data) = &(((struct PsdInterface *) psdstruct)->pif_EPs);
                count++;
            }
            if((ti = FindTagItem(IFA_AlternateIfList, tags)))
            {
                *((struct List **) ti->ti_Data) = &(((struct PsdInterface *) psdstruct)->pif_AlterIfs);
                count++;
            }
            break;

        case PGA_ERRORMSG:
            if((ti = FindTagItem(EMA_DateStamp, tags)))
            {
                *((struct DateStamp **) ti->ti_Data) = &(((struct PsdErrorMsg *) psdstruct)->pem_DateStamp);
                count++;
            }
            break;

        case PGA_PIPE:
            if((ti = FindTagItem(PPA_IORequest, tags)))
            {
                *((struct IOUsbHWReq **) ti->ti_Data) = &(((struct PsdPipe *) psdstruct)->pp_IOReq);
                count++;
            }
            break;

        case PGA_STACKCFG:
            if((ti = FindTagItem(GCA_InsertionSound, tags)))
            {
                count++;
                *((STRPTR *) ti->ti_Data) = ps->ps_PoPo.po_InsertSndFile;
            }
            if((ti = FindTagItem(GCA_RemovalSound, tags)))
            {
                count++;
                *((STRPTR *) ti->ti_Data) = ps->ps_PoPo.po_RemoveSndFile;
            }
            break;
    }
    if(packtab)
    {
        return((LONG) (UnpackStructureTags(psdstruct, (ULONG *) packtab, tags)+count));
    } else {
        return(-1);
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdSetAttrsA()" */
AROS_LH3(LONG, psdSetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, psdstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, ps, 23, psd)
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti;
    ULONG count = 0;
    ULONG *packtab = NULL;
    BOOL savepopocfg = FALSE;
    BOOL checkcfgupdate = FALSE;
    BOOL powercalc = FALSE;
    LONG res;

    KPRINTF(1, ("psdSetAttrsA(%ld, %p, %p)\n", type, psdstruct, tags));

    if(type <= PGA_LAST)
    {
        packtab = (ULONG *) PsdPTArray[type];
    }

    switch(type)
    {
        case PGA_DEVICE:
            if(FindTagItem(DA_InhibitPopup, tags) || FindTagItem(DA_InhibitClassBind, tags))
            {
                savepopocfg = TRUE;
            }
            if(FindTagItem(DA_OverridePowerInfo, tags))
            {
                savepopocfg = TRUE;
                powercalc = TRUE;
            }
            break;

        case PGA_STACK:
            psdstruct = ps;
            break;

        case PGA_STACKCFG:
            if((ti = FindTagItem(GCA_InsertionSound, tags)))
            {
                count++;
                if(strcmp(ps->ps_PoPo.po_InsertSndFile, (STRPTR) ti->ti_Data))
                {
                    psdFreeVec(ps->ps_PoPo.po_InsertSndFile);
                    ps->ps_PoPo.po_InsertSndFile = psdCopyStr((STRPTR) ti->ti_Data);
                }
            }
            if((ti = FindTagItem(GCA_RemovalSound, tags)))
            {
                count++;
                if(strcmp(ps->ps_PoPo.po_RemoveSndFile, (STRPTR) ti->ti_Data))
                {
                    psdFreeVec(ps->ps_PoPo.po_RemoveSndFile);
                    ps->ps_PoPo.po_RemoveSndFile = psdCopyStr((STRPTR) ti->ti_Data);
                }
            }
            checkcfgupdate = TRUE;
            break;

        case PGA_PIPESTREAM:
        {
            struct PsdPipeStream *pps = (struct PsdPipeStream *) psdstruct;
            struct PsdPipe *pp;
            ULONG oldbufsize = pps->pps_BufferSize;
            ULONG oldnumpipes = pps->pps_NumPipes;
            ULONG cnt;

            KPRINTF(1, ("SetAttrs PIPESTREAM\n"));
            ObtainSemaphore(&pps->pps_AccessLock);
            if((ti = FindTagItem(PSA_MessagePort, tags)))
            {
                count++;
                if((pps->pps_Flags & PSFF_OWNMSGPORT) && pps->pps_MsgPort)
                {
                    KPRINTF(1, ("Deleting old MsgPort\n"));
                    DeleteMsgPort(pps->pps_MsgPort);
                    pps->pps_MsgPort = NULL;
                }
                pps->pps_Flags &= ~PSFF_OWNMSGPORT;
            }
            count += PackStructureTags(psdstruct, packtab, tags);
            KPRINTF(1, ("Pipes = %ld (old: %ld), BufferSize = %ld (old: %ld)\n",
                    pps->pps_NumPipes, oldnumpipes, pps->pps_BufferSize, oldbufsize));
            if(pps->pps_NumPipes < 1)
            {
                pps->pps_NumPipes = 1; /* minimal */
            }
            if(pps->pps_BufferSize < pps->pps_Endpoint->pep_MaxPktSize)
            {
                pps->pps_BufferSize = pps->pps_Endpoint->pep_MaxPktSize; /* minimal */
            }
            if(!pps->pps_MsgPort)
            {
                if((pps->pps_MsgPort = CreateMsgPort()))
                {
                    KPRINTF(1, ("Creating MsgPort\n"));
                    pps->pps_Flags |= PSFF_OWNMSGPORT;
                }
            }
            /* do we need to reallocate? */
            if((oldbufsize != pps->pps_BufferSize) ||
               (oldnumpipes != pps->pps_NumPipes) ||
               (!pps->pps_Pipes) ||
               (!pps->pps_Buffer))
            {
                if(pps->pps_Pipes)
                {
                    KPRINTF(1, ("freeing %ld old pipes\n", oldnumpipes));
                    for(cnt = 0; cnt < oldnumpipes; cnt++)
                    {
                        pp = pps->pps_Pipes[cnt];
                        //if(pp->pp_IOReq.iouh_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE)
                        {
                            KPRINTF(1, ("Abort %ld\n", cnt));
                            psdAbortPipe(pp);
                            KPRINTF(1, ("Wait %ld\n", cnt));
                            psdWaitPipe(pp);
                        }
                        KPRINTF(1, ("Free %ld\n", cnt));
                        psdFreePipe(pp);
                    }
                    psdFreeVec(pps->pps_Pipes);
                }
                psdFreeVec(pps->pps_Buffer);
                /* reset stuff */
                NewList(&pps->pps_FreePipes);
                NewList(&pps->pps_ReadyPipes);
                pps->pps_Offset = 0;
                pps->pps_BytesPending = 0;
                pps->pps_ReqBytes = 0;
                pps->pps_ActivePipe = NULL;
                pps->pps_Buffer = psdAllocVec(pps->pps_NumPipes * pps->pps_BufferSize);
                pps->pps_Pipes = psdAllocVec(pps->pps_NumPipes * sizeof(struct PsdPipe *));
                if(pps->pps_Pipes && pps->pps_Buffer)
                {
                    KPRINTF(1, ("allocating %ld new pipes\n", pps->pps_NumPipes));
                    for(cnt = 0; cnt < pps->pps_NumPipes; cnt++)
                    {
                        pp = psdAllocPipe(pps->pps_Device, pps->pps_MsgPort, pps->pps_Endpoint);
                        if((pps->pps_Pipes[cnt] = pp))
                        {
                            pp->pp_Num = cnt;
                            if(pps->pps_Flags & PSFF_NOSHORTPKT) pp->pp_IOReq.iouh_Flags |= UHFF_NOSHORTPKT;
                            if(pps->pps_Flags & PSFF_NAKTIMEOUT) pp->pp_IOReq.iouh_Flags |= UHFF_NAKTIMEOUT;
                            if(pps->pps_Flags & PSFF_ALLOWRUNT) pp->pp_IOReq.iouh_Flags |= UHFF_ALLOWRUNTPKTS;
                            pp->pp_IOReq.iouh_NakTimeout = pps->pps_NakTimeoutTime;
                            AddTail(&pps->pps_FreePipes, &pp->pp_Msg.mn_Node);
                        } else {
                            KPRINTF(1, ("Allocating Pipe %ld failed!\n", cnt));
                        }
                    }
                } else {
                    KPRINTF(1, ("Allocating Pipe array failed!\n"));
                    psdFreeVec(pps->pps_Buffer);
                    pps->pps_Buffer = NULL;
                    psdFreeVec(pps->pps_Pipes);
                    pps->pps_Pipes = NULL;
                }
            }
            ReleaseSemaphore(&pps->pps_AccessLock);
            return((LONG) count);
        }
    }

    if(packtab)
    {
        res = (LONG) PackStructureTags(psdstruct, packtab, tags);
    } else {
        res = -1;
    }
    if(savepopocfg)
    {
        struct PsdDevice *pd = (struct PsdDevice *) psdstruct;
        struct PsdIFFContext *pic;

        pic = psdGetUsbDevCfg("Trident", pd->pd_IDString, NULL);
        if(!pic)
        {
            psdSetUsbDevCfg("Trident", pd->pd_IDString, NULL, NULL);
            pic = psdGetUsbDevCfg("Trident", pd->pd_IDString, NULL);
        }
        if(pic)
        {
            pAddCfgChunk(ps, pic, &pd->pd_PoPoCfg);
            checkcfgupdate = TRUE;
        }
    }
    if(checkcfgupdate)
    {
        pUpdateGlobalCfg(ps, (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head);
        pCheckCfgChanged(ps);
    }
    if(powercalc)
    {
        psdCalculatePower(((struct PsdDevice *) psdstruct)->pd_Hardware);
    }
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdSpawnSubTask()" */
AROS_LH3(struct Task *, psdSpawnSubTask,
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(APTR, initpc, A1),
         AROS_LHA(APTR, userdata, A2),
         LIBBASETYPEPTR, ps, 39, psd)
{
    AROS_LIBFUNC_INIT
#define SUBTASKSTACKSIZE 8192
    struct
    {
        struct MemList mrm_ml;
        struct MemEntry mtm_me[2];
    } memlist;

    struct MemList *newmemlist;
    struct MemEntry *me;
    struct Task *nt;
    struct Process *subtask;

    if(!(name && initpc))
    {
        return(NULL);
    }

    /* If there's dos available, create a process instead of a task */
    if(pOpenDOS(ps))
    {
         subtask = CreateNewProcTags(NP_Entry, initpc,
                                     NP_StackSize, SUBTASKSTACKSIZE,
                                     NP_Priority, ps->ps_GlobalCfg->pgc_SubTaskPri,
                                     NP_Name, name,
                                     NP_CopyVars, FALSE,
                                     NP_UserData, userdata,
                                     TAG_END);
         return((struct Task *) subtask);
    }

    /* Allocate memory of memlist */

    memlist.mrm_ml.ml_Node.ln_Type = NT_MEMORY;
    memlist.mrm_ml.ml_Node.ln_Pri = 0;
    memlist.mrm_ml.ml_Node.ln_Name = NULL;
    memlist.mrm_ml.ml_NumEntries = 3;
    me = &memlist.mrm_ml.ml_ME[0];
    me[1].me_Un.meu_Reqs = memlist.mrm_ml.ml_ME[0].me_Un.meu_Reqs = MEMF_CLEAR|MEMF_PUBLIC;
    me[0].me_Length = sizeof(struct Task);
    me[1].me_Length = SUBTASKSTACKSIZE;
    me[2].me_Un.meu_Reqs = MEMF_PUBLIC;
    me[2].me_Length = strlen(name) + 1;

#ifdef __AROS__
    newmemlist = NewAllocEntry(&memlist.mrm_ml, NULL);
    if (!newmemlist)
#else
    newmemlist = AllocEntry(&memlist.mrm_ml);
    if((IPTR) newmemlist & 0x80000000)
#endif
    {
        return(NULL);
    }
    me = &newmemlist->ml_ME[0];
    nt = me[0].me_Un.meu_Addr;
    nt->tc_Node.ln_Name = me[2].me_Un.meu_Addr;
    strcpy(nt->tc_Node.ln_Name, name);
    nt->tc_Node.ln_Type = NT_TASK;
    nt->tc_Node.ln_Pri = ps->ps_GlobalCfg->pgc_SubTaskPri;
    nt->tc_SPLower = me[1].me_Un.meu_Addr;
    nt->tc_SPUpper = nt->tc_SPReg = (APTR) ((IPTR) nt->tc_SPLower + SUBTASKSTACKSIZE);
    nt->tc_UserData = userdata;
    NewList(&nt->tc_MemEntry);
    AddTail(&nt->tc_MemEntry, (struct Node *) newmemlist);
    KPRINTF(1, ("TDNestCnt=%ld\n", SysBase->TDNestCnt));
    if((nt = AddTask(nt, initpc, NULL)))
    {
        XPRINTF(10, ("Started task %p (%s)\n", nt, name));
        return(nt);
    }
    FreeEntry(newmemlist);
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdNumToStr()" */
AROS_LH3(STRPTR, psdNumToStr,
         AROS_LHA(UWORD, type, D0),
         AROS_LHA(LONG, idx, D1),
         AROS_LHA(STRPTR, defstr, A0),
         LIBBASETYPEPTR, ps, 38, psd)
{
    AROS_LIBFUNC_INIT
    switch(type)
    {
        case NTS_IOERR:
        {
            const struct PsdWStringMap *psm = usbhwioerrstr;
            while(psm->psm_ID)
            {
                if(psm->psm_ID == idx)
                {
                    return(psm->psm_String);
                }
                psm++;
            }
            break;
        }

        case NTS_LANGID:
        {
            const struct PsdUWStringMap *psm = usblangids;
            while(psm->psm_ID)
            {
                if(psm->psm_ID == idx)
                {
                    return(psm->psm_String);
                }
                psm++;
            }
            break;
        }

        case NTS_TRANSTYPE:
            switch(idx)
            {
                case USEAF_CONTROL:
                    return("control");
                case USEAF_ISOCHRONOUS:
                    return("isochronous");
                case USEAF_BULK:
                    return("bulk");
                case USEAF_INTERRUPT:
                    return("interrupt");
            }
            break;

        case NTS_SYNCTYPE:
            switch(idx)
            {
                case USEAF_NOSYNC:
                    return("no synchronization");
                case USEAF_ASYNC:
                    return("asynchronous");
                case USEAF_ADAPTIVE:
                    return("adaptive");
                case USEAF_SYNC:
                    return("synchronous");
            }
            break;

        case NTS_USAGETYPE:
            switch(idx)
            {
                case USEAF_DATA:
                    return("data");
                case USEAF_FEEDBACK:
                    return("feedback");
                case USEAF_IMPLFEEDBACK:
                    return("implicit feedback data");
            }
            break;

        case NTS_VENDORID:
        {
            const struct PsdUWStringMap *psm = usbvendorids;
            while(psm->psm_ID)
            {
                if(psm->psm_ID == idx)
                {
                    return(psm->psm_String);
                }
                psm++;
            }
            break;
        }

        case NTS_CLASSCODE:
        {
            const struct PsdWStringMap *psm = usbclasscodestr;
            while(psm->psm_ID)
            {
                if(psm->psm_ID == idx)
                {
                    return(psm->psm_String);
                }
                psm++;
            }
            break;
        }

        case NTS_DESCRIPTOR:
        {
            const struct PsdULStringMap *psm = usbdesctypestr;
            while(psm->psm_ID)
            {
                if(psm->psm_ID == idx)
                {
                    return(psm->psm_String);
                }
                psm++;
            }
            break;
        }

        case NTS_COMBOCLASS:
        {
            const struct PsdULStringMap *psm = usbcomboclasscodestr;
            if(idx & (NTSCCF_CLASS|NTSCCF_SUBCLASS|NTSCCF_PROTO))
            {
                while(psm->psm_ID)
                {
                    BOOL take;
                    take = TRUE;
                    if(psm->psm_ID & NTSCCF_CLASS)
                    {
                        if((!(idx & NTSCCF_CLASS)) || ((idx & 0x0000ff) != (psm->psm_ID & 0x0000ff)))
                        {
                            take = FALSE;
                        }
                    }
                    if(psm->psm_ID & NTSCCF_SUBCLASS)
                    {
                        if((!(idx & NTSCCF_SUBCLASS)) || ((idx & 0x00ff00) != (psm->psm_ID & 0x00ff00)))
                        {
                            take = FALSE;
                        }
                    }
                    if(psm->psm_ID & NTSCCF_PROTO)
                    {
                        if((!(idx & NTSCCF_PROTO)) || ((idx & 0xff0000) != (psm->psm_ID & 0xff0000)))
                        {
                            take = FALSE;
                        }
                    }
                    if(take)
                    {
                        return(psm->psm_String);
                    }
                    psm++;
                }
            }
            break;
        }
    }
    return(defstr);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Endpoint *** */

/* /// "pFreeEndpoint()" */
void pFreeEndpoint(struct PsdEndpoint *pep)
{
    LIBBASETYPEPTR ps = pep->pep_Interface->pif_Config->pc_Device->pd_Hardware->phw_Base;
    KPRINTF(2, ("    FreeEndpoint()\n"));
    Remove(&pep->pep_Node);
    psdFreeVec(pep);
}
/* \\\ */

/* /// "pAllocEndpoint()" */
struct PsdEndpoint * pAllocEndpoint(struct PsdInterface *pif)
{
    LIBBASETYPEPTR ps = pif->pif_Config->pc_Device->pd_Hardware->phw_Base;
    struct PsdEndpoint *pep;
    if((pep = psdAllocVec(sizeof(struct PsdEndpoint))))
    {
        pep->pep_Interface = pif;
        AddTail(&pif->pif_EPs, &pep->pep_Node);
        return(pep);
    }
    return(NULL);
}
/* \\\ */

/* /// "psdFindEndpointA()" */
AROS_LH3(struct PsdEndpoint *, psdFindEndpointA,
         AROS_LHA(struct PsdInterface *, pif, A0),
         AROS_LHA(struct PsdEndpoint *, pep, A2),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, ps, 67, psd)
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti;
    BOOL takeit;

    KPRINTF(2, ("psdFindEndpointA(%p, %p, %p)\n", pif, pep, tags));
    if(!pep)
    {
        pep = (struct PsdEndpoint *) pif->pif_EPs.lh_Head;
    } else {
        pep = (struct PsdEndpoint *) pep->pep_Node.ln_Succ;
    }
    while(pep->pep_Node.ln_Succ)
    {
        takeit = TRUE;
        if((ti = FindTagItem(EA_IsIn, tags)))
        {
            if((ti->ti_Data && !pep->pep_Direction) || (!ti->ti_Data && pep->pep_Direction))
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(EA_EndpointNum, tags)))
        {
            if(ti->ti_Data != pep->pep_EPNum)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(EA_TransferType, tags)))
        {
            if(ti->ti_Data != pep->pep_TransType)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(EA_MaxPktSize, tags)))
        {
            if(ti->ti_Data != pep->pep_MaxPktSize)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(EA_Interval, tags)))
        {
            if(ti->ti_Data != pep->pep_Interval)
            {
                takeit = FALSE;
            }
        }

        if(takeit)
        {
            return(pep);
        }
        pep = (struct PsdEndpoint *) pep->pep_Node.ln_Succ;
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Interface *** */

/* /// "pFreeInterface()" */
void pFreeInterface(struct PsdInterface *pif)
{
    LIBBASETYPEPTR ps = pif->pif_Config->pc_Device->pd_Hardware->phw_Base;
    struct PsdEndpoint *pep = (struct PsdEndpoint *) pif->pif_EPs.lh_Head;
    struct PsdInterface *altif = (struct PsdInterface *) pif->pif_AlterIfs.lh_Head;
    KPRINTF(2, ("   FreeInterface()\n"));
    /* Remove alternate interfaces */
    while(altif->pif_Node.ln_Succ)
    {
        pFreeInterface(altif);
        altif = (struct PsdInterface *) pif->pif_AlterIfs.lh_Head;
    }
    /* Remove endpoints */
    while(pep->pep_Node.ln_Succ)
    {
        pFreeEndpoint(pep);
        pep = (struct PsdEndpoint *) pif->pif_EPs.lh_Head;
    }
    psdFreeVec(pif->pif_IfStr);
    psdFreeVec(pif->pif_IDString);
    Remove(&pif->pif_Node);
    psdFreeVec(pif);
}
/* \\\ */

/* /// "pAllocInterface()" */
struct PsdInterface * pAllocInterface(struct PsdConfig *pc)
{
    LIBBASETYPEPTR ps = pc->pc_Device->pd_Hardware->phw_Base;
    struct PsdInterface *pif;
    if((pif = psdAllocVec(sizeof(struct PsdInterface))))
    {
        pif->pif_Config = pc;
        NewList(&pif->pif_EPs);
        NewList(&pif->pif_AlterIfs);
        AddTail(&pc->pc_Interfaces, &pif->pif_Node);
        return(pif);
    }
    return(NULL);
}
/* \\\ */

/* /// "psdFindInterfaceA()" */
AROS_LH3(struct PsdInterface *, psdFindInterfaceA,
         AROS_LHA(struct PsdDevice *, pd, A0),
         AROS_LHA(struct PsdInterface *, pif, A2),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, ps, 66, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdConfig *pc;
    struct TagItem *ti;
    BOOL takeit;
    BOOL searchalt = FALSE;
    BOOL isalt = FALSE;
    struct PsdInterface *oldpif = NULL;

    KPRINTF(2, ("psdFindInterfaceA(%p, %p, %p)\n", pd, pif, tags));
    if(!pif)
    {
        pc = pd->pd_CurrentConfig;
        if(pc)
        {
            pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
        }
        if(!pif)
        {
            return(NULL);
        }
    } else {
        if(FindTagItem(IFA_AlternateNum, tags))
        {
            searchalt = TRUE;
        }
        if(pif->pif_ParentIf)
        {
            // special case: we are in an alternate interface right now
            searchalt = TRUE;
            if(pif->pif_Node.ln_Succ)
            {
                isalt = TRUE;
                oldpif = pif->pif_ParentIf;
                pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
            } else {
                pif = (struct PsdInterface *) pif->pif_ParentIf->pif_Node.ln_Succ;
            }
        } else {
            // go into alt interfaces
            if(searchalt && pif->pif_AlterIfs.lh_Head->ln_Succ)
            {
                isalt = TRUE;
                oldpif = pif;
                pif = (struct PsdInterface *) pif->pif_AlterIfs.lh_Head;
            } else {
                pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
            }
        }
    }

    while(pif->pif_Node.ln_Succ)
    {
        takeit = TRUE;
        if((ti = FindTagItem(IFA_InterfaceNum, tags)))
        {
            if(ti->ti_Data != pif->pif_IfNum)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(IFA_AlternateNum, tags)))
        {
            searchalt = TRUE;
            if(ti->ti_Data <= 0xff) // if alternate number is greater than 0xff, don't check compliance, but just enable alternate interface searching
            {
                if(ti->ti_Data != pif->pif_Alternate)
                {
                    takeit = FALSE;
                }
            }
        }
        if((ti = FindTagItem(IFA_NumEndpoints, tags)))
        {
            if(ti->ti_Data != pif->pif_NumEPs)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(IFA_Class, tags)))
        {
            if(ti->ti_Data != pif->pif_IfClass)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(IFA_SubClass, tags)))
        {
            if(ti->ti_Data != pif->pif_IfSubClass)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(IFA_Protocol, tags)))
        {
            if(ti->ti_Data != pif->pif_IfProto)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(IFA_Binding, tags)))
        {
            if((APTR) ti->ti_Data != pif->pif_IfBinding)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(IFA_InterfaceName, tags)))
        {
            if(strcmp((STRPTR) ti->ti_Data, pif->pif_IfStr))
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(IFA_IDString, tags)))
        {
            if(strcmp((STRPTR) ti->ti_Data, pif->pif_IDString))
            {
                takeit = FALSE;
            }
        }

        if(takeit)
        {
            return(pif);
        }
        if(searchalt)
        {
            if(isalt)
            {
                pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
                if(!pif->pif_Node.ln_Succ)
                {
                    pif = (struct PsdInterface *) oldpif->pif_Node.ln_Succ;
                    isalt = FALSE;
                }
            } else {
                oldpif = pif;
                pif = (struct PsdInterface *) pif->pif_AlterIfs.lh_Head;
                if(!pif->pif_Node.ln_Succ)
                {
                    pif = (struct PsdInterface *) oldpif->pif_Node.ln_Succ;
                } else {
                    isalt = TRUE;
                }
            }
        } else {
            pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
        }
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Config *** */

/* /// "pFreeConfig()" */
void pFreeConfig(struct PsdConfig *pc)
{
    LIBBASETYPEPTR ps = pc->pc_Device->pd_Hardware->phw_Base;
    struct PsdInterface *pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
    KPRINTF(2, ("  FreeConfig()\n"));
    while(pif->pif_Node.ln_Succ)
    {
        psdReleaseIfBinding(pif);
        pFreeInterface(pif);
        pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
    }
    psdFreeVec(pc->pc_CfgStr);
    Remove(&pc->pc_Node);
    psdFreeVec(pc);
}
/* \\\ */

/* /// "pAllocConfig()" */
struct PsdConfig * pAllocConfig(struct PsdDevice *pd)
{
    LIBBASETYPEPTR ps = pd->pd_Hardware->phw_Base;
    struct PsdConfig *pc;
    KPRINTF(2, ("  AllocConfig()\n"));
    if((pc = psdAllocVec(sizeof(struct PsdConfig))))
    {
        pc->pc_Device = pd;
        NewList(&pc->pc_Interfaces);
        AddTail(&pd->pd_Configs, &pc->pc_Node);
        return(pc);
    }
    return(NULL);
}
/* \\\ */

/* *** Descriptors *** */

/* /// "pFreeDescriptor()" */
void pFreeDescriptor(struct PsdDescriptor *pdd)
{
    LIBBASETYPEPTR ps = pdd->pdd_Device->pd_Hardware->phw_Base;
    KPRINTF(2, ("  FreeDescriptor()\n"));
    //psdFreeVec(pdd->pdd_Data); // part of the structure alloc
    Remove(&pdd->pdd_Node);
    psdFreeVec(pdd);
}
/* \\\ */

/* /// "pAllocDescriptor()" */
struct PsdDescriptor * pAllocDescriptor(struct PsdDevice *pd, UBYTE *buf)
{
    LIBBASETYPEPTR ps = pd->pd_Hardware->phw_Base;
    struct PsdDescriptor *pdd;

    KPRINTF(2, ("  AllocDescriptor()\n"));
    if((pdd = psdAllocVec(sizeof(struct PsdDescriptor) + (ULONG) buf[0])))
    {
        pdd->pdd_Device = pd;
        pdd->pdd_Data = ((UBYTE *) pdd) + sizeof(struct PsdDescriptor);
        pdd->pdd_Length = buf[0];
        pdd->pdd_Type = buf[1];
        if((pdd->pdd_Type >= UDT_CS_UNDEFINED) && (pdd->pdd_Type <= UDT_CS_ENDPOINT))
        {
            pdd->pdd_CSSubType = buf[2];
        }
        pdd->pdd_Name = psdNumToStr(NTS_DESCRIPTOR, (LONG) pdd->pdd_Type, "<unknown>");
        CopyMem(buf, pdd->pdd_Data, (ULONG) buf[0]);
        AddTail(&pd->pd_Descriptors, &pdd->pdd_Node);
        return(pdd);
    }
    return(NULL);
}
/* \\\ */

/* /// "psdFindDescriptorA()" */
AROS_LH3(struct PsdDescriptor *, psdFindDescriptorA,
         AROS_LHA(struct PsdDevice *, pd, A0),
         AROS_LHA(struct PsdDescriptor *, pdd, A2),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, ps, 91, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdConfig *pc = pd->pd_CurrentConfig;
    struct TagItem *ti;
    BOOL takeit;

    KPRINTF(2, ("psdFindDescriptorA(%p, %p, %p)\n", pd, pdd, tags));
    if(!pdd)
    {
        pdd = (struct PsdDescriptor *) pd->pd_Descriptors.lh_Head;
    } else {
        pdd = (struct PsdDescriptor *) pdd->pdd_Node.ln_Succ;
    }

    while(pdd->pdd_Node.ln_Succ)
    {
        takeit = TRUE;

        if((ti = FindTagItem(DDA_Config, tags)))
        {
            // special case to workaround default: with NULL given, all configs are matched
            if(ti->ti_Data && (((struct PsdConfig *) ti->ti_Data) != pdd->pdd_Config))
            {
                takeit = FALSE;
            }
        } else {
            // only take descriptors from the current configuration by default
            if(pc != pdd->pdd_Config)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DDA_Interface, tags)))
        {
            if(((struct PsdInterface *) ti->ti_Data) != pdd->pdd_Interface)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DDA_Endpoint, tags)))
        {
            if(((struct PsdEndpoint *) ti->ti_Data) != pdd->pdd_Endpoint)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DDA_DescriptorType, tags)))
        {
            if(ti->ti_Data != pdd->pdd_Type)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DDA_CS_SubType, tags)))
        {
            if(ti->ti_Data != pdd->pdd_CSSubType)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DDA_DescriptorLength, tags)))
        {
            if(ti->ti_Data != pdd->pdd_Length)
            {
                takeit = FALSE;
            }
        }

        if(takeit)
        {
            return(pdd);
        }
        pdd = (struct PsdDescriptor *) pdd->pdd_Node.ln_Succ;
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Device *** */

/* /// "pFreeBindings()" */
void pFreeBindings(LIBBASETYPEPTR ps, struct PsdDevice *pd)
{
    struct PsdHardware *phw = pd->pd_Hardware;
    struct PsdConfig *pc;
    struct PsdInterface *pif;
    KPRINTF(3, (" FreeBindings(%p)\n", pd));

    /* move device to list of dead devices first
       This caused a lot of trouble as it could
       happen that a device got into class scan
       right after the bindings had been released. */
    psdLockWritePBase();
    Remove(&pd->pd_Node);
    AddTail(&phw->phw_DeadDevices, &pd->pd_Node);
    psdUnlockPBase();

    /* If there are bindings, get rid of them. */
    psdLockWriteDevice(pd);
    psdReleaseDevBinding(pd);

    pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
    while(pc->pc_Node.ln_Succ)
    {
        pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
        while(pif->pif_Node.ln_Succ)
        {
            psdReleaseIfBinding(pif);
            pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
        }
        pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
    }
    psdUnlockDevice(pd);
}
/* \\\ */

/* /// "pFreeDevice()" */
void pFreeDevice(LIBBASETYPEPTR ps, struct PsdDevice *pd)
{
    struct PsdHardware *phw = pd->pd_Hardware;
    struct PsdConfig *pc;
    struct PsdDescriptor *pdd;

    psdCalculatePower(phw);
    psdLockWriteDevice(pd);
    if(pd->pd_UseCnt)
    {
        KPRINTF(20, ("Couldn't free device, use cnt %ld\n", pd->pd_UseCnt));
        pd->pd_Flags &= ~PDFF_CONNECTED;
        pd->pd_Flags |= PDFF_DELEXPUNGE;
        psdUnlockDevice(pd);
    } else {
        pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
        while(pc->pc_Node.ln_Succ)
        {
            pFreeConfig(pc);
            pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
        }

        pdd = (struct PsdDescriptor *) pd->pd_Descriptors.lh_Head;
        while(pdd->pdd_Node.ln_Succ)
        {
            pFreeDescriptor(pdd);
            pdd = (struct PsdDescriptor *) pd->pd_Descriptors.lh_Head;
        }

        psdFreeVec(pd->pd_LangIDArray);
        pd->pd_LangIDArray = NULL;
        psdFreeVec(pd->pd_MnfctrStr);
        pd->pd_MnfctrStr = NULL;
        /*if(!ps->ps_PoPo.po_Task) // keep name at least
        {
            psdFreeVec(pd->pd_ProductStr);
            pd->pd_ProductStr = NULL;
        }*/
        psdFreeVec(pd->pd_OldProductStr);
        pd->pd_OldProductStr = NULL;
        psdFreeVec(pd->pd_SerNumStr);
        pd->pd_SerNumStr = NULL;
        psdFreeVec(pd->pd_IDString);
        pd->pd_IDString = NULL;
        if(pd->pd_DevAddr)
        {
            KPRINTF(5,("Released DevAddr %ld\n", pd->pd_DevAddr));
            phw->phw_DevArray[pd->pd_DevAddr] = NULL;
        }
        psdUnlockDevice(pd);
        psdLockWritePBase();
        Remove(&pd->pd_Node);
        psdUnlockPBase();
        pDeleteSem(ps, &pd->pd_Lock);
        /* cannot free this vector -- tasks might still call LockDevice */
        //psdFreeVec(pd);
    }
    KPRINTF(3, ("FreeDevice done\n"));
}
/* \\\ */

/* /// "psdFreeDevice()" */
AROS_LH1(void, psdFreeDevice,
         AROS_LHA(struct PsdDevice *, pd, A0),
         LIBBASETYPEPTR, ps, 16, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdHardware *phw = pd->pd_Hardware;
    struct PsdConfig *pc;
    struct PsdInterface *pif;
    struct PsdRTIsoHandler *prt;
    struct PsdRTIsoHandler *nextprt;

    KPRINTF(3, (" FreeDevice(%p)\n", pd));

    /* move device to list of dead devices first
       This caused a lot of trouble as it could
       happen that a device got into class scan
       right after the bindings had been released. */
    psdLockWritePBase();
    Remove(&pd->pd_Node);
    AddTail(&phw->phw_DeadDevices, &pd->pd_Node);
    pd->pd_Flags &= ~PDFF_DELEXPUNGE;
    psdUnlockPBase();

    psdLockWriteDevice(pd);

    /* Inform all ISO handlers about the device going offline */
    prt = (struct PsdRTIsoHandler *) pd->pd_RTIsoHandlers.lh_Head;
    while((nextprt = (struct PsdRTIsoHandler *) prt->prt_Node.ln_Succ))
    {
        if(prt->prt_ReleaseHook)
        {
            CallHookPkt(prt->prt_ReleaseHook, prt, NULL);
        }
        prt = nextprt;
    }

    /* If there are bindings, get rid of them. */
    psdHubReleaseDevBinding(pd);

    pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
    while(pc->pc_Node.ln_Succ)
    {
        pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
        while(pif->pif_Node.ln_Succ)
        {
            psdHubReleaseIfBinding(pif);
            pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
        }
        pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
    }
    psdUnlockDevice(pd);

    pFreeDevice(ps, pd);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdAllocDevice()" */
AROS_LH1(struct PsdDevice *, psdAllocDevice,
         AROS_LHA(struct PsdHardware *, phw, A0),
         LIBBASETYPEPTR, ps, 15, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdDevice *pd;
    KPRINTF(2, ("psdAllocDevice(%p)\n", phw));
    if((pd = psdAllocVec(sizeof(struct PsdDevice))))
    {
        pd->pd_Hardware = phw;
        pd->pd_Hub = NULL;
        pd->pd_MaxPktSize0 = 8;

        pInitSem(ps, &pd->pd_Lock, "Device");

        NewList(&pd->pd_Configs);
        NewList(&pd->pd_Descriptors);
        NewList(&pd->pd_RTIsoHandlers);

        // init prefs
        pd->pd_PoPoCfg.poc_ChunkID = AROS_LONG2BE(IFFCHNK_POPUP);
        pd->pd_PoPoCfg.poc_Length = AROS_LONG2BE(sizeof(struct PsdPoPoCfg) - 8);
        pd->pd_PoPoCfg.poc_InhibitPopup = FALSE;
        pd->pd_PoPoCfg.poc_NoClassBind = FALSE;
        pd->pd_PoPoCfg.poc_OverridePowerInfo = POCP_TRUST_DEVICE;

        psdLockWritePBase();
        AddTail(&phw->phw_Devices, &pd->pd_Node);
        psdUnlockPBase();
        return(pd);
    } else {
        KPRINTF(20, ("psdAllocDevice(): out of memory!\n"));
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdLockReadDevice()" */
AROS_LH1(void, psdLockReadDevice,
         AROS_LHA(struct PsdDevice *, pd, A0),
         LIBBASETYPEPTR, ps, 17, psd)
{
    AROS_LIBFUNC_INIT
    KPRINTF(2, ("psdLockReadDevice(%p, %p)\n", pd, FindTask(NULL)));
    pLockSemShared(ps, &pd->pd_Lock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdLockWriteDevice()" */
AROS_LH1(void, psdLockWriteDevice,
         AROS_LHA(struct PsdDevice *, pd, A0),
         LIBBASETYPEPTR, ps, 18, psd)
{
    AROS_LIBFUNC_INIT
    KPRINTF(2, ("psdLockWriteDevice(%p, %p)\n", pd, FindTask(NULL)));
    pLockSemExcl(ps, &pd->pd_Lock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdUnlockDevice()" */
AROS_LH1(void, psdUnlockDevice,
         AROS_LHA(struct PsdDevice *, pd, A0),
         LIBBASETYPEPTR, ps, 19, psd)
{
    AROS_LIBFUNC_INIT
    KPRINTF(2, ("psdUnlockDevice(%p, %p)\n", pd, FindTask(NULL)));
    pUnlockSem(ps, &pd->pd_Lock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "pAllocDevAddr()" */
UWORD pAllocDevAddr(struct PsdDevice *pd)
{
    struct PsdHardware *phw = pd->pd_Hardware;
    UWORD da;
    if(pd->pd_DevAddr)
    {
        return(pd->pd_DevAddr);
    }
    for(da = 1; da < 128; da++)
    {
        if(!phw->phw_DevArray[da])
        {
            phw->phw_DevArray[da] = pd;
            pd->pd_DevAddr = da;
            return(da);
        }
    }
    return(0);
}
/* \\\ */

/* /// "psdGetStringDescriptor()" */
AROS_LH2(STRPTR, psdGetStringDescriptor,
         AROS_LHA(struct PsdPipe *, pp, A1),
         AROS_LHA(UWORD, idx, D0),
         LIBBASETYPEPTR, ps, 33, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdDevice *pd = pp->pp_Device;
    ULONG len;
    UBYTE buf[256];
    UWORD *tmpptr;
    UWORD *tmpbuf;
    STRPTR rs;
    STRPTR cbuf;
    LONG ioerr;
    UWORD widechar;
    KPRINTF(1, ("psdGetStringDescriptor(%p, %ld)\n", pp, idx));

    buf[0] = 0;
    if(!pd->pd_LangIDArray)
    {
        KPRINTF(10,("Trying to get language array...\n"));
        psdPipeSetup(pp, URTF_IN|URTF_STANDARD|URTF_DEVICE,
                     USR_GET_DESCRIPTOR, UDT_STRING<<8, 0);
        ioerr = psdDoPipe(pp, buf, 2);
        if(ioerr == UHIOERR_OVERFLOW)
        {
            ioerr = 0;
            psdAddErrorMsg0(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Language array overflow.");
        }
        if(ioerr)
        {
            ioerr = psdDoPipe(pp, buf, 256);
            if(ioerr == UHIOERR_RUNTPACKET)
            {
                ioerr = 0;
            }
        }
        if(!ioerr)
        {
            len = buf[0];
            if((pd->pd_LangIDArray = psdAllocVec(max(len, 4))))
            {
                tmpbuf = tmpptr = pd->pd_LangIDArray;
                KPRINTF(1, ("Getting LangID Array length %ld\n", len));
                // generate minimal sized array
                if(len < 4)
                {
                    len = 4;
                    *tmpbuf++ = 0;
                    *tmpbuf = AROS_WORD2LE(0x0409);
                    /*psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Language array descriptor too small (len %ld), using dummy.",
                                   len);*/
                    ioerr = 0;
                } else {
                    ioerr = psdDoPipe(pp, tmpbuf++, len);
                }
                if(!ioerr)
                {
                    len >>= 1;
                    while(--len)
                    {
                        KPRINTF(1, ("LangID: %04lx\n", AROS_WORD2LE(*tmpbuf)));
                        *tmpptr++ = AROS_WORD2LE(*tmpbuf);
                        tmpbuf++;
                    }
                    *tmpptr = 0;
                    tmpptr = pd->pd_LangIDArray;
                    pd->pd_CurrLangID = *tmpptr;
                    while(*tmpptr)
                    {
                        if(*tmpptr == 0x0409)
                        {
                            pd->pd_CurrLangID = *tmpptr;
                            break;
                        }
                        tmpptr++;
                    }
                } else {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Reading language array descriptor (len %ld) failed: %s (%ld)",
                                   len, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    KPRINTF(15, ("Error reading lang array descriptor (%ld) failed %ld\n", len, ioerr));
                    *tmpptr = 0;
                }
            } else {
                KPRINTF(20, ("No langid array memory!\n"));
            }
        } else {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Reading language array descriptor (len %ld) failed: %s (%ld)",
                           2, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            KPRINTF(15, ("Error reading lang array descriptor (2) failed %ld\n", ioerr));
            /* Create empty array */
            if((pd->pd_LangIDArray = psdAllocVec(2)))
            {
                *pd->pd_LangIDArray = 0;
            }
        }
    }
    buf[0] = 0;
    psdPipeSetup(pp, URTF_IN|URTF_STANDARD|URTF_DEVICE,
                 USR_GET_DESCRIPTOR, (UDT_STRING<<8)|idx, pd->pd_CurrLangID);
    ioerr = psdDoPipe(pp, buf, 2);
    if(ioerr == UHIOERR_OVERFLOW)
    {
        ioerr = 0;
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "String %ld overflow.", idx);
    }
    if(ioerr)
    {
        ioerr = psdDoPipe(pp, buf, 256);
    }
    if(!ioerr)
    {
        len = buf[0];
        if(len > 2)
        {
            tmpptr = (UWORD *) buf;
            KPRINTF(1, ("Getting String Descriptor %ld, length %ld\n", idx, len));
            ioerr = psdDoPipe(pp, tmpptr++, len);
            if(ioerr == UHIOERR_RUNTPACKET)
            {
                len = pp->pp_IOReq.iouh_Actual;
                if(len > 3)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "String descriptor %ld truncated to %ld, requested %ld",
                                   idx, len, buf[0]);
                    ioerr = 0;
                }
            }
            else if(ioerr)
            {
                ioerr = psdDoPipe(pp, buf, 256);
            }
            if(!ioerr)
            {
                if((cbuf = rs = psdAllocVec(len>>1)))
                {
                    len >>= 1;
                    while(--len)
                    {
                        widechar = *tmpptr++;
                        widechar = AROS_LE2WORD(widechar);
                        if(widechar == 0)
                        {
                            break;
                        }
                        if((widechar < 0x20) || (widechar > 255))
                        {
                            *cbuf++ = '?';
                        } else {
                            *cbuf++ = widechar;
                        }
                    }
                    *cbuf = 0;
                    KPRINTF(1, ("String \"%s\"\n", rs));
                    if(*rs)
                    {
                        return(rs);
                    } else {
                        psdFreeVec(rs);
                        return(NULL);
                    }
                } else {
                    KPRINTF(20, ("No memory for string!\n"));
                }
            } else {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "Reading string descriptor %ld (len %ld) failed: %s (%ld)",
                               idx, len, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                KPRINTF(15, ("Error reading string descriptor %ld (%ld) failed %ld\n",
                       idx, len, ioerr));
            }
        } else {
            KPRINTF(5, ("Empty string\n"));
            return(psdCopyStr(""));
        }
    } else {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "Reading string descriptor %ld (len %ld) failed: %s (%ld)",
                       idx, 2, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        KPRINTF(15, ("Error reading string descriptor %ld (2) failed %ld\n", idx, ioerr));
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdSetDeviceConfig()" */
AROS_LH2(BOOL, psdSetDeviceConfig,
         AROS_LHA(struct PsdPipe *, pp, A1),
         AROS_LHA(UWORD, cfgnum, D0),
         LIBBASETYPEPTR, ps, 34, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdConfig *pc;
    struct PsdDevice *pd = pp->pp_Device;
    //UBYTE buf[8]; // VIA babble bug safety buffer (8 instead of 2)
    LONG ioerr;
    BOOL res = FALSE;

    KPRINTF(2, ("Setting configuration to %ld...\n", cfgnum));
    psdPipeSetup(pp, URTF_STANDARD|URTF_DEVICE,
                 USR_SET_CONFIGURATION, cfgnum, 0);
    ioerr = psdDoPipe(pp, NULL, 0);
    if(!ioerr)
    {
#if 0 // MacOS X does not verify the configuration set. And as we don't check the results anyway, don't obtain current configuration to avoid bad devices breaking down
        psdPipeSetup(pp, URTF_IN|URTF_STANDARD|URTF_DEVICE,
                     USR_GET_CONFIGURATION, 0, 0);
        ioerr = psdDoPipe(pp, buf, 1);
        if(!ioerr)
        {
            pd->pd_CurrCfg = buf[0];
            if(cfgnum != buf[0])
            {
                pd->pd_CurrCfg = cfgnum;
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "Broken: SetConfig/GetConfig mismatch (%ld != %ld) for %s!",
                               cfgnum, buf[0], pp->pp_Device->pd_ProductStr);
            }
            res = TRUE;
        } else {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "GET_CONFIGURATION failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            pd->pd_CurrCfg = cfgnum;
            KPRINTF(15, ("GET_CONFIGURATION failed %ld!\n", ioerr));
        }
#else
        pd->pd_CurrCfg = cfgnum;
        res = TRUE;
#endif
    } else {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "SET_CONFIGURATION failed: %s (%ld)",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        KPRINTF(15, ("SET_CONFIGURATION failed %ld!\n", ioerr));
    }
    // update direct link
    Forbid();
    pd->pd_CurrentConfig = NULL;
    pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
    while(pc->pc_Node.ln_Succ)
    {
        if(pc->pc_CfgNum == pd->pd_CurrCfg)
        {
            pd->pd_CurrentConfig = pc;
            break;
        }
        pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
    }
    Permit();
    if(!pd->pd_CurrentConfig)
    {
        psdAddErrorMsg0(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "No current configuration, huh?");
    } else {
        UWORD status = 0;
        // power saving stuff
        if(ps->ps_GlobalCfg->pgc_PowerSaving && (pd->pd_CurrentConfig->pc_Attr & USCAF_REMOTE_WAKEUP))
        {
            psdPipeSetup(pp, URTF_STANDARD|URTF_DEVICE,
                         USR_SET_FEATURE, UFS_DEVICE_REMOTE_WAKEUP, 0);
            ioerr = psdDoPipe(pp, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "SET_DEVICE_REMOTE_WAKEUP failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                KPRINTF(15, ("SET_DEVICE_REMOTE_WAKEUP failed %ld!\n", ioerr));
            }
            psdPipeSetup(pp, URTF_IN|URTF_STANDARD|URTF_DEVICE, USR_GET_STATUS, 0, 0);
            ioerr = psdDoPipe(pp, &status, 2);
            if(!ioerr)
            {
                if(status & U_GSF_REMOTE_WAKEUP)
                {
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "Enabled remote wakeup feature for '%s'.",
                                   pd->pd_ProductStr);
                } else {
                    pd->pd_CurrentConfig->pc_Attr &= ~USCAF_REMOTE_WAKEUP;
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Remote wakeup feature for '%s' could not be enabled.",
                                   pd->pd_ProductStr);
                }
            } else {
                /*psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "GET_STATUS failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);*/
                KPRINTF(15, ("GET_STATUS failed %ld!\n", ioerr));
            }
        } else {
            psdPipeSetup(pp, URTF_IN|URTF_STANDARD|URTF_DEVICE, USR_GET_STATUS, 0, 0);
            ioerr = psdDoPipe(pp, &status, 2);
        }
        if(!ioerr)
        {
            if((status & U_GSF_SELF_POWERED) && (!(pd->pd_CurrentConfig->pc_Attr & USCAF_SELF_POWERED)))
            {
                pd->pd_CurrentConfig->pc_Attr |= USCAF_SELF_POWERED;
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "Device '%s' says it is currently self-powered. Fixing config.",
                               pd->pd_ProductStr);
            }
            else if((!(status & U_GSF_SELF_POWERED)) && (pd->pd_CurrentConfig->pc_Attr & USCAF_SELF_POWERED))
            {
                pd->pd_CurrentConfig->pc_Attr &= ~USCAF_SELF_POWERED;
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "Device '%s' says it is currently bus-powered. Fixing config.",
                               pd->pd_ProductStr);
            }
        } else {
            /*psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "GET_STATUS failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);*/
            KPRINTF(15, ("GET_STATUS failed %ld!\n", ioerr));
        }
    }

    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdSetAltInterface()" */
AROS_LH2(BOOL, psdSetAltInterface,
         AROS_LHA(struct PsdPipe *, pp, A1),
         AROS_LHA(struct PsdInterface *, pif, A0),
         LIBBASETYPEPTR, ps, 43, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdConfig *pc = pif->pif_Config;
    struct PsdInterface *curif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
    struct PsdInterface *tmpif;
    struct PsdDevice *pd = pc->pc_Device;
    UBYTE buf[8]; // VIA babble bug safety buffer (8 instead of 2)
    LONG ioerr;
    UWORD ifnum = pif->pif_IfNum;
    UWORD altnum = pif->pif_Alternate;

    KPRINTF(2, ("Setting interface %ld to alt %ld...\n", ifnum, altnum));
    psdLockWriteDevice(pd);

    /* Find root config structure */
    while(curif->pif_Node.ln_Succ)
    {
        if(curif->pif_IfNum == ifnum)
        {
            break;
        }
        curif = (struct PsdInterface *) curif->pif_Node.ln_Succ;
    }
    if(!curif->pif_Node.ln_Succ)
    {
        KPRINTF(20, ("Where did you get that fucking interface from?!?"));
        psdUnlockDevice(pd);
        return(FALSE);
    }
    if(curif == pif) /* Is already the current alternate setting */
    {
        psdUnlockDevice(pd);
        return(TRUE);
    }
    KPRINTF(1, ("really setting interface...\n"));
    if(pp)
    {
        psdPipeSetup(pp, URTF_STANDARD|URTF_INTERFACE,
                     USR_SET_INTERFACE, altnum, ifnum);
        ioerr = psdDoPipe(pp, NULL, 0);
    } else {
        ioerr = 0;
    }
    if((!ioerr) || (ioerr == UHIOERR_STALL))
    {
        if(pp)
        {
            psdPipeSetup(pp, URTF_IN|URTF_STANDARD|URTF_INTERFACE,
                         USR_GET_INTERFACE, 0, ifnum);
            ioerr = psdDoPipe(pp, buf, 1);
        } else {
            buf[0] = altnum;
        }
        if(!ioerr)
        {
            if(altnum == buf[0])
            {
                KPRINTF(1, ("resorting list..."));
                /*psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "Changed to alt %ld",
                               altnum);*/
                Forbid();
                /* Remove pif from alt list */
                Remove(&pif->pif_Node);
                pif->pif_ParentIf = NULL;
                /* Now move bindings */
                pif->pif_ClsBinding = curif->pif_ClsBinding;
                pif->pif_IfBinding = curif->pif_IfBinding;
                curif->pif_IfBinding = NULL;
                curif->pif_ClsBinding = NULL;
                /* Insert it after root interface */
                Insert(&pc->pc_Interfaces, (struct Node *) &pif->pif_Node, (struct Node *) &curif->pif_Node);
                /* Unlink root interface */
                Remove(&curif->pif_Node);
                /* Now move all remaining alt interfaces to the new root interface */
                tmpif = (struct PsdInterface *) curif->pif_AlterIfs.lh_Head;
                while(tmpif->pif_Node.ln_Succ)
                {
                    Remove(&tmpif->pif_Node);
                    AddTail(&pif->pif_AlterIfs, &tmpif->pif_Node);
                    tmpif->pif_ParentIf = pif;
                    tmpif = (struct PsdInterface *) curif->pif_AlterIfs.lh_Head;
                }
                /* Add old root to the end of the alt list */
                AddTail(&pif->pif_AlterIfs, &curif->pif_Node);
                curif->pif_ParentIf = pif;
                Permit();
                psdUnlockDevice(pd);
                return(TRUE);
            } else {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "Attempt to change interface %ld to alt %ld remained at alt %ld.",
                               ifnum, altnum, buf[0]);
            }
        } else {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "GET_INTERFACE(%ld) failed: %s (%ld)",
                           ifnum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            KPRINTF(15, ("GET_INTERFACE failed %ld!\n", ioerr));
        }
    } else {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "SET_INTERFACE(%ld)=%ld failed: %s (%ld)",
                       ifnum, altnum,
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        KPRINTF(15, ("SET_INTERFACE failed %ld!\n", ioerr));
    }
    psdUnlockDevice(pd);
    return(FALSE);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdEnumerateDevice()" */
AROS_LH1(struct PsdDevice *, psdEnumerateDevice,
         AROS_LHA(struct PsdPipe *, pp, A1),
         LIBBASETYPEPTR, ps, 20, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdDevice *pd = pp->pp_Device;
    struct PsdDevice *itpd = pp->pp_Device;
    struct PsdConfig *pc;
    struct PsdInterface *pif;
    struct UsbStdDevDesc usdd;
    UWORD oldflags;
    ULONG oldnaktimeout;
    LONG ioerr;
    STRPTR classname;
    STRPTR vendorname;
    ULONG devclass;
    BOOL hasprodname;
    BOOL haspopupinhibit;
    UWORD cfgnum;
    struct PsdIFFContext *pic;
    ULONG *chnk;
    UBYTE dummybuf[8];

    KPRINTF(2, ("psdEnumerateDevice(%p)\n", pp));

    psdLockWriteDevice(pd);
    if(pAllocDevAddr(pd))
    {
        oldflags = pp->pp_IOReq.iouh_Flags;
        oldnaktimeout = pp->pp_IOReq.iouh_NakTimeout;
        pp->pp_IOReq.iouh_Flags |= UHFF_NAKTIMEOUT;
        pp->pp_IOReq.iouh_NakTimeout = 1000;
        pp->pp_IOReq.iouh_DevAddr = 0;

        psdPipeSetup(pp, URTF_IN|URTF_STANDARD|URTF_DEVICE,
                     USR_GET_DESCRIPTOR, UDT_DEVICE<<8, 0);
        ioerr = psdDoPipe(pp, dummybuf, 8);
        if(ioerr && (ioerr != UHIOERR_RUNTPACKET))
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "GET_DESCRIPTOR (len %ld) failed: %s (%ld)",
                           8, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            KPRINTF(15, ("GET_DESCRIPTOR (8) failed %ld!\n", ioerr));
        }
        KPRINTF(1, ("Setting DevAddr %ld...\n", pd->pd_DevAddr));
        psdPipeSetup(pp, URTF_STANDARD|URTF_DEVICE,
                     USR_SET_ADDRESS, pd->pd_DevAddr, 0);
        ioerr = psdDoPipe(pp, NULL, 0);
        /* This is tricky: Maybe the device has accepted the command,
           but failed to send an ACK. Now, every resend trial will
           go to the wrong address! */
        if((ioerr == UHIOERR_TIMEOUT) || (ioerr == UHIOERR_STALL))
        {
            KPRINTF(1, ("First attempt failed, retrying new address\n"));
            /*pp->pp_IOReq.iouh_DevAddr = pd->pd_DevAddr;*/
            psdDelayMS(250);
            ioerr = psdDoPipe(pp, NULL, 0);
            /*pp->pp_IOReq.iouh_DevAddr = 0;*/
        }
        if(!ioerr)
        {
            pd->pd_Flags |= PDFF_HASDEVADDR|PDFF_CONNECTED;
            pp->pp_IOReq.iouh_DevAddr = pd->pd_DevAddr;

            psdDelayMS(50); /* Allowed time to settle */

            KPRINTF(1, ("Getting MaxPktSize0...\n"));
            psdPipeSetup(pp, URTF_IN|URTF_STANDARD|URTF_DEVICE,
                         USR_GET_DESCRIPTOR, UDT_DEVICE<<8, 0);
            ioerr = psdDoPipe(pp, &usdd, 8);
            if(!ioerr)
            {
                switch(usdd.bMaxPacketSize0)
                {
                    case 8:
                    case 16:
                    case 32:
                    case 64:
                        pp->pp_IOReq.iouh_MaxPktSize = pd->pd_MaxPktSize0 = usdd.bMaxPacketSize0;
                        break;
                    default:
#ifdef AROS_USB30_CODE
                        if( (AROS_LE2WORD(usdd.bcdUSB) >= 0x0300) && (usdd.bMaxPacketSize0 == 9) ) {
                            pp->pp_IOReq.iouh_MaxPktSize = pd->pd_MaxPktSize0 = usdd.bMaxPacketSize0;
                            break;
                        }
#endif
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Illegal MaxPktSize0=%ld for endpoint 0", (ULONG) usdd.bMaxPacketSize0);
                        KPRINTF(2, ("Illegal MaxPktSize0=%ld!\n", usdd.bMaxPacketSize0));
                        //pp->pp_IOReq.iouh_MaxPktSize = pd->pd_MaxPktSize0 = 8;
                        ioerr = UHIOERR_CRCERROR;
                        break;
                }
            }
            if(!ioerr)
            {
                KPRINTF(1, ("  MaxPktSize0 = %ld\n", pd->pd_MaxPktSize0));
                KPRINTF(1, ("Getting full descriptor...\n"));
                ioerr = psdDoPipe(pp, &usdd, sizeof(struct UsbStdDevDesc));
                if(!ioerr)
                {
                    pAllocDescriptor(pd, (UBYTE *) &usdd);
                    pd->pd_Flags |= PDFF_HASDEVDESC;
                    pd->pd_USBVers = AROS_WORD2LE(usdd.bcdUSB);
                    pd->pd_DevClass = usdd.bDeviceClass;
                    pd->pd_DevSubClass = usdd.bDeviceSubClass;
                    pd->pd_DevProto = usdd.bDeviceProtocol;
                    pd->pd_VendorID = AROS_WORD2LE(usdd.idVendor);
                    pd->pd_ProductID = AROS_WORD2LE(usdd.idProduct);
                    pd->pd_DevVers = AROS_WORD2LE(usdd.bcdDevice);
                    vendorname = psdNumToStr(NTS_VENDORID, (LONG) pd->pd_VendorID, NULL);

                    // patch to early determine highspeed roothubs
                    if((!pd->pd_Hub) && (pd->pd_USBVers >= 0x200) && (pd->pd_USBVers < 0x300))
                    {
                        pd->pd_Flags |= PDFF_HIGHSPEED;
                    }
                    #ifdef AROS_USB30_CODE
                    if((!pd->pd_Hub) && (pd->pd_USBVers >= 0x300))
                    {
                        pd->pd_Flags |= PDFF_SUPERSPEED;
                    }
                    #endif

                    if(usdd.iManufacturer)
                    {
                        pd->pd_MnfctrStr = psdGetStringDescriptor(pp, usdd.iManufacturer);
                    }
                    if(usdd.iProduct)
                    {
                        pd->pd_ProductStr = psdGetStringDescriptor(pp, usdd.iProduct);
                    }
                    if(usdd.iSerialNumber)
                    {
                        pd->pd_SerNumStr = psdGetStringDescriptor(pp, usdd.iSerialNumber);
                    }
                    if(!pd->pd_MnfctrStr)
                    {
                        pd->pd_MnfctrStr = psdCopyStr(vendorname ? vendorname : (STRPTR) "n/a");
                    }
                    if(!pd->pd_ProductStr)
                    {
                        hasprodname = FALSE;
                        classname = psdNumToStr(NTS_CLASSCODE, (LONG) pd->pd_DevClass, NULL);
                        if(classname)
                        {
                            pd->pd_ProductStr = psdCopyStrFmt("%s: Vdr=%04lx/PID=%04lx",
                                            classname, pd->pd_VendorID, pd->pd_ProductID);
                        } else {
                            pd->pd_ProductStr = psdCopyStrFmt("Cls=%ld/Vdr=%04lx/PID=%04lx",
                                            pd->pd_DevClass, pd->pd_VendorID, pd->pd_ProductID);
                        }
                    } else {
                        hasprodname = TRUE;
                    }
                    if(!pd->pd_SerNumStr)
                    {
                        pd->pd_SerNumStr = psdCopyStr("n/a");
                    }

                    KPRINTF(2, ("Product     : %s\n"
                                "Manufacturer: %s\n"
                                "SerialNumber: %s\n",
                                pd->pd_ProductStr, pd->pd_MnfctrStr, pd->pd_SerNumStr));
                    KPRINTF(2, ("USBVersion: %04lx\n"
                                "Class     : %ld\n"
                                "SubClass  : %ld\n"
                                "DevProto  : %ld\n"
                                "VendorID  : %ld\n"
                                "ProductID : %ld\n"
                                "DevVers   : %04lx\n",
                                pd->pd_USBVers, pd->pd_DevClass, pd->pd_DevSubClass, pd->pd_DevProto,
                                pd->pd_VendorID, pd->pd_ProductID, pd->pd_DevVers));

                    /* check for clones */
                    itpd = NULL;
                    while((itpd = psdGetNextDevice(itpd)))
                    {
                        if(itpd != pd)
                        {
                            if((itpd->pd_ProductID == pd->pd_ProductID) &&
                               (itpd->pd_VendorID == pd->pd_VendorID) &&
                               (!strcmp(itpd->pd_SerNumStr, pd->pd_SerNumStr)) &&
                               (itpd->pd_CloneCount == pd->pd_CloneCount))
                            {
                                pd->pd_CloneCount++;
                                itpd = NULL;
                            }
                        }
                    }

                    pd->pd_IDString = psdCopyStrFmt("%s-%04lx-%04lx-%s-%02lx", pd->pd_ProductStr, pd->pd_VendorID, pd->pd_ProductID, pd->pd_SerNumStr, pd->pd_CloneCount);

                    pStripString(ps, pd->pd_MnfctrStr);
                    pStripString(ps, pd->pd_ProductStr);
                    pStripString(ps, pd->pd_SerNumStr);

                    /* get custom name of device */
                    pLockSemExcl(ps, &ps->ps_ConfigLock); // Exclusive lock to avoid deadlock situation when promoting read to write
                    pd->pd_OldProductStr = pd->pd_ProductStr;
                    pd->pd_ProductStr = NULL;
                    haspopupinhibit = FALSE;
                    pic = psdGetUsbDevCfg("Trident", pd->pd_IDString, NULL);
                    if(pic)
                    {
                        pd->pd_IsNewToMe = FALSE;
                        if((pd->pd_ProductStr = pGetStringChunk(ps, pic, IFFCHNK_NAME)))
                        {
                            hasprodname = TRUE;
                        }
                        if((chnk = pFindCfgChunk(ps, pic, IFFCHNK_POPUP)))
                        {
                            struct PsdPoPoCfg *poc = (struct PsdPoPoCfg *) chnk;
                            CopyMem(((UBYTE *) poc) + 8, ((UBYTE *) &pd->pd_PoPoCfg) + 8, min(AROS_LONG2BE(poc->poc_Length), AROS_LONG2BE(pd->pd_PoPoCfg.poc_Length)));
                            haspopupinhibit = TRUE;
                        }
                    } else {
                        pd->pd_IsNewToMe = TRUE;
                        psdSetUsbDevCfg("Trident", pd->pd_IDString, NULL, NULL);
                    }
                    if(!pd->pd_ProductStr)
                    {
                        pd->pd_ProductStr = psdCopyStr(pd->pd_OldProductStr);
                    }
                    if(!haspopupinhibit)
                    {
                        if(pd->pd_DevClass == HUB_CLASSCODE) // hubs default to true
                        {
                            pd->pd_PoPoCfg.poc_InhibitPopup = TRUE;
                        }
                    }
                    pUnlockSem(ps, &ps->ps_ConfigLock);

                    pd->pd_NumCfgs = usdd.bNumConfigurations;
                    KPRINTF(10, ("Device has %ld different configurations\n", pd->pd_NumCfgs));

                    if(pGetDevConfig(pp))
                    {
                        cfgnum = 1;
                        if(pd->pd_Configs.lh_Head->ln_Succ)
                        {
                            cfgnum = ((struct PsdConfig *) pd->pd_Configs.lh_Head)->pc_CfgNum;
                        }
                        psdSetDeviceConfig(pp, cfgnum); /* *** FIXME *** Workaround for USB2.0 devices */
                        {
                            if(!hasprodname)
                            {
                                devclass = pd->pd_DevClass;
                                pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
                                while(pc->pc_Node.ln_Succ)
                                {
                                    pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                                    while(pif->pif_Node.ln_Succ)
                                    {
                                        if(pif->pif_IfClass)
                                        {
                                            if(!devclass)
                                            {
                                                devclass = pif->pif_IfClass;
                                            } else {
                                                if(devclass != pif->pif_IfClass)
                                                {
                                                    devclass = 0;
                                                    break;
                                                }
                                            }
                                        }
                                        pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
                                    }
                                    pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
                                }
                                if(devclass)
                                {
                                    classname = psdNumToStr(NTS_CLASSCODE, (LONG) devclass, NULL);
                                    if(classname)
                                    {
                                        psdFreeVec(pd->pd_ProductStr);
                                        if(vendorname)
                                        {
                                            pd->pd_ProductStr = psdCopyStrFmt("%s (%s/%04lx)",
                                                                              classname, vendorname, pd->pd_ProductID);
                                        } else {
                                            pd->pd_ProductStr = psdCopyStrFmt("%s (%04lx/%04lx)",
                                                                              classname, pd->pd_VendorID, pd->pd_ProductID);
                                        }
                                    }
                                }
                            }
                            pFixBrokenConfig(pp);
                            pp->pp_IOReq.iouh_Flags = oldflags;
                            pp->pp_IOReq.iouh_NakTimeout = oldnaktimeout;
                            psdUnlockDevice(pd);
                            psdCalculatePower(pd->pd_Hardware);
                            return(pd);
                        } /*else {
                            KPRINTF(15, ("SetDeviceConfig(1) failed\n"));
                        }*/
                    } else {
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                       "Could not acquire device configuration for %s",
                                       pd->pd_ProductStr ? pd->pd_ProductStr : (STRPTR) "new device");
                        KPRINTF(15, ("GetDevConfig() failed\n"));
                    }
                    /* Although the device failed to configure fully, maybe
                       some firmware will able to use this device anyway? */
                    pp->pp_IOReq.iouh_Flags = oldflags;
                    pp->pp_IOReq.iouh_NakTimeout = oldnaktimeout;

                    psdUnlockDevice(pd);
                    return(pd);
                } else {
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                   "GET_DESCRIPTOR (len %ld) failed: %s (%ld)",
                                   18, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    KPRINTF(15, ("GET_DESCRIPTOR (18) failed %ld!\n", ioerr));
                }
            } else {
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                               "GET_DESCRIPTOR (len %ld) failed: %s (%ld)",
                               8, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                KPRINTF(15, ("GET_DESCRIPTOR (8) failed %ld!\n", ioerr));
            }
        } else {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                           "SET_ADDRESS failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            KPRINTF(15, ("SET_ADDRESS failed %ld!\n", ioerr));

        }
        pp->pp_IOReq.iouh_Flags = oldflags;
        pp->pp_IOReq.iouh_NakTimeout = oldnaktimeout;
    } else {
        psdAddErrorMsg0(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "This cannot happen! More than 127 devices on the bus???");
        KPRINTF(20, ("out of addresses???\n"));
    }
    psdAddErrorMsg0(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Device enumeration failed, sorry.");
    psdUnlockDevice(pd);
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdGetNextDevice()" */
AROS_LH1(struct PsdDevice *, psdGetNextDevice,
         AROS_LHA(struct PsdDevice *, pd, A0),
         LIBBASETYPEPTR, ps, 21, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdHardware *phw;

    KPRINTF(1, ("psdGetNextDevice(%p)\n", pd));
    if(pd)
    {
        /* Is there another device node in the current hardware? */
        if(pd->pd_Node.ln_Succ->ln_Succ)
        {
            return((struct PsdDevice *) pd->pd_Node.ln_Succ);
        }
        /* No, then check if there's another hardware to scan */
        phw = (struct PsdHardware *) pd->pd_Hardware->phw_Node.ln_Succ;
    } else {
        /* No context, start with first hardware */
        phw = (struct PsdHardware *) ps->ps_Hardware.lh_Head;
    }
    while(phw->phw_Node.ln_Succ)
    {
        pd = (struct PsdDevice *) phw->phw_Devices.lh_Head;
        /* Is this an valid entry, or is the list empty? */
        if(pd->pd_Node.ln_Succ)
        {
            return(pd);
        }
        phw = (struct PsdHardware *) phw->phw_Node.ln_Succ;
    }
    /* End of list reached */
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdSuspendBindings()" */
AROS_LH1(BOOL, psdSuspendBindings,
         AROS_LHA(struct PsdDevice *, pd, A0),
         LIBBASETYPEPTR, ps, 100, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdUsbClass *puc;
    struct PsdConfig *pc;
    struct PsdInterface *pif;
    BOOL res = FALSE;
    IPTR suspendable;
    BOOL force = FALSE;

    KPRINTF(5, ("psdSuspendBindings(%p)\n", pd));
    if(pd)
    {
        if(ps->ps_GlobalCfg->pgc_ForceSuspend && (pd->pd_CurrentConfig->pc_Attr & USCAF_REMOTE_WAKEUP))
        {
            force = TRUE;
        }
        // ask existing bindings to go to suspend first -- if they don't support it, break off
        if(pd->pd_DevBinding)
        {
            if(pd->pd_Flags & PDFF_APPBINDING)
            {
                if(!force)
                {
                    // can't suspend application binding devices
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                   "Cannot suspend with application binding on '%s'.",
                                   pd->pd_ProductStr);
                    return FALSE;
                }
                psdReleaseDevBinding(pd);
            }
            if((puc = pd->pd_ClsBinding))
            {
                suspendable = 0;
                usbGetAttrs(UGA_CLASS, NULL, UCCA_SupportsSuspend, &suspendable, TAG_END);
                if(suspendable)
                {
                    res = usbDoMethod(UCM_AttemptSuspendDevice, pd->pd_DevBinding);
                    if(!res)
                    {
                        // didn't want to suspend
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                       "Class '%s' failed to suspend device '%s'.",
                                       puc->puc_Node.ln_Name, pd->pd_ProductStr);
                        return FALSE;
                    }
                } else {
                    if(pd->pd_IOBusyCount)
                    {
                        if(!force)
                        {
                            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                           "Class '%s' does not support suspending.",
                                           puc->puc_Node.ln_Name);
                            return FALSE;
                        } else {
                            psdReleaseDevBinding(pd);
                        }
                    } else {
                        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "Class '%s' does not support suspending, but has no active IO. Suspending anyway.",
                                       puc->puc_Node.ln_Name);
                    }
                }
            }
        }
        pc = pd->pd_CurrentConfig;
        pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
        while(pif->pif_Node.ln_Succ)
        {
            if(pif->pif_IfBinding)
            {
                if((puc = pif->pif_ClsBinding))
                {
                    suspendable = 0;
                    usbGetAttrs(UGA_CLASS, NULL, UCCA_SupportsSuspend, &suspendable, TAG_END);
                    if(suspendable)
                    {
                        res = usbDoMethod(UCM_AttemptSuspendDevice, pif->pif_IfBinding);
                        if(!res)
                        {
                            // didn't want to suspend
                            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                           "%s failed to suspend device '%s'.",
                                           puc->puc_Node.ln_Name, pd->pd_ProductStr);
                            return FALSE;
                        }
                    } else {
                        if(pd->pd_IOBusyCount)
                        {
                            if(!force)
                            {

                                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                               "%s does not support suspending.",
                                               puc->puc_Node.ln_Name);
                                return FALSE;
                            } else {
                                psdReleaseIfBinding(pif);
                            }
                        } else {
                            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                           "%s does not support suspending, but has no active IO. Suspending anyway.",
                                           puc->puc_Node.ln_Name);
                        }
                    }
                }
            }
            pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
        }
        return TRUE;
    }
    return FALSE;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdSuspendDevice()" */
AROS_LH1(BOOL, psdSuspendDevice,
         AROS_LHA(struct PsdDevice *, pd, A0),
         LIBBASETYPEPTR, ps, 98, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdUsbClass *puc;
    struct PsdDevice *hubpd;
    APTR binding;
    BOOL res = FALSE;

    KPRINTF(5, ("psdSuspendDevice(%p)\n", pd));
    if(pd)
    {
        if(pd->pd_Flags & PDFF_SUSPENDED)
        {
            return TRUE;
        }
        hubpd = pd->pd_Hub;
        if(!hubpd) // suspend root hub
        {
            // suspend whole USB, using the HCI UHCMD_USBSUSPEND command
            // FIXME currently unsupported!
            psdAddErrorMsg0(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Suspending of root hub currently not supported.");
            return FALSE;
        }

        psdLockWriteDevice(pd);
        res = psdSuspendBindings(pd);
        psdUnlockDevice(pd);
        if(res)
        {
            psdLockReadDevice(pd);
            if((binding = hubpd->pd_DevBinding) && (puc = hubpd->pd_ClsBinding))
            {
                res = usbDoMethod(UCM_HubSuspendDevice, binding, pd);
            }
            psdUnlockDevice(pd);
        }
    }
    if(!res)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "Suspending of device '%s' failed.",
                       pd->pd_ProductStr);
    }
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdResumeBindings()" */
AROS_LH1(BOOL, psdResumeBindings,
         AROS_LHA(struct PsdDevice *, pd, A0),
         LIBBASETYPEPTR, ps, 101, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdUsbClass *puc;
    struct PsdConfig *pc;
    struct PsdInterface *pif;
    BOOL res = FALSE;
    BOOL rescan = FALSE;

    KPRINTF(5, ("psdResumeBindings(%p)\n", pd));
    if(pd)
    {
        // ask existing bindings to resume -- if they don't support it, rebind
        if(pd->pd_DevBinding)
        {
            if(!(pd->pd_Flags & PDFF_APPBINDING))
            {
                if((puc = pd->pd_ClsBinding))
                {
                    res = usbDoMethod(UCM_AttemptResumeDevice, pd->pd_DevBinding);
                    if(!res)
                    {
                        // if the device couldn't resume, better get rid of the binding
                        psdReleaseDevBinding(pd);
                        rescan = TRUE;
                    }
                }
            }
        }
        pc = pd->pd_CurrentConfig;
        pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
        while(pif->pif_Node.ln_Succ)
        {
            if(pif->pif_IfBinding)
            {
                if((puc = pif->pif_ClsBinding))
                {
                    res = usbDoMethod(UCM_AttemptResumeDevice, pif->pif_IfBinding);
                    if(!res)
                    {
                        // didn't want to suspend
                        psdReleaseIfBinding(pif);
                        rescan = TRUE;
                    }
                }
                break;
            }
            pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
        }
        if(rescan)
        {
            psdClassScan();
        }
    }
    return(TRUE);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdResumeDevice()" */
AROS_LH1(BOOL, psdResumeDevice,
         AROS_LHA(struct PsdDevice *, pd, A0),
         LIBBASETYPEPTR, ps, 99, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdUsbClass *puc;
    struct PsdDevice *hubpd;
    APTR binding;
    BOOL res = FALSE;

    KPRINTF(5, ("psdResumeDevice(%p)\n", pd));
    if(pd)
    {
        if(!(pd->pd_Flags & PDFF_SUSPENDED))
        {
            return(TRUE);
        }
        hubpd = pd->pd_Hub;
        if(!hubpd) // resume root hub
        {
            // resume whole USB, using the HCI UHCMD_USBRESUME command
            // FIXME currently unsupported!
            return(FALSE);
        }
        psdLockWriteDevice(pd);
        if((binding = hubpd->pd_DevBinding) && (puc = hubpd->pd_ClsBinding))
        {
            res = usbDoMethod(UCM_HubResumeDevice, binding, pd);
        }
        psdUnlockDevice(pd);

        if(res)
        {
            psdResumeBindings(pd);
        }
    }

    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdFindDeviceA()" */
AROS_LH2(struct PsdDevice *, psdFindDeviceA,
         AROS_LHA(struct PsdDevice *, pd, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, ps, 44, psd)
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti;
    BOOL takeit;
    KPRINTF(2, ("psdFindDeviceA(%p, %p)\n", pd, tags));
    while((pd = psdGetNextDevice(pd)))
    {
        takeit = TRUE;
        if((ti = FindTagItem(DA_ProductID, tags)))
        {
            if(ti->ti_Data != pd->pd_ProductID)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DA_VendorID, tags)))
        {
            if(ti->ti_Data != pd->pd_VendorID)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DA_Class, tags)))
        {
            if(ti->ti_Data != pd->pd_DevClass)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DA_SubClass, tags)))
        {
            if(ti->ti_Data != pd->pd_DevSubClass)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DA_Protocol, tags)))
        {
            if(ti->ti_Data != pd->pd_DevProto)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DA_Version, tags)))
        {
            if(ti->ti_Data != pd->pd_DevVers)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DA_SerialNumber, tags)))
        {
            if(strcmp((STRPTR) ti->ti_Data, pd->pd_SerNumStr))
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DA_ProductName, tags)))
        {
            if(strcmp((STRPTR) ti->ti_Data, pd->pd_ProductStr))
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DA_Manufacturer, tags)))
        {
            if(strcmp((STRPTR) ti->ti_Data, pd->pd_MnfctrStr))
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DA_IDString, tags)))
        {
            if(strcmp((STRPTR) ti->ti_Data, pd->pd_IDString))
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DA_Binding, tags)))
        {
            if(ti->ti_Data != (IPTR) pd->pd_DevBinding)
            {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(DA_HubDevice, tags)))
        {
            if(ti->ti_Data != (IPTR) pd->pd_Hub)
            {
                takeit = FALSE;
            }
        }

        if(takeit)
        {
            return(pd);
        }
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Hardware *** */

/* /// "pFindHardware()" */
struct PsdHardware * pFindHardware(LIBBASETYPEPTR ps, STRPTR name, ULONG unit)
{
    struct PsdHardware *phw;
    Forbid();
    while(*name)
    {
        phw = (struct PsdHardware *) ps->ps_Hardware.lh_Head;
        while(phw->phw_Node.ln_Succ)
        {
            if((phw->phw_Unit == unit) && (!strcmp(phw->phw_DevName, name)))
            {
                Permit();
                return(phw);
            }
            phw = (struct PsdHardware *) phw->phw_Node.ln_Succ;
        }
        do
        {
            if((*name == '/') || (*name == ':'))
            {
                ++name;
                break;
            }
        } while(*(++name));
    }
    Permit();
    return(NULL);
}
/* \\\ */

/* /// "psdEnumerateHardware()" */
AROS_LH1(struct PsdDevice *, psdEnumerateHardware,
         AROS_LHA(struct PsdHardware *, phw, A0),
         LIBBASETYPEPTR, ps, 14, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdDevice *pd = NULL;
    struct PsdPipe *pp;
    struct MsgPort *mp;
    KPRINTF(2, ("psdEnumerateHardware(%p)\n", phw));

    if((mp = CreateMsgPort()))
    {
        Forbid();
        if((pd = psdAllocDevice(phw)))
        {
            Permit();
            if((pp = psdAllocPipe(pd, mp, NULL)))
            {
                //pp->pp_IOReq.iouh_Flags |= UHFF_NAKTIMEOUT;
                //pp->pp_IOReq.iouh_NakTimeout = 1000;
                pd->pd_Flags |= PDFF_CONNECTED;
                pp->pp_IOReq.iouh_Req.io_Command = UHCMD_USBRESET;
                psdDoPipe(pp, NULL, 0);
                pp->pp_IOReq.iouh_Req.io_Command = UHCMD_CONTROLXFER;
                psdDelayMS(100); // wait for root hub to settle
                if(psdEnumerateDevice(pp))
                {
                    KPRINTF(1, ("Enumeration finished!\n"));
                    psdAddErrorMsg0(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Root hub has been enumerated.");
                    psdFreePipe(pp);
                    DeleteMsgPort(mp);
                    phw->phw_RootDevice = pd;
                    psdSendEvent(EHMB_ADDDEVICE, pd, NULL);
                    return(pd);
                }
                psdFreePipe(pp);
            }
            pFreeBindings(ps, pd);
            pFreeDevice(ps, pd);
        } else {
            Permit();
        }
        DeleteMsgPort(mp);
    }
    psdAddErrorMsg0(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Root hub enumeration failed. Blame your hardware driver programmer.");
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdRemHardware()" */
AROS_LH1(void, psdRemHardware,
         AROS_LHA(struct PsdHardware *, phw, A0),
         LIBBASETYPEPTR, ps, 13, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdDevice *pd;
    ULONG cnt;

    KPRINTF(5, ("FreeHardware(%p)\n", phw));

    pd = (struct PsdDevice *) phw->phw_Devices.lh_Head;
    while(pd->pd_Node.ln_Succ)
    {
        pFreeBindings(ps, pd);
        pFreeDevice(ps, pd);
        psdSendEvent(EHMB_REMDEVICE, pd, NULL);
        pd = (struct PsdDevice *) phw->phw_Devices.lh_Head;
    }
    cnt = 0;
    pd = (struct PsdDevice *) phw->phw_DeadDevices.lh_Head;
    while(pd->pd_Node.ln_Succ)
    {
        if(pd->pd_UseCnt)
        {
            KPRINTF(20, ("Can't remove device, usecnt %ld\n", pd->pd_UseCnt));
            if(++cnt == 5)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "Can't remove device '%s', there are still %ld pipes in use...",
                               pd->pd_ProductStr, pd->pd_UseCnt);
            }
            if(++cnt == 30)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "Okay, going down with device '%s' anyway, maybe the driver crashed?",
                               pd->pd_ProductStr);
                pd->pd_UseCnt = 0;
                cnt--;
            } else {
                psdDelayMS(1000);
            }
        } else {
            pFreeDevice(ps, pd);
            //psdSendEvent(EHMB_REMDEVICE, pd, NULL);
        }
        pd = (struct PsdDevice *) phw->phw_DeadDevices.lh_Head;
    }

    Forbid();
    /* Note that the subtask unlinks the hardware! */
    phw->phw_ReadySignal = SIGB_SINGLE;
    phw->phw_ReadySigTask = FindTask(NULL);
    if(phw->phw_Task)
    {
        Signal(phw->phw_Task, SIGBREAKF_CTRL_C);
    }
    Permit();
    while(phw->phw_Task)
    {
        Wait(1L<<phw->phw_ReadySignal);
    }
    //FreeSignal(phw->phw_ReadySignal);
    KPRINTF(1, ("FreeHardware(%p) freevec name\n", phw));
    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "Removed hardware %s/%ld. Bye bye!",
                   phw->phw_DevName, phw->phw_Unit);
    psdFreeVec(phw->phw_DevName);
    psdFreeVec(phw->phw_ProductName);
    psdFreeVec(phw->phw_Manufacturer);
    psdFreeVec(phw->phw_Description);
    psdFreeVec(phw->phw_Copyright);
    psdFreeVec(phw);
    psdSendEvent(EHMB_REMHARDWARE, phw, NULL);
    KPRINTF(1, ("FreeHardware(%p) done\n", phw));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdAddHardware()" */
AROS_LH2(struct PsdHardware *,psdAddHardware,
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(ULONG, unit, D0),
         LIBBASETYPEPTR, ps, 12, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdHardware *phw;
    char buf[64];
    struct Task *tmptask;
    KPRINTF(5, ("psdAddHardware(%s, %ld)\n", name, unit));

    if((phw = psdAllocVec(sizeof(struct PsdHardware))))
    {
        NewList(&phw->phw_Devices);
        NewList(&phw->phw_DeadDevices);
        phw->phw_Unit = unit;
        phw->phw_Base = ps;
        if((phw->phw_Node.ln_Name = phw->phw_DevName = psdCopyStr(name)))
        {
            psdSafeRawDoFmt(buf, 64, "usbhw<%s/%ld>", name, unit);
            phw->phw_ReadySignal = SIGB_SINGLE;
            phw->phw_ReadySigTask = FindTask(NULL);
            SetSignal(0, SIGF_SINGLE); // clear single bit
            if((tmptask = psdSpawnSubTask(buf, pDeviceTask, phw)))
            {
                psdBorrowLocksWait(tmptask, 1UL<<phw->phw_ReadySignal);
                if(phw->phw_Task)
                {
                    phw->phw_ReadySigTask = NULL;
                    //FreeSignal(phw->phw_ReadySignal);
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "New hardware %s/%ld added (%s).",
                                   phw->phw_DevName,
                                   phw->phw_Unit,
                                   phw->phw_ProductName);
                    psdSendEvent(EHMB_ADDHARDWARE, phw, NULL);
                    return(phw);
                }
            }
            phw->phw_ReadySigTask = NULL;
            //FreeSignal(phw->phw_ReadySignal);
            psdFreeVec(phw->phw_DevName);
        }
        psdFreeVec(phw);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdCalculatePower()" */
AROS_LH1(void, psdCalculatePower,
         AROS_LHA(struct PsdHardware *, phw, A0),
         LIBBASETYPEPTR, ps, 78, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdDevice *roothub = NULL;
    struct PsdDevice *pd;

    psdLockReadPBase();
    /* goto root */
    pd = (struct PsdDevice *) phw->phw_Devices.lh_Head;
    while(pd->pd_Node.ln_Succ)
    {
        if(!pd->pd_Hub)
        {
            roothub = pd;
            break;
        }
        pd = (struct PsdDevice *) pd->pd_Node.ln_Succ;
    }
    if(!roothub)
    {
        psdUnlockPBase();
        return;
    }
    roothub->pd_PowerDrain = 0;
    roothub->pd_PowerSupply = 500;

    /* calculate drain */
    pPowerRecurseDrain(ps, roothub);

    /* calculate supply */
    pPowerRecurseSupply(ps, roothub);
    psdUnlockPBase();
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Pipes *** */

/* /// "psdAllocPipe()" */
AROS_LH3(struct PsdPipe *, psdAllocPipe,
         AROS_LHA(struct PsdDevice *, pd, A0),
         AROS_LHA(struct MsgPort *, mp, A1),
         AROS_LHA(struct PsdEndpoint *, pep, A2),
         LIBBASETYPEPTR, ps, 24, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdPipe *pp;
    struct PsdDevice *hubpd;

    KPRINTF(2, ("psdAllocPipe(%p, %p, %p)\n", pd, mp, pep));
    if(!mp || !pd)
    {
        return(NULL);
    }
    if(pep && (pep->pep_TransType == USEAF_ISOCHRONOUS) && (!(pep->pep_Interface->pif_Config->pc_Device->pd_Hardware->phw_Capabilities & UHCF_ISO)))
    {
        psdAddErrorMsg0(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Your HW controller driver does not support iso transfers. Sorry.");
        return(NULL);
    }

    if((pp = psdAllocVec(sizeof(struct PsdPipe))))
    {
        pp->pp_Msg.mn_Node.ln_Type = NT_FREEMSG;
        pp->pp_MsgPort = pp->pp_Msg.mn_ReplyPort = mp;
        pp->pp_Msg.mn_Length = sizeof(struct PsdPipe);
        pp->pp_Device = pd;
        pp->pp_Endpoint = pep;
        pp->pp_IOReq = *(pd->pd_Hardware->phw_RootIOReq);
        pp->pp_IOReq.iouh_DevAddr = pd->pd_DevAddr;
        if(pd->pd_Flags & PDFF_LOWSPEED)
        {
            pp->pp_IOReq.iouh_Flags |= UHFF_LOWSPEED;
        }
        if(pd->pd_Flags & PDFF_HIGHSPEED)
        {
            pp->pp_IOReq.iouh_Flags |= UHFF_HIGHSPEED;
            if(pep)
            {
                switch(pep->pep_NumTransMuFr)
                {
                    case 2:
                        pp->pp_IOReq.iouh_Flags |= UHFF_MULTI_2;
                        break;
                    case 3:
                        pp->pp_IOReq.iouh_Flags |= UHFF_MULTI_3;
                        break;

                    default:
                        pp->pp_IOReq.iouh_Flags |= UHFF_MULTI_1;
                }
            } else {
                pp->pp_IOReq.iouh_Flags |= UHFF_MULTI_1;
            }
        }
        #ifdef AROS_USB30_CODE
        if(pd->pd_Flags & PDFF_SUPERSPEED)
        {
            pp->pp_IOReq.iouh_Flags |= UHFF_SUPERSPEED;
        }
        #endif
        if(pd->pd_Flags & PDFF_NEEDSSPLIT)
        {
            /* USB1.1 device connected to a USB2.0 hub */
            pp->pp_IOReq.iouh_Flags |= UHFF_SPLITTRANS;
            hubpd = pd->pd_Hub;
            pp->pp_IOReq.iouh_SplitHubPort = pd->pd_HubPort;
            // find the root USB 2.0 hub in the tree
            while(hubpd && !(hubpd->pd_Flags & PDFF_HIGHSPEED))
            {
                pp->pp_IOReq.iouh_SplitHubPort = hubpd->pd_HubPort;
                hubpd = hubpd->pd_Hub;
            }
            if(!hubpd)
            {
                psdAddErrorMsg0(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Internal error obtaining split transaction hub!");
                psdFreeVec(pp);
                return(NULL);
            }
            pp->pp_IOReq.iouh_Flags |= (hubpd->pd_HubThinkTime<<UHFS_THINKTIME);
            pp->pp_IOReq.iouh_SplitHubAddr = hubpd->pd_DevAddr;
        }
        if(pep)
        {
            switch(pep->pep_TransType)
            {
                case USEAF_CONTROL:
                    pp->pp_IOReq.iouh_Req.io_Command = UHCMD_CONTROLXFER;
                    break;
                case USEAF_ISOCHRONOUS:
                    pp->pp_IOReq.iouh_Req.io_Command = UHCMD_ISOXFER;
                    break;
                case USEAF_BULK:
                    pp->pp_IOReq.iouh_Req.io_Command = UHCMD_BULKXFER;
                    break;
                case USEAF_INTERRUPT:
                    pp->pp_IOReq.iouh_Req.io_Command = UHCMD_INTXFER;
                    break;
                default:
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                   "AllocPipe(): Illegal transfer type %ld",
                                   pep->pep_TransType);
                    KPRINTF(20, ("Illegal transfer type for endpoint!"));
                    psdFreeVec(pp);
                    return(NULL);

            }
            pp->pp_IOReq.iouh_Dir = (pep->pep_Direction ? UHDIR_IN : UHDIR_OUT);
            pp->pp_IOReq.iouh_Endpoint = pep->pep_EPNum;
            pp->pp_IOReq.iouh_MaxPktSize = pep->pep_MaxPktSize;
            pp->pp_IOReq.iouh_Interval = pep->pep_Interval;
        } else {
            pp->pp_IOReq.iouh_Req.io_Command = UHCMD_CONTROLXFER;
            pp->pp_IOReq.iouh_Dir = UHDIR_SETUP;
            pp->pp_IOReq.iouh_Endpoint = 0;
            pp->pp_IOReq.iouh_MaxPktSize = pd->pd_MaxPktSize0;
        }
        pd->pd_UseCnt++;
        return(pp);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdFreePipe()" */
AROS_LH1(void, psdFreePipe,
         AROS_LHA(struct PsdPipe *, pp, A1),
         LIBBASETYPEPTR, ps, 25, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdDevice *pd;
    if(!pp)
    {
        return;
    }
    KPRINTF(2, ("psdFreePipe(%p)\n", pp));
    pd = pp->pp_Device;

    if(pp->pp_Msg.mn_Node.ln_Type == NT_MESSAGE)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "Tried to free pipe on %s that was still pending!", pd->pd_ProductStr);
        psdAbortPipe(pp);
        psdWaitPipe(pp);
    }

    if(!(--pd->pd_UseCnt) && (pd->pd_Flags & PDFF_DELEXPUNGE))
    {
        KPRINTF(20, ("Finally getting rid of device %s\n", pd->pd_ProductStr));
        pFreeDevice(ps, pd);
        //psdSendEvent(EHMB_REMDEVICE, pd, NULL);
    }

    psdFreeVec(pp);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdPipeSetup()" */
AROS_LH5(void, psdPipeSetup,
         AROS_LHA(struct PsdPipe *, pp, A1),
         AROS_LHA(UWORD, rt, D0),
         AROS_LHA(UWORD, rq, D1),
         AROS_LHA(UWORD, val, D2),
         AROS_LHA(UWORD, idx, D3),
         LIBBASETYPEPTR, ps, 26, psd)
{
    AROS_LIBFUNC_INIT
    struct UsbSetupData *usd = &pp->pp_IOReq.iouh_SetupData;

    KPRINTF(1, ("psdSetupPipe(%p, (%02lx %02lx %04lx %04lx))\n",
                pp, rt, rq, val, idx));
    usd->bmRequestType = rt;
    usd->bRequest = rq;
    usd->wValue = AROS_WORD2LE(val);
    usd->wIndex = AROS_WORD2LE(idx);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdDoPipe()" */
AROS_LH3(LONG, psdDoPipe,
         AROS_LHA(struct PsdPipe *, pp, A1),
         AROS_LHA(APTR, data, A0),
         AROS_LHA(ULONG, len, D0),
         LIBBASETYPEPTR, ps, 27, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdDevice *pd = pp->pp_Device;
    KPRINTF(200, ("psdDoPipe(%p, %p, %ld)\n", pp, data, len));

    if(pd->pd_Flags & PDFF_CONNECTED)
    {
        if(pd->pd_Flags & PDFF_SUSPENDED)
        {
            // make sure the device is up and running before trying to send a new pipe
            psdResumeDevice(pd);
        }

        pp->pp_IOReq.iouh_Data = data;
        pp->pp_IOReq.iouh_Length = len;
        if(!pp->pp_Endpoint)
        {
            pp->pp_IOReq.iouh_SetupData.wLength = AROS_WORD2LE(len);
        }
        PutMsg(&pd->pd_Hardware->phw_TaskMsgPort, &pp->pp_Msg);
        ++pd->pd_IOBusyCount;
        GetSysTime((APTR) &pd->pd_LastActivity);
        return(psdWaitPipe(pp));
    } else {
        psdDelayMS(50);
        pp->pp_IOReq.iouh_Actual = 0;
        pp->pp_Msg.mn_Node.ln_Type = NT_FREEMSG;
        return(pp->pp_IOReq.iouh_Req.io_Error = UHIOERR_TIMEOUT);
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdSendPipe()" */
AROS_LH3(void, psdSendPipe,
         AROS_LHA(struct PsdPipe *, pp, A1),
         AROS_LHA(APTR, data, A0),
         AROS_LHA(ULONG, len, D0),
         LIBBASETYPEPTR, ps, 28, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdDevice *pd = pp->pp_Device;
    KPRINTF(200, ("psdSendPipe(%p, %p, %ld)\n", pp, data, len));
    if(pd->pd_Flags & PDFF_CONNECTED)
    {
        if(pd->pd_Flags & PDFF_SUSPENDED)
        {
            // make sure the device is up and running before trying to send a new pipe
            psdResumeDevice(pd);
        }

        pp->pp_IOReq.iouh_Data = data;
        pp->pp_IOReq.iouh_Length = len;
        if(!pp->pp_Endpoint)
        {
            pp->pp_IOReq.iouh_SetupData.wLength = AROS_WORD2LE(len);
        }
        PutMsg(&pd->pd_Hardware->phw_TaskMsgPort, &pp->pp_Msg);
        GetSysTime((APTR) &pd->pd_LastActivity);
        ++pd->pd_IOBusyCount;
    } else {
        psdDelayMS(50);
        pp->pp_IOReq.iouh_Actual = 0;
        //pp->pp_Msg.mn_Node.ln_Type = NT_REPLYMSG;
        pp->pp_IOReq.iouh_Req.io_Error = UHIOERR_TIMEOUT;
        ReplyMsg(&pp->pp_Msg);
        ++pd->pd_IOBusyCount;
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdAbortPipe()" */
AROS_LH1(void, psdAbortPipe,
         AROS_LHA(struct PsdPipe *, pp, A1),
         LIBBASETYPEPTR, ps, 29, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdPipe *npp;

    KPRINTF(5, ("psdAbortPipe(%p)\n", pp));
    if(pp->pp_Msg.mn_Node.ln_Type != NT_MESSAGE)
    {
        KPRINTF(5, ("Nothing to abort %02lx\n", pp->pp_IOReq.iouh_Req.io_Message.mn_Node.ln_Type));
        return;
    }
    if((npp = psdAllocVec(sizeof(struct PsdPipe))))
    {
        //npp->pp_Msg.mn_Node.ln_Type = NT_MESSAGE;
        npp->pp_Device = pp->pp_Device;
        npp->pp_MsgPort = npp->pp_Msg.mn_ReplyPort = pp->pp_MsgPort;
        npp->pp_Msg.mn_Length = sizeof(struct PsdPipe);

        npp->pp_AbortPipe = pp;
        PutMsg(&pp->pp_Device->pd_Hardware->phw_TaskMsgPort, &npp->pp_Msg);
        psdWaitPipe(npp);
        psdFreeVec(npp);
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdWaitPipe()" */
AROS_LH1(LONG, psdWaitPipe,
         AROS_LHA(struct PsdPipe *, pp, A1),
         LIBBASETYPEPTR, ps, 30, psd)
{
    AROS_LIBFUNC_INIT
    ULONG sigs = 0;
    struct PsdDevice *pd = pp->pp_Device;
    LONG ioerr;
    KPRINTF(5, ("psdWaitPipe(%p)\n", pp));
    while(pp->pp_Msg.mn_Node.ln_Type == NT_MESSAGE)
    {
        KPRINTF(5, ("ln_Type = %02lx\n", pp->pp_Msg.mn_Node.ln_Type));
        sigs |= Wait(1L<<pp->pp_MsgPort->mp_SigBit);
        KPRINTF(5, ("sigs = %p\n", sigs));
    }
#if 1 // broken?
    Forbid();
    if(pp->pp_Msg.mn_Node.ln_Type == NT_REPLYMSG)
    {
        pp->pp_Msg.mn_Node.ln_Type = NT_FREEMSG;
        Remove(&pp->pp_Msg.mn_Node);
    }
    //if(pp->pp_MsgPort->mp_MsgList.lh_Head->ln_Succ)
    {
        // avoid signals getting lost for other messages arriving.
        SetSignal(sigs, sigs);
    }
    Permit();
#else
    Forbid();
    Remove(&pp->pp_Msg.mn_Node);
    Permit();
#endif
    ioerr = pp->pp_IOReq.iouh_Req.io_Error;
    switch(ioerr)
    {
        case UHIOERR_TIMEOUT:
           pd->pd_DeadCount++;
           // fall through
        case UHIOERR_NAKTIMEOUT:
           pd->pd_DeadCount++;
        case UHIOERR_CRCERROR:
           pd->pd_DeadCount++;
           break;
        case UHIOERR_RUNTPACKET:
        default:
           if(pd->pd_DeadCount)
           {
               pd->pd_DeadCount >>= 1;
               /*psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                              "Device %s starts recovering with %s (%ld)!",
                              pd->pd_ProductStr,
                              psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);*/
           }
    }
    KPRINTF(200, ("psdWaitPipe(%p)=%ld\n", pp, ioerr));
    --pd->pd_IOBusyCount;
    GetSysTime((APTR) &pd->pd_LastActivity);

    if((pd->pd_DeadCount > 19) || ((pd->pd_DeadCount > 14) && (pd->pd_Flags & (PDFF_HASDEVADDR|PDFF_HASDEVDESC))))
    {
        if(!(pd->pd_Flags & PDFF_DEAD))
        {
            pd->pd_Flags |= PDFF_DEAD;
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                            "Device %s probably dropped dead!", pd->pd_ProductStr);

            psdSendEvent(EHMB_DEVICEDEAD, pp->pp_Device, NULL);
        }
    } else {
        if((!pd->pd_DeadCount) && ((pd->pd_Flags & (PDFF_DEAD|PDFF_CONNECTED)) == (PDFF_DEAD|PDFF_CONNECTED)))
        {
            pd->pd_Flags &= ~PDFF_DEAD;
            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                            "Uuuhuuuhh, the zombie %s returned from the dead!", pd->pd_ProductStr);
        }
    }
    return(ioerr);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdCheckPipe()" */
AROS_LH1(struct PsdPipe *, psdCheckPipe,
         AROS_LHA(struct PsdPipe *, pp, A1),
         LIBBASETYPEPTR, ps, 71, psd)
{
    AROS_LIBFUNC_INIT
    KPRINTF(5, ("psdCheckPipe(%p)\n", pp));
    if(pp->pp_Msg.mn_Node.ln_Type == NT_MESSAGE)
    {
        return(NULL);
    }
    return(pp);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdGetPipeActual()" */
AROS_LH1(ULONG, psdGetPipeActual,
         AROS_LHA(struct PsdPipe *, pp, A1),
         LIBBASETYPEPTR, ps, 31, psd)
{
    AROS_LIBFUNC_INIT
    KPRINTF(1, ("psdGetPipeActual(%p)\n", pp));
    return(pp->pp_IOReq.iouh_Actual);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdGetPipeError()" */
AROS_LH1(LONG, psdGetPipeError,
         AROS_LHA(struct PsdPipe *, pp, A1),
         LIBBASETYPEPTR, ps, 32, psd)
{
    AROS_LIBFUNC_INIT
    KPRINTF(1, ("psdGetPipeError(%p)\n", pp));
    return((LONG) pp->pp_IOReq.iouh_Req.io_Error);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Streams *** */

/* /// "psdOpenStreamA()" */
AROS_LH2(struct PsdPipeStream *, psdOpenStreamA,
         AROS_LHA(struct PsdEndpoint *, pep, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, ps, 72, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdPipeStream *pps;

    KPRINTF(2, ("psdOpenStream(%p, %p)\n", pep, tags));
    if(!pep)
    {
        return(NULL);
    }
    if((pps = psdAllocVec(sizeof(struct PsdPipeStream))))
    {
        pps->pps_Device = pep->pep_Interface->pif_Config->pc_Device;
        pps->pps_Endpoint = pep;
        NewList(&pps->pps_FreePipes);
        NewList(&pps->pps_ReadyPipes);
        InitSemaphore(&pps->pps_AccessLock);
        pps->pps_NakTimeoutTime = 5000;
        if(pep->pep_Direction)
        {
            /* Defaults for IN */
            pps->pps_NumPipes = 4;
            pps->pps_Flags = PSFF_READAHEAD|PSFF_BUFFERREAD|PSFF_ALLOWRUNT;
            pps->pps_BufferSize = 32*pps->pps_Endpoint->pep_MaxPktSize;
        } else {
            /* Defaults for OUT */
            pps->pps_NumPipes = 4;
            pps->pps_Flags = PSFF_NOSHORTPKT;
            pps->pps_BufferSize = 4*pps->pps_Endpoint->pep_MaxPktSize;
        }

        psdSetAttrsA(PGA_PIPESTREAM, pps, tags);
        if(!pps->pps_Pipes)
        {
            psdCloseStream(pps);
            pps = NULL;
        }
        return(pps);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdCloseStream()" */
AROS_LH1(void, psdCloseStream,
         AROS_LHA(struct PsdPipeStream *, pps, A1),
         LIBBASETYPEPTR, ps, 73, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdPipe *pp;
    ULONG cnt;

    KPRINTF(2, ("psdCloseStream(%p)\n", pps));
    if(!pps)
    {
        return;
    }
    psdStreamFlush(pps);
    ObtainSemaphore(&pps->pps_AccessLock);
    if(pps->pps_Pipes)
    {
        for(cnt = 0; cnt < pps->pps_NumPipes; cnt++)
        {
            pp = pps->pps_Pipes[cnt];
            //if(pp->pp_IOReq.iouh_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE)
            {
                KPRINTF(1, ("Abort %ld\n", cnt));
                psdAbortPipe(pp);
                KPRINTF(1, ("Wait %ld\n", cnt));
                psdWaitPipe(pp);
            }
            KPRINTF(1, ("Free %ld\n", cnt));
            psdFreePipe(pp);
        }
        psdFreeVec(pps->pps_Pipes);
        if((pps->pps_Flags & PSFF_OWNMSGPORT) && pps->pps_MsgPort)
        {
            DeleteMsgPort(pps->pps_MsgPort);
        }
    }
    psdFreeVec(pps->pps_Buffer);
    ReleaseSemaphore(&pps->pps_AccessLock);
    psdFreeVec(pps);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdStreamRead()" */
AROS_LH3(LONG, psdStreamRead,
         AROS_LHA(struct PsdPipeStream *, pps, A1),
         AROS_LHA(UBYTE *, buffer, A0),
         AROS_LHA(LONG, length, D0),
         LIBBASETYPEPTR, ps, 74, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdPipe *pp;
    ULONG cnt;
    LONG ioerr;
    LONG remlen;
    BOOL term = FALSE;
    LONG actual = 0;
    ULONG sigmask;

    UBYTE *bufptr;
    UBYTE *srcptr;
    UBYTE *tarrptr;
    ULONG tcnt;
    UBYTE cchar;

    KPRINTF(2, ("psdStreamRead(%p, %p, %ld)\n", pps, buffer, length));
    if(!pps)
    {
        return(-1);
    }
    ObtainSemaphore(&pps->pps_AccessLock);
    KPRINTF(2, ("Sema\n"));
    pps->pps_Error = 0;
    if((!pps->pps_Pipes) || (!pps->pps_Endpoint->pep_Direction))
    {
        KPRINTF(2, ("Wrong direction!\n"));
        pps->pps_Error = UHIOERR_BADPARAMS;
        ReleaseSemaphore(&pps->pps_AccessLock);
        return(-1);
    }
    if(!(pps->pps_Flags & PSFF_ASYNCIO))
    {
        if(pps->pps_Flags & PSFF_BUFFERREAD)
        {
            /* buffered reading */
            do
            {
                /* check for incoming packets */
                while((pp = (struct PsdPipe *) GetMsg(pps->pps_MsgPort)))
                {
                    KPRINTF(1, ("PktBack(%p, %p, %ld/%ld)=%ld\n",
                            pp, pp->pp_IOReq.iouh_Data, pp->pp_IOReq.iouh_Actual,
                            pp->pp_IOReq.iouh_Length, pp->pp_IOReq.iouh_Req.io_Error));

                    pps->pps_ReqBytes -= pp->pp_IOReq.iouh_Length;
                    ioerr = pp->pp_IOReq.iouh_Req.io_Error;
                    if((ioerr == UHIOERR_NAKTIMEOUT) && pp->pp_IOReq.iouh_Actual)
                    {
                        ioerr = 0;
                    }

                    if(ioerr)
                    {
                        pps->pps_Error = ioerr;
                        term = TRUE;
                        if(ioerr != UHIOERR_TIMEOUT)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) "StreamRead",
                                           "Packet(%s) failed: %s (%ld)", (STRPTR) "b",
                                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                        }
                        /* stop automatic queueing */
                        pps->pps_Flags &= ~PSFF_READAHEAD;
                        AddTail(&pps->pps_FreePipes, &pp->pp_Msg.mn_Node);
                    } else {
                        /* successfully received packet */
                        pps->pps_BytesPending += pp->pp_IOReq.iouh_Actual;
                        AddTail(&pps->pps_ReadyPipes, &pp->pp_Msg.mn_Node);
                    }
                }
                if(length == -1) /* get all that's there (STRONGLY DISCOURAGED! Might cause buffer overflows) */
                {
                    length = pps->pps_BytesPending;
                }
                /* check for buffered data */
                while(length && pps->pps_BytesPending)
                {
                    pp = (struct PsdPipe *) pps->pps_ReadyPipes.lh_Head;
                    if(!pp->pp_Msg.mn_Node.ln_Succ) /* debug */
                    {
                        psdAddErrorMsg0(RETURN_FAIL, (STRPTR) "StreamRead", "Readqueue empty!");
                        ReleaseSemaphore(&pps->pps_AccessLock);
                        return(-1);
                    }
                    if(pp->pp_IOReq.iouh_Actual < pps->pps_Offset)
                    {
                        psdAddErrorMsg(RETURN_FAIL, (STRPTR) "StreamRead",
                                       "Actual %ld < offset %ld!", pp->pp_IOReq.iouh_Actual, pps->pps_Offset);
                        ReleaseSemaphore(&pps->pps_AccessLock);
                        return(-1);
                    }
                    remlen = pp->pp_IOReq.iouh_Actual - pps->pps_Offset;
                    if(length < remlen)
                    {
                        KPRINTF(1, ("PktBit(%p, %p, %ld)\n", pp, buffer, length));
                        remlen = length;
                    } else {
                        KPRINTF(1, ("PktRem(%p, %p, %ld)\n", pp, buffer, remlen));
                    }
                    /* copy packet */
                    if(pp->pp_Flags & PFF_INPLACE)
                    {
                        KPRINTF(1, ("PktRemIP(%p, %p, %ld)\n", pp, buffer, remlen));
                    } else {
                        if(pps->pps_TermArray)
                        {
                            /* EOF Mode */
                            KPRINTF(1, ("PktCpyEOF(%p, %p, %ld)\n", pp, buffer, remlen));
                            bufptr = buffer;
                            srcptr = &(((UBYTE *) pp->pp_IOReq.iouh_Data)[pps->pps_Offset]);
                            tarrptr = pps->pps_TermArray;
                            cnt = remlen;
                            remlen = 0;
                            if(cnt)
                            {
                                do
                                {
                                    cchar = *bufptr++ = *srcptr++;
                                    remlen++;
                                    tcnt = 0;
                                    do
                                    {
                                        if(cchar < tarrptr[tcnt])
                                        {
                                            break;
                                        }
                                        else if(cchar == tarrptr[tcnt])
                                        {
                                            cnt = 1;
                                            term = TRUE;
                                            KPRINTF(2, ("EOF char %02lx found, length = %ld\n", cchar, remlen));
                                            break;
                                        }
                                        if(tcnt < 7)
                                        {
                                            if(tarrptr[tcnt] == tarrptr[tcnt+1])
                                            {
                                                break;
                                            }
                                        }
                                    } while(++tcnt < 8);
                                } while(--cnt);
                            }
                        } else {
                            KPRINTF(1, ("PktCpy(%p, %p, %ld)\n", pp, buffer, remlen));
                            /* quick non-eof mode */
                            CopyMem(&(((UBYTE *) pp->pp_IOReq.iouh_Data)[pps->pps_Offset]), buffer, remlen);
                        }
                    }
                    actual += remlen;
                    length -= remlen;
                    buffer += remlen;
                    pps->pps_BytesPending -= remlen;
                    pps->pps_Offset += remlen;
                    /* end of packet reached? */
                    if(pps->pps_Offset == pp->pp_IOReq.iouh_Actual)
                    {
                        pps->pps_Offset = 0;
                        Remove(&pp->pp_Msg.mn_Node);
                        AddTail(&pps->pps_FreePipes, &pp->pp_Msg.mn_Node);
                        /* check for short packet */
                        if((pps->pps_Flags & PSFF_SHORTTERM) && (pp->pp_IOReq.iouh_Actual % pp->pp_IOReq.iouh_MaxPktSize))
                        {
                            term = TRUE;
                        }
                    }
                    if(term)
                    {
                        break;
                    }
                }
                /* start sending out requests */
                remlen = length;
                pp = (struct PsdPipe *) pps->pps_FreePipes.lh_Head;
                if(!(pps->pps_BytesPending || pps->pps_ReqBytes || pps->pps_TermArray || (length < pps->pps_BufferSize)))
                {
                    /* faster non-buffered mode */
                    if(pp->pp_Msg.mn_Node.ln_Succ)
                    {
                        pp->pp_Flags |= PFF_INPLACE;
                        Remove(&pp->pp_Msg.mn_Node);
                        remlen = length - (length % pp->pp_IOReq.iouh_MaxPktSize);
                        KPRINTF(1, ("OutFast(%p, %p, %ld/%ld)\n",
                                pp, buffer, remlen, length));
                        psdSendPipe(pp, buffer, remlen);
                        pps->pps_ReqBytes += remlen;
                        pp = (struct PsdPipe *) pps->pps_FreePipes.lh_Head;
                    }
                }
                /* slower buffered mode */
                while(pp->pp_Msg.mn_Node.ln_Succ && ((remlen > pps->pps_ReqBytes) || (pps->pps_Flags & PSFF_READAHEAD)))
                {
                    pp->pp_Flags &= ~PFF_INPLACE;
                    Remove(&pp->pp_Msg.mn_Node);
                    if((pps->pps_Flags & PSFF_READAHEAD) || (remlen % pp->pp_IOReq.iouh_MaxPktSize))
                    {
                        KPRINTF(1, ("OutSlow(%p, %p, %ld)\n",
                                pp, &pps->pps_Buffer[pp->pp_Num * pps->pps_BufferSize], pps->pps_BufferSize));
                        remlen = pps->pps_BufferSize;
                    } else {
                        KPRINTF(1, ("OutExact(%p, %p, %ld)\n",
                                pp, &pps->pps_Buffer[pp->pp_Num * pps->pps_BufferSize], remlen));
                    }
                    psdSendPipe(pp, &pps->pps_Buffer[pp->pp_Num * pps->pps_BufferSize], remlen);
                    pps->pps_ReqBytes += remlen;
                    pp = (struct PsdPipe *) pps->pps_FreePipes.lh_Head;
                }
                if((!length) || (pps->pps_Flags & PSFF_DONOTWAIT))
                {
                    term = TRUE;
                }
                if(!term)
                {
                    sigmask = (1UL<<pps->pps_MsgPort->mp_SigBit)|pps->pps_AbortSigMask;
                    KPRINTF(1, ("WaitPort (%p)\n", sigmask));
                    sigmask = Wait(sigmask);
                    KPRINTF(1, ("Wait back (%p)\n", sigmask));
                    if(sigmask & pps->pps_AbortSigMask)
                    {
                        KPRINTF(1, ("Aborted!\n"));
                        term = TRUE;
                        Signal(FindTask(NULL), pps->pps_AbortSigMask & sigmask);
                    }
                }
            } while(!term);
        } else {
            /* plain reading (might lose data) */
            if(pps->pps_TermArray || (pps->pps_Flags & PSFF_READAHEAD))
            {
                psdAddErrorMsg0(RETURN_WARN, (STRPTR) "StreamRead", "This mode combination for the stream is not supported!");
            }
            /* start sending out requests */
            pp = (struct PsdPipe *) pps->pps_FreePipes.lh_Head;
            if(pp->pp_Msg.mn_Node.ln_Succ && length)
            {
                ioerr = psdDoPipe(pp, buffer, length);
                if(ioerr)
                {
                    pps->pps_Error = ioerr;
                    if(ioerr != UHIOERR_TIMEOUT)
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) "StreamRead",
                                       "Packet(%s) failed: %s (%ld)", (STRPTR) "u",
                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    }
                }
                actual = pp->pp_IOReq.iouh_Actual;
            }
        }
    }
    ReleaseSemaphore(&pps->pps_AccessLock);
    return(actual);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdStreamWrite()" */
AROS_LH3(LONG, psdStreamWrite,
         AROS_LHA(struct PsdPipeStream *, pps, A1),
         AROS_LHA(UBYTE *, buffer, A0),
         AROS_LHA(LONG, length, D0),
         LIBBASETYPEPTR, ps, 75, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdPipe *pp;
    struct PsdPipe *newpp;
    ULONG cnt;
    LONG ioerr;
    LONG remlen;
    LONG actual = 0;
    ULONG sigmask;

    UBYTE *bufptr;
    UBYTE *srcptr;
    UBYTE *tarrptr;
    ULONG tcnt;
    UBYTE cchar;

    KPRINTF(2, ("psdStreamWrite(%p, %p, %ld)\n", pps, buffer, length));
    if(!pps)
    {
        return(-1);
    }
    ObtainSemaphore(&pps->pps_AccessLock);
    pps->pps_Error = 0;
    if((!pps->pps_Pipes) || pps->pps_Endpoint->pep_Direction)
    {
        KPRINTF(2, ("Wrong direction!\n"));
        pps->pps_Error = UHIOERR_BADPARAMS;
        ReleaseSemaphore(&pps->pps_AccessLock);
        return(-1);
    }
    if(length == -1) /* null terminated string mode */
    {
        KPRINTF(2, ("EOL mode!\n"));
        length = strlen(buffer);
    }
    if((tarrptr = pps->pps_TermArray)) /* EOF Mode */
    {
       KPRINTF(1, ("EOFSearch(%p, %ld)\n", buffer, length));
       srcptr = buffer;
       cnt = length;
       length = 0;
       if(cnt)
       {
           do
           {
               cchar = *srcptr++;
               length++;
               tcnt = 0;
               do
               {
                   if(cchar < tarrptr[tcnt])
                   {
                       break;
                   }
                   else if(cchar == tarrptr[tcnt])
                   {
                       cnt = 1;
                       KPRINTF(2, ("EOF char %02lx found, length = %ld\n", cchar, length));
                       break;
                   }
                   if(tcnt)
                   {
                       if(tarrptr[tcnt] == tarrptr[tcnt+1])
                       {
                           break;
                       }
                   }
               } while(++tcnt < 8);
           } while(--cnt);
       }
    }
    if(!(pps->pps_Flags & PSFF_ASYNCIO))
    {
        pp = (struct PsdPipe *) pps->pps_FreePipes.lh_Head;
        if(pp->pp_Msg.mn_Node.ln_Succ && length)
        {
            if(pps->pps_Flags & PSFF_BUFFERWRITE)
            {
                /* buffered writing */
                if(pps->pps_BytesPending)
                {
                    remlen = pps->pps_BytesPending % pp->pp_IOReq.iouh_MaxPktSize;
                    /* align to packet boundary */
                    if(remlen + length >= pp->pp_IOReq.iouh_MaxPktSize)
                    {
                        /* new data crosses at least on packet size */
                        if(pps->pps_BytesPending + length <= pps->pps_BufferSize)
                        {
                            /* copy everything up to the last (!) boundary */
                            remlen = pps->pps_BytesPending + length;
                            remlen = remlen - (remlen % pp->pp_IOReq.iouh_MaxPktSize);
                            remlen -= pps->pps_BytesPending;
                            KPRINTF(1, ("PendOptCpy(%p, %ld+%ld/%ld)\n", buffer, pps->pps_BytesPending, remlen, length));
                        } else {
                            /* just calculate amount to copy to the next boundary */
                            remlen = pp->pp_IOReq.iouh_MaxPktSize - remlen;
                            KPRINTF(1, ("PendOneCpy(%p, %ld+%ld/%ld)\n", buffer, pps->pps_BytesPending, remlen, length));
                        }
                        CopyMem(buffer, &pps->pps_Buffer[pps->pps_BytesPending], remlen);
                        pps->pps_BytesPending += remlen;
                        actual += remlen;
                        buffer += remlen;
                        length -= remlen;
                    } else {
                        KPRINTF(1, ("PendAdd(%p, %ld+%ld)\n", buffer, pps->pps_BytesPending, length));
                        /* only a few bytes, see if we can fit them */
                        CopyMem(buffer, &pps->pps_Buffer[pps->pps_BytesPending], length);
                        pps->pps_BytesPending += length;
                        actual += length;
                        //buffer += length; /* not needed */
                        length = 0;
                    }
                    /* flush some buffers */
                    if((length >= pp->pp_IOReq.iouh_MaxPktSize) ||
                       ((pps->pps_BytesPending >= (pps->pps_BufferSize>>1)) && (pps->pps_BytesPending >= pp->pp_IOReq.iouh_MaxPktSize)))
                    {
                        remlen = pps->pps_BytesPending - (pps->pps_BytesPending % pp->pp_IOReq.iouh_MaxPktSize);
                        KPRINTF(1, ("PendFlush(%ld/%ld)\n", remlen, pps->pps_BytesPending));
                        Remove(&pp->pp_Msg.mn_Node);
                        psdSendPipe(pp, pps->pps_Buffer, remlen);
                        pps->pps_ActivePipe = pp;
                        while(!(newpp = (struct PsdPipe *) GetMsg(pps->pps_MsgPort)))
                        {
                            sigmask = (1UL<<pps->pps_MsgPort->mp_SigBit)|pps->pps_AbortSigMask;
                            sigmask = Wait(sigmask);
                            if(sigmask & pps->pps_AbortSigMask)
                            {
                                KPRINTF(1, ("Kill signal detected!\n"));
                                Signal(FindTask(NULL), pps->pps_AbortSigMask & sigmask);
                                break;
                            }
                        }
                        if(!newpp)
                        {
                            psdAbortPipe(pp);
                        }
                        ioerr = psdWaitPipe(pp);
                        pps->pps_ActivePipe = NULL;
                        AddTail(&pps->pps_FreePipes, &pp->pp_Msg.mn_Node);

                        /* move end of buffer */
                        cnt = pps->pps_BytesPending;
                        tcnt = pp->pp_IOReq.iouh_Actual;
                        pps->pps_BytesPending -= tcnt;
                        bufptr = pps->pps_Buffer;
                        srcptr = bufptr + tcnt;
                        cnt -= tcnt;
                        if(cnt)
                        {
                            do
                            {
                               *bufptr++ = *srcptr++;
                            } while(--cnt);
                        }
                        if(ioerr)
                        {
                            pps->pps_Error = ioerr;
                            if(ioerr != UHIOERR_TIMEOUT)
                            {
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) "StreamWrite",
                                               "Packet(%s) failed: %s (%ld)", (STRPTR) "b",
                                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            }
                            ReleaseSemaphore(&pps->pps_AccessLock);
                            return(actual);
                        }
                    }
                }
                /* send out large chunk (avoid copying) */
                if(length >= pp->pp_IOReq.iouh_MaxPktSize)
                {
                    remlen = length - (length % pp->pp_IOReq.iouh_MaxPktSize);
                    KPRINTF(1, ("BulkFlush(%p, %ld/%ld)\n", buffer, remlen, length));
                    Remove(&pp->pp_Msg.mn_Node);
                    psdSendPipe(pp, buffer, remlen);
                    pps->pps_ActivePipe = pp;
                    while(!(newpp = (struct PsdPipe *) GetMsg(pps->pps_MsgPort)))
                    {
                        sigmask = (1UL<<pps->pps_MsgPort->mp_SigBit)|pps->pps_AbortSigMask;
                        sigmask = Wait(sigmask);
                        if(sigmask & pps->pps_AbortSigMask)
                        {
                            KPRINTF(1, ("Kill signal detected!\n"));
                            Signal(FindTask(NULL), pps->pps_AbortSigMask & sigmask);
                            break;
                        }
                    }
                    if(!newpp)
                    {
                        psdAbortPipe(pp);
                    }
                    ioerr = psdWaitPipe(pp);
                    pps->pps_ActivePipe = NULL;
                    AddTail(&pps->pps_FreePipes, &pp->pp_Msg.mn_Node);

                    actual += pp->pp_IOReq.iouh_Actual;
                    buffer += pp->pp_IOReq.iouh_Actual;
                    length -= pp->pp_IOReq.iouh_Actual;
                    if(ioerr)
                    {
                        pps->pps_Error = ioerr;
                        if(ioerr != UHIOERR_TIMEOUT)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) "StreamWrite",
                                           "Packet(%s) failed: %s (%ld)", (STRPTR) "c",
                                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                        }
                        ReleaseSemaphore(&pps->pps_AccessLock);
                        return(actual);
                    }
                }
                /* buffer remaining bytes */
                if(length)
                {
                    KPRINTF(1, ("BufAdd(%p, %ld)\n", buffer, length));
                    /* only a few bytes left, so lets buffer them */
                    CopyMem(buffer, &pps->pps_Buffer[pps->pps_BytesPending], length);
                    pps->pps_BytesPending += length;
                    actual += length;
                }
            } else {
                /* plain writing */
                /* start sending out requests */
                KPRINTF(1, ("PlainWrite(%p, %ld)\n", buffer, length));
                Remove(&pp->pp_Msg.mn_Node);
                psdSendPipe(pp, buffer, length);
                pps->pps_ActivePipe = pp;
                while(!(newpp = (struct PsdPipe *) GetMsg(pps->pps_MsgPort)))
                {
                    sigmask = (1UL<<pps->pps_MsgPort->mp_SigBit)|pps->pps_AbortSigMask;
                    sigmask = Wait(sigmask);
                    if(sigmask & pps->pps_AbortSigMask)
                    {
                        KPRINTF(1, ("Kill signal detected!\n"));
                        Signal(FindTask(NULL), pps->pps_AbortSigMask & sigmask);
                        break;
                    }
                }
                if(!newpp)
                {
                    psdAbortPipe(pp);
                }
                ioerr = psdWaitPipe(pp);
                pps->pps_ActivePipe = NULL;
                AddTail(&pps->pps_FreePipes, &pp->pp_Msg.mn_Node);
                if(ioerr)
                {
                    pps->pps_Error = ioerr;
                    if(ioerr != UHIOERR_TIMEOUT)
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) "StreamWrite",
                                       "Packet(%s) failed: %s (%ld)", (STRPTR) "u",
                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    }
                }
                actual = pp->pp_IOReq.iouh_Actual;
            }
        } else {
            KPRINTF(2, ("No free pipe!\n"));
        }
    }
    ReleaseSemaphore(&pps->pps_AccessLock);
    return(actual);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdStreamFlush()" */
AROS_LH1(LONG, psdStreamFlush,
         AROS_LHA(struct PsdPipeStream *, pps, A1),
         LIBBASETYPEPTR, ps, 76, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdPipe *pp;
    ULONG cnt;
    LONG ioerr;
    LONG ret = FALSE;

    KPRINTF(2, ("psdStreamFlush(%p)\n", pps));
    if(!pps)
    {
        return(-1);
    }
    ObtainSemaphore(&pps->pps_AccessLock);
    pps->pps_Error = 0;
    if(pps->pps_Endpoint->pep_Direction)
    {
        /* IN */
        KPRINTF(2, ("Flushing in...\n"));
        for(cnt = 0; cnt < pps->pps_NumPipes; cnt++)
        {
            psdAbortPipe(pps->pps_Pipes[cnt]);
        }
        for(cnt = 0; cnt < pps->pps_NumPipes; cnt++)
        {
            psdWaitPipe(pps->pps_Pipes[cnt]);
        }
        pp = (struct PsdPipe *) pps->pps_ReadyPipes.lh_Head;
        while(pp->pp_Msg.mn_Node.ln_Succ)
        {
            Remove(&pp->pp_Msg.mn_Node);
            AddTail(&pps->pps_FreePipes, &pp->pp_Msg.mn_Node);
            pp = (struct PsdPipe *) pps->pps_ReadyPipes.lh_Head;
        }
        while((pp = (struct PsdPipe *) GetMsg(pps->pps_MsgPort)))
        {
            AddTail(&pps->pps_FreePipes, &pp->pp_Msg.mn_Node);
        }
        pps->pps_ReqBytes = 0;
        pps->pps_BytesPending = 0;
        pps->pps_Offset = 0;
        ret = TRUE;
    } else {
        /* OUT */
        pp = (struct PsdPipe *) pps->pps_FreePipes.lh_Head;
        if(pp->pp_Msg.mn_Node.ln_Succ)
        {
            ret = TRUE;
            if(pps->pps_BytesPending)
            {
                KPRINTF(2, ("Flushing out %ld...\n", pps->pps_BytesPending));
                Remove(&pp->pp_Msg.mn_Node);
                ioerr = psdDoPipe(pp, pps->pps_Buffer, pps->pps_BytesPending);
                AddTail(&pps->pps_FreePipes, &pp->pp_Msg.mn_Node);
                pps->pps_BytesPending = 0;
                if(ioerr)
                {
                    pps->pps_Error = ioerr;
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) "StreamFlush",
                                   "Packet(%s) failed: %s (%ld)", (STRPTR) "f",
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    ret = FALSE;
                }
            } else {
                KPRINTF(2, ("Nothing to flush\n"));
            }
        }
    }
    ReleaseSemaphore(&pps->pps_AccessLock);
    return(ret);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdGetStreamError()" */
AROS_LH1(LONG, psdGetStreamError,
         AROS_LHA(struct PsdPipeStream *, pps, A1),
         LIBBASETYPEPTR, ps, 77, psd)
{
    AROS_LIBFUNC_INIT
    KPRINTF(1, ("psdGetStreamError(%p)\n", pps));
    if(pps)
    {
        return((LONG) pps->pps_Error);
    } else {
        return(-1);
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Realtime Iso */

/* /// "psdAllocRTIsoHandler()" */
AROS_LH2(struct PsdRTIsoHandler *, psdAllocRTIsoHandlerA,
         AROS_LHA(struct PsdEndpoint *, pep, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, ps, 93, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdRTIsoHandler *prt;
    struct PsdPipe *pp;
    LONG ioerr;

    KPRINTF(2, ("psdAllocRTIso(%p, %p)\n", pep, tags));
    if(!pep)
    {
        return(NULL);
    }
    if(pep->pep_TransType != USEAF_ISOCHRONOUS)
    {
        return(NULL);
    }
    if(!(pep->pep_Interface->pif_Config->pc_Device->pd_Hardware->phw_Capabilities & UHCF_RT_ISO))
    {
        psdAddErrorMsg0(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Your HW controller driver does not support realtime iso transfers. Sorry.");
        return(NULL);
    }
    if((prt = psdAllocVec(sizeof(struct PsdRTIsoHandler))))
    {
        prt->prt_Device = pep->pep_Interface->pif_Config->pc_Device;
        prt->prt_Endpoint = pep;
        prt->prt_RTIso.urti_OutPrefetch = 2048;
        if((pp = prt->prt_Pipe = psdAllocPipe(prt->prt_Device, (struct MsgPort *) 0xffffffff, pep)))
        {
            pp->pp_MsgPort = pp->pp_Msg.mn_ReplyPort = NULL;
            psdSetAttrsA(PGA_RTISO, prt, tags);
            pp->pp_IOReq.iouh_Req.io_Command = UHCMD_ADDISOHANDLER;
            pp->pp_IOReq.iouh_Data = &prt->prt_RTIso;
            // hardware must support quick IO for this to work!
            ioerr = DoIO((struct IORequest *) &pp->pp_IOReq);
            if(!ioerr)
            {
                Forbid();
                AddTail(&prt->prt_Device->pd_RTIsoHandlers, &prt->prt_Node);
                Permit();
                return(prt);
            } else {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "Adding RT Iso Handler failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            psdFreePipe(prt->prt_Pipe);
        }
        psdFreeVec(prt);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdFreeRTIsoHandler()" */
AROS_LH1(void, psdFreeRTIsoHandler,
         AROS_LHA(struct PsdRTIsoHandler *, prt, A1),
         LIBBASETYPEPTR, ps, 94, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdPipe *pp;

    if(!prt)
    {
        return;
    }
    Forbid();
    Remove(&prt->prt_Node);
    Permit();
    pp = prt->prt_Pipe;
    pp->pp_IOReq.iouh_Req.io_Command = UHCMD_REMISOHANDLER;
    DoIO((struct IORequest *) &pp->pp_IOReq);
    psdFreePipe(pp);
    psdFreeVec(prt);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdStartRTIso()" */
AROS_LH1(LONG, psdStartRTIso,
         AROS_LHA(struct PsdRTIsoHandler *, prt, A1),
         LIBBASETYPEPTR, ps, 95, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdPipe *pp;
    LONG ioerr;

    if(!prt)
    {
        return UHIOERR_BADPARAMS;
    }
    pp = prt->prt_Pipe;
    if(pp->pp_Device->pd_Flags & PDFF_SUSPENDED)
    {
        // make sure the device is up and running before trying to send a new pipe
        psdResumeDevice(pp->pp_Device);
    }
    pp->pp_IOReq.iouh_Req.io_Command = UHCMD_STARTRTISO;
    ioerr = DoIO((struct IORequest *) &pp->pp_IOReq);
    if(!ioerr)
    {
        ++pp->pp_Device->pd_IOBusyCount;
    }
    return(ioerr);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdStopRTIso()" */
AROS_LH1(LONG, psdStopRTIso,
         AROS_LHA(struct PsdRTIsoHandler *, prt, A1),
         LIBBASETYPEPTR, ps, 96, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdPipe *pp;
    LONG ioerr;

    if(!prt)
    {
        return UHIOERR_BADPARAMS;
    }
    pp = prt->prt_Pipe;
    pp->pp_IOReq.iouh_Req.io_Command = UHCMD_STOPRTISO;
    ioerr = DoIO((struct IORequest *) &pp->pp_IOReq);
    if(!ioerr)
    {
        --pp->pp_Device->pd_IOBusyCount;
    }
    return(ioerr);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Classes *** */

/* /// "psdAddClass()" */
AROS_LH2(struct PsdUsbClass *, psdAddClass,
         AROS_LHA(STRPTR, name, A1),
         AROS_LHA(ULONG, vers, D0),
         LIBBASETYPEPTR, ps, 35, psd)
{
    AROS_LIBFUNC_INIT
    struct Library *cls = NULL;
    struct PsdUsbClass *puc;
    IPTR pri = 0;
    STRPTR desc;
    UWORD msgoff;
    STRPTR origname = name;
    STRPTR evilmsg[8] = { "Say hello to %s V%ld.%ld (%s).",
                          "Whoah! %s V%ld.%ld surprised as %s.",
                          "The door bell rang for %s V%ld.%ld (%s).",
                          "Welcome %s V%ld.%ld (%s) to the party.",

                          "Don't laugh at %s V%ld.%ld for %s.",
                          "Time has come for %s V%ld.%ld (%s) to join the show.",
                          "Start blaming %s V%ld.%ld for helping at %s.",
                          "Ain't %s V%ld.%ld useful for %s?" };

    KPRINTF(5, ("psdAddClass(%s, %ld)\n", name, vers));

    while(*name)
    {
        if((cls = OpenLibrary(name, vers)))
        {
            break;
        }
        do
        {
            if((*name == '/') || (*name == ':'))
            {
                ++name;
                break;
            }
        } while(*(++name));
    }
    if(cls)
    {
        Forbid();
        if(FindName(&ps->ps_Classes, cls->lib_Node.ln_Name))
        {
            Permit();
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Attempted to add class %s twice. Nothing is good enough for people like you.",
                           name);
            KPRINTF(20, ("attempt to add class twice!\n"));
            CloseLibrary(cls);
            return(NULL);
        }
        Permit();
        if((puc = psdAllocVec(sizeof(struct PsdUsbClass))))
        {
            puc->puc_Base = ps;
            puc->puc_ClassBase = cls;
            puc->puc_Node.ln_Name = puc->puc_ClassName = psdCopyStr(cls->lib_Node.ln_Name);
            puc->puc_FullPath = psdCopyStr(origname);

            usbGetAttrs(UGA_CLASS, NULL,
                        UCCA_Priority, &pri,
                        UCCA_Description, &desc,
                        TAG_END);

            puc->puc_Node.ln_Pri = pri;
            psdLockWritePBase();
            Enqueue(&ps->ps_Classes, &puc->puc_Node);
            psdUnlockPBase();
            msgoff = ps->ps_FunnyCount++ & 7;

            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                           evilmsg[msgoff],
                           cls->lib_Node.ln_Name, cls->lib_Version, cls->lib_Revision, desc);
            psdSendEvent(EHMB_ADDCLASS, puc, NULL);
            return(puc);
        }
        CloseLibrary(cls);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdRemClass()" */
AROS_LH1(void, psdRemClass,
         AROS_LHA(struct PsdUsbClass *, puc, A1),
         LIBBASETYPEPTR, ps, 36, psd)
{
    AROS_LIBFUNC_INIT
    KPRINTF(5, ("psdRemClass(%p)\n", puc));
    psdLockWritePBase();
    Remove(&puc->puc_Node);
    psdUnlockPBase();

    /* Check if there are still bindings remaining */
    while(puc->puc_UseCnt)
    {
        struct PsdDevice *pd;
        struct PsdConfig *pc;
        struct PsdInterface *pif;

        KPRINTF(20, ("This should never happen: Class %s still in use (%ld), can't close!\n",
                puc->puc_ClassBase->lib_Node.ln_Name, puc->puc_UseCnt));

        /* Well, try to release the open bindings in a best effort attempt */
        psdLockReadPBase();
        pd = NULL;
        while((pd = psdGetNextDevice(pd)))
        {
            if(pd->pd_DevBinding && (pd->pd_ClsBinding == puc) && (!(pd->pd_Flags & PDFF_APPBINDING)))
            {
                psdUnlockPBase();
                psdReleaseDevBinding(pd);
                psdLockReadPBase();
                pd = NULL; /* restart */
                continue;
            }
            pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
            while(pc->pc_Node.ln_Succ)
            {
                pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                while(pif->pif_Node.ln_Succ)
                {
                    if(pif->pif_IfBinding && (pif->pif_ClsBinding == puc))
                    {
                        psdUnlockPBase();
                        psdReleaseIfBinding(pif);
                        psdLockReadPBase();
                        pd = NULL; /* restart */
                        continue;
                    }
                    pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
                }
                pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
            }
        }
        psdUnlockPBase();
        if(puc->puc_UseCnt)
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                           "This should never happen! Class %s still in use (cnt=%ld). Could not get rid of it! Sorry, we're broke.",
                           puc->puc_ClassBase->lib_Node.ln_Name, puc->puc_UseCnt);

            /*psdDelayMS(2000);*/
        }
    }
    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "I shot class %s, but I didn't kill the deputy.",
                   puc->puc_ClassBase->lib_Node.ln_Name);
    CloseLibrary(puc->puc_ClassBase);
    psdFreeVec(puc->puc_ClassName);
    psdFreeVec(puc->puc_FullPath);
    psdFreeVec(puc);
    psdSendEvent(EHMB_REMCLASS, puc, NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Error Msgs *** */

/* /// "psdAddErrorMsgA()" */
AROS_LH4(struct PsdErrorMsg *, psdAddErrorMsgA,
         AROS_LHA(UWORD, level, D0),
         AROS_LHA(STRPTR, origin, A0),
         AROS_LHA(STRPTR, fmtstr, A1),
         AROS_LHA(IPTR *, fmtdata, A2),
         LIBBASETYPEPTR, ps, 40, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdErrorMsg *pem;
    if(((!ps->ps_GlobalCfg->pgc_LogInfo) && (level < RETURN_WARN)) ||
       ((!ps->ps_GlobalCfg->pgc_LogWarning) && (level >= RETURN_WARN) && (level < RETURN_ERROR)) ||
       ((!ps->ps_GlobalCfg->pgc_LogError) && (level >= RETURN_ERROR) && (level < RETURN_FAIL)) ||
       ((!ps->ps_GlobalCfg->pgc_LogFailure) && (level >= RETURN_FAIL)))
    {
        return(NULL);
    }
    if((pem = psdAllocVec(sizeof(struct PsdErrorMsg))))
    {
        pem->pem_Base = ps;
        pem->pem_Level = level;
        if((pem->pem_Origin = psdCopyStr(origin)))
        {
            if((pem->pem_Msg = psdCopyStrFmtA(fmtstr, fmtdata)))
            {
		if (ps->ps_Flags & PSF_KLOG) {
		    KPrintF("[%s] %s\n", origin, pem->pem_Msg);
		}

                if(pOpenDOS(ps))
                {
                    DateStamp(&pem->pem_DateStamp);
                } else {
                    struct timerequest tr = ps->ps_TimerIOReq;
                    tr.tr_node.io_Command = TR_GETSYSTIME;
                    DoIO((struct IORequest *) &tr);
                    pem->pem_DateStamp.ds_Days = tr.tr_time.tv_secs / (24*60*60);
                    pem->pem_DateStamp.ds_Minute = (tr.tr_time.tv_secs / 60) % 60;
                    pem->pem_DateStamp.ds_Tick = (tr.tr_time.tv_secs % 60) * 50;
                }
                Forbid();
                AddTail(&ps->ps_ErrorMsgs, &pem->pem_Node);
                Permit();
                psdSendEvent(EHMB_ADDERRORMSG, pem, NULL);
                return(pem);
            }
            psdFreeVec(pem->pem_Origin);
        }
        psdFreeVec(pem);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdRemErrorMsg()" */
AROS_LH1(void, psdRemErrorMsg,
         AROS_LHA(struct PsdErrorMsg *, pem, A0),
         LIBBASETYPEPTR, ps, 41, psd)
{
    AROS_LIBFUNC_INIT
    KPRINTF(1, ("psdRemErrorMsg()\n"));
    Forbid();
    Remove(&pem->pem_Node);
    Permit();
    psdFreeVec(pem->pem_Origin);
    psdFreeVec(pem->pem_Msg);
    psdFreeVec(pem);
    psdSendEvent(EHMB_REMERRORMSG, pem, NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Bindings *** */

/* /// "psdClassScan()" */
AROS_LH0(void, psdClassScan,
         LIBBASETYPEPTR, ps, 37, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdHardware *phw;
    struct PsdDevice *pd;
    struct PsdUsbClass *puc;

    psdLockReadPBase();

    if((FindTask(NULL)->tc_Node.ln_Type != NT_PROCESS) && (!ps->ps_ConfigRead))
    {
        // it's the first time we were reading the config and DOS was not available
        ps->ps_StartedAsTask = TRUE;
    }

    puc = (struct PsdUsbClass *) ps->ps_Classes.lh_Head;
    if(!puc->puc_Node.ln_Succ)
    {
        psdAddErrorMsg0(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "ClassScan attempted with no classes installed!");
        psdUnlockPBase();
        return;
    }

    phw = (struct PsdHardware *) ps->ps_Hardware.lh_Head;
    while(phw->phw_Node.ln_Succ)
    {
        if((pd = phw->phw_RootDevice))
        {
            // for the root, do it ourselves, the rest is done by each hub task
            psdHubClassScan(pd);
        }
        phw = (struct PsdHardware *) phw->phw_Node.ln_Succ;
    }
    psdUnlockPBase();
    //psdSendEvent(EHMB_CLSSCANRDY, NULL, NULL);
    KPRINTF(5, ("************ Scanning finished!\n"));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdDoHubMethodA()" */
AROS_LH3(LONG, psdDoHubMethodA,
         AROS_LHA(struct PsdDevice *, pd, A0),
         AROS_LHA(ULONG, methodid, D0),
         AROS_LHA(APTR, methoddata, A1),
         LIBBASETYPEPTR, ps, 92, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdUsbClass *puc;
    KPRINTF(2, ("psdDoHubMethodA(%p)\n", pd));

    if(pd)
    {
        if(pd->pd_Hub)
        {
            if((pd->pd_Hub->pd_DevBinding) && (puc = pd->pd_Hub->pd_ClsBinding))
            {
                return(usbDoMethodA(methodid, methoddata));
            }
        }
    }
    return 0;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdClaimAppBindingA()" */
AROS_LH1(struct PsdAppBinding *, psdClaimAppBindingA,
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, ps, 45, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdDevice *pd;
    struct PsdConfig *pc;
    struct PsdInterface *pif;
    struct PsdDevice *hubpd;
    struct PsdAppBinding tmppab;
    struct PsdAppBinding *pab = NULL;
    struct PsdUsbClass *puc;

    APTR binding;

    KPRINTF(2, ("psdClaimAppBindingA(%p)\n", tags));

    tmppab.pab_Device = NULL;
    tmppab.pab_ReleaseHook = NULL;
    tmppab.pab_Task = NULL;
    tmppab.pab_ForceRelease = FALSE;
    psdSetAttrsA(PGA_APPBINDING, &tmppab, tags);
    if(tmppab.pab_Device && tmppab.pab_ReleaseHook)
    {
        pd = tmppab.pab_Device;

        // force release of other bindings first
        if(tmppab.pab_ForceRelease)
        {
            /* If there are bindings, get rid of them. */
            if(pd->pd_DevBinding)
            {
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "%s really wants to bind to %s, so I'm letting the old binding go.",
                               FindTask(NULL)->tc_Node.ln_Name,
                               pd->pd_ProductStr);

                psdReleaseDevBinding(pd);
            } else {
                pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
                while(pc->pc_Node.ln_Succ)
                {
                    pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                    while(pif->pif_Node.ln_Succ)
                    {
                        if(pif->pif_IfBinding)
                        {
                            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                           "%s really wants to bind to %s, so I'm letting the old binding go.",
                                           FindTask(NULL)->tc_Node.ln_Name,
                                           pd->pd_ProductStr);
                            psdReleaseIfBinding(pif);
                        }
                        pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
                    }
                    pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
                }
            }
        }
        hubpd = pd->pd_Hub;
        if(!hubpd) // claim app binding at the root hub -- improbable, but possible.
        {
            pab = psdHubClaimAppBindingA(tags);
        } else {
            if((binding = hubpd->pd_DevBinding) && (puc = hubpd->pd_ClsBinding))
            {
                pab = (struct PsdAppBinding *) usbDoMethod(UCM_HubClaimAppBinding, binding, tags);
            }
        }
        if(pab)
        {
            // fill in task names
            pab->pab_Task = FindTask(NULL);
            pab->pab_Node.ln_Name = pab->pab_Task->tc_Node.ln_Name;
            psdSendEvent(EHMB_ADDBINDING, pd, NULL);
            return(pab);
        }
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdReleaseAppBinding()" */
AROS_LH1(void, psdReleaseAppBinding,
         AROS_LHA(struct PsdAppBinding *, pab, A0),
         LIBBASETYPEPTR, ps, 46, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdDevice *pd;
    struct PsdDevice *hubpd;
    struct PsdUsbClass *puc;
    APTR binding;

    KPRINTF(2, ("psdReleaseAppBinding(%p)\n", pab));

    if(pab)
    {
        pd = pab->pab_Device;
        hubpd = pd->pd_Hub;
        if(!hubpd) // release binding of hub (improbable)
        {
            psdHubReleaseDevBinding(pd);
            return;
        }
        if((binding = hubpd->pd_DevBinding) && (puc = hubpd->pd_ClsBinding))
        {
            usbDoMethod(UCM_HubReleaseDevBinding, binding, pd);
        }
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdReleaseDevBinding()" */
AROS_LH1(void, psdReleaseDevBinding,
         AROS_LHA(struct PsdDevice *, pd, A0),
         LIBBASETYPEPTR, ps, 50, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdUsbClass *puc;
    struct PsdDevice *hubpd;
    APTR binding;

    KPRINTF(5, ("psdReleaseDevBinding(%p)\n", pd));
    if(pd->pd_DevBinding)
    {
        hubpd = pd->pd_Hub;
        if(!hubpd) // release binding of hub
        {
            psdHubReleaseDevBinding(pd);
            return;
        }
        if((binding = hubpd->pd_DevBinding) && (puc = hubpd->pd_ClsBinding))
        {
            usbDoMethod(UCM_HubReleaseDevBinding, binding, pd);
        }
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdReleaseIfBinding()" */
AROS_LH1(void, psdReleaseIfBinding,
         AROS_LHA(struct PsdInterface *, pif, A0),
         LIBBASETYPEPTR, ps, 51, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdUsbClass *puc;
    struct PsdDevice *hubpd;
    APTR binding;

    KPRINTF(5, ("psdReleaseIfBinding(%p)\n", pif));
    if(pif->pif_IfBinding && pif->pif_ClsBinding)
    {
        hubpd = pif->pif_Config->pc_Device->pd_Hub;
        if(!hubpd) // release binding of hub (improbable)
        {
            psdHubReleaseIfBinding(pif);
            return;
        }
        if((binding = hubpd->pd_DevBinding) && (puc = hubpd->pd_ClsBinding))
        {
            usbDoMethod(UCM_HubReleaseIfBinding, binding, pif);
        }
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdUnbindAll()" */
AROS_LH0(void, psdUnbindAll,
         LIBBASETYPEPTR, ps, 61, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdHardware *phw;
    struct PsdDevice *pd;
    struct PsdConfig *pc;
    struct PsdInterface *pif;
    BOOL restart;

    KPRINTF(10, ("pUnbindAll()\n"));
    /* FIXME What happens if devices or hardware gets removed during the process? Need notify semaphore */
    psdLockReadPBase();
    do
    {
        restart = FALSE;
        phw = (struct PsdHardware *) ps->ps_Hardware.lh_Head;
        while(phw->phw_Node.ln_Succ)
        {
            pd = (struct PsdDevice *) phw->phw_Devices.lh_Head;
            while(pd->pd_Node.ln_Succ)
            {
                /* If there are bindings, get rid of them. */
                if(pd->pd_DevBinding)
                {
                    psdUnlockPBase();
                    psdReleaseDevBinding(pd);
                    psdLockReadPBase();
                    restart = TRUE;
                    break;
                }
                pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
                while(pc->pc_Node.ln_Succ)
                {
                    pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                    while(pif->pif_Node.ln_Succ)
                    {
                        if(pif->pif_IfBinding)
                        {
                            psdUnlockPBase();
                            psdReleaseIfBinding(pif);
                            psdLockReadPBase();
                            restart = TRUE;
                            break;
                        }
                        pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
                    }
                    if(restart)
                    {
                        break;
                    }
                    pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
                }
                if(restart)
                {
                    break;
                }
                pd = (struct PsdDevice *) pd->pd_Node.ln_Succ;
            }
            if(restart)
            {
                break;
            }
            phw = (struct PsdHardware *) phw->phw_Node.ln_Succ;
        }
    } while(restart);
    psdUnlockPBase();
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdHubClassScan()" */
AROS_LH1(void, psdHubClassScan,
         AROS_LHA(struct PsdDevice *, pd, A0),
         LIBBASETYPEPTR, ps, 82, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdUsbClass *puc;
    struct PsdConfig *pc;
    struct PsdInterface *pif;
    struct PsdInterface *firstpif;
    struct PsdPipe *pp = NULL;
    struct MsgPort *mp;
    APTR binding;
    UWORD hasifbinding;
    BOOL mainif;
    STRPTR owner;

    KPRINTF(5, ("psdClassScan()\n"));

    if(!(mp = CreateMsgPort()))
    {
        return;
    }
    psdLockReadPBase();
    psdLockWriteDevice(pd);
    while(!(pd->pd_PoPoCfg.poc_NoClassBind || pd->pd_DevBinding))
    {
        if(!(pp = psdAllocPipe(pd, mp, NULL)))
        {
            break;
        }
        KPRINTF(5, ("Doing ClassScan on Device: %s\n", pd->pd_ProductStr));
        hasifbinding = 0;
        /* First look if there is any interface binding. We may not change
           the current config in this case! */
        pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
        while(pc->pc_Node.ln_Succ)
        {
            pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;

            while(pif->pif_Node.ln_Succ)
            {
                if(pif->pif_IfBinding)
                {
                    hasifbinding = pc->pc_CfgNum;
                    break;
                }
                pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
            }
            pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
        }

        owner = psdGetForcedBinding(pd->pd_IDString, NULL);
        if((!hasifbinding) && owner)
        {
            puc = (struct PsdUsbClass *) ps->ps_Classes.lh_Head;
            while(puc->puc_Node.ln_Succ)
            {
                if(!strcmp(owner, puc->puc_ClassName))
                {
                    if((pd->pd_DevBinding = (APTR) usbDoMethod(UCM_ForceDeviceBinding, pd)))
                    {
                        pd->pd_ClsBinding = puc;
                        puc->puc_UseCnt++;
                        psdSendEvent(EHMB_ADDBINDING, pd, NULL);
                    } else {
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                       "Forced device binding of %s to %s failed.", pd->pd_ProductStr, owner);
                    }
                    break;
                }
                puc = (struct PsdUsbClass *) puc->puc_Node.ln_Succ;
            }
            /* no more scanning required, abort here */
            break;
        }

        /* Second attempt */
        pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
        while(pc->pc_Node.ln_Succ)
        {
            if((!hasifbinding) || (hasifbinding == pc->pc_CfgNum))
            {
                /* If the current config is not the one selected, change it */
                if(pd->pd_CurrCfg != pc->pc_CfgNum)
                {
                    psdSetDeviceConfig(pp, pc->pc_CfgNum);
                }
                KPRINTF(5, ("  Config %ld\n", pc->pc_CfgNum));
                /* If something went wrong above, we must exclude this config */
                if(pd->pd_CurrCfg == pc->pc_CfgNum)
                {
                    pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                    while(pif->pif_Node.ln_Succ)
                    {
                        KPRINTF(5, ("    Interface %ld\n", pif->pif_IfNum));
                        firstpif = pif;
                        mainif = TRUE;
                        if(!pif->pif_IfBinding)
                        {
                            binding = NULL;
                            do
                            {
                                if(!psdSetAltInterface(pp, pif))
                                {
                                    pif->pif_IfBinding = NULL;
                                    /* Okay, this alternate setting failed. Try to get next one */
                                    if(!mainif)
                                    {
                                        pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
                                        if(pif->pif_Node.ln_Succ)
                                        {
                                            KPRINTF(5, ("CONT!\n"));
                                            continue;
                                        } else {
                                            KPRINTF(5, ("BREAK!\n"));
                                            pif = firstpif;
                                            break;
                                        }
                                    }
                                }
                                owner = psdGetForcedBinding(pd->pd_IDString, pif->pif_IDString);
                                puc = (struct PsdUsbClass *) ps->ps_Classes.lh_Head;
                                while(puc->puc_Node.ln_Succ)
                                {
                                    KPRINTF(5, (">>>PING %s!\n", puc->puc_ClassName));
                                    if(owner)
                                    {
                                        if(!strcmp(owner, puc->puc_ClassName))
                                        {
                                            binding = (APTR) usbDoMethod(UCM_ForceInterfaceBinding, pif);
                                            if(!binding)
                                            {
                                                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                               "Forced interface binding of %s to %s failed.", pd->pd_ProductStr, owner);
                                            }
                                        }
                                        if(!binding)
                                        {
                                            puc = (struct PsdUsbClass *) puc->puc_Node.ln_Succ;
                                            continue;
                                        }
                                    } else {
                                        binding = (APTR) usbDoMethod(UCM_AttemptInterfaceBinding, pif);
                                    }
                                    Forbid();
                                    KPRINTF(5, ("<<<PONG!!\n"));
                                    if(binding)
                                    {
                                        KPRINTF(5, ("Got binding!\n"));
                                        /* Find root config structure */
                                        pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                                        while(pif->pif_Node.ln_Succ)
                                        {
                                            if(pif->pif_IfNum == firstpif->pif_IfNum)
                                            {
                                                break;
                                            }
                                            pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
                                        }
                                        if(!pif->pif_Node.ln_Succ)
                                        {
                                            KPRINTF(5, ("Fucked it up!\n"));
                                            psdAddErrorMsg0(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Something incredibly stupid happend. I've given up.");
                                            Permit();
                                            break;
                                        }
                                        pif->pif_IfBinding = binding;
                                        pif->pif_ClsBinding = puc;
                                        hasifbinding = pc->pc_CfgNum;
                                        puc->puc_UseCnt++;
                                        psdSendEvent(EHMB_ADDBINDING, pd, NULL);
                                        Permit();
                                        break;
                                    }
                                    Permit();
                                    puc = (struct PsdUsbClass *) puc->puc_Node.ln_Succ;
                                }
                                if(binding)
                                {
                                    break;
                                }
                                //break; /* FIXME: DISABLED ALTSCANNING */
                                /* Check alternate setting */
                                if(pif->pif_AlterIfs.lh_Head->ln_Succ)
                                {
                                     /* There are some alternative interfaces, start at top */
                                     pif = (struct PsdInterface *) pif->pif_AlterIfs.lh_Head;
                                     mainif = FALSE;
                                }
                            } while(pif != firstpif);
                            //pif->pif_IfBinding = binding;
                            if(!binding)
                            {
                                psdSetAltInterface(pp, pif);
                            }
                            /* Hohum, search current main interface then */
                            pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                            while(pif->pif_Node.ln_Succ)
                            {
                                if(pif->pif_IfNum == firstpif->pif_IfNum)
                                {
                                    break;
                                }
                                pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
                            }
                        }
                        pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
                    }
                }
            }
            KPRINTF(5, ("End, next ConfigCheck!\n"));
            pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
        }
        /* Could not establish interface binding, try device binding then */
        //psdUnlockPBase();
        if(!hasifbinding)
        {
            //pd->pd_DevBinding = (APTR) ~0UL;
            binding = NULL;
            owner = psdGetForcedBinding(pd->pd_IDString, NULL);
            puc = (struct PsdUsbClass *) ps->ps_Classes.lh_Head;
            while(puc->puc_Node.ln_Succ)
            {
                binding = NULL;
                if(owner)
                {
                    if(!strcmp(owner, puc->puc_ClassName))
                    {
                        binding = (APTR) usbDoMethod(UCM_ForceDeviceBinding, pd, TAG_END);
                        if(!binding)
                        {
                            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                           "Forced device binding of %s to %s failed.", pd->pd_ProductStr, owner);
                        }
                    }
                    if(!binding)
                    {
                        puc = (struct PsdUsbClass *) puc->puc_Node.ln_Succ;
                        continue;
                    }
                } else {
                    binding = (APTR) usbDoMethod(UCM_AttemptDeviceBinding, pd);
                }
                if(binding)
                {
                    pd->pd_DevBinding = binding;
                    pd->pd_ClsBinding = puc;
                    puc->puc_UseCnt++;
                    psdSendEvent(EHMB_ADDBINDING, pd, NULL);
                    break;
                }
                puc = (struct PsdUsbClass *) puc->puc_Node.ln_Succ;
            }
            pd->pd_DevBinding = binding;
        }
        break;
    }
    if(pp)
    {
        psdFreePipe(pp);
    }
    // call hub class scan code
    if((binding = pd->pd_DevBinding) && (puc = pd->pd_ClsBinding))
    {
        usbDoMethod(UCM_HubClassScan, binding);
    }
    psdUnlockDevice(pd);
    psdUnlockPBase();
    DeleteMsgPort(mp);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdHubClaimAppBindingA()" */
AROS_LH1(struct PsdAppBinding *, psdHubClaimAppBindingA,
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, ps, 83, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdDevice *pd;
    struct PsdAppBinding *pab;
    struct PsdConfig *pc;
    struct PsdInterface *pif;

    BOOL hasbinding = FALSE;
    KPRINTF(2, ("psdHubClaimAppBindingA(%p)\n", tags));

    if((pab = psdAllocVec(sizeof(struct PsdAppBinding))))
    {
        psdSetAttrsA(PGA_APPBINDING, pab, tags);
        if(pab->pab_Device && pab->pab_ReleaseHook)
        {
            pd = pab->pab_Device;
            if(pd->pd_DevBinding)
            {
                hasbinding = TRUE;
            } else {
                pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
                while(pc->pc_Node.ln_Succ)
                {
                    pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;

                    while(pif->pif_Node.ln_Succ)
                    {
                        if(pif->pif_IfBinding)
                        {
                            hasbinding = TRUE;
                            break;
                        }
                        pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
                    }
                    pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
                }
            }
            if(!hasbinding)
            {
                pd->pd_Flags |= PDFF_APPBINDING;
                pd->pd_DevBinding = pab;
                pd->pd_ClsBinding = NULL;
                return(pab);
            }
        }
        psdFreeVec(pab);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdHubReleaseDevBinding()" */
AROS_LH1(void, psdHubReleaseDevBinding,
         AROS_LHA(struct PsdDevice *, pd, A0),
         LIBBASETYPEPTR, ps, 84, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdUsbClass *puc;
    APTR binding;
    struct PsdAppBinding *pab;

    KPRINTF(5, ("psdHubReleaseDevBinding(%p)\n", pd));
    if(pd)
    {
        psdLockWriteDevice(pd);
        if((binding = pd->pd_DevBinding))
        {
            pd->pd_DevBinding = NULL;
            if(pd->pd_Flags & PDFF_APPBINDING)
            {
                pab = (struct PsdAppBinding *) binding;
                CallHookPkt(pab->pab_ReleaseHook, pab, (APTR) pab->pab_UserData);
                pd->pd_ClsBinding = NULL;
                pd->pd_Flags &= ~PDFF_APPBINDING;
                psdFreeVec(pab);
                psdSendEvent(EHMB_REMBINDING, pd, NULL);
            } else {
                puc = pd->pd_ClsBinding;
                if(puc)
                {
                    pd->pd_ClsBinding = NULL;
                    usbDoMethod(UCM_ReleaseDeviceBinding, binding);
                    puc->puc_UseCnt--;
                    psdSendEvent(EHMB_REMBINDING, pd, NULL);
                }
            }
        }
        psdUnlockDevice(pd);
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdHubReleaseIfBinding()" */
AROS_LH1(void, psdHubReleaseIfBinding,
         AROS_LHA(struct PsdInterface *, pif, A0),
         LIBBASETYPEPTR, ps, 85, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdUsbClass *puc;
    struct PsdDevice *pd;
    APTR binding;

    KPRINTF(5, ("psdHubReleaseIfBinding(%p)\n", pif));

    if(pif)
    {
        pd = pif->pif_Config->pc_Device;
        psdLockWriteDevice(pd);
        if((binding = pif->pif_IfBinding))
        {
            pif->pif_IfBinding = NULL;
            puc = pif->pif_ClsBinding;
            if(puc)
            {
                pif->pif_ClsBinding = NULL;
                usbDoMethod(UCM_ReleaseInterfaceBinding, binding);
                puc->puc_UseCnt--;
            }
            psdSendEvent(EHMB_REMBINDING, pd, NULL);
        }
        psdUnlockDevice(pd);
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Events *** */

/* /// "psdAddEventHandler()" */
AROS_LH2(struct PsdEventHook *, psdAddEventHandler,
         AROS_LHA(struct MsgPort *, mp, A1),
         AROS_LHA(ULONG, msgmask, D0),
         LIBBASETYPEPTR, ps, 47, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdEventHook *peh = NULL;

    KPRINTF(5, ("psdAddEventHandler(%p, %p)\n", mp, msgmask));

    if(mp)
    {
        ObtainSemaphore(&ps->ps_ReentrantLock);
        if((peh = psdAllocVec(sizeof(struct PsdEventHook))))
        {
            peh->peh_MsgPort = mp;
            peh->peh_MsgMask = msgmask;
            AddTail(&ps->ps_EventHooks, &peh->peh_Node);
        }
        ReleaseSemaphore(&ps->ps_ReentrantLock);
    }
    return(peh);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdRemEventHandler()" */
AROS_LH1(void, psdRemEventHandler,
         AROS_LHA(struct PsdEventHook *, peh, A0),
         LIBBASETYPEPTR, ps, 48, psd)
{
    AROS_LIBFUNC_INIT
    struct Message *msg;

    KPRINTF(5, ("psdRemEventHandler(%p)\n", peh));
    if(!peh)
    {
        return;
    }
    ObtainSemaphore(&ps->ps_ReentrantLock);
    Remove(&peh->peh_Node);
    while((msg = GetMsg(peh->peh_MsgPort)))
    {
        ReplyMsg(msg);
    }
    ReleaseSemaphore(&ps->ps_ReentrantLock);
    pGarbageCollectEvents(ps);
    psdFreeVec(peh);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdSendEvent()" */
AROS_LH3(void, psdSendEvent,
         AROS_LHA(ULONG, ehmt, D0),
         AROS_LHA(APTR, param1, A0),
         AROS_LHA(APTR, param2, A1),
         LIBBASETYPEPTR, ps, 49, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdEventNote *pen;
    struct PsdEventHook *peh;
    ULONG msgmask = (1L<<ehmt);

    KPRINTF(1, ("psdSendEvent(%p, %p, %p)\n", ehmt, param1, param2));

    pGarbageCollectEvents(ps);
    ObtainSemaphore(&ps->ps_ReentrantLock);
    peh = (struct PsdEventHook *) ps->ps_EventHooks.lh_Head;
    while(peh->peh_Node.ln_Succ)
    {
        if(peh->peh_MsgMask & msgmask)
        {
            if((pen = psdAllocVec(sizeof(struct PsdEventNote))))
            {
                pen->pen_Msg.mn_ReplyPort = &ps->ps_EventReplyPort;
                pen->pen_Msg.mn_Length = sizeof(struct PsdEventNote);
                pen->pen_Event = ehmt;
                pen->pen_Param1 = param1;
                pen->pen_Param2 = param2;
                PutMsg(peh->peh_MsgPort, &pen->pen_Msg);
            }
        }
        peh = (struct PsdEventHook *) peh->peh_Node.ln_Succ;
    }
    ReleaseSemaphore(&ps->ps_ReentrantLock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Configuration *** */

/* /// "psdReadCfg()" */
AROS_LH2(BOOL, psdReadCfg,
         AROS_LHA(struct PsdIFFContext *, pic, A0),
         AROS_LHA(APTR, formdata, A1),
         LIBBASETYPEPTR, ps, 52, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdIFFContext *subpic;
    LONG len;
    ULONG chlen;
    ULONG *buf = formdata;
    BOOL res = TRUE;
    KPRINTF(10, ("psdReadCfg(%p, %p)\n", pic, formdata));

    pLockSemExcl(ps, &ps->ps_ConfigLock);
    if(!pic)
    {
        pic = (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head;
        if(!(pic->pic_Node.ln_Succ))
        {
            pUnlockSem(ps, &ps->ps_ConfigLock);
            return(FALSE);
        }
    }
    if((AROS_LONG2BE(*buf) != ID_FORM) || (AROS_LONG2BE(buf[2]) != pic->pic_FormID))
    {
        psdAddErrorMsg0(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Tried to replace a cfg form with a chunk or with an alien form!");
        pUnlockSem(ps, &ps->ps_ConfigLock);
        return(FALSE);
    }
    subpic = (struct PsdIFFContext *) pic->pic_SubForms.lh_Head;
    while(subpic->pic_Node.ln_Succ)
    {
        pFreeForm(ps, subpic);
        subpic = (struct PsdIFFContext *) pic->pic_SubForms.lh_Head;
    }
    pic->pic_ChunksLen = 0;
    len = (AROS_LONG2BE(buf[1]) - 3) & ~1UL;
    buf += 3;
    while(len >= 8)
    {
        if(!(pAddCfgChunk(ps, pic, buf)))
        {
            break;
        }
        chlen = (AROS_LONG2BE(buf[1]) + 9) & ~1UL;
        len -= chlen;
        buf = (ULONG *) (((UBYTE *) buf) + chlen);
    }
    if(len)
    {
        psdAddErrorMsg0(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Tried to add a nasty corrupted FORM chunk! Configuration is probably b0rken!");
        res = 0;
    }

    pUnlockSem(ps, &ps->ps_ConfigLock);
    ps->ps_CheckConfigReq = TRUE;
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdLoadCfgFromDisk()" */
AROS_LH1(BOOL, psdLoadCfgFromDisk,
         AROS_LHA(STRPTR, filename, A1),
         LIBBASETYPEPTR, ps, 79, psd)
{
    AROS_LIBFUNC_INIT
    ULONG *buf;
    BOOL loaded = FALSE;
    BPTR filehandle;
    ULONG formhead[3];
    ULONG formlen;

    XPRINTF(10, ("Loading config file: %s\n", filename));

    if(!filename)
    {
        loaded = psdLoadCfgFromDisk("ENV:Sys/poseidon.prefs");
        if(loaded)
        {
            return(TRUE);
        }

        loaded = psdLoadCfgFromDisk("ENVARC:Sys/poseidon.prefs");

        return(loaded);
    }

    if(!pOpenDOS(ps))
    {
    	KPRINTF(1, ("dos.library not available yet\n"));
        return(FALSE);
    }

    filehandle = Open(filename, MODE_OLDFILE);
    KPRINTF(1, ("File handle 0x%p\n", filehandle));
    if(filehandle)
    {
        if(Read(filehandle, formhead, 12) == 12)
        {
            KPRINTF(1, ("Read header\n"));
            if((AROS_LONG2BE(formhead[0]) == ID_FORM) && (AROS_LONG2BE(formhead[2]) == IFFFORM_PSDCFG))
            {
                formlen = AROS_LONG2BE(formhead[1]);
                KPRINTF(1, ("Header OK, %lu bytes\n", formlen));

                buf = (ULONG *) psdAllocVec(formlen + 8);
                if(buf)
                {
                    buf[0] = formhead[0];
                    buf[1] = formhead[1];
                    buf[2] = formhead[2];
                    if(Read(filehandle, &buf[3], formlen - 4) == formlen - 4)
                    {
                    	KPRINTF(1, ("Data read OK\n"));

                        psdReadCfg(NULL, buf);
                        psdParseCfg();
                        
                        KPRINTF(1, ("All done\n"));
                        loaded = TRUE;
                    }
                    psdFreeVec(buf);
                }
            }
        }
        Close(filehandle);
    } else {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "Failed to load config from '%s'!",
                       filename);
    }
    if(loaded)
    {
        ps->ps_SavedConfigHash = ps->ps_ConfigHash;
    }
    return(loaded);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdSaveCfgToDisk()" */
AROS_LH2(BOOL, psdSaveCfgToDisk,
         AROS_LHA(STRPTR, filename, A1),
         AROS_LHA(BOOL, executable, D0),
         LIBBASETYPEPTR, ps, 80, psd)
{
    AROS_LIBFUNC_INIT
    ULONG *buf;
    BOOL saved = FALSE;
    BPTR filehandle;

    if(!filename)
    {
        saved = psdSaveCfgToDisk("ENVARC:Sys/poseidon.prefs", FALSE);
        saved &= psdSaveCfgToDisk("ENV:Sys/poseidon.prefs", FALSE);
        return(saved);
    }

    if(!pOpenDOS(ps))
    {
        return(FALSE);
    }
    pLockSemShared(ps, &ps->ps_ConfigLock);

    buf = (ULONG *) psdWriteCfg(NULL);
    if(buf)
    {
        /* Write file */
        filehandle = Open(filename, MODE_NEWFILE);
        if(filehandle)
        {
            Write(filehandle, buf, (AROS_LONG2BE(buf[1])+9) & ~1UL);
            Close(filehandle);
            saved = TRUE;
        } else {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Failed to write config to '%s'!",
                           filename);
        }
        psdFreeVec(buf);
    }
    pUnlockSem(ps, &ps->ps_ConfigLock);
    if(saved)
    {
        ps->ps_SavedConfigHash = ps->ps_ConfigHash;
    }
    return(saved);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdWriteCfg()" */
AROS_LH1(APTR, psdWriteCfg,
         AROS_LHA(struct PsdIFFContext *, pic, A0),
         LIBBASETYPEPTR, ps, 53, psd)
{
    AROS_LIBFUNC_INIT
    ULONG len;
    APTR buf = NULL;

    KPRINTF(10, ("psdWriteCfg(%p)\n", pic));

    pLockSemShared(ps, &ps->ps_ConfigLock);
    if(!pic)
    {
        pic = (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head;
        if(!(pic->pic_Node.ln_Succ))
        {
            pUnlockSem(ps, &ps->ps_ConfigLock);
            return(NULL);
        }
    }
    pUpdateGlobalCfg(ps, pic);
    ps->ps_CheckConfigReq = TRUE;
    len = pGetFormLength(pic);
    if((buf = psdAllocVec(len)))
    {
        pInternalWriteForm(pic, buf);
    }
    pUnlockSem(ps, &ps->ps_ConfigLock);
    return(buf);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdFindCfgForm()" */
AROS_LH2(struct PsdIFFContext *, psdFindCfgForm,
         AROS_LHA(struct PsdIFFContext *, pic, A0),
         AROS_LHA(ULONG, formid, D0),
         LIBBASETYPEPTR, ps, 54, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdIFFContext *subpic;

    KPRINTF(160, ("psdFindCfgForm(0x%p, 0x%08lx)\n", pic, formid));
    pLockSemShared(ps, &ps->ps_ConfigLock);
    if(!pic)
    {
        pic = (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head;
        if(!(pic->pic_Node.ln_Succ))
        {
            pUnlockSem(ps, &ps->ps_ConfigLock);
            return(NULL);
        }
    }
    subpic = (struct PsdIFFContext *) pic->pic_SubForms.lh_Head;
    while(subpic->pic_Node.ln_Succ)
    {
        if(subpic->pic_FormID == formid)
        {
            pUnlockSem(ps, &ps->ps_ConfigLock);
            return(subpic);
        }
        subpic = (struct PsdIFFContext *) subpic->pic_Node.ln_Succ;
    }
    pUnlockSem(ps, &ps->ps_ConfigLock);
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdNextCfgForm()" */
AROS_LH1(struct PsdIFFContext *, psdNextCfgForm,
         AROS_LHA(struct PsdIFFContext *, pic, A0),
         LIBBASETYPEPTR, ps, 55, psd)
{
    AROS_LIBFUNC_INIT
    ULONG formid;
    KPRINTF(160, ("psdNextCfgForm(%p)\n", pic));

    if(!pic)
    {
        return(NULL);
    }
    pLockSemShared(ps, &ps->ps_ConfigLock);
    formid = pic->pic_FormID;
    pic = (struct PsdIFFContext *) pic->pic_Node.ln_Succ;
    while(pic->pic_Node.ln_Succ)
    {
        if(pic->pic_FormID == formid)
        {
            pUnlockSem(ps, &ps->ps_ConfigLock);

            KPRINTF(1, ("Found context 0x%p\n", pic));
            return(pic);
        }
        pic = (struct PsdIFFContext *) pic->pic_Node.ln_Succ;
    }
    pUnlockSem(ps, &ps->ps_ConfigLock);
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdAllocCfgForm()" */
AROS_LH1(struct PsdIFFContext *, psdAllocCfgForm,
         AROS_LHA(ULONG, formid, D0),
         LIBBASETYPEPTR, ps, 86, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdIFFContext *pic;
    KPRINTF(10, ("psdAllocCfgForm(%p)\n", formid));
    if((pic = psdAllocVec(sizeof(struct PsdIFFContext))))
    {
        NewList(&pic->pic_SubForms);
        //pic->pic_Parent = parent;
        pic->pic_FormID = formid;
        pic->pic_FormLength = 4;
        pic->pic_Chunks = NULL;
        pic->pic_ChunksLen = 0;
        pic->pic_BufferLen = 0;
        Forbid();
        AddTail(&ps->ps_AlienConfigs, &pic->pic_Node);
        Permit();
    }
    return(pic);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdRemCfgForm()" */
AROS_LH1(void, psdRemCfgForm,
         AROS_LHA(struct PsdIFFContext *, pic, A0),
         LIBBASETYPEPTR, ps, 56, psd)
{
    AROS_LIBFUNC_INIT
    KPRINTF(10, ("psdRemCfgForm(%p)\n", pic));

    pLockSemExcl(ps, &ps->ps_ConfigLock);
    if(!pic)
    {
        pic = (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head;
        if(!(pic->pic_Node.ln_Succ))
        {
            pUnlockSem(ps, &ps->ps_ConfigLock);
            return;
        }
    }
    pFreeForm(ps, pic);
    pUnlockSem(ps, &ps->ps_ConfigLock);
    ps->ps_CheckConfigReq = TRUE;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdAddCfgEntry()" */
AROS_LH2(struct PsdIFFContext *, psdAddCfgEntry,
         AROS_LHA(struct PsdIFFContext *, pic, A0),
         AROS_LHA(APTR, formdata, A1),
         LIBBASETYPEPTR, ps, 57, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdIFFContext *res;

    KPRINTF(10, ("psdAddCfgEntry(%p, %p)\n", pic, formdata));
    pLockSemExcl(ps, &ps->ps_ConfigLock);
    if(!pic)
    {
        pic = (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head;
        if(!(pic->pic_Node.ln_Succ))
        {
            pUnlockSem(ps, &ps->ps_ConfigLock);
            return(NULL);
        }
    }
    res = pAddCfgChunk(ps, pic, formdata);
    pUnlockSem(ps, &ps->ps_ConfigLock);
    ps->ps_CheckConfigReq = TRUE;
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdRemCfgChunk()" */
AROS_LH2(BOOL, psdRemCfgChunk,
         AROS_LHA(struct PsdIFFContext *, pic, A0),
         AROS_LHA(ULONG, chnkid, D0),
         LIBBASETYPEPTR, ps, 58, psd)
{
    AROS_LIBFUNC_INIT
    BOOL res = FALSE;

    KPRINTF(10, ("psdRemCfgChunk(%p, %p)\n", pic, chnkid));
    pLockSemExcl(ps, &ps->ps_ConfigLock);
    if(!pic)
    {
        pic = (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head;
        if(!(pic->pic_Node.ln_Succ))
        {
            pUnlockSem(ps, &ps->ps_ConfigLock);
            return(FALSE);
        }
    }
    if(chnkid)
    {
        res = pRemCfgChunk(ps, pic, chnkid);
    } else {
        struct PsdIFFContext *subpic = (struct PsdIFFContext *) pic->pic_SubForms.lh_Head;
        while(subpic->pic_Node.ln_Succ)
        {
            pFreeForm(ps, subpic);
            res = TRUE;
            subpic = (struct PsdIFFContext *) pic->pic_SubForms.lh_Head;
        }
        if(pic->pic_ChunksLen)
        {
            res = TRUE;
        }
        pic->pic_ChunksLen = 0;
        pic->pic_FormLength = 4;
    }

    pUnlockSem(ps, &ps->ps_ConfigLock);
    ps->ps_CheckConfigReq = TRUE;
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdGetCfgChunk()" */
AROS_LH2(APTR, psdGetCfgChunk,
         AROS_LHA(struct PsdIFFContext *, pic, A0),
         AROS_LHA(ULONG, chnkid, D0),
         LIBBASETYPEPTR, ps, 59, psd)
{
    AROS_LIBFUNC_INIT
    ULONG *chnk;
    ULONG *res = NULL;

    KPRINTF(10, ("psdGetCfgChunk(%p, 0x%08lx)\n", pic, chnkid));

    pLockSemShared(ps, &ps->ps_ConfigLock);
    if(!pic)
    {
        pic = (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head;
        if(!(pic->pic_Node.ln_Succ))
        {
            pUnlockSem(ps, &ps->ps_ConfigLock);
            return(NULL);
        }
    }
    pUpdateGlobalCfg(ps, pic);
    chnk = pFindCfgChunk(ps, pic, chnkid);
    if(chnk)
    {
        res = psdAllocVec(AROS_LONG2BE(chnk[1])+8);
        if(res)
        {
            memcpy(res, chnk, AROS_LONG2BE(chnk[1])+8);
        }
    }
    pUnlockSem(ps, &ps->ps_ConfigLock);
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdParseCfg()" */
AROS_LH0(void, psdParseCfg,
         LIBBASETYPEPTR, ps, 60, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdIFFContext *pic;
    struct PsdIFFContext *subpic;
    ULONG *chnk;
    STRPTR name;
    ULONG unit;
    struct PsdHardware *phw;
    struct PsdUsbClass *puc;
    BOOL removeall = TRUE;
    BOOL nodos = (FindTask(NULL)->tc_Node.ln_Type != NT_PROCESS);
    IPTR restartme;

    XPRINTF(10, ("psdParseCfg()\n"));

    pLockSemShared(ps, &ps->ps_ConfigLock);
    pCheckCfgChanged(ps);
    pic = psdFindCfgForm(NULL, IFFFORM_STACKCFG);
    if(!pic)
    {
        pUnlockSem(ps, &ps->ps_ConfigLock);
        return;
    }

    // if no config for hardware is found, we don't remove the devices,
    // because this could render the system useless (no USB mice or
    // keyboards to configure the hardware!)
    if(!psdFindCfgForm(pic, IFFFORM_UHWDEVICE))
    {
    	XPRINTF(10, ("No hardware data present\n"));
        removeall = FALSE;
    }

    psdLockReadPBase();

    /* select all hardware devices for removal */
    phw = (struct PsdHardware *) ps->ps_Hardware.lh_Head;
    while(phw->phw_Node.ln_Succ)
    {
        phw->phw_RemoveMe = removeall;
        phw = (struct PsdHardware *) phw->phw_Node.ln_Succ;
    }

    /* select all classes for removal */
    puc = (struct PsdUsbClass *) ps->ps_Classes.lh_Head;
    while(puc->puc_Node.ln_Succ)
    {
	/*
	 * For kickstart-resident classes we check usage count, and
	 * remove them only if it's zero.
	 * These classes can be responsible for devices which we can use
	 * at boot time. If we happen to remove them, we can end up with
	 * no input or storage devices at all.
	 */
    	if (FindResident(puc->puc_ClassName))
    	    puc->puc_RemoveMe = (puc->puc_UseCnt == 0);
    	else
            puc->puc_RemoveMe = TRUE;

        puc = (struct PsdUsbClass *) puc->puc_Node.ln_Succ;
    }

    psdUnlockPBase();

    /* Get Hardware config */
    subpic = psdFindCfgForm(pic, IFFFORM_UHWDEVICE);
    while(subpic)
    {
        chnk = pFindCfgChunk(ps, subpic, IFFCHNK_NAME);
        if(chnk)
        {
            name = (STRPTR) &chnk[2];
            unit = 0;
            chnk = pFindCfgChunk(ps, subpic, IFFCHNK_UNIT);
            if(chnk)
            {
                unit = chnk[2];
            }
            if(!pFindCfgChunk(ps, subpic, IFFCHNK_OFFLINE))
            {
                phw = pFindHardware(ps, name, unit);
                XPRINTF(5, ("Have configuration for device 0x%p (%s unit %u)\n", phw, name, unit));
                if(phw)
                {
                    phw->phw_RemoveMe = FALSE;
                }
            }
        }
        subpic = psdNextCfgForm(subpic);
    }

    /* Get Class config */
    subpic = psdFindCfgForm(pic, IFFFORM_USBCLASS);
    while(subpic)
    {
        chnk = pFindCfgChunk(ps, subpic, IFFCHNK_NAME);
        if(chnk)
        {
            name = (STRPTR) &chnk[2];
            puc = (struct PsdUsbClass *) pFindName(ps, &ps->ps_Classes, name);
            XPRINTF(5, ("Have configuration for class 0x%p (%s)\n", puc, name));
            if(puc)
            {
                puc->puc_RemoveMe = FALSE;
            }
        }
        subpic = psdNextCfgForm(subpic);
    }

    // unlock config while removing to avoid deadlocks.
    pUnlockSem(ps, &ps->ps_ConfigLock);

    /* now remove remaining classes not found in the config */
    puc = (struct PsdUsbClass *) ps->ps_Classes.lh_Head;
    while(puc->puc_Node.ln_Succ)
    {
        if(puc->puc_RemoveMe)
        {
	    XPRINTF(5, ("Removing class %s\n", puc->puc_ClassName));
            psdRemClass(puc);
            puc = (struct PsdUsbClass *) ps->ps_Classes.lh_Head;
        } else {
            puc = (struct PsdUsbClass *) puc->puc_Node.ln_Succ;
        }
    }

    /* now remove all remaining hardware not found in the config */
    phw = (struct PsdHardware *) ps->ps_Hardware.lh_Head;
    while(phw->phw_Node.ln_Succ)
    {
        if(phw->phw_RemoveMe)
        {
	    XPRINTF(5, ("Removing device %s unit %u\n", phw->phw_DevName, phw->phw_Unit));
            psdRemHardware(phw);
            phw = (struct PsdHardware *) ps->ps_Hardware.lh_Head;
        } else {
            phw = (struct PsdHardware *) phw->phw_Node.ln_Succ;
        }
    }

    pLockSemShared(ps, &ps->ps_ConfigLock);
    pic = psdFindCfgForm(NULL, IFFFORM_STACKCFG);
    if(!pic)
    {
        pUnlockSem(ps, &ps->ps_ConfigLock);
        // oops!
        return;
    }

    /* Add missing Classes */
    subpic = psdFindCfgForm(pic, IFFFORM_USBCLASS);
    while(subpic)
    {
        chnk = pFindCfgChunk(ps, subpic, IFFCHNK_NAME);
        if(chnk)
        {
            /* *** FIXME *** POSSIBLE DEADLOCK WHEN CLASS TRIES TO DO CONFIG STUFF IN
               AN EXTERNAL TASK INSIDE LIBOPEN CODE */
            name = (STRPTR) &chnk[2];
            puc = (struct PsdUsbClass *) pFindName(ps, &ps->ps_Classes, name);
            if(!puc)
            {
                psdAddClass(name, 0);
            }
        }
        subpic = psdNextCfgForm(subpic);
    }

    /* Now really mount Hardware found in config */
    subpic = psdFindCfgForm(pic, IFFFORM_UHWDEVICE);
    while(subpic)
    {
        chnk = pFindCfgChunk(ps, subpic, IFFCHNK_NAME);
        if(chnk)
        {
            name = (STRPTR) &chnk[2];
            unit = 0;
            chnk = pFindCfgChunk(ps, subpic, IFFCHNK_UNIT);
            if(chnk)
            {
                unit = chnk[2];
            }
            if(!pFindCfgChunk(ps, subpic, IFFCHNK_OFFLINE))
            {
                phw = pFindHardware(ps, name, unit);
                if(!phw)
                {
                    phw = psdAddHardware(name, unit);
                    if(phw)
                    {
                        psdEnumerateHardware(phw);
                    }
                }
            }
        }
        subpic = psdNextCfgForm(subpic);
    }
    pUnlockSem(ps, &ps->ps_ConfigLock);

    if(!nodos && ps->ps_StartedAsTask)
    {
        // last time we were reading the config before DOS, so maybe we need to
        // unbind some classes that need to be overruled by newly available classes,
        // such as hid.class overruling bootmouse & bootkeyboard.
        // so unbind those classes that promote themselves as AfterDOS

        psdLockReadPBase();
        psdAddErrorMsg0(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Checking AfterDOS...");
        puc = (struct PsdUsbClass *) ps->ps_Classes.lh_Head;
        while(puc->puc_Node.ln_Succ)
        {
            restartme = FALSE;
            usbGetAttrs(UGA_CLASS, NULL,
                        UCCA_AfterDOSRestart, &restartme,
                        TAG_END);

            if(restartme && puc->puc_UseCnt)
            {
                struct PsdDevice *pd;
                struct PsdConfig *pc;
                struct PsdInterface *pif;

                /* Well, try to release the open bindings in a best effort attempt */
                pd = NULL;
                while((pd = psdGetNextDevice(pd)))
                {
                    if(pd->pd_DevBinding && (pd->pd_ClsBinding == puc) && (!(pd->pd_Flags & PDFF_APPBINDING)))
                    {
                        psdUnlockPBase();
                        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "AfterDOS: Temporarily releasing %s %s binding to %s.",
                                       puc->puc_ClassName, "device", pd->pd_ProductStr);
                        psdReleaseDevBinding(pd);
                        psdLockReadPBase();
                        pd = NULL; /* restart */
                        continue;
                    }
                    pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
                    while(pc->pc_Node.ln_Succ)
                    {
                        pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                        while(pif->pif_Node.ln_Succ)
                        {
                            if(pif->pif_IfBinding && (pif->pif_ClsBinding == puc))
                            {
                                psdUnlockPBase();
                                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                               "AfterDOS: Temporarily releasing %s %s binding to %s.",
                                               puc->puc_ClassName, "interface", pd->pd_ProductStr);
                                psdReleaseIfBinding(pif);
                                psdLockReadPBase();
                                pd = NULL; /* restart */
                                continue;
                            }
                            pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
                        }
                        pc = (struct PsdConfig *) pc->pc_Node.ln_Succ;
                    }
                }
            }
            usbDoMethodA(UCM_DOSAvailableEvent, NULL);
            puc = (struct PsdUsbClass *) puc->puc_Node.ln_Succ;
        }
        ps->ps_StartedAsTask = FALSE;
        psdUnlockPBase();
    }

    if(nodos && (!ps->ps_ConfigRead))
    {
        // it's the first time we were reading the config and DOS was not available
        ps->ps_StartedAsTask = TRUE;
    }
    ps->ps_ConfigRead = TRUE;
    ps->ps_SavedConfigHash = ps->ps_ConfigHash; // update saved hash

    /* do a class scan */
    psdClassScan();

    if(nodos && ps->ps_GlobalCfg->pgc_BootDelay)
    {
        // wait for hubs to settle
        psdDelayMS(1000);
        puc = (struct PsdUsbClass *) FindName(&ps->ps_Classes, "massstorage.class");
        if(puc && puc->puc_UseCnt)
        {
            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "Delaying further execution by %ld second(s) (boot delay).",
                               ps->ps_GlobalCfg->pgc_BootDelay);
            if(ps->ps_GlobalCfg->pgc_BootDelay >= 1);
            {
                psdDelayMS((ps->ps_GlobalCfg->pgc_BootDelay-1)*1000);
            }
        } else {
            psdAddErrorMsg0(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Boot delay skipped, no mass storage devices found.");
        }
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdSetClsCfg()" */
AROS_LH2(BOOL, psdSetClsCfg,
         AROS_LHA(STRPTR, owner, A0),
         AROS_LHA(APTR, form, A1),
         LIBBASETYPEPTR, ps, 62, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdIFFContext *pic;
    BOOL result = FALSE;

    KPRINTF(10, ("psdSetClsCfg(%s, %p)\n", owner, form));
    pLockSemExcl(ps, &ps->ps_ConfigLock);
    pic = psdFindCfgForm(NULL, IFFFORM_CLASSCFG);
    while(pic)
    {
        if(pMatchStringChunk(ps, pic, IFFCHNK_OWNER, owner))
        {
            pic = psdFindCfgForm(pic, IFFFORM_CLASSDATA);
            if(pic)
            {
                if(form)
                {
                    result = psdReadCfg(pic, form);
                } else {
                    psdRemCfgChunk(pic, 0);
                    result = TRUE;
                }
                break;
            } else {
                break;
            }
        }
        pic = psdNextCfgForm(pic);
    }
    if(result)
    {
        pUnlockSem(ps, &ps->ps_ConfigLock);
        pCheckCfgChanged(ps);
        return(result);
    }
    pic = (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head;
    if(pic->pic_Node.ln_Succ)
    {
        pic = pAllocForm(ps, pic, IFFFORM_CLASSCFG);
        if(pic)
        {
            if(pAddStringChunk(ps, pic, IFFCHNK_OWNER, owner))
            {
                if(form)
                {
                    if(pAddCfgChunk(ps, pic, form))
                    {
                        pUnlockSem(ps, &ps->ps_ConfigLock);
                        pCheckCfgChanged(ps);
                        return(TRUE);
                    }
                } else {
                    ULONG buf[3];
                    buf[0] = AROS_LONG2BE(ID_FORM);
                    buf[1] = AROS_LONG2BE(4);
                    buf[2] = AROS_LONG2BE(IFFFORM_CLASSDATA);
                    if(pAddCfgChunk(ps, pic, buf))
                    {
                        pUnlockSem(ps, &ps->ps_ConfigLock);
                        pCheckCfgChanged(ps);
                        return(TRUE);
                    }
                }
            }
        }
    }
    pUnlockSem(ps, &ps->ps_ConfigLock);
    pCheckCfgChanged(ps);
    return(FALSE);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdGetClsCfg()" */
AROS_LH1(struct PsdIFFContext *, psdGetClsCfg,
         AROS_LHA(STRPTR, owner, A0),
         LIBBASETYPEPTR, ps, 63, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdIFFContext *pic;

    KPRINTF(10, ("psdGetClsCfg(%s)\n", owner));
    pic = psdFindCfgForm(NULL, IFFFORM_CLASSCFG);
    while(pic)
    {
        if(pMatchStringChunk(ps, pic, IFFCHNK_OWNER, owner))
        {
            return(psdFindCfgForm(pic, IFFFORM_CLASSDATA));
        }
        pic = psdNextCfgForm(pic);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdSetUsbDevCfg()" */
AROS_LH4(BOOL, psdSetUsbDevCfg,
         AROS_LHA(STRPTR, owner, A0),
         AROS_LHA(STRPTR, devid, A2),
         AROS_LHA(STRPTR, ifid, A3),
         AROS_LHA(APTR, form, A1),
         LIBBASETYPEPTR, ps, 64, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdIFFContext *pic;
    struct PsdIFFContext *cpic = NULL;
    struct PsdIFFContext *mpic = NULL;
    BOOL result = FALSE;

    KPRINTF(10, ("psdSetUsbDevCfg(%s, %s, %s, %p)\n", owner, devid, ifid, form));
    pLockSemExcl(ps, &ps->ps_ConfigLock);
    /* Find device config form. It contains all device config data */
    pic = psdFindCfgForm(NULL, IFFFORM_DEVICECFG);
    while(pic)
    {
        /* Find DEVID-Chunk. Check if it matches our device id */
        if(pMatchStringChunk(ps, pic, IFFCHNK_DEVID, devid))
        {
            cpic = NULL;
            /* We found the correct device. Now if we need to store interface data, find the interface first */
            if(ifid)
            {
                /* Search interface config form */
                mpic = psdFindCfgForm(pic, IFFFORM_IFCFGDATA);
                while(mpic)
                {
                    /* Found the form. Find the the ID String for the interface */
                    if(pMatchStringChunk(ps, mpic, IFFCHNK_IFID, ifid))
                    {
                        /* ID did match, now check for owner */
                        if(pMatchStringChunk(ps, mpic, IFFCHNK_OWNER, owner))
                        {
                            /* found it! So there is already a config saved in there. Search for dev config data form */
                            cpic = psdFindCfgForm(mpic, IFFFORM_IFCLSDATA);
                            if(!cpic)
                            {
                                /* not found, generate it */
                                cpic = pAllocForm(ps, mpic, IFFFORM_IFCLSDATA);
                            }
                            break;
                        }
                    }
                    mpic = psdNextCfgForm(mpic);
                }
                if(!cpic)
                {
                    if((mpic = pAllocForm(ps, pic, IFFFORM_IFCFGDATA)))
                    {
                        if(pAddStringChunk(ps, mpic, IFFCHNK_OWNER, owner))
                        {
                            if(pAddStringChunk(ps, mpic, IFFCHNK_IFID, ifid))
                            {
                                cpic = pAllocForm(ps, mpic, IFFFORM_IFCLSDATA);
                            }
                        }
                    }
                }
            } else {
                /* Search for device config */
                mpic = psdFindCfgForm(pic, IFFFORM_DEVCFGDATA);
                while(mpic)
                {
                    /* search for the right owner */
                    if(pMatchStringChunk(ps, mpic, IFFCHNK_OWNER, owner))
                    {
                        /* found it! So there is already a config saved in there. Search for dev config data form */
                        cpic = psdFindCfgForm(mpic, IFFFORM_DEVCLSDATA);
                        if(!cpic)
                        {
                            /* not found, generate it */
                            cpic = pAllocForm(ps, mpic, IFFFORM_DEVCLSDATA);
                        }
                        break;
                    }
                    mpic = psdNextCfgForm(mpic);
                }
                if(!cpic) /* no device config form */
                {
                    if((mpic = pAllocForm(ps, pic, IFFFORM_DEVCFGDATA)))
                    {
                        if(pAddStringChunk(ps, mpic, IFFCHNK_OWNER, owner))
                        {
                            cpic = pAllocForm(ps, mpic, IFFFORM_DEVCLSDATA);
                        }
                    }
                }
            }
            if(cpic)
            {
                if(form)
                {
                    result = psdReadCfg(cpic, form);
                } else {
                    psdRemCfgChunk(cpic, 0);
                    result = TRUE;
                }
                break;
            }
        }
        pic = psdNextCfgForm(pic);
    }
    if(result)
    {
        pUnlockSem(ps, &ps->ps_ConfigLock);
        pCheckCfgChanged(ps);
        return(result);
    }
    cpic = NULL;
    pic = (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head;
    if(pic->pic_Node.ln_Succ)
    {
        pic = pAllocForm(ps, pic, IFFFORM_DEVICECFG);
        if(pic)
        {
            if(pAddStringChunk(ps, pic, IFFCHNK_DEVID, devid))
            {
                if(ifid)
                {
                    if((mpic = pAllocForm(ps, pic, IFFFORM_IFCFGDATA)))
                    {
                        if(pAddStringChunk(ps, mpic, IFFCHNK_OWNER, owner))
                        {
                            if(pAddStringChunk(ps, mpic, IFFCHNK_IFID, ifid))
                            {
                                cpic = pAllocForm(ps, mpic, IFFFORM_IFCLSDATA);
                            }
                        }
                    }
                } else {
                    if((mpic = pAllocForm(ps, pic, IFFFORM_DEVCFGDATA)))
                    {
                        if(pAddStringChunk(ps, mpic, IFFCHNK_OWNER, owner))
                        {
                            cpic = pAllocForm(ps, mpic, IFFFORM_DEVCLSDATA);
                        }
                    }
                }
                if(cpic)
                {
                    if(form)
                    {
                        result = psdReadCfg(cpic, form);
                    } else {
                        psdRemCfgChunk(cpic, 0);
                        result = TRUE;
                    }
                }
            }
        }
    }
    pUnlockSem(ps, &ps->ps_ConfigLock);
    pCheckCfgChanged(ps);
    return(result);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdGetUsbDevCfg()" */
AROS_LH3(struct PsdIFFContext *, psdGetUsbDevCfg,
         AROS_LHA(STRPTR, owner, A0),
         AROS_LHA(STRPTR, devid, A2),
         AROS_LHA(STRPTR, ifid, A3),
         LIBBASETYPEPTR, ps, 65, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdIFFContext *pic;
    struct PsdIFFContext *cpic = NULL;
    struct PsdIFFContext *mpic = NULL;

    KPRINTF(10, ("psdGetUsbDevCfg(%s, %s, %s)\n", owner, devid, ifid));
    pLockSemShared(ps, &ps->ps_ConfigLock);
    /* Find device config form. It contains all device config data */
    pic = psdFindCfgForm(NULL, IFFFORM_DEVICECFG);
    while(pic)
    {
        /* Find DEVID-Chunk. Check if it matches our device id */
        if(pMatchStringChunk(ps, pic, IFFCHNK_DEVID, devid))
        {
            cpic = NULL;
            /* We found the correct device. Now if we need to store interface data, find the interface first */
            if(ifid)
            {
                /* Search interface config form */
                mpic = psdFindCfgForm(pic, IFFFORM_IFCFGDATA);
                while(mpic)
                {
                    /* Found the form. Find the the ID String for the interface */
                    if(pMatchStringChunk(ps, mpic, IFFCHNK_IFID, ifid))
                    {
                        /* ID did match, now check for owner */
                        if(pMatchStringChunk(ps, mpic, IFFCHNK_OWNER, owner))
                        {
                            /* found it! So there is already a config saved in there. Search for dev config data form */
                            cpic = psdFindCfgForm(mpic, IFFFORM_IFCLSDATA);
                            break;
                        }
                    }
                    mpic = psdNextCfgForm(mpic);
                }
            } else {
                /* Search for device config */
                mpic = psdFindCfgForm(pic, IFFFORM_DEVCFGDATA);
                while(mpic)
                {
                    /* search for the right owner */
                    if(pMatchStringChunk(ps, mpic, IFFCHNK_OWNER, owner))
                    {
                        /* found it! So there is already a config saved in there. Search for dev config data form */
                        cpic = psdFindCfgForm(mpic, IFFFORM_DEVCLSDATA);
                        break;
                    }
                    mpic = psdNextCfgForm(mpic);
                }
            }
            break;
        }
        pic = psdNextCfgForm(pic);
    }
    pUnlockSem(ps, &ps->ps_ConfigLock);
    KPRINTF(1, ("Result %p\n", cpic));
    return(cpic);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdSetForcedBinding()" */
AROS_LH3(BOOL, psdSetForcedBinding,
         AROS_LHA(STRPTR, owner, A2),
         AROS_LHA(STRPTR, devid, A0),
         AROS_LHA(STRPTR, ifid, A1),
         LIBBASETYPEPTR, ps, 69, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdIFFContext *pic;
    struct PsdIFFContext *mpic = NULL;
    ULONG olen = 0;
    BOOL result = FALSE;

    if(owner)
    {
        olen = strlen(owner);
    }
    pLockSemExcl(ps, &ps->ps_ConfigLock);
    /* Find device config form. It contains all device config data */
    pic = psdFindCfgForm(NULL, IFFFORM_DEVICECFG);
    while(pic)
    {
        /* Find DEVID-Chunk. Check if it matches our device id */
        if(pMatchStringChunk(ps, pic, IFFCHNK_DEVID, devid))
        {
            /* We found the correct device. Now if we need to store interface data, find the interface first */
            if(ifid)
            {
                /* Search interface config form */
                mpic = psdFindCfgForm(pic, IFFFORM_IFCFGDATA);
                while(mpic)
                {
                    /* Found the form. Find the the ID String for the interface */
                    if(pMatchStringChunk(ps, mpic, IFFCHNK_IFID, ifid))
                    {
                        /* ID did match, insert/replace forced binding */
                        if(olen)
                        {
                            if(pAddStringChunk(ps, mpic, IFFCHNK_FORCEDBIND, owner))
                            {
                                result = TRUE;
                            }
                        } else {
                            pRemCfgChunk(ps, mpic, IFFCHNK_FORCEDBIND);
                            result = TRUE;
                        }
                    }
                    mpic = psdNextCfgForm(mpic);
                }
                if(!olen)
                {
                    result = TRUE;
                }
                if((!result) && olen)
                {
                    if((mpic = pAllocForm(ps, pic, IFFFORM_IFCFGDATA)))
                    {
                        if(pAddStringChunk(ps, mpic, IFFCHNK_OWNER, owner))
                        {
                            if(pAddStringChunk(ps, mpic, IFFCHNK_FORCEDBIND, owner))
                            {
                                if(pAddStringChunk(ps, mpic, IFFCHNK_IFID, ifid))
                                {
                                    result = TRUE;
                                }
                            }
                        }
                    }
                }
            } else {
                /* Add FBND chunk */
                if(olen)
                {
                    if(pAddStringChunk(ps, pic, IFFCHNK_FORCEDBIND, owner))
                    {
                        result = TRUE;
                    }
                } else {
                    pRemCfgChunk(ps, pic, IFFCHNK_FORCEDBIND);
                    result = TRUE;
                }
            }
            break;
        }
        pic = psdNextCfgForm(pic);
    }
    if(!olen)
    {
        result = TRUE;
    }
    if(result)
    {
        pUnlockSem(ps, &ps->ps_ConfigLock);
        pCheckCfgChanged(ps);
        return(result);
    }
    pic = (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head;
    if(pic->pic_Node.ln_Succ)
    {
        pic = pAllocForm(ps, pic, IFFFORM_DEVICECFG);
        if(pic)
        {
            if(pAddStringChunk(ps, pic, IFFCHNK_DEVID, devid))
            {
                if(ifid)
                {
                    if((mpic = pAllocForm(ps, pic, IFFFORM_IFCFGDATA)))
                    {
                        if(pAddStringChunk(ps, mpic, IFFCHNK_OWNER, owner))
                        {
                            if(pAddStringChunk(ps, mpic, IFFCHNK_FORCEDBIND, owner))
                            {
                                if(pAddStringChunk(ps, mpic, IFFCHNK_IFID, ifid))
                                {
                                    result = TRUE;
                                }
                            }
                        }
                    }
                } else {
                    /* Add FBND chunk */
                    if(pAddStringChunk(ps, pic, IFFCHNK_FORCEDBIND, owner))
                    {
                        result = TRUE;
                    }
                }
            }
        }
    }
    pUnlockSem(ps, &ps->ps_ConfigLock);
    pCheckCfgChanged(ps);
    return(result);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdGetForcedBinding()" */
AROS_LH2(STRPTR, psdGetForcedBinding,
         AROS_LHA(STRPTR, devid, A0),
         AROS_LHA(STRPTR, ifid, A1),
         LIBBASETYPEPTR, ps, 70, psd)
{
    AROS_LIBFUNC_INIT
    struct PsdIFFContext *pic;
    struct PsdIFFContext *mpic = NULL;
    ULONG *chunk;
    STRPTR owner = NULL;

    pLockSemShared(ps, &ps->ps_ConfigLock);
    /* Find device config form. It contains all device config data */
    pic = psdFindCfgForm(NULL, IFFFORM_DEVICECFG);
    while(pic)
    {
        /* Find DEVID-Chunk. Check if it matches our device id */
        if(pMatchStringChunk(ps, pic, IFFCHNK_DEVID, devid))
        {
            /* We found the correct device. Now if we need to store interface data, find the interface first */
            if(ifid)
            {
                /* Search interface config form */
                mpic = psdFindCfgForm(pic, IFFFORM_IFCFGDATA);
                while(mpic)
                {
                    /* Found the form. Find the the ID String for the interface */
                    if(pMatchStringChunk(ps, mpic, IFFCHNK_IFID, ifid))
                    {
                        /* ID did match, now check for forced binding */
                        chunk = pFindCfgChunk(ps, mpic, IFFCHNK_FORCEDBIND);
                        if(chunk)
                        {
                            owner = (STRPTR) &chunk[2];
                            break;
                        }
                    }
                    mpic = psdNextCfgForm(mpic);
                }
            } else {
                /* Search for device forced binding */
                chunk = pFindCfgChunk(ps, pic, IFFCHNK_FORCEDBIND);
                if(chunk)
                {
                    owner = (STRPTR) &chunk[2];
                    break;
                }
            }
            break;
        }
        pic = psdNextCfgForm(pic);
    }
    pUnlockSem(ps, &ps->ps_ConfigLock);
    return(owner);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdAddStringChunk()" */
AROS_LH3(BOOL, psdAddStringChunk,
         AROS_LHA(struct PsdIFFContext *, pic, A0),
         AROS_LHA(ULONG, chunkid, D0),
         AROS_LHA(CONST_STRPTR, str, A1),
         LIBBASETYPEPTR, ps, 87, psd)
{
    AROS_LIBFUNC_INIT
    BOOL res;
    KPRINTF(10, ("psdAddStringChunk(%p, %p, %s)\n", pic, chunkid, str));
    pLockSemExcl(ps, &ps->ps_ConfigLock);
    res = pAddStringChunk(ps, pic, chunkid, str);
    pUnlockSem(ps, &ps->ps_ConfigLock);
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdMatchStringChunk()" */
AROS_LH3(BOOL, psdMatchStringChunk,
         AROS_LHA(struct PsdIFFContext *, pic, A0),
         AROS_LHA(ULONG, chunkid, D0),
         AROS_LHA(CONST_STRPTR, str, A1),
         LIBBASETYPEPTR, ps, 88, psd)
{
    AROS_LIBFUNC_INIT
    BOOL res;
    KPRINTF(10, ("psdMatchStringChunk(%p, %p, %s)\n", pic, chunkid, str));
    pLockSemShared(ps, &ps->ps_ConfigLock);
    res = pMatchStringChunk(ps, pic, chunkid, str);
    pUnlockSem(ps, &ps->ps_ConfigLock);
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "psdGetStringChunk()" */
AROS_LH2(STRPTR, psdGetStringChunk,
         AROS_LHA(struct PsdIFFContext *, pic, A0),
         AROS_LHA(ULONG, chunkid, D0),
         LIBBASETYPEPTR, ps, 89, psd)
{
    AROS_LIBFUNC_INIT
    STRPTR str;
    KPRINTF(10, ("psdGetStringChunk(%p, %p)\n", pic, chunkid));
    pLockSemShared(ps, &ps->ps_ConfigLock);
    str = pGetStringChunk(ps, pic, chunkid);
    pUnlockSem(ps, &ps->ps_ConfigLock);
    return(str);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Configuration (non-library subroutines) *** */

/* /// "pAllocForm()" */
struct PsdIFFContext * pAllocForm(LIBBASETYPEPTR ps, struct PsdIFFContext *parent, ULONG formid)
{
    struct PsdIFFContext *pic;
    KPRINTF(10, ("pAllocForm(%p, %p)\n", parent, formid));
    if((pic = psdAllocVec(sizeof(struct PsdIFFContext))))
    {
        NewList(&pic->pic_SubForms);
        //pic->pic_Parent = parent;
        pic->pic_FormID = formid;
        pic->pic_FormLength = 4;
        pic->pic_Chunks = NULL;
        pic->pic_ChunksLen = 0;
        pic->pic_BufferLen = 0;
        Forbid();
        if(parent)
        {
            AddTail(&parent->pic_SubForms, &pic->pic_Node);
        } else {
            AddTail(&ps->ps_ConfigRoot, &pic->pic_Node);
        }
        Permit();
    }
    return(pic);
}
/* \\\ */

/* /// "pFreeForm()" */
void pFreeForm(LIBBASETYPEPTR ps, struct PsdIFFContext *pic)
{
    struct PsdIFFContext *subpic = (struct PsdIFFContext *) pic->pic_SubForms.lh_Head;
    KPRINTF(10, ("pFreeForm(%p)\n", pic));
    Remove(&pic->pic_Node);
    while(subpic->pic_Node.ln_Succ)
    {
        pFreeForm(ps, subpic);
        subpic = (struct PsdIFFContext *) pic->pic_SubForms.lh_Head;
    }
    psdFreeVec(pic->pic_Chunks);
    psdFreeVec(pic);
}
/* \\\ */

/* /// "pGetFormLength()" */
ULONG pGetFormLength(struct PsdIFFContext *pic)
{
    ULONG len = (5 + pic->pic_ChunksLen) & ~1UL;
    struct PsdIFFContext *subpic = (struct PsdIFFContext *) pic->pic_SubForms.lh_Head;
    //KPRINTF(10, ("pGetFormLength(%p)\n", pic));
    while(subpic->pic_Node.ln_Succ)
    {
        len += pGetFormLength(subpic);
        subpic = (struct PsdIFFContext *) subpic->pic_Node.ln_Succ;
    }
    pic->pic_FormLength = len;
    //KPRINTF(10, ("FormLen=%ld\n", len+8));
    return(len + 8);
}
/* \\\ */

/* /// "pFindCfgChunk()" */
APTR pFindCfgChunk(LIBBASETYPEPTR ps, struct PsdIFFContext *pic, ULONG chnkid)
{
    ULONG *buf = pic->pic_Chunks;
    ULONG len = pic->pic_ChunksLen;
    ULONG chlen;
    KPRINTF(10, ("pFindCfgChunk(%p, %p)\n", pic, chnkid));

    while(len)
    {
        if(AROS_LONG2BE(*buf) == chnkid)
        {
            KPRINTF(10, ("Found at %p\n", buf));
            return(buf);
        }
        chlen = (AROS_LONG2BE(buf[1]) + 9) & ~1UL;
        len -= chlen;
        buf = (ULONG *) (((UBYTE *) buf) + chlen);
    }
    KPRINTF(10, ("Not found!\n"));
    return(NULL);
}
/* \\\ */

/* /// "pRemCfgChunk()" */
BOOL pRemCfgChunk(LIBBASETYPEPTR ps, struct PsdIFFContext *pic, ULONG chnkid)
{
    ULONG *buf = pic->pic_Chunks;
    ULONG len = pic->pic_ChunksLen;
    ULONG chlen;
    KPRINTF(10, ("pRemCfgChunk(%p, %p)\n", pic, chnkid));

    while(len)
    {
        chlen = ((AROS_LONG2BE(buf[1])) + 9) & ~1UL;
        if(AROS_LONG2BE(*buf) == chnkid)
        {
            len -= chlen;
            if(len)
            {
                memcpy(buf, &((UBYTE *) buf)[chlen], (size_t) len);
            }
            pic->pic_ChunksLen -= chlen;
            KPRINTF(10, ("Deleted %ld bytes to %ld chunk len\n", chlen, pic->pic_ChunksLen));
            return(TRUE);
        }
        len -= chlen;
        buf = (ULONG *) (((UBYTE *) buf) + chlen);
    }
    KPRINTF(10, ("Not found!\n"));
    return(FALSE);
}
/* \\\ */

/* /// "pAddCfgChunk()" */
struct PsdIFFContext * pAddCfgChunk(LIBBASETYPEPTR ps, struct PsdIFFContext *pic, APTR chunk)
{
    LONG len;
    LONG chlen;
    ULONG *buf = chunk;
    ULONG *newbuf;
    struct PsdIFFContext *subpic;
    KPRINTF(10, ("pAddCfgChunk(%p, %p)\n", pic, chunk));
    if(AROS_LONG2BE(*buf) == ID_FORM)
    {
        buf++;
        len = ((AROS_LONG2BE(*buf)) - 3) & ~1UL;
        buf++;
        if((subpic = pAllocForm(ps, pic, AROS_LONG2BE(*buf))))
        {
            buf++;
            while(len >= 8)
            {
                if(!(pAddCfgChunk(ps, subpic, buf)))
                {
                    break;
                }
                chlen = (AROS_LONG2BE(buf[1]) + 9) & ~1UL;
                len -= chlen;
                buf = (ULONG *) (((UBYTE *) buf) + chlen);
            }
            if(len)
            {
                psdAddErrorMsg0(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Tried to add a nasty corrupted FORM chunk! Configuration is probably b0rken!");
                return(NULL);
            }
        } else {
            return(NULL);
        }
        return(subpic);
    } else {
        pRemCfgChunk(ps, pic, AROS_LONG2BE(*buf));
        len = (AROS_LONG2BE(buf[1]) + 9) & ~1UL;
        if(pic->pic_ChunksLen+len > pic->pic_BufferLen)
        {
            KPRINTF(10, ("expanding buffer from %ld to %ld to fit %ld bytes\n", pic->pic_BufferLen, (pic->pic_ChunksLen+len)<<1, pic->pic_ChunksLen+len));

            /* Expand buffer */
            if((newbuf = psdAllocVec((pic->pic_ChunksLen+len)<<1)))
            {
                if(pic->pic_ChunksLen)
                {
                    memcpy(newbuf, pic->pic_Chunks, (size_t) pic->pic_ChunksLen);
                    psdFreeVec(pic->pic_Chunks);
                }
                pic->pic_Chunks = newbuf;
                pic->pic_BufferLen = (pic->pic_ChunksLen+len)<<1;
            } else {
                return(NULL);
            }
        }
        memcpy(&(((UBYTE *) pic->pic_Chunks)[pic->pic_ChunksLen]), chunk, (size_t) len);
        pic->pic_ChunksLen += len;
        return(pic);
    }
}
/* \\\ */

/* /// "pInternalWriteForm()" */
ULONG * pInternalWriteForm(struct PsdIFFContext *pic, ULONG *buf)
{
    struct PsdIFFContext *subpic = (struct PsdIFFContext *) pic->pic_SubForms.lh_Head;
    //KPRINTF(10, ("pInternalWriteForm(%p, %p)", pic, buf));
    *buf++ = AROS_LONG2BE(ID_FORM);
    *buf++ = AROS_LONG2BE(pic->pic_FormLength);
    *buf++ = AROS_LONG2BE(pic->pic_FormID);
    if(pic->pic_ChunksLen)
    {
        memcpy(buf, pic->pic_Chunks, (size_t) pic->pic_ChunksLen);
        buf = (ULONG *) (((UBYTE *) buf) + pic->pic_ChunksLen);
    }
    while(subpic->pic_Node.ln_Succ)
    {
        buf = pInternalWriteForm(subpic, buf);
        subpic = (struct PsdIFFContext *) subpic->pic_Node.ln_Succ;
    }
    return(buf);
}
/* \\\ */

/* /// "pCalcCfgCRC()" */
ULONG pCalcCfgCRC(struct PsdIFFContext *pic)
{
    struct PsdIFFContext *subpic = (struct PsdIFFContext *) pic->pic_SubForms.lh_Head;
    ULONG len;
    ULONG crc = pic->pic_FormID;
    UWORD *ptr;

    //KPRINTF(10, ("pInternalWriteForm(%p, %p)", pic, buf));
    if(pic->pic_ChunksLen)
    {
        len = pic->pic_ChunksLen>>1;
        if(len)
        {
            ptr = (UWORD *) pic->pic_Chunks;
            do
            {
                crc = ((crc<<1)|(crc>>31))^(*ptr++);
            } while(--len);
        }
    }
    while(subpic->pic_Node.ln_Succ)
    {
        crc ^= pCalcCfgCRC(subpic);
        subpic = (struct PsdIFFContext *) subpic->pic_Node.ln_Succ;
    }
    return(crc);
}
/* \\\ */

/* /// "pCheckCfgChanged()" */
BOOL pCheckCfgChanged(LIBBASETYPEPTR ps)
{
    ULONG crc;
    struct PsdIFFContext *pic;
    struct PsdIFFContext *subpic;
    STRPTR tmpstr;

    pLockSemShared(ps, &ps->ps_ConfigLock);
    ps->ps_CheckConfigReq = FALSE;
    pic = (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head;
    if(!(pic->pic_Node.ln_Succ))
    {
        pUnlockSem(ps, &ps->ps_ConfigLock);
        return(FALSE);
    }
    crc = pCalcCfgCRC(pic);
    if(crc != ps->ps_ConfigHash)
    {
        ULONG *chnk;
        ps->ps_ConfigHash = crc;
        /* Get Global config */
        if((subpic = psdFindCfgForm(pic, IFFFORM_STACKCFG)))
        {
            if((chnk = pFindCfgChunk(ps, subpic, IFFCHNK_GLOBALCFG)))
            {
                CopyMem(&chnk[2], ((UBYTE *) ps->ps_GlobalCfg) + 8, min(AROS_LONG2BE(chnk[1]), AROS_LONG2BE(ps->ps_GlobalCfg->pgc_Length)));
            }
            if(!pMatchStringChunk(ps, subpic, IFFCHNK_INSERTSND, ps->ps_PoPo.po_InsertSndFile))
            {
                if((tmpstr = pGetStringChunk(ps, subpic, IFFCHNK_INSERTSND)))
                {
                    psdFreeVec(ps->ps_PoPo.po_InsertSndFile);
                    ps->ps_PoPo.po_InsertSndFile = tmpstr;
                }
            }
            if(!pMatchStringChunk(ps, subpic, IFFCHNK_REMOVESND, ps->ps_PoPo.po_RemoveSndFile))
            {
                if((tmpstr = pGetStringChunk(ps, subpic, IFFCHNK_REMOVESND)))
                {
                    psdFreeVec(ps->ps_PoPo.po_RemoveSndFile);
                    ps->ps_PoPo.po_RemoveSndFile = tmpstr;
                }
            }
        }
        pUnlockSem(ps, &ps->ps_ConfigLock);
        psdSendEvent(EHMB_CONFIGCHG, NULL, NULL);
        return(TRUE);
    }
    pUnlockSem(ps, &ps->ps_ConfigLock);
    return(FALSE);
}
/* \\\ */

/* /// "pAddStringChunk()" */
BOOL pAddStringChunk(LIBBASETYPEPTR ps, struct PsdIFFContext *pic, ULONG chunkid, CONST_STRPTR str)
{
    BOOL res = FALSE;
    ULONG len = strlen(str);
    ULONG *chnk = (ULONG *) psdAllocVec((ULONG) len+8+2);
    if(chnk)
    {
        chnk[0] = AROS_LONG2BE(chunkid);
        chnk[1] = AROS_LONG2BE(len+1);
        strcpy((STRPTR) &chnk[2], str);
        if(pAddCfgChunk(ps, pic, chnk))
        {
            res = TRUE;
        }
        psdFreeVec(chnk);
    }
    return(res);
}
/* \\\ */

/* /// "pMatchStringChunk()" */
BOOL pMatchStringChunk(LIBBASETYPEPTR ps, struct PsdIFFContext *pic, ULONG chunkid, CONST_STRPTR str)
{
    ULONG *chunk;
    ULONG len;
    STRPTR srcptr;
    if((chunk = pFindCfgChunk(ps, pic, chunkid)))
    {
        srcptr = (STRPTR) &chunk[2];
        len = AROS_LONG2BE(chunk[1]);
        while(len-- && *srcptr)
        {
            if(*str++ != *srcptr++)
            {
                return(FALSE);
            }
        }
        if(!*str)
        {
            return(TRUE);
        }
    }
    return(FALSE);
}
/* \\\ */

/* /// "pGetStringChunk()" */
STRPTR pGetStringChunk(LIBBASETYPEPTR ps, struct PsdIFFContext *pic, ULONG chunkid)
{
    ULONG *chunk;
    STRPTR str;
    if((chunk = pFindCfgChunk(ps, pic, chunkid)))
    {
        if((str = (STRPTR) psdAllocVec(AROS_LONG2BE(chunk[1]) + 1)))
        {
            memcpy(str, &chunk[2], (size_t) AROS_LONG2BE(chunk[1]));
            return(str);
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "pUpdateGlobalCfg()" */
void pUpdateGlobalCfg(LIBBASETYPEPTR ps, struct PsdIFFContext *pic)
{
    struct PsdIFFContext *tmppic;
    /* Set Global config */
    if(pic == (struct PsdIFFContext *) ps->ps_ConfigRoot.lh_Head)
    {
        if((tmppic = psdFindCfgForm(NULL, IFFFORM_STACKCFG)))
        {
            pAddCfgChunk(ps, tmppic, ps->ps_GlobalCfg);
            pAddStringChunk(ps, tmppic, IFFCHNK_INSERTSND, ps->ps_PoPo.po_InsertSndFile);
            pAddStringChunk(ps, tmppic, IFFCHNK_REMOVESND, ps->ps_PoPo.po_RemoveSndFile);
        }
    }
}
/* \\\ */

/* *** Misc (non library functions) ***/

/* /// "pGetDevConfig()" */
BOOL pGetDevConfig(struct PsdPipe *pp)
{
    struct PsdDevice *pd = pp->pp_Device;
    LIBBASETYPEPTR ps = pd->pd_Hardware->phw_Base;
    UBYTE *tempbuf;
    struct UsbStdCfgDesc uscd;
    ULONG len;
    LONG ioerr;
    STRPTR classname;
    UWORD curcfg = 0;

    KPRINTF(1, ("Getting configuration descriptor...\n"));
    psdLockWriteDevice(pd);
    while(curcfg < pd->pd_NumCfgs)
    {
        psdPipeSetup(pp, URTF_IN|URTF_STANDARD|URTF_DEVICE,
                     USR_GET_DESCRIPTOR, (UDT_CONFIGURATION<<8)|curcfg, 0);

        /*tempbuf = psdAllocVec(256);
        ioerr = psdDoPipe(pp, tempbuf, 34);
        if(ioerr == UHIOERR_RUNTPACKET)
        {
            ioerr = 0;
        }
        memcpy(&uscd, tempbuf, 9);*/
        ioerr = psdDoPipe(pp, &uscd, 9);//sizeof(struct UsbStdCfgDesc));
        if(!ioerr)
        {
            KPRINTF(1, ("Config type: %ld\n", (ULONG) uscd.bDescriptorType));
            len = (ULONG) AROS_WORD2LE(uscd.wTotalLength);
            KPRINTF(1, ("Configsize %ld, total size %ld\n", (ULONG) uscd.bLength, len));
            if((tempbuf = psdAllocVec(len)))
            //if(1)
            {
                KPRINTF(1, ("Getting whole configuration descriptor...\n"));
                ioerr = psdDoPipe(pp, tempbuf, len);
                if(!ioerr)
                {
                    struct PsdConfig *pc = NULL;
                    struct PsdInterface *pif = NULL;
                    struct PsdInterface *altif = NULL;
                    struct PsdEndpoint *pep = NULL;
                    struct PsdDescriptor *pdd = NULL;
                    UBYTE *dbuf = tempbuf;
                    UBYTE *bufend;
                    ULONG dlen;
                    bufend = &dbuf[len];
                    while(dbuf < bufend)
                    {
                        dlen = dbuf[0]; /* bLength */
                        if(dlen < 2)
                        {
                            break;
                        }
                        if(&dbuf[dlen] > bufend)
                        {
                            psdAddErrorMsg0(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "End of descriptor past buffer!");
                        }
                        switch(dbuf[1]) /* bDescriptorType */
                        {
                            case UDT_CONFIGURATION:
                            {
                                struct UsbStdCfgDesc *usc = (struct UsbStdCfgDesc *) dbuf;
                                pif = NULL;
                                altif = NULL;
                                pep = NULL;
                                if((pc = pAllocConfig(pd)))
                                {
                                    pd->pd_Flags |= PDFF_CONFIGURED;
                                    pc->pc_NumIfs = usc->bNumInterfaces;
                                    pc->pc_CfgNum = usc->bConfigurationValue;
                                    pc->pc_Attr   = usc->bmAttributes;
                                    pc->pc_MaxPower = usc->bMaxPower<<1;

                                    KPRINTF(1, ("  Config %ld\n", pc->pc_CfgNum));
                                    if(usc->iConfiguration)
                                    {
                                        pc->pc_CfgStr = psdGetStringDescriptor(pp, usc->iConfiguration);
                                    }
                                    if(!pc->pc_CfgStr)
                                    {
                                        pc->pc_CfgStr = psdCopyStrFmt("Configuration %ld", pc->pc_CfgNum);
                                    }
                                } else {
                                    KPRINTF(20, ("  Config allocation failed\n"));
                                }
                                break;
                            }

                            case UDT_INTERFACE:
                            {
                                struct UsbStdIfDesc *usif = (struct UsbStdIfDesc *) dbuf;
                                pep = NULL;
                                if(pc)
                                {
                                    if((altif = pAllocInterface(pc)))
                                    {
                                        altif->pif_IfNum = usif->bInterfaceNumber;
                                        altif->pif_Alternate = usif->bAlternateSetting;
                                        altif->pif_NumEPs = usif->bNumEndpoints;
                                        altif->pif_IfClass = usif->bInterfaceClass;
                                        altif->pif_IfSubClass = usif->bInterfaceSubClass;
                                        altif->pif_IfProto = usif->bInterfaceProtocol;
                                        KPRINTF(2, ("    Interface %ld\n", altif->pif_IfNum));
                                        if(usif->iInterface)
                                        {
                                            altif->pif_IfStr = psdGetStringDescriptor(pp, usif->iInterface);
                                        }
                                        if(!altif->pif_IfStr)
                                        {
                                            classname = psdNumToStr(NTS_CLASSCODE, (LONG) altif->pif_IfClass, NULL);
                                            if(classname)
                                            {
                                                altif->pif_IfStr = psdCopyStrFmt("%s interface (%ld)", classname, altif->pif_IfNum);
                                            } else {
                                                altif->pif_IfStr = psdCopyStrFmt("Interface %ld", altif->pif_IfNum);
                                            }
                                        }
                                        KPRINTF(2, ("      IfName    : %s\n"
                                                    "      Alternate : %ld\n"
                                                    "      NumEPs    : %ld\n"
                                                    "      IfClass   : %ld\n"
                                                    "      IfSubClass: %ld\n"
                                                    "      IfProto   : %ld\n",
                                                    altif->pif_IfStr, altif->pif_Alternate,
                                                    altif->pif_NumEPs,
                                                    altif->pif_IfClass,
                                                    altif->pif_IfSubClass, altif->pif_IfProto));
                                        if(pc->pc_CfgNum == 1)
                                        {
                                            altif->pif_IDString = psdCopyStrFmt("%02lx-%02lx-%02lx-%02lx-%02lx",
                                                                                altif->pif_IfNum, altif->pif_Alternate,
                                                                                altif->pif_IfClass, altif->pif_IfSubClass,
                                                                                altif->pif_IfProto);
                                        } else {
                                            // for more than one config, add config number (retain backwards compatibility with most devices)
                                            altif->pif_IDString = psdCopyStrFmt("%02lx-%02lx-%02lx-%02lx-%02lx-%02lx",
                                                                                pc->pc_CfgNum,
                                                                                altif->pif_IfNum, altif->pif_Alternate,
                                                                                altif->pif_IfClass, altif->pif_IfSubClass,
                                                                                altif->pif_IfProto);
                                        }

                                        /* Move the interface to the alternatives if possible */
                                        if(altif->pif_Alternate)
                                        {
                                            if(!pif)
                                            {
                                                psdAddErrorMsg0(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Alternate interface without prior main interface!");
                                                KPRINTF(20, ("    Alternate interface without prior main interface\n"));
                                                pif = altif;
                                            } else {
                                                Remove(&altif->pif_Node);
                                                AddTail(&pif->pif_AlterIfs, &altif->pif_Node);
                                                altif->pif_ParentIf = pif;
                                            }
                                        } else {
                                            altif->pif_ParentIf = NULL;
                                            pif = altif;
                                        }
                                    } else {
                                        KPRINTF(20, ("    Interface allocation failed\n"));
                                    }
                                } else {
                                    psdAddErrorMsg0(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Interface without prior config descriptor!");
                                    KPRINTF(20, ("    Interface descriptor without Config\n"));
                                }
                                break;
                            }

                            case UDT_ENDPOINT:
                            {
                                struct UsbStdEPDesc *usep = (struct UsbStdEPDesc *) dbuf;
                                if(altif)
                                {
                                    if((pep = pAllocEndpoint(altif)))
                                    {
                                        STRPTR eptype;
                                        pep->pep_EPNum = usep->bEndpointAddress & 0x0f;
                                        pep->pep_Direction = usep->bEndpointAddress>>7;
                                        pep->pep_TransType = usep->bmAttributes & 0x03;
                                        pep->pep_SyncType = (usep->bmAttributes>>2) & 0x03;
                                        pep->pep_UsageType = (usep->bmAttributes>>4) & 0x03;
                                        eptype = (pep->pep_TransType == USEAF_INTERRUPT) ? "int" : "iso";

                                        pep->pep_MaxPktSize = AROS_WORD2LE(usep->wMaxPacketSize) & 0x07ff;
                                        pep->pep_NumTransMuFr = ((AROS_WORD2LE(usep->wMaxPacketSize)>>11) & 3) + 1;
                                        if(pep->pep_NumTransMuFr == 4)
                                        {
                                            psdAddErrorMsg0(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Endpoint contains illegal Num Trans µFrame value!");
                                            pep->pep_NumTransMuFr = 1;
                                        }

                                        pep->pep_Interval = usep->bInterval;
                                        if(pd->pd_Flags & PDFF_HIGHSPEED)
                                        {
                                            switch(pep->pep_TransType)
                                            {
                                                case USEAF_CONTROL:
                                                case USEAF_BULK:
                                                    //pep->pep_Interval = 0; // no use here, NAK rate not of interest
                                                    break;

                                                case USEAF_ISOCHRONOUS:
                                                    if(pep->pep_MaxPktSize > 1024)
                                                    {
                                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                                       "Endpoint contains %s (%ld) MaxPktSize value!",
                                                                       (STRPTR) "too high", pep->pep_MaxPktSize);
                                                        pep->pep_MaxPktSize = 1024;
                                                    }

                                                case USEAF_INTERRUPT:
                                                    if(!pep->pep_Interval)
                                                    {
                                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                                       "%sspeed %s endpoint contains %s interval value! Fixing.",
                                                                       (STRPTR) "High", eptype, (STRPTR) "zero");
                                                        pep->pep_Interval = 1;
                                                    }
                                                    else if(pep->pep_Interval > 16)
                                                    {
                                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                                       "%sspeed %s endpoint contains %s interval value! Fixing.",
                                                                       (STRPTR) "High", eptype, (STRPTR) "too high");
                                                        pep->pep_Interval = 16;
                                                    }
                                                    pep->pep_Interval = 1<<(pep->pep_Interval-1);
                                                    break;
                                            }
                                        }
                                        else if(pd->pd_Flags & PDFF_LOWSPEED)
                                        {
                                            switch(pep->pep_TransType)
                                            {
                                                case USEAF_INTERRUPT:
                                                    if(pep->pep_Interval < 8)
                                                    {
                                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "%sspeed %s endpoint contains %s interval value! Fixing.",
                                                                       (STRPTR) "Low", eptype, (STRPTR) "too low");
                                                        pep->pep_Interval = 8;
                                                    }
                                                    break;

                                                case USEAF_CONTROL:
                                                case USEAF_BULK:
                                                    pep->pep_Interval = 0; // no use here
                                                    break;

                                                case USEAF_ISOCHRONOUS:
                                                    psdAddErrorMsg0(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Lowspeed devices cannot have isochronous endpoints!");
                                                    break;
                                            }
                                        } else {
                                            switch(pep->pep_TransType)
                                            {
                                                case USEAF_INTERRUPT:
                                                    if(!pep->pep_Interval)
                                                    {
                                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "%sspeed %s endpoint contains %s interval value! Fixing.",
                                                                       (STRPTR) "Full", eptype, (STRPTR) "zero");
                                                        pep->pep_Interval = 1;
                                                    }
                                                    break;

                                                case USEAF_CONTROL:
                                                case USEAF_BULK:
                                                    pep->pep_Interval = 0; // no use here
                                                    break;

                                                case USEAF_ISOCHRONOUS:
                                                    if(pep->pep_MaxPktSize > 1023)
                                                    {
                                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Endpoint contains too high (%ld) MaxPktSize value! Fixing.", pep->pep_MaxPktSize);
                                                        pep->pep_MaxPktSize = 1023;
                                                    }
                                                    if(!pep->pep_Interval)
                                                    {
                                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "%sspeed %s endpoint contains %s interval value! Fixing.",
                                                                       (STRPTR) "Full", eptype, (STRPTR) "zero");
                                                        pep->pep_Interval = 1;
                                                    }
                                                    else if(pep->pep_Interval > 16)
                                                    {
                                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "%sspeed %s endpoint contains %s interval value! Fixing.",
                                                                       (STRPTR) "Full", eptype, (STRPTR) "too high");
                                                        pep->pep_Interval = 16;
                                                    }
                                                    pep->pep_Interval = 1<<(pep->pep_Interval-1);
                                                    break;
                                            }
                                        }

                                        KPRINTF(2, ("      Endpoint %ld\n", pep->pep_EPNum));
                                        KPRINTF(2, ("        Direction : %s\n"
                                                    "        TransType : %ld\n"
                                                    "        MaxPktSize: %ld\n"
                                                    "        Interval  : %ld\n",
                                                    (pep->pep_Direction ? "IN" : "OUT"),
                                                    pep->pep_TransType, pep->pep_MaxPktSize,
                                                    pep->pep_Interval));

                                    } else {
                                        KPRINTF(20, ("      Endpoint allocation failed\n"));
                                    }
                                } else {
                                    psdAddErrorMsg0(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Endpoint without prior interface descriptor!");
                                    KPRINTF(20, ("      Endpoint descriptor without Interface\n"));
                                }
                                break;
                            }

                            case UDT_DEVICE:
                            case UDT_HUB:
                            case UDT_HID:
                            case UDT_REPORT:
                            case UDT_PHYSICAL:
                            case UDT_CS_INTERFACE:
                            case UDT_CS_ENDPOINT:
                            case UDT_DEVICE_QUALIFIER:
                            case UDT_OTHERSPEED_QUALIFIER:
                            case UDT_INTERFACE_POWER:
                            case UDT_OTG:
                                //psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Skipping descriptor %02lx (pc=%p, pif=%p altpif=%p).", dbuf[1], pc, pif, altif);
                                KPRINTF(1, ("Skipping unknown descriptor %ld.\n", dbuf[1]));
                                break;

                            default:
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Skipping unknown descriptor %02lx.", dbuf[1]);
                                KPRINTF(1, ("Skipping unknown descriptor %ld.\n", dbuf[1]));
                                break;
                        }
                        // add descriptor to device
                        pdd = pAllocDescriptor(pd, dbuf);
                        if(pdd)
                        {
                            STRPTR descname = NULL;

                            pdd->pdd_Config = pc;
                            pdd->pdd_Interface = altif;
                            pdd->pdd_Endpoint = pep;
                            if(pdd->pdd_Interface)
                            {
                                if((pdd->pdd_Type >= UDT_CS_UNDEFINED) && (pdd->pdd_Type <= UDT_CS_ENDPOINT))
                                {
                                    descname = psdNumToStr(NTS_DESCRIPTOR, (LONG) (pdd->pdd_CSSubType<<24)|(pif->pif_IfSubClass<<16)|(pif->pif_IfClass<<8)|pdd->pdd_Type, NULL);
                                    if(!descname)
                                    {
                                        descname = psdNumToStr(NTS_DESCRIPTOR, (LONG) (pdd->pdd_CSSubType<<24)|(pif->pif_IfClass<<8)|pdd->pdd_Type, NULL);
                                    }
                                }
                                if(!descname)
                                {
                                    descname = psdNumToStr(NTS_DESCRIPTOR, (LONG) (pif->pif_IfSubClass<<16)|(pif->pif_IfClass<<8)|pdd->pdd_Type, NULL);
                                }
                                if(!descname)
                                {
                                    descname = psdNumToStr(NTS_DESCRIPTOR, (LONG) (pif->pif_IfClass<<8)|pdd->pdd_Type, NULL);
                                }
                            }
                            if(descname)
                            {
                                pdd->pdd_Name = descname;
                            }
                        }
                        dbuf += dlen;
                    }
                    KPRINTF(1, ("Configuration acquired!\n"));
                    psdFreeVec(tempbuf);
                    curcfg++;
                    continue;
                    //psdUnlockDevice(pd);
                    //return(TRUE);
                } else {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "GET_DESCRIPTOR (len %ld) failed: %s (%ld)",
                                   len, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    KPRINTF(15, ("GET_DESCRIPTOR failed %ld!\n", ioerr));
                }
                psdFreeVec(tempbuf);
            } else {
                KPRINTF(20, ("No memory for %ld bytes config temp buffer!\n", len));
            }
        } else {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "GET_DESCRIPTOR (len %ld) failed: %s (%ld)",
                           9, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            KPRINTF(15, ("GET_DESCRIPTOR (9) failed %ld!\n", ioerr));
        }
        psdUnlockDevice(pd);
        return(FALSE);
    }
    psdUnlockDevice(pd);
    return(TRUE);
}
/* \\\ */

/* /// "pPowerRecurseDrain()" */
ULONG pPowerRecurseDrain(LIBBASETYPEPTR ps, struct PsdDevice *pd)
{
    struct PsdDevice *nextpd;
    struct PsdConfig *pc;
    UWORD maxdrain = 666;
    UWORD childdrain;
    BOOL selfpwd = TRUE;
    pd->pd_PowerDrain = 0;

    /* look at config */
    if((pc = pd->pd_CurrentConfig))
    {

        /* if suspended, no more than 500µA are drained */
        if(pd->pd_Flags & PDFF_SUSPENDED)
        {
            pd->pd_PowerDrain = (pc->pc_MaxPower >= 100) ? 3 : 1;
            return(pd->pd_PowerDrain);
        }
        selfpwd = ((pc->pc_Attr & USCAF_SELF_POWERED) && (pd->pd_PoPoCfg.poc_OverridePowerInfo != POCP_BUS_POWERED)) ||
                  (pd->pd_PoPoCfg.poc_OverridePowerInfo == POCP_SELF_POWERED);
        maxdrain = selfpwd ? 500 : 100;
    }

    /* examine children */
    nextpd = (struct PsdDevice *) pd->pd_Hardware->phw_Devices.lh_Head;
    while(nextpd->pd_Node.ln_Succ)
    {
        if(nextpd->pd_Hub == pd)
        {
            childdrain = pPowerRecurseDrain(ps, nextpd);
            // limit the drain to the maximum power suckage
            pd->pd_PowerDrain += (childdrain > maxdrain) ? maxdrain : childdrain;
        }
        nextpd = (struct PsdDevice *) nextpd->pd_Node.ln_Succ;
    }

    /* look at config */
    if(selfpwd)
    {
        pd->pd_PowerDrain = 0;
    } else {
        pd->pd_PowerDrain += pc->pc_MaxPower;
    }
    return(pd->pd_PowerDrain);
}
/* \\\ */

/* /// "pPowerRecurseSupply()" */
void pPowerRecurseSupply(LIBBASETYPEPTR ps, struct PsdDevice *pd)
{
    struct PsdDevice *nextpd;
    struct PsdConfig *pc;
    UWORD ports = 0;
    UWORD supply = 666;
    BOOL selfpwd = TRUE;

    /* look at config */
    if((pc = pd->pd_CurrentConfig))
    {
        selfpwd = ((pc->pc_Attr & USCAF_SELF_POWERED) && (pd->pd_PoPoCfg.poc_OverridePowerInfo != POCP_BUS_POWERED)) ||
                  (pd->pd_PoPoCfg.poc_OverridePowerInfo == POCP_SELF_POWERED);
    }

    /* count children */
    nextpd = (struct PsdDevice *) pd->pd_Hardware->phw_Devices.lh_Head;
    while(nextpd->pd_Node.ln_Succ)
    {
        if(nextpd->pd_Hub == pd) // this device is a child of us (we're a hub!)
        {
            ports++;
        }
        nextpd = (struct PsdDevice *) nextpd->pd_Node.ln_Succ;
    }

    /* look at config */
    if(selfpwd)
    {
        if(pc)
        {
            pd->pd_PowerSupply = ports ? 500*ports + pc->pc_MaxPower : pc->pc_MaxPower;
        }
        supply = 500; // each downstream port gets the full monty
    } else {
        // the parent hub has already set the amount of supply for this port
        if(pd->pd_PowerSupply >= pc->pc_MaxPower)
        {
            // the downstream ports get the remaining divided attention
            if(ports)
            {
                // avoid division by zero
                supply = (pd->pd_PowerSupply - pc->pc_MaxPower) / ports;
                if(supply > 100)
                {
                    // limit to 100 mA per port
                    supply = 100;
                }
            }
        } else {
            supply = 1; // bad luck, out of power
        }
    }

    /* set supply */
    if(ports) /* needs to be a hub */
    {
        // propagate supply down to the children
        nextpd = (struct PsdDevice *) pd->pd_Hardware->phw_Devices.lh_Head;
        while(nextpd->pd_Node.ln_Succ)
        {
            if(nextpd->pd_Hub == pd)
            {
                nextpd->pd_PowerSupply = supply;
                pPowerRecurseSupply(ps, nextpd);
            }
            nextpd = (struct PsdDevice *) nextpd->pd_Node.ln_Succ;
        }
    }
    if(pd->pd_PowerDrain > pd->pd_PowerSupply)
    {
        if(!(pd->pd_Flags & PDFF_LOWPOWER))
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Detected low power condition for '%s'.", pd->pd_ProductStr);
            pd->pd_Flags |= PDFF_LOWPOWER;
            psdSendEvent(EHMB_DEVICELOWPW, pd, NULL);
        }
    } else {
        if(pd->pd_Flags & PDFF_LOWPOWER)
        {
            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                           "Low power condition resolved for '%s'.", pd->pd_ProductStr);
            pd->pd_Flags &= ~PDFF_LOWPOWER;
        }
    }
}
/* \\\ */

/* /// "pGarbageCollectEvents()" */
void pGarbageCollectEvents(LIBBASETYPEPTR ps)
{
    struct PsdEventNote *pen;
    while((pen = (struct PsdEventNote *) GetMsg(&ps->ps_EventReplyPort)))
    {
        psdFreeVec(pen);
    }
}
/* \\\ */

/* /// "pFindName()" */
struct Node * pFindName(LIBBASETYPEPTR ps, struct List *list, STRPTR name)
{
    struct Node *res = NULL;

    Forbid();
    while(*name)
    {
        res = FindName(list, name);
        if(res)
        {
            break;
        }
        do
        {
            if((*name == '/') || (*name == ':'))
            {
                ++name;
                break;
            }
        } while(*(++name));
    }
    Permit();
    return(res);
}
/* \\\ */

/* /// "pStripString()" */
void pStripString(LIBBASETYPEPTR ps, STRPTR str)
{
    STRPTR srcptr = str;
    STRPTR tarptr = str;
    STRPTR lastgoodchar = str;
    BOOL leadingspaces = TRUE;
    UBYTE ch;
    ULONG len = 0;

    while((ch = *srcptr++))
    {
        len++;
        if(ch == ' ')
        {
            if(!leadingspaces)
            {
                *tarptr++ = ch;
            }
        } else {
            *tarptr++ = ch;
            lastgoodchar = tarptr;
            leadingspaces = FALSE;
        }
    }
    *lastgoodchar = 0;
    // empty string?
    if((str == lastgoodchar) && (len > 6))
    {
        strcpy(str, "<empty>");
    }
}
/* \\\ */

/* /// "pFixBrokenConfig()" */
BOOL pFixBrokenConfig(struct PsdPipe *pp)
{
    struct PsdDevice *pd = pp->pp_Device;
    LIBBASETYPEPTR ps = pd->pd_Hardware->phw_Base;
    struct PsdConfig *pc;
    struct PsdInterface *pif;
    BOOL fixed = FALSE;

    switch(pd->pd_VendorID)
    {
        case 0x03eb: /* Atmel */
            if(pd->pd_ProductID == 0x3312)
            {
                psdFreeVec(pd->pd_ProductStr);
                pd->pd_ProductStr = psdCopyStr("Highway/Subway Root Hub");
            }
            break;

        case 0x04e6: /* E-Shuttle */
            if(pd->pd_ProductID == 0x0001) /* LS120 */
            {
                pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
                /* Get msd interface and fix it */
                pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                if(pif->pif_IfClass != MASSSTORE_CLASSCODE)
                {
                    fixed = TRUE;
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Fixing broken %s interface descriptor!", (STRPTR) "E-Shuttle LS120");
                    pif->pif_IfClass = MASSSTORE_CLASSCODE;
                    pif->pif_IfSubClass = MS_ATAPI_SUBCLASS;
                    pif->pif_IfProto = MS_PROTO_CB;
                }
            }
            break;

        case 0x054C: /* Sony */
            if((pd->pd_ProductID == 0x002E) || (pd->pd_ProductID == 0x0010)) /* Handycam */
            {
                fixed = TRUE;
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Fixing broken %s interface descriptor!", (STRPTR) "Sony MSD");
                pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
                /* Get msd interface and fix it */
                pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                pif->pif_IfSubClass = MS_RBC_SUBCLASS;
            }
            break;

        case 0x057b: /* Y-E Data */
            if(pd->pd_ProductID == 0x0000) /* Flashbuster U */
            {
                pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
                /* Get msd interface and fix it */
                pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                if(pif->pif_IfClass != MASSSTORE_CLASSCODE)
                {
                    fixed = TRUE;
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Fixing broken %s interface descriptor!", (STRPTR) "Y-E Data USB Floppy");
                    pif->pif_IfClass = MASSSTORE_CLASSCODE;
                    pif->pif_IfSubClass = MS_UFI_SUBCLASS;
                    pif->pif_IfProto = (pd->pd_DevVers < 0x0300) ? MS_PROTO_CB : MS_PROTO_CBI;
                }
            }
            break;

        case 0x04ce: /* ScanLogic */
            if(pd->pd_ProductID == 0x0002)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Fixing broken %s interface descriptor!", (STRPTR) "ScanLogic");
                pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
                /* Get msd interface and fix it */
                pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                fixed = TRUE;
                pif->pif_IfSubClass = MS_SCSI_SUBCLASS;
            }
            break;

        case 0x0584: /* Ratoc cardreader */
            if(pd->pd_ProductID == 0x0008)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Fixing broken %s interface descriptor!", (STRPTR) "RATOC");
                pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
                /* Get msd interface and fix it */
                pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                fixed = TRUE;
                pif->pif_IfClass = MASSSTORE_CLASSCODE;
                pif->pif_IfSubClass = MS_SCSI_SUBCLASS;
                pif->pif_IfProto = MS_PROTO_BULK;
            }
            break;

        case 0x04b8: /* Epson */
            if(pd->pd_ProductID == 0x0602) /* EPX Storage device (Card slot in Printer) */
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Fixing broken %s interface descriptor!", (STRPTR) "Epson storage");
                pc = (struct PsdConfig *) pd->pd_Configs.lh_Head;
                /* Get msd interface and fix it */
                pif = (struct PsdInterface *) pc->pc_Interfaces.lh_Head;
                fixed = TRUE;
                pif->pif_IfClass = MASSSTORE_CLASSCODE;
                pif->pif_IfSubClass = MS_SCSI_SUBCLASS;
                pif->pif_IfProto = MS_PROTO_BULK;
            }
            break;

        default:
            break;
    }
    return(fixed);
}
/* \\\ */

/* /// "pOpenDOS()" */
BOOL pOpenDOS(LIBBASETYPEPTR ps)
{
    if(DOSBase)
    {
        return TRUE;
    }
    if((DOSBase = OpenLibrary("dos.library", 39)))
    {
        return TRUE;
    }
    return FALSE;
}
/* \\\ */

/* *** Class Scan Task *** */

/* /// "pStartEventHandler()" */
BOOL pStartEventHandler(LIBBASETYPEPTR ps)
{
    struct PsdHandlerTask *ph = &ps->ps_EventHandler;

    ObtainSemaphore(&ps->ps_PoPoLock);
    if(ph->ph_Task)
    {
        ReleaseSemaphore(&ps->ps_PoPoLock);
        return(TRUE);
    }
    ph->ph_ReadySignal = SIGB_SINGLE;
    ph->ph_ReadySigTask = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE); // clear single bit
    if(psdSpawnSubTask("Poseidon Event Broadcast", pEventHandlerTask, ps))
    {
        Wait(1UL<<ph->ph_ReadySignal);
    }
    ph->ph_ReadySigTask = NULL;
    //FreeSignal(ph->ph_ReadySignal);
    if(ph->ph_Task)
    {
        ReleaseSemaphore(&ps->ps_PoPoLock);
        psdAddErrorMsg0(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Event broadcaster started.");
        return(TRUE);
    }
    ReleaseSemaphore(&ps->ps_PoPoLock);
    return(FALSE);
}
/* \\\ */

/* *** Hardware Driver Task *** */

/* /// "pQuickForwardRequest()" */
AROS_UFH1(void, pQuickForwardRequest,
          AROS_UFHA(struct MsgPort *, msgport, A1))
{
    AROS_USERFUNC_INIT
    struct PsdHardware *phw = (struct PsdHardware *) msgport->mp_Node.ln_Name;
    struct PsdPipe *pp;

    while((pp = (struct PsdPipe *) RemHead(&msgport->mp_MsgList)))
    {
        if(pp->pp_AbortPipe)
        {
            KPRINTF(2, ("Abort pipe %p\n", pp->pp_AbortPipe));
            AbortIO((struct IORequest *) &pp->pp_AbortPipe->pp_IOReq);
            ReplyMsg(&pp->pp_Msg);
            KPRINTF(2, ("Replying evil pipe %p\n", pp));
        } else {
            KPRINTF(1, ("Forwarding pipe %p\n", pp));
            pp->pp_IOReq.iouh_UserData = pp;
            SendIO((struct IORequest *) &pp->pp_IOReq);
            ++phw->phw_MsgCount;
        }
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "pQuickReplyRequest()" */
AROS_UFH1(void, pQuickReplyRequest,
          AROS_UFHA(struct MsgPort *, msgport, A1))
{
    AROS_USERFUNC_INIT
    struct PsdHardware *phw = (struct PsdHardware *) msgport->mp_Node.ln_Name;
    struct IOUsbHWReq *ioreq;

    while((ioreq = (struct IOUsbHWReq *) RemHead(&msgport->mp_MsgList)))
    {
        KPRINTF(1, ("Replying pipe %p\n", ioreq->iouh_UserData));
        ReplyMsg(&((struct PsdPipe *) ioreq->iouh_UserData)->pp_Msg);
        --phw->phw_MsgCount;
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "pDeviceTask()" */
AROS_UFH0(void, pDeviceTask)
{
    AROS_USERFUNC_INIT
    LIBBASETYPEPTR ps;
    struct PsdHardware *phw;
    struct Task *thistask;
    ULONG sigs;
    ULONG sigmask;
    LONG ioerr;
    struct TagItem taglist[11];
    struct TagItem *tag;
    struct PsdPipe *pp;
    struct IOUsbHWReq *ioreq;

    STRPTR prodname = NULL;
    STRPTR manufacturer = NULL;
    STRPTR description = NULL;
    STRPTR copyright = NULL;
    ULONG  version = 0;
    ULONG  revision = 0;
    ULONG  driververs = 0x0100;
    ULONG  caps = UHCF_ISO;
    STRPTR devname;
    ULONG cnt;

    if(!(ps = (LIBBASETYPEPTR) OpenLibrary("poseidon.library", 4)))
    {
        Alert(AG_OpenLib);
        return;
    }
    thistask = FindTask(NULL);
    SetTaskPri(thistask, 21);
    phw = thistask->tc_UserData;

#ifndef PA_CALLBACK // undocumented exec feature
#define PA_CALLBACK 3
#endif

    phw->phw_TaskMsgPort.mp_Node.ln_Type = NT_MSGPORT;
    phw->phw_TaskMsgPort.mp_Node.ln_Name = (APTR) phw;
    phw->phw_TaskMsgPort.mp_Flags = PA_SIGNAL;
    phw->phw_TaskMsgPort.mp_SigTask = thistask;
    phw->phw_TaskMsgPort.mp_SigBit = AllocSignal(-1L);
    NewList(&phw->phw_TaskMsgPort.mp_MsgList);

    phw->phw_DevMsgPort.mp_Node.ln_Type = NT_MSGPORT;
    phw->phw_DevMsgPort.mp_Node.ln_Name = (APTR) phw;
    phw->phw_DevMsgPort.mp_Flags = PA_SIGNAL;
    phw->phw_DevMsgPort.mp_SigTask = thistask;
    phw->phw_DevMsgPort.mp_SigBit = AllocSignal(-1L);
    NewList(&phw->phw_DevMsgPort.mp_MsgList);

    if((phw->phw_RootIOReq = (struct IOUsbHWReq *) CreateIORequest(&phw->phw_DevMsgPort, sizeof(struct IOUsbHWReq))))
    {
        devname = phw->phw_DevName;
        ioerr = -1;
        while(*devname)
        {
            if(!(ioerr = OpenDevice(devname, phw->phw_Unit, (struct IORequest *) phw->phw_RootIOReq, 0)))
            {
                break;
            }
            do
            {
                if((*devname == '/') || (*devname == ':'))
                {
                    ++devname;
                    break;
                }
            } while(*(++devname));
        }

        if(!ioerr)
        {
            phw->phw_Node.ln_Name = phw->phw_RootIOReq->iouh_Req.io_Device->dd_Library.lib_Node.ln_Name;
            tag = taglist;
            tag->ti_Tag = UHA_ProductName;
            tag->ti_Data = (IPTR) &prodname;
            ++tag;
            tag->ti_Tag = UHA_Manufacturer;
            tag->ti_Data = (IPTR) &manufacturer;
            ++tag;
            tag->ti_Tag = UHA_Description;
            tag->ti_Data = (IPTR) &description;
            ++tag;
            tag->ti_Tag = UHA_Version;
            tag->ti_Data = (IPTR) &version;
            ++tag;
            tag->ti_Tag = UHA_Revision;
            tag->ti_Data = (IPTR) &revision;
            ++tag;
            tag->ti_Tag = UHA_Copyright;
            tag->ti_Data = (IPTR) &copyright;
            ++tag;
            tag->ti_Tag = UHA_DriverVersion;
            tag->ti_Data = (IPTR) &driververs;
            ++tag;
            tag->ti_Tag = UHA_Capabilities;
            tag->ti_Data = (IPTR) &caps;
            ++tag;
            tag->ti_Tag = TAG_END;
            phw->phw_RootIOReq->iouh_Data = taglist;
            phw->phw_RootIOReq->iouh_Req.io_Command = UHCMD_QUERYDEVICE;
            DoIO((struct IORequest *) phw->phw_RootIOReq);

            phw->phw_ProductName = psdCopyStr(prodname ? prodname : (STRPTR) "n/a");
            phw->phw_Manufacturer = psdCopyStr(manufacturer ? manufacturer : (STRPTR) "n/a");
            phw->phw_Description = psdCopyStr(description ? description : (STRPTR) "n/a");
            phw->phw_Copyright = psdCopyStr(copyright ? copyright : (STRPTR) "n/a");
            phw->phw_Version = version;
            phw->phw_Revision = revision;
            phw->phw_DriverVers = driververs;
            phw->phw_Capabilities = caps;

            sigmask = SIGBREAKF_CTRL_C;
            if(caps & UHCF_QUICKIO)
            {
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Enabling QuickIO for %s.", prodname);
                phw->phw_TaskMsgPort.mp_Flags = PA_CALLBACK;
                phw->phw_TaskMsgPort.mp_SigTask = (APTR) pQuickForwardRequest;

                phw->phw_DevMsgPort.mp_Flags = PA_CALLBACK;
                phw->phw_DevMsgPort.mp_SigTask = (APTR) pQuickReplyRequest;
            } else {
                sigmask |= (1UL<<phw->phw_DevMsgPort.mp_SigBit)|(1UL<<phw->phw_TaskMsgPort.mp_SigBit);
            }

            KPRINTF(10, ("%s ready!\n", thistask->tc_Node.ln_Name));
            phw->phw_Task = thistask;

            psdLockWritePBase();
            AddTail(&ps->ps_Hardware, &phw->phw_Node);
            psdUnlockPBase();

            Forbid();
            if(phw->phw_ReadySigTask)
            {
                Signal(phw->phw_ReadySigTask, 1L<<phw->phw_ReadySignal);
            }
            Permit();
            do
            {
                KPRINTF(1, ("Main loop wait.\n"));
                while((pp = (struct PsdPipe *) GetMsg(&phw->phw_TaskMsgPort)))
                {
                    if(pp->pp_AbortPipe)
                    {
                        KPRINTF(2, ("Abort pipe %p\n", pp->pp_AbortPipe));
                        AbortIO((struct IORequest *) &pp->pp_AbortPipe->pp_IOReq);
                        ReplyMsg(&pp->pp_Msg);
                        KPRINTF(2, ("Replying evil pipe %p\n", pp));
                    } else {
                        KPRINTF(1, ("Forwarding pipe %p\n", pp));
                        pp->pp_IOReq.iouh_UserData = pp;
                        SendIO((struct IORequest *) &pp->pp_IOReq);
                        ++phw->phw_MsgCount;
                    }
                }
                while((ioreq = (struct IOUsbHWReq *) GetMsg(&phw->phw_DevMsgPort)))
                {
                    KPRINTF(1, ("Replying pipe %p\n", ioreq->iouh_UserData));
                    ReplyMsg(&((struct PsdPipe *) ioreq->iouh_UserData)->pp_Msg);
                    --phw->phw_MsgCount;
                }
                sigs = Wait(sigmask);
            } while(!(sigs & SIGBREAKF_CTRL_C));
            /* Flush all pending IO Requests */
            phw->phw_RootIOReq->iouh_Req.io_Command = CMD_FLUSH;
            DoIO((struct IORequest *) phw->phw_RootIOReq);
            cnt = 0;
            while(phw->phw_MsgCount)
            {
                KPRINTF(20, ("Still %ld iorequests pending!\n", phw->phw_MsgCount));
                psdDelayMS(100);
                if(++cnt == 50)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "There are still %ld IORequests pending, before unit can go down. Driver buggy?",
                                   phw->phw_MsgCount);
                }
                if(cnt == 300)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Okay, I've waited long enough, sod these %ld IORequests.",
                                   phw->phw_MsgCount);
                    phw->phw_MsgCount = 0;
                    break;
                }
                while((ioreq = (struct IOUsbHWReq *) GetMsg(&phw->phw_DevMsgPort)))
                {
                    KPRINTF(1, ("Replying pipe %p\n", ioreq->iouh_UserData));
                    ReplyMsg(&((struct PsdPipe *) ioreq->iouh_UserData)->pp_Msg);
                    --phw->phw_MsgCount;
                }
            }
            psdLockWritePBase();
            Remove(&phw->phw_Node);
            psdUnlockPBase();
            CloseDevice((struct IORequest *) phw->phw_RootIOReq);
        } else {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                           "Opening %s unit %ld failed %s (%ld).",
                           phw->phw_DevName, phw->phw_Unit, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        }
        DeleteIORequest((struct IORequest *) phw->phw_RootIOReq);
        phw->phw_RootIOReq = NULL;
    }
    FreeSignal((LONG) phw->phw_TaskMsgPort.mp_SigBit);
    FreeSignal((LONG) phw->phw_DevMsgPort.mp_SigBit);

    CloseLibrary((struct Library *) ps);
    phw->phw_Task = NULL;

    Forbid();
    if(phw->phw_ReadySigTask)
    {
        Signal(phw->phw_ReadySigTask, 1L<<phw->phw_ReadySignal);
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "pEventHandlerTask()" */
AROS_UFH0(void, pEventHandlerTask)
{
    AROS_USERFUNC_INIT
    LIBBASETYPEPTR ps;
    struct Task *thistask;
    struct timeval currtime;
    ULONG sigs;
    ULONG sigmask;
    struct PsdUsbClass *puc;
    struct PsdHandlerTask *ph;
    struct PsdEventNote *pen;
    ULONG counter;
    ULONG cfgchanged;

    thistask = FindTask(NULL);
    ps = thistask->tc_UserData;
    ph = &ps->ps_EventHandler;
    SetTaskPri(thistask, 0);

    if((ph->ph_MsgPort = CreateMsgPort()))
    {
        if((ph->ph_TimerMsgPort = CreateMsgPort()))
        {
            if((ph->ph_TimerIOReq = (struct timerequest *) CreateIORequest(ph->ph_TimerMsgPort, sizeof(struct timerequest))))
            {
                if(!(OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *) ph->ph_TimerIOReq, 0)))
                {
                    ph->ph_TimerIOReq->tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
                    ph->ph_TimerIOReq->tr_node.io_Message.mn_Node.ln_Name = "EventHandler";
                    ph->ph_TimerIOReq->tr_node.io_Command = TR_ADDREQUEST;

                    ph->ph_EventHandler = psdAddEventHandler(ph->ph_MsgPort, EHMF_CONFIGCHG);
                    if(ph->ph_EventHandler)
                    {
                        ph->ph_Task = thistask;
                        Forbid();
                        if(ph->ph_ReadySigTask)
                        {
                            Signal(ph->ph_ReadySigTask, 1L<<ph->ph_ReadySignal);
                        }
                        Permit();

                        ph->ph_TimerIOReq->tr_time.tv_micro = 500*1000;
                        SendIO(&ph->ph_TimerIOReq->tr_node);
                        sigmask = (1UL<<ph->ph_MsgPort->mp_SigBit)|(1UL<<ph->ph_TimerMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
                        counter = 0;
                        cfgchanged = 0;
                        do
                        {
                            if(ps->ps_CheckConfigReq)
                            {
                                pCheckCfgChanged(ps);
                            }
                            while((pen = (struct PsdEventNote *) GetMsg(ph->ph_MsgPort)))
                            {
                                switch(pen->pen_Event)
                                {
                                    case EHMB_CONFIGCHG:
                                        if(!cfgchanged)
                                        {
                                            cfgchanged = counter;
                                        }
                                        break;

                                }
                                ReplyMsg(&pen->pen_Msg);
                            }
                            if(CheckIO(&ph->ph_TimerIOReq->tr_node))
                            {
                                BOOL startpopo;
                                WaitIO(&ph->ph_TimerIOReq->tr_node);
                                ph->ph_TimerIOReq->tr_time.tv_micro = 500*1000;
                                SendIO(&ph->ph_TimerIOReq->tr_node);
                                counter++;
                                startpopo = !((counter & 3) || ps->ps_PoPo.po_Task);
                                if((ps->ps_GlobalCfg->pgc_PopupDeviceNew == PGCP_NEVER) &&
                                   (!ps->ps_GlobalCfg->pgc_PopupDeviceDeath) &&
                                   (!ps->ps_GlobalCfg->pgc_PopupDeviceGone))
                                {
                                    startpopo = FALSE; // no need to start popo, no windows wanted
                                }
                                if(startpopo)
                                {
                                    struct PsdPoPo *po = &ps->ps_PoPo;

                                    po->po_ReadySignal = SIGB_SINGLE;
                                    po->po_ReadySigTask = FindTask(NULL);
                                    SetSignal(0, SIGF_SINGLE); // clear single bit
                                    if(psdSpawnSubTask("PoPo (Poseidon Popups)", pPoPoGUITask, ps))
                                    {
                                        Wait(1UL<<po->po_ReadySignal);
                                    }
                                    po->po_ReadySigTask = NULL;
                                    //FreeSignal(po->po_ReadySignal);
                                    if(po->po_Task)
                                    {
                                        psdAddErrorMsg0(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "PoPo kicks ass.");
                                    }
                                }
                                if((cfgchanged + 2) == counter)
                                {
                                    KPRINTF(10, ("Sending information about config changed to all classes.\n"));
                                    /* Inform all classes */
                                    psdLockReadPBase();
                                    puc = (struct PsdUsbClass *) ps->ps_Classes.lh_Head;
                                    while(puc->puc_Node.ln_Succ)
                                    {
                                        usbDoMethod(UCM_ConfigChangedEvent);
                                        puc = (struct PsdUsbClass *) puc->puc_Node.ln_Succ;
                                    }
                                    psdUnlockPBase();
                                    cfgchanged = 0;
                                }
                                // power saving stuff, check every second
                                if((counter & 1) && ps->ps_GlobalCfg->pgc_PowerSaving)
                                {
                                    struct PsdDevice *pd = NULL;
                                    struct PsdInterface *pif;
                                    GetSysTime((APTR) &currtime);
                                    while((pd = psdGetNextDevice(pd)))
                                    {
                                        if((pd->pd_DevClass != HUB_CLASSCODE) &&
                                           ((pd->pd_Flags & (PDFF_CONFIGURED|PDFF_DEAD|PDFF_SUSPENDED|PDFF_APPBINDING|PDFF_DELEXPUNGE)) == PDFF_CONFIGURED))
                                        {
                                            if(pd->pd_LastActivity.tv_secs && ((currtime.tv_secs - pd->pd_LastActivity.tv_secs) > ps->ps_GlobalCfg->pgc_SuspendTimeout))
                                            {
                                                BOOL doit = TRUE;
                                                IPTR suspendable;
                                                if(!((pd->pd_CurrentConfig->pc_Attr & USCAF_REMOTE_WAKEUP) && ps->ps_GlobalCfg->pgc_ForceSuspend))
                                                {
                                                    if(pd->pd_DevBinding && ((puc = pd->pd_ClsBinding)))
                                                    {
                                                        suspendable = 0;
                                                        usbGetAttrs(UGA_CLASS, NULL, UCCA_SupportsSuspend, &suspendable, TAG_END);
                                                        if(!suspendable)
                                                        {
                                                            doit = FALSE;
                                                        }
                                                    }
                                                    pif = (struct PsdInterface *) pd->pd_CurrentConfig->pc_Interfaces.lh_Head;
                                                    while(pif->pif_Node.ln_Succ)
                                                    {
                                                        if(pif->pif_IfBinding && ((puc = pif->pif_ClsBinding)))
                                                        {
                                                            suspendable = 0;
                                                            usbGetAttrs(UGA_CLASS, NULL, UCCA_SupportsSuspend, &suspendable, TAG_END);
                                                            if(!suspendable)
                                                            {
                                                                doit = FALSE;
                                                                break;
                                                            }
                                                        }
                                                        pif = (struct PsdInterface *) pif->pif_Node.ln_Succ;
                                                    }
                                                }
                                                if(doit)
                                                {
                                                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Suspending '%s'.", pd->pd_ProductStr);
                                                    psdSuspendDevice(pd);
                                                }
                                                pd->pd_LastActivity.tv_secs = 0;
                                            }
                                        }
                                    }
                                }
                            }
                            sigs = Wait(sigmask);
                        } while(!(sigs & SIGBREAKF_CTRL_C));
                        psdRemEventHandler(ph->ph_EventHandler);
                        ph->ph_EventHandler = NULL;
                        AbortIO(&ph->ph_TimerIOReq->tr_node);
                        WaitIO(&ph->ph_TimerIOReq->tr_node);
                    }
                    CloseDevice((struct IORequest *) ph->ph_TimerIOReq);
                }
                DeleteIORequest((struct IORequest *) ph->ph_TimerIOReq);
            }
            DeleteMsgPort(ph->ph_TimerMsgPort);
        }
        DeleteMsgPort(ph->ph_MsgPort);
        ph->ph_MsgPort = NULL;
    }
    Forbid();
    ph->ph_Task = NULL;
    if(ph->ph_ReadySigTask)
    {
        Signal(ph->ph_ReadySigTask, 1L<<ph->ph_ReadySignal);
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */

/*****************************************************************/

/* /// "Packtables for psdGetAttrs() and psdSetAttrs() " */
/* Pack table for PsdBase */
static const ULONG PsdBasePT[] =
{
    PACK_STARTTABLE(PA_Dummy),
    PACK_ENTRY(PA_Dummy, PA_ConfigRead, PsdBase, ps_ConfigRead, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(PA_Dummy, PA_GlobalConfig, PsdBase, ps_GlobalCfg, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(PA_Dummy, PA_MemPoolUsage, PsdBase, ps_MemAllocated, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(PA_Dummy, PA_CurrConfigHash, PsdBase, ps_ConfigHash, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(PA_Dummy, PA_SavedConfigHash, PsdBase, ps_SavedConfigHash, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(PA_Dummy, PA_ReleaseVersion, PsdBase, ps_ReleaseVersion, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(PA_Dummy, PA_OSVersion, PsdBase, ps_OSVersion, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

/* Pack table for PsdErrorMsg */
static const ULONG PsdErrorMsgPT[] =
{
    PACK_STARTTABLE(EMA_Dummy),
    PACK_ENTRY(EMA_Dummy, EMA_Level, PsdErrorMsg, pem_Level, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(EMA_Dummy, EMA_Origin, PsdErrorMsg, pem_Origin, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(EMA_Dummy, EMA_Msg, PsdErrorMsg, pem_Msg, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

/* Pack table for PsdUsbClass */
static const ULONG PsdUsbClassPT[] =
{
    PACK_STARTTABLE(UCA_Dummy),
    PACK_ENTRY(UCA_Dummy, UCA_ClassBase, PsdUsbClass, puc_ClassBase, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(UCA_Dummy, UCA_ClassName, PsdUsbClass, puc_ClassName, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(UCA_Dummy, UCA_FullPath, PsdUsbClass, puc_FullPath, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(UCA_Dummy, UCA_UseCount, PsdUsbClass, puc_UseCnt, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

/* Pack table for PsdHardware */
static const ULONG PsdHardwarePT[] =
{
    PACK_STARTTABLE(HA_Dummy),
    PACK_ENTRY(HA_Dummy, HA_DeviceName, PsdHardware, phw_DevName, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(HA_Dummy, HA_DeviceUnit, PsdHardware, phw_Unit, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(HA_Dummy, HA_ProductName, PsdHardware, phw_ProductName, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(HA_Dummy, HA_Manufacturer, PsdHardware, phw_Manufacturer, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(HA_Dummy, HA_Description, PsdHardware, phw_Description, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(HA_Dummy, HA_Copyright, PsdHardware, phw_Copyright, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(HA_Dummy, HA_Version, PsdHardware, phw_Version, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(HA_Dummy, HA_Revision, PsdHardware, phw_Revision, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(HA_Dummy, HA_DriverVersion, PsdHardware, phw_DriverVers, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

/* Pack table for PsdDevice */
static const ULONG PsdDevicePT[] =
{
    PACK_STARTTABLE(DA_Dummy),
    PACK_ENTRY(DA_Dummy, DA_Address, PsdDevice, pd_DevAddr, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_NumConfigs, PsdDevice, pd_NumCfgs, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_CurrConfig, PsdDevice, pd_CurrCfg, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_Config, PsdDevice, pd_CurrentConfig, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_HubDevice, PsdDevice, pd_Hub, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(DA_Dummy, DA_UsbVersion, PsdDevice, pd_USBVers, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_Class, PsdDevice, pd_DevClass, PKCTRL_WORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_SubClass, PsdDevice, pd_DevSubClass, PKCTRL_WORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_Protocol, PsdDevice, pd_DevProto, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_VendorID, PsdDevice, pd_VendorID, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_MaxPktSize0, PsdDevice, pd_MaxPktSize0, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_ProductID, PsdDevice, pd_ProductID, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_Version, PsdDevice, pd_DevVers, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_Manufacturer, PsdDevice, pd_MnfctrStr, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_ProductName, PsdDevice, pd_ProductStr, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(DA_Dummy, DA_OrigProductName, PsdDevice, pd_OldProductStr, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_SerialNumber, PsdDevice, pd_SerNumStr, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_Hardware, PsdDevice, pd_Hardware, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_Binding, PsdDevice, pd_DevBinding, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(DA_Dummy, DA_BindingClass, PsdDevice, pd_ClsBinding, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(DA_Dummy, DA_LangIDArray, PsdDevice, pd_LangIDArray, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_CurrLangID, PsdDevice, pd_CurrLangID, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(DA_Dummy, DA_IDString, PsdDevice, pd_IDString, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_CloneCount, PsdDevice, pd_CloneCount, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_AtHubPortNumber, PsdDevice, pd_HubPort, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(DA_Dummy, DA_PowerDrained, PsdDevice, pd_PowerDrain, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_PowerSupply, PsdDevice, pd_PowerSupply, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_IsNewToMe, PsdDevice, pd_IsNewToMe, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DA_Dummy, DA_InhibitPopup, PsdDevice, pd_PoPoCfg.poc_InhibitPopup, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(DA_Dummy, DA_InhibitClassBind, PsdDevice, pd_PoPoCfg.poc_NoClassBind, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(DA_Dummy, DA_OverridePowerInfo, PsdDevice, pd_PoPoCfg.poc_OverridePowerInfo, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(DA_Dummy, DA_HubThinkTime, PsdDevice, pd_HubThinkTime, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_WORDBIT(DA_Dummy, DA_IsLowspeed, PsdDevice, pd_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PDFF_LOWSPEED),
    PACK_WORDBIT(DA_Dummy, DA_IsHighspeed, PsdDevice, pd_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PDFF_HIGHSPEED),
    PACK_WORDBIT(DA_Dummy, DA_IsConnected, PsdDevice, pd_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PDFF_CONNECTED),
    PACK_WORDBIT(DA_Dummy, DA_HasAddress, PsdDevice, pd_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PDFF_HASDEVADDR),
    PACK_WORDBIT(DA_Dummy, DA_HasDevDesc, PsdDevice, pd_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PDFF_HASDEVDESC),
    PACK_WORDBIT(DA_Dummy, DA_IsConfigured, PsdDevice, pd_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PDFF_CONFIGURED),
    PACK_WORDBIT(DA_Dummy, DA_IsDead, PsdDevice, pd_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, PDFF_DEAD),
    PACK_WORDBIT(DA_Dummy, DA_IsSuspended, PsdDevice, pd_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PDFF_SUSPENDED),
    PACK_WORDBIT(DA_Dummy, DA_HasAppBinding, PsdDevice, pd_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, PDFF_APPBINDING),
    PACK_WORDBIT(DA_Dummy, DA_NeedsSplitTrans, PsdDevice, pd_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PDFF_NEEDSSPLIT),
    PACK_WORDBIT(DA_Dummy, DA_LowPower, PsdDevice, pd_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, PDFF_LOWPOWER),
#ifdef AROS_USB30_CODE
    PACK_WORDBIT(DA_Dummy, DA_IsSuperspeed, PsdDevice, pd_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PDFF_SUPERSPEED),
#endif
    PACK_ENDTABLE
};

/* Pack table for PsdConfig */
static const ULONG PsdConfigPT[] =
{
    PACK_STARTTABLE(CA_Dummy),
    PACK_ENTRY(CA_Dummy, CA_ConfigNum, PsdConfig, pc_CfgNum, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(CA_Dummy, CA_MaxPower, PsdConfig, pc_MaxPower, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(CA_Dummy, CA_ConfigName, PsdConfig, pc_CfgStr, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(CA_Dummy, CA_NumInterfaces, PsdConfig, pc_NumIfs, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(CA_Dummy, CA_Attrs, PsdConfig, pc_Attr, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(CA_Dummy, CA_Device, PsdConfig, pc_Device, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_WORDBIT(CA_Dummy, CA_SelfPowered, PsdConfig, pc_Attr, PKCTRL_BIT|PKCTRL_PACKUNPACK, USCAF_SELF_POWERED),
    PACK_WORDBIT(CA_Dummy, CA_RemoteWakeup, PsdConfig, pc_Attr, PKCTRL_BIT|PKCTRL_UNPACKONLY, USCAF_REMOTE_WAKEUP),
    PACK_ENDTABLE
};

/* Pack table for PsdDescriptor */
static const ULONG PsdDescriptorPT[] =
{
    PACK_STARTTABLE(DDA_Dummy),
    PACK_ENTRY(DDA_Dummy, DDA_Device, PsdDescriptor, pdd_Device, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DDA_Dummy, DDA_Config, PsdDescriptor, pdd_Config, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DDA_Dummy, DDA_Interface, PsdDescriptor, pdd_Interface, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DDA_Dummy, DDA_Endpoint, PsdDescriptor, pdd_Endpoint, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DDA_Dummy, DDA_Name, PsdDescriptor, pdd_Name, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DDA_Dummy, DDA_DescriptorType, PsdDescriptor, pdd_Type, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DDA_Dummy, DDA_CS_SubType, PsdDescriptor, pdd_CSSubType, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DDA_Dummy, DDA_DescriptorData, PsdDescriptor, pdd_Data, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(DDA_Dummy, DDA_DescriptorLength, PsdDescriptor, pdd_Length, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

/* Pack table for PsdInterface */
static const ULONG PsdInterfacePT[] =
{
    PACK_STARTTABLE(IFA_Dummy),
    PACK_ENTRY(IFA_Dummy, IFA_InterfaceNum, PsdInterface, pif_IfNum, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(IFA_Dummy, IFA_AlternateNum, PsdInterface, pif_Alternate, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(IFA_Dummy, IFA_NumEndpoints, PsdInterface, pif_NumEPs, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(IFA_Dummy, IFA_Class, PsdInterface, pif_IfClass, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(IFA_Dummy, IFA_SubClass, PsdInterface, pif_IfSubClass, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(IFA_Dummy, IFA_Protocol, PsdInterface, pif_IfProto, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(IFA_Dummy, IFA_InterfaceName, PsdInterface, pif_IfStr, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(IFA_Dummy, IFA_Config, PsdInterface, pif_Config, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(IFA_Dummy, IFA_Binding, PsdInterface, pif_IfBinding, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(IFA_Dummy, IFA_BindingClass, PsdInterface, pif_ClsBinding, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(IFA_Dummy, IFA_IDString, PsdInterface, pif_IDString, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

/* Pack table for PsdEndpoint */
static const ULONG PsdEndpointPT[] =
{
    PACK_STARTTABLE(EA_Dummy),
    PACK_ENTRY(EA_Dummy, EA_EndpointNum, PsdEndpoint, pep_EPNum, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(EA_Dummy, EA_TransferType, PsdEndpoint, pep_TransType, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(EA_Dummy, EA_MaxPktSize, PsdEndpoint, pep_MaxPktSize, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(EA_Dummy, EA_Interval, PsdEndpoint, pep_Interval, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(EA_Dummy, EA_NumTransMuFrame, PsdEndpoint, pep_NumTransMuFr, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(EA_Dummy, EA_SyncType, PsdEndpoint, pep_SyncType, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(EA_Dummy, EA_UsageType, PsdEndpoint, pep_UsageType, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(EA_Dummy, EA_Interface, PsdEndpoint, pep_Interface, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_WORDBIT(EA_Dummy, EA_IsIn, PsdEndpoint, pep_Direction, PKCTRL_BIT|PKCTRL_UNPACKONLY, 1),
    PACK_ENDTABLE
};

/* Pack table for PsdPipe */
static const ULONG PsdPipePT[] =
{
    PACK_STARTTABLE(PPA_Dummy),
    PACK_ENTRY(PPA_Dummy, PPA_Endpoint, PsdPipe, pp_Endpoint, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(PPA_Dummy, PPA_Error, PsdPipe, pp_IOReq.iouh_Req.io_Error, PKCTRL_BYTE|PKCTRL_UNPACKONLY),
    PACK_ENTRY(PPA_Dummy, PPA_Actual, PsdPipe, pp_IOReq.iouh_Actual, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(PPA_Dummy, PPA_EndpointNum, PsdPipe, pp_IOReq.iouh_Endpoint, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(PPA_Dummy, PPA_DeviceAddress, PsdPipe, pp_IOReq.iouh_DevAddr, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(PPA_Dummy, PPA_MaxPktSize, PsdPipe, pp_IOReq.iouh_MaxPktSize, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(PPA_Dummy, PPA_NakTimeoutTime, PsdPipe, pp_IOReq.iouh_NakTimeout, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(PPA_Dummy, PPA_Interval, PsdPipe, pp_IOReq.iouh_Interval, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_WORDBIT(PPA_Dummy, PPA_NoShortPackets, PsdPipe, pp_IOReq.iouh_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, UHFF_NOSHORTPKT),
    PACK_WORDBIT(PPA_Dummy, PPA_NakTimeout, PsdPipe, pp_IOReq.iouh_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, UHFF_NAKTIMEOUT),
    PACK_WORDBIT(PPA_Dummy, PPA_AllowRuntPackets, PsdPipe, pp_IOReq.iouh_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, UHFF_ALLOWRUNTPKTS),
    PACK_ENDTABLE
};

/* Pack table for PsdAppBinding */
static const ULONG PsdAppBindingPT[] =
{
    PACK_STARTTABLE(ABA_Dummy),
    PACK_ENTRY(ABA_Dummy, ABA_ReleaseHook, PsdAppBinding, pab_ReleaseHook, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(ABA_Dummy, ABA_Device, PsdAppBinding, pab_Device, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(ABA_Dummy, ABA_UserData, PsdAppBinding, pab_UserData, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(ABA_Dummy, ABA_Task, PsdAppBinding, pab_Task, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(ABA_Dummy, ABA_ForceRelease, PsdAppBinding, pab_ForceRelease, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENDTABLE
};

/* Pack table for PsdAppBinding */
static const ULONG PsdEventNotePT[] =
{
    PACK_STARTTABLE(ENA_Dummy),
    PACK_ENTRY(ENA_Dummy, ENA_EventID, PsdEventNote, pen_Event, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(ENA_Dummy, ENA_Param1, PsdEventNote, pen_Param1, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(ENA_Dummy, ENA_Param2, PsdEventNote, pen_Param2, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

/* Pack table for PsdGlobalCfg */
static const ULONG PsdGlobalCfgPT[] =
{
    PACK_STARTTABLE(GCA_Dummy),
    PACK_ENTRY(GCA_Dummy, GCA_LogInfo, PsdGlobalCfg, pgc_LogInfo, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_LogWarning, PsdGlobalCfg, pgc_LogWarning, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_LogError, PsdGlobalCfg, pgc_LogError, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_LogFailure, PsdGlobalCfg, pgc_LogFailure, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_BootDelay, PsdGlobalCfg, pgc_BootDelay, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_SubTaskPri, PsdGlobalCfg, pgc_SubTaskPri, PKCTRL_WORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_PopupDeviceNew, PsdGlobalCfg, pgc_PopupDeviceNew, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_PopupDeviceGone, PsdGlobalCfg, pgc_PopupDeviceGone, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_PopupDeviceDeath, PsdGlobalCfg, pgc_PopupDeviceDeath, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_PopupCloseDelay, PsdGlobalCfg, pgc_PopupCloseDelay, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_PopupActivateWin, PsdGlobalCfg, pgc_PopupActivateWin, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_PopupWinToFront, PsdGlobalCfg, pgc_PopupWinToFront, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_AutoDisableLP, PsdGlobalCfg, pgc_AutoDisableLP, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_AutoDisableDead, PsdGlobalCfg, pgc_AutoDisableDead, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_AutoRestartDead, PsdGlobalCfg, pgc_AutoRestartDead, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_PowerSaving, PsdGlobalCfg, pgc_PowerSaving, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_ForceSuspend, PsdGlobalCfg, pgc_ForceSuspend, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_SuspendTimeout, PsdGlobalCfg, pgc_SuspendTimeout, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(GCA_Dummy, GCA_PrefsVersion, PsdGlobalCfg, pgc_PrefsVersion, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENDTABLE
};

/* Pack table for PsdPipeStream */
static const ULONG PsdPipeStreamPT[] =
{
    PACK_STARTTABLE(PSA_Dummy),
    PACK_ENTRY(PSA_Dummy, PSA_MessagePort, PsdPipeStream, pps_MsgPort, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(PSA_Dummy, PSA_NumPipes, PsdPipeStream, pps_NumPipes, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(PSA_Dummy, PSA_BufferSize, PsdPipeStream, pps_BufferSize, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(PSA_Dummy, PSA_NakTimeoutTime, PsdPipeStream, pps_NakTimeoutTime, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(PSA_Dummy, PSA_BytesPending, PsdPipeStream, pps_BytesPending, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(PSA_Dummy, PSA_Error, PsdPipeStream, pps_Error, PKCTRL_LONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(PSA_Dummy, PSA_TermArray, PsdPipeStream, pps_TermArray, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(PSA_Dummy, PSA_AbortSigMask, PsdPipeStream, pps_AbortSigMask, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(PSA_Dummy, PSA_ActivePipe, PsdPipeStream, pps_ActivePipe, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_WORDBIT(PSA_Dummy, PSA_AsyncIO, PsdPipeStream, pps_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PSFF_ASYNCIO),
    PACK_WORDBIT(PSA_Dummy, PSA_ShortPktTerm, PsdPipeStream, pps_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PSFF_SHORTTERM),
    PACK_WORDBIT(PSA_Dummy, PSA_ReadAhead, PsdPipeStream, pps_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PSFF_READAHEAD),
    PACK_WORDBIT(PSA_Dummy, PSA_BufferedRead, PsdPipeStream, pps_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PSFF_BUFFERREAD),
    PACK_WORDBIT(PSA_Dummy, PSA_BufferedWrite, PsdPipeStream, pps_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PSFF_BUFFERWRITE),
    PACK_WORDBIT(PSA_Dummy, PSA_NoZeroPktTerm, PsdPipeStream, pps_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PSFF_NOSHORTPKT),
    PACK_WORDBIT(PSA_Dummy, PSA_NakTimeout, PsdPipeStream, pps_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PSFF_NAKTIMEOUT),
    PACK_WORDBIT(PSA_Dummy, PSA_AllowRuntPackets, PsdPipeStream, pps_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PSFF_ALLOWRUNT),
    PACK_WORDBIT(PSA_Dummy, PSA_DoNotWait, PsdPipeStream, pps_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, PSFF_DONOTWAIT),
    PACK_ENDTABLE
};

/* Pack table for PsdRTIsoHandler */
static const ULONG PsdRTIsoHandlerPT[] =
{
    PACK_STARTTABLE(RTA_Dummy),
    PACK_ENTRY(RTA_Dummy, RTA_InRequestHook, PsdRTIsoHandler, prt_RTIso.urti_InReqHook, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(RTA_Dummy, RTA_OutRequestHook, PsdRTIsoHandler, prt_RTIso.urti_OutReqHook, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(RTA_Dummy, RTA_InDoneHook, PsdRTIsoHandler, prt_RTIso.urti_InDoneHook, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(RTA_Dummy, RTA_OutDoneHook, PsdRTIsoHandler, prt_RTIso.urti_OutDoneHook, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(RTA_Dummy, RTA_ReleaseHook, PsdRTIsoHandler, prt_ReleaseHook, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(RTA_Dummy, RTA_OutPrefetchSize, PsdRTIsoHandler, prt_RTIso.urti_OutPrefetch, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENDTABLE
};

/* PGA assignment table */
static const ULONG *PsdPTArray[] =
{
    NULL,
    PsdBasePT,
    PsdUsbClassPT,
    PsdHardwarePT,
    PsdDevicePT,
    PsdConfigPT,
    PsdInterfacePT,
    PsdEndpointPT,
    PsdErrorMsgPT,
    PsdPipePT,
    PsdAppBindingPT,
    PsdEventNotePT,
    PsdGlobalCfgPT,
    PsdPipeStreamPT,
    PsdDescriptorPT,
    PsdRTIsoHandlerPT
};
/* \\\ */

