/*
 *----------------------------------------------------------------------------
 *                     bluetooth.library main file
 *----------------------------------------------------------------------------
 *
 * The Bluetooth host stack for AROS. Structure and conventions follow
 * poseidon.library (Chris Hodges) so that the two stacks can be maintained
 * side by side; the locking, memory, string, event and configuration code
 * is a direct port.
 */

#include "debug.h"

#include "bluetooth.library.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/btclass.h>
#include <proto/timer.h>

#include <string.h>

#ifdef __AROS__
#include <aros/bootloader.h>
#include <proto/bootloader.h>
#endif

#define NewList(list) NEWLIST(list)

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))

/* Static data */
const char GM_UNIQUENAME(libname)[]     = MOD_NAME_STRING;

#define BtClsBase bc->bc_ClassBase
#define DOSBase BluetoothBase->bt_DosBase
#define TimerBase BluetoothBase->bt_TimerIOReq.tr_node.io_Device

/* LibInit */
static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR BluetoothBase)
{
    KPRINTF(10, ("libInit BluetoothBase: 0x%p SysBase: 0x%p\n",
                 BluetoothBase, SysBase));

    BluetoothBase->bt_StackInit = FALSE;
    BluetoothBase->bt_UtilityBase = (struct UtilityBase *) OpenLibrary("utility.library", 39);

#define UtilityBase BluetoothBase->bt_UtilityBase

    if (UtilityBase) {
#ifdef __AROS__
        APTR BootLoaderBase = OpenResource("bootloader.resource");

        if (BootLoaderBase) {
            struct List *args = GetBootInfo(BL_Args);

            if (args) {
                struct Node *node;

                for (node = args->lh_Head; node->ln_Succ; node = node->ln_Succ) {
                    if (stricmp(node->ln_Name, "btdebug") == 0) {
                        BluetoothBase->bt_Flags = BTF_KLOG;
                        break;
                    }
                }
            }
        }
#endif

        NewList(&BluetoothBase->bt_Hardware);
        NewList(&BluetoothBase->bt_Classes);
        NewList(&BluetoothBase->bt_FirmwareLoaders);
        InitSemaphore(&BluetoothBase->bt_FirmwareLock);
        NewList(&BluetoothBase->bt_ErrorMsgs);
        NewList(&BluetoothBase->bt_EventHooks);
        memset(&BluetoothBase->bt_EventReplyPort, 0, sizeof(BluetoothBase->bt_EventReplyPort));
        BluetoothBase->bt_EventReplyPort.mp_Flags = PA_IGNORE;
        NewList(&BluetoothBase->bt_EventReplyPort.mp_MsgList);
        NewList(&BluetoothBase->bt_ConfigRoot);
        NewList(&BluetoothBase->bt_AlienConfigs);

        NewList(&BluetoothBase->bt_DeadlockDebug);

        InitSemaphore(&BluetoothBase->bt_ReentrantLock);

        if((BluetoothBase->bt_MemPool = CreatePool(MEMF_CLEAR|MEMF_PUBLIC|MEMF_SEM_PROTECTED, 16384, 1024))) {
            if((BluetoothBase->bt_SemaMemPool = CreatePool(MEMF_CLEAR|MEMF_PUBLIC, 16*sizeof(struct BtReadLock), sizeof(struct BtBorrowLock)))) {
                bInitSem(BluetoothBase, &BluetoothBase->bt_Lock, "BTBase");
                bInitSem(BluetoothBase, &BluetoothBase->bt_ConfigLock, "ConfigLock");
                KPRINTF(20, ("libInit: Done!\n"));
                return TRUE;
            }
            DeletePool(BluetoothBase->bt_MemPool);
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
static int GM_UNIQUENAME(libOpen)(LIBBASETYPEPTR BluetoothBase)
{
    struct BtIFFContext *pic;

    KPRINTF(10, ("libOpen BluetoothBase: 0x%p\n", BluetoothBase));
    ObtainSemaphore(&BluetoothBase->bt_ReentrantLock);
    if(!BluetoothBase->bt_StackInit) {
        BluetoothBase->bt_TimerIOReq.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
        BluetoothBase->bt_TimerIOReq.tr_node.io_Message.mn_ReplyPort    = NULL;
        BluetoothBase->bt_TimerIOReq.tr_node.io_Message.mn_Length       = sizeof(struct timerequest);
        if(!OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) &BluetoothBase->bt_TimerIOReq, 0)) {
            BluetoothBase->bt_TimerIOReq.tr_node.io_Message.mn_Node.ln_Name = "Bluetooth";
            BluetoothBase->bt_TimerIOReq.tr_node.io_Command = TR_ADDREQUEST;

            BluetoothBase->bt_ReleaseVersion = RELEASEVERSION;
            BluetoothBase->bt_OSVersion = MAKE_ID('A','R','O','S');

            pic = bAllocForm(BluetoothBase, NULL, IFFFORM_BTCFG);
            if((BluetoothBase->bt_GlobalCfg = btAllocVec(sizeof(struct BtGlobalCfg)))) {
                struct BtGlobalCfg *bgc = BluetoothBase->bt_GlobalCfg;
                bgc->bgc_ChunkID = AROS_LONG2BE(IFFCHNK_GLOBALCFG);
                bgc->bgc_Length = AROS_LONG2BE(sizeof(struct BtGlobalCfg)-8);
                bgc->bgc_LogInfo = TRUE;
                bgc->bgc_LogWarning = TRUE;
                bgc->bgc_LogError = TRUE;
                bgc->bgc_LogFailure = TRUE;
                bgc->bgc_BootDelay = 0;
                bgc->bgc_SubTaskPri = 5;
                bgc->bgc_PopupDeviceNew = BGCP_NEVER;
                bgc->bgc_PopupDeviceGone = FALSE;
                bgc->bgc_PopupPairing = TRUE;
                bgc->bgc_PopupCloseDelay = 5;
                bgc->bgc_PopupActivateWin = FALSE;
                bgc->bgc_PopupWinToFront = TRUE;
                bgc->bgc_Discoverable = FALSE;
                bgc->bgc_Connectable = TRUE;
                bgc->bgc_DiscoveryTime = 12;
                bgc->bgc_AutoConnect = TRUE;
                bgc->bgc_LocalName[0] = 0;
                bgc->bgc_PrefsVersion = 0; // is updated on writing
                BluetoothBase->bt_ConfigRead = FALSE;
                if(pic) {
                    pic = bAllocForm(BluetoothBase, pic, IFFFORM_BTSTACKCFG);
                    if(pic) {
                        bAddCfgChunk(BluetoothBase, pic, BluetoothBase->bt_GlobalCfg);
                    }
                }

                {
                    STRPTR tmpstr;
                    tmpstr = btCopyStr((STRPTR) VERSION_STRING);
                    if(tmpstr) {
                        tmpstr[strlen(tmpstr)-2] = 0;
                        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Welcome to %s (%p)!", tmpstr, BluetoothBase->bt_ReleaseVersion);
                        btFreeVec(tmpstr);
                    } else {
                        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Welcome to %s", VERSION_STRING);
                    }
                }

                KPRINTF(10, ("libOpen: Ok\n"));
                BluetoothBase->bt_StackInit = TRUE;
                ReleaseSemaphore(&BluetoothBase->bt_ReentrantLock);
                bStartEventHandler(BluetoothBase);

                return TRUE;
            } else {
                KPRINTF(20, ("libOpen: No memory for cfg!\n"));
            }
        } else {
            KPRINTF(20, ("libOpen: OpenDevice(timer.device) failed!\n"));
        }
        ReleaseSemaphore(&BluetoothBase->bt_ReentrantLock);
        return FALSE;
    }
    ReleaseSemaphore(&BluetoothBase->bt_ReentrantLock);
    KPRINTF(5, ("libOpen: openCnt = %ld\n", BluetoothBase->bt_Library.lib_OpenCnt));
    return TRUE;
}

int GM_UNIQUENAME(libExpunge)(LIBBASETYPEPTR BluetoothBase)
{
    struct BtHardware *bth = (struct BtHardware *) BluetoothBase->bt_Hardware.lh_Head;
    struct BtClass *bc = (struct BtClass *) BluetoothBase->bt_Classes.lh_Head;
    struct BtErrorMsg *bem = (struct BtErrorMsg *) BluetoothBase->bt_ErrorMsgs.lh_Head;
    struct BtIFFContext *pic = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
    KPRINTF(10, ("libExpunge BluetoothBase: 0x%p\n", BluetoothBase));
    while(bth->bth_Node.ln_Succ) {
        btRemHardware(bth);
        bth = (struct BtHardware *) BluetoothBase->bt_Hardware.lh_Head;
    }
    while(bc->bc_Node.ln_Succ) {
        btRemClass(bc);
        bc = (struct BtClass *) BluetoothBase->bt_Classes.lh_Head;
    }
    while(bem->bem_Node.ln_Succ) {
        btRemErrorMsg(bem);
        bem = (struct BtErrorMsg *) BluetoothBase->bt_ErrorMsgs.lh_Head;
    }

    while(pic->bic_Node.ln_Succ) {
        bFreeForm(BluetoothBase, pic);
        pic = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
    }

    bStopPopup(BluetoothBase);
    if(BluetoothBase->bt_EventHandler.bh_Task) {
        BluetoothBase->bt_EventHandler.bh_ReadySignal = SIGB_SINGLE;
        BluetoothBase->bt_EventHandler.bh_ReadySigTask = FindTask(NULL);
        Signal(BluetoothBase->bt_EventHandler.bh_Task, SIGBREAKF_CTRL_C);
        while(BluetoothBase->bt_EventHandler.bh_Task) {
            Wait(1L<<BluetoothBase->bt_EventHandler.bh_ReadySignal);
        }
        BluetoothBase->bt_EventHandler.bh_ReadySigTask = NULL;
    }
    bGarbageCollectEvents(BluetoothBase);

    if(BluetoothBase->bt_StackInit) {
        CloseDevice((struct IORequest *) &BluetoothBase->bt_TimerIOReq);
    }
    DeletePool(BluetoothBase->bt_SemaMemPool);
    DeletePool(BluetoothBase->bt_MemPool);

    KPRINTF(1, ("libExpunge: closelibrary utilitybase 0x%p\n",
                UtilityBase));
    CloseLibrary((struct Library *) UtilityBase);

    CloseLibrary(DOSBase);

    KPRINTF(1, ("libExpunge: removing library node 0x%p\n",
                &BluetoothBase->bt_Library.lib_Node));
    Remove(&BluetoothBase->bt_Library.lib_Node);

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

/* *** Memory *** */

struct btMemHeader
{
    APTR  bmem_raw;  /* Pointer returned by AllocPooled() */
    ULONG size;      /* Size requested by caller */
};

/* /// "btAllocVec()" */
AROS_LH1(APTR, btAllocVec,
         AROS_LHA(ULONG, size, D0),
         LIBBASETYPEPTR, BluetoothBase, 5, bt)
{
    AROS_LIBFUNC_INIT
    struct btMemHeader *hdr;
    APTR raw;
    ULONG alloc_size;
    UBYTE *p;
    UBYTE *aligned;
    IPTR mask;

    KPRINTF(1, ("btAllocVec(%ld)\n", size));

    alloc_size = size + sizeof(struct btMemHeader) + (AROS_WORSTALIGN - 1);

    raw = AllocPooled(BluetoothBase->bt_MemPool, alloc_size);
    if (raw)
    {
        p    = (UBYTE *)raw + sizeof(struct btMemHeader);
        mask = (IPTR)AROS_WORSTALIGN - 1;

        aligned = (UBYTE *)(((IPTR)p + mask) & ~mask);
        hdr     = (struct btMemHeader *)(aligned - sizeof(struct btMemHeader));

        hdr->bmem_raw = raw;
        hdr->size     = size;

        BluetoothBase->bt_MemAllocated += size;
        return (APTR)aligned;
    }

    return NULL;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btFreeVec()" */
AROS_LH1(void, btFreeVec,
         AROS_LHA(APTR, pmem, A1),
         LIBBASETYPEPTR, BluetoothBase, 6, bt)
{
    AROS_LIBFUNC_INIT
    struct btMemHeader *hdr;
    ULONG size;
    ULONG alloc_size;

    KPRINTF(1, ("btFreeVec(%p)\n", pmem));

    if (pmem)
    {
        hdr  = ((struct btMemHeader *)pmem) - 1;
        size = hdr->size;

        BluetoothBase->bt_MemAllocated -= size;
        alloc_size = size + sizeof(struct btMemHeader) + (AROS_WORSTALIGN - 1);
        FreePooled(BluetoothBase->bt_MemPool, hdr->bmem_raw, alloc_size);
    }

    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Library base locking *** */

/* /// "bDebugSemaInfo()" */
static void bDebugSemaInfo(LIBBASETYPEPTR BluetoothBase, struct BtSemaInfo *bsi)
{
    struct BtReadLock *brl;
    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "Semaphore %p %s (Excl/SharedLockCount %ld/%ld) (Owner: %s):",
                   bsi->bsi_LockSem,
                   bsi->bsi_LockSem->bls_Node.ln_Name,
                   bsi->bsi_LockSem->bls_ExclLockCount,
                   bsi->bsi_LockSem->bls_SharedLockCount,
                   bsi->bsi_LockSem->bls_Owner ? (const char *)bsi->bsi_LockSem->bls_Owner->tc_Node.ln_Name : "None");

    brl = (struct BtReadLock *) bsi->bsi_LockSem->bls_WaitQueue.lh_Head;
    while(brl->brl_Node.ln_Succ) {
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "  Waiting Task: %p (%s) %s",
                       brl->brl_Task, brl->brl_Task->tc_Node.ln_Name,
                       brl->brl_IsExcl ? "Excl" : "Shared");
        brl = (struct BtReadLock *) brl->brl_Node.ln_Succ;
    }
    brl = (struct BtReadLock *) bsi->bsi_LockSem->bls_ReadLocks.lh_Head;
    while(brl->brl_Node.ln_Succ) {
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "  Readlock Task: %p (%s), Count %ld",
                       brl->brl_Task, brl->brl_Task->tc_Node.ln_Name,
                       brl->brl_Count);
        brl = (struct BtReadLock *) brl->brl_Node.ln_Succ;
    }
}
/* \\\ */

/* /// "bInitSem()" */
void bInitSem(LIBBASETYPEPTR BluetoothBase, struct BtLockSem *bls, STRPTR name)
{
    struct BtSemaInfo *bsi = NULL;
    NewList(&bls->bls_WaitQueue);
    NewList(&bls->bls_ReadLocks);
    bls->bls_Node.ln_Name = name;
    bls->bls_Owner = NULL;
    bls->bls_ExclLockCount = 0;
    bls->bls_SharedLockCount = 0;
    bls->bls_Dead = FALSE;

    Forbid();
    bsi = (struct BtSemaInfo *) AllocPooled(BluetoothBase->bt_SemaMemPool, sizeof(struct BtSemaInfo));
    if(!bsi) {
        Permit();
        return;
    }
    bsi->bsi_LockSem = bls;
    AddTail(&BluetoothBase->bt_DeadlockDebug, &bsi->bsi_Node);
    Permit();
}
/* \\\ */

/* /// "bDeleteSem()" */
void bDeleteSem(LIBBASETYPEPTR BluetoothBase, struct BtLockSem *bls)
{
    struct BtSemaInfo *bsi;
    Forbid();
    bls->bls_Dead = TRUE;
    bsi = (struct BtSemaInfo *) BluetoothBase->bt_DeadlockDebug.lh_Head;
    while(bsi->bsi_Node.ln_Succ) {
        if(bsi->bsi_LockSem == bls) {
            if(bls->bls_SharedLockCount + bls->bls_ExclLockCount) {
                btAddErrorMsg0(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Semaphore still locked when attempting to delete it!\n");
                bDebugSemaInfo(BluetoothBase, bsi);
            } else {
                Remove(&bsi->bsi_Node);
                FreePooled(BluetoothBase->bt_SemaMemPool, bsi, sizeof(struct BtSemaInfo));
            }
            break;
        }
        bsi = (struct BtSemaInfo *) bsi->bsi_Node.ln_Succ;
    }
    Permit();
}
/* \\\ */

/* /// "bLockSemExcl()" */
void bLockSemExcl(LIBBASETYPEPTR BluetoothBase, struct BtLockSem *bls)
{
    struct BtReadLock waitbrl;
    struct Task *thistask = FindTask(NULL);

    waitbrl.brl_Task = thistask;
    waitbrl.brl_IsExcl = TRUE;

    Forbid();
    do {
        // it's already mine!!
        if(thistask == bls->bls_Owner) {
            break;
        }
        if(!bls->bls_ExclLockCount) {
            // easy case: no shared locks, no exclusive locker
            if(!bls->bls_SharedLockCount) {
                break;
            }
            // sole readlock promotion case
            if((bls->bls_SharedLockCount == 1) && ((struct BtReadLock *) bls->bls_ReadLocks.lh_Head)->brl_Task == thistask) {
                KPRINTF(1, ("Promoting read lock (%p) to write lock!\n", thistask));
                break;
            }
        }

        // okay, bad luck, we've got to wait somehow
        AddHead(&bls->bls_WaitQueue, &waitbrl.brl_Node);
        thistask->tc_SigRecvd &= ~SIGF_SINGLE;

        Wait(SIGF_SINGLE);

        Remove(&waitbrl.brl_Node);
    } while(TRUE);
    bls->bls_Owner = thistask;
    bls->bls_ExclLockCount++;
    Permit();
}
/* \\\ */

/* /// "bLockSemShared()" */
void bLockSemShared(LIBBASETYPEPTR BluetoothBase, struct BtLockSem *bls)
{
    struct BtReadLock *brl;
    struct Task *thistask = FindTask(NULL);

    Forbid();
    // is this already locked exclusively by me?
    if(thistask == bls->bls_Owner) {
        // yes? then just increase exclusive lock count
        bls->bls_ExclLockCount++;
        Permit();
        return;
    }

    // find existing readlock
    brl = (struct BtReadLock *) bls->bls_ReadLocks.lh_Head;
    while(brl->brl_Node.ln_Succ) {
        if(brl->brl_Task == thistask) {
            KPRINTF(1, ("Increasing ReadLock (%p) count to %ld\n", thistask, brl->brl_Count));
            brl->brl_Count++;
            Permit();
            return;
        }
        brl = (struct BtReadLock *) brl->brl_Node.ln_Succ;
    }

    // this is a new readlock, generate context
    if(!(brl = (struct BtReadLock *) AllocPooled(BluetoothBase->bt_SemaMemPool, sizeof(struct BtReadLock)))) {
        KPRINTF(20, ("No mem for shared lock! context (%p) on %p\n", thistask, bls));
        // try exclusive lock as fallback (needs no memory)
        Permit();
        bLockSemExcl(BluetoothBase, bls);
        return;
    }

    KPRINTF(1, ("New ReadLockShared context (%p) on %p\n", thistask, bls));
    brl->brl_Task = thistask;
    brl->brl_Count = 0;
    brl->brl_IsExcl = FALSE;

    // if it's exclusively locked, wait for this lock to vanish
    while(bls->bls_Owner) {
        AddTail(&bls->bls_WaitQueue, &brl->brl_Node);
        thistask->tc_SigRecvd &= ~SIGF_SINGLE;

        Wait(SIGF_SINGLE);

        Remove(&brl->brl_Node);
    }

    if(brl->brl_IsExcl) {
        // we got promoted by BorrowLocks during the process! So we don't need the shared stuff anymore
        FreePooled(BluetoothBase->bt_SemaMemPool, brl, sizeof(struct BtReadLock));
        bls->bls_Owner = thistask;
        bls->bls_ExclLockCount++;
    } else {
        // got the lock!
        AddHead(&bls->bls_ReadLocks, &brl->brl_Node);
        brl->brl_Count++;
        bls->bls_SharedLockCount++;
    }
    Permit();
    return;
}
/* \\\ */

/* /// "bUnlockSem()" */
void bUnlockSem(LIBBASETYPEPTR BluetoothBase, struct BtLockSem *bls)
{
    struct BtReadLock *brl;
    struct Task *thistask = FindTask(NULL);
    BOOL gotit = FALSE;

    Forbid();
    if(bls->bls_Owner) {
        // exclusively locked, this means unlocking task must be owner
        if(bls->bls_Owner != thistask) {
            Permit();
            btDebugSemaphores();
            btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Attempt to unlock exclusive semaphore %p not owned by task %s!",
                           bls, thistask->tc_Node.ln_Name);
            return;

        }
        if(--bls->bls_ExclLockCount) {
            // still locked
            Permit();
            return;
        }
        bls->bls_Owner = NULL;
        // otherwise drop through and notify
    } else {
        if(!bls->bls_SharedLockCount) {
            Permit();
            btDebugSemaphores();
            btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Attempt to unlock (free) semaphore %p once too often by task %s!",
                           bls, thistask->tc_Node.ln_Name);
            return;
        }
        // find readlock
        brl = (struct BtReadLock *) bls->bls_ReadLocks.lh_Head;
        while(brl->brl_Node.ln_Succ) {
            if(brl->brl_Task == thistask) {
                if(--brl->brl_Count) {
                    // can't be the last lock, so just reduce count and return
                    Permit();
                    return;
                }
                // remove read lock, it's no longer needed
                KPRINTF(1, ("Removing read lock context (%p) on %p!\n", thistask, bls));
                Remove(&brl->brl_Node);
                FreePooled(BluetoothBase->bt_SemaMemPool, brl, sizeof(struct BtReadLock));
                gotit = TRUE;
                // losing a designated lock
                bls->bls_SharedLockCount--;
                break;
            }
            brl = (struct BtReadLock *) brl->brl_Node.ln_Succ;
        }
        if(!gotit) {
            Permit();
            btDebugSemaphores();
            btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Attempt to unlock (shared) semaphore %p once too often by task %s!",
                           bls, thistask->tc_Node.ln_Name);
            return;
        }
    }

    // notify waiting tasks
    brl = (struct BtReadLock *) bls->bls_WaitQueue.lh_Head;
    while(brl->brl_Node.ln_Succ) {
        Signal(brl->brl_Task, SIGF_SINGLE);
        brl = (struct BtReadLock *) brl->brl_Node.ln_Succ;
    }
    Permit();
}
/* \\\ */

/* /// "btDebugSemaphores()" */
AROS_LH0(void, btDebugSemaphores,
         LIBBASETYPEPTR, BluetoothBase, 85, bt)
{
    AROS_LIBFUNC_INIT
    struct Task *thistask = FindTask(NULL);
    struct BtSemaInfo *bsi;

    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "Debug Semaphores (%p)", thistask);

    Forbid();
    bsi = (struct BtSemaInfo *) BluetoothBase->bt_DeadlockDebug.lh_Head;
    while(bsi->bsi_Node.ln_Succ) {
        bDebugSemaInfo(BluetoothBase, bsi);
        bsi = (struct BtSemaInfo *) bsi->bsi_Node.ln_Succ;
    }
    Permit();
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btLockReadBase()" */
AROS_LH0(void, btLockReadBase,
         LIBBASETYPEPTR, BluetoothBase, 8, bt)
{
    AROS_LIBFUNC_INIT
    KPRINTF(2, ("btLockReadBase(%p)\n", FindTask(NULL)));
    bLockSemShared(BluetoothBase, &BluetoothBase->bt_Lock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btLockWriteBase()" */
AROS_LH0(void, btLockWriteBase,
         LIBBASETYPEPTR, BluetoothBase, 7, bt)
{
    AROS_LIBFUNC_INIT
    KPRINTF(2, ("btLockWriteBase(%p)\n", FindTask(NULL)));
    bLockSemExcl(BluetoothBase, &BluetoothBase->bt_Lock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btUnlockBase()" */
AROS_LH0(void, btUnlockBase,
         LIBBASETYPEPTR, BluetoothBase, 9, bt)
{
    AROS_LIBFUNC_INIT
    KPRINTF(2, ("btUnlockBase(%p)\n", FindTask(NULL)));
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_Lock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btBorrowLocksWait()" */
AROS_LH2(ULONG, btBorrowLocksWait,
         AROS_LHA(struct Task *, task, A1),
         AROS_LHA(ULONG, signals, D0),
         LIBBASETYPEPTR, BluetoothBase, 74, bt)
{
    AROS_LIBFUNC_INIT
    struct Task *thistask = FindTask(NULL);
    ULONG cnt = 0;
    ULONG sigmask;
    struct BtSemaInfo *bsi;
    struct BtLockSem *bls;
    struct BtReadLock *brl;
    struct BtBorrowLock *bbl;
    struct List borrows;
    struct List reclaims;
    BOOL moveowner;

    XPRINTF(10, ("Borrowing locks from %p (%s) to %p (%s)!\n",
                 thistask, thistask->tc_Node.ln_Name, task, task->tc_Node.ln_Name));

    Forbid();
    bsi = (struct BtSemaInfo *) BluetoothBase->bt_DeadlockDebug.lh_Head;
    while(bsi->bsi_Node.ln_Succ) {
        bls = bsi->bsi_LockSem;
        if(bls->bls_Owner == thistask) {
            cnt++;
        }
        if(bls->bls_SharedLockCount) {
            struct BtReadLock *brl = (struct BtReadLock *) bls->bls_ReadLocks.lh_Head;
            do {
                if(brl->brl_Task == thistask) {
                    cnt++;
                    break;
                }
            } while((brl = (struct BtReadLock *) brl->brl_Node.ln_Succ)->brl_Node.ln_Succ);
        }
        bsi = (struct BtSemaInfo *) bsi->bsi_Node.ln_Succ;
    }
    if(!cnt) {
        Permit();
        XPRINTF(10, ("Nothing to borrow!\n"));
        return(Wait(signals));
    }

    NewList(&borrows);
    NewList(&reclaims);
    XPRINTF(10, ("Borrowing %ld locks\n", cnt));

    bsi = (struct BtSemaInfo *) BluetoothBase->bt_DeadlockDebug.lh_Head;
    while(bsi->bsi_Node.ln_Succ) {
        moveowner = TRUE;
        bls = bsi->bsi_LockSem;
        if(bls->bls_Owner == thistask) {
            brl = (struct BtReadLock *) bls->bls_WaitQueue.lh_Head;
            while(brl->brl_Node.ln_Succ) {
                if(brl->brl_Task == task) {
                    if(!brl->brl_IsExcl) {
                        brl->brl_IsExcl = TRUE;
                        XPRINTF(10, ("Promo waiting lock to excl\n"));
                    }
                    Remove(&brl->brl_Node);
                    AddHead(&bls->bls_WaitQueue, &brl->brl_Node);
                    if((bbl = (struct BtBorrowLock *) AllocPooled(BluetoothBase->bt_SemaMemPool, sizeof(struct BtBorrowLock)))) {
                        bbl->bbl_LockSem = bls;
                        bbl->bbl_ExclLockCount = bls->bls_ExclLockCount;
                        AddTail(&reclaims, &bbl->bbl_Node);

                        bls->bls_ExclLockCount = 0;
                        bls->bls_Owner = NULL;
                        Signal(task, SIGF_SINGLE);
                        XPRINTF(10, ("Waiting lock %p transfer\n", bls));
                    }
                    moveowner = FALSE;
                    break;
                }
                brl = (struct BtReadLock *) brl->brl_Node.ln_Succ;
            }
            if(moveowner) {
                if((bbl = (struct BtBorrowLock *) AllocPooled(BluetoothBase->bt_SemaMemPool, sizeof(struct BtBorrowLock)))) {
                    bbl->bbl_LockSem = bls;
                    bbl->bbl_ExclLockCount = bls->bls_ExclLockCount;
                    AddTail(&borrows, &bbl->bbl_Node);
                    bls->bls_Owner = task;
                    XPRINTF(10, ("Lock %p transfer\n", bls));
                }
            }
        }
        if(bls->bls_SharedLockCount) {
            brl = (struct BtReadLock *) bls->bls_ReadLocks.lh_Head;
            do {
                if(brl->brl_Task == thistask) {
                    struct BtReadLock *brl2 = (struct BtReadLock *) bls->bls_WaitQueue.lh_Head;
                    while(brl2->brl_Node.ln_Succ) {
                        if(brl2->brl_Task == task) {
                            Remove(&brl2->brl_Node);
                            AddHead(&bls->bls_WaitQueue, &brl2->brl_Node);
                            if((bbl = (struct BtBorrowLock *) AllocPooled(BluetoothBase->bt_SemaMemPool, sizeof(struct BtBorrowLock)))) {
                                bbl->bbl_LockSem = bls;
                                bbl->bbl_ReadLock = brl;
                                bbl->bbl_Count = brl->brl_Count;
                                AddHead(&reclaims, &bbl->bbl_Node);

                                Remove(&brl->brl_Node);
                                FreePooled(BluetoothBase->bt_SemaMemPool, brl, sizeof(struct BtReadLock));
                                bls->bls_SharedLockCount--;
                                Signal(task, SIGF_SINGLE);
                            }
                            moveowner = FALSE;
                            XPRINTF(10, ("Waiting shared lock %p transfer\n", bls));
                            break;
                        }
                        brl2 = (struct BtReadLock *) brl2->brl_Node.ln_Succ;
                    }
                    if(moveowner) {
                        brl2 = (struct BtReadLock *) bls->bls_ReadLocks.lh_Head;
                        do {
                            if(brl2->brl_Task == task) {
                                if((bbl = (struct BtBorrowLock *) AllocPooled(BluetoothBase->bt_SemaMemPool, sizeof(struct BtBorrowLock)))) {
                                    bbl->bbl_LockSem = bls;
                                    bbl->bbl_ReadLock = brl2;
                                    bbl->bbl_Count = brl->brl_Count;
                                    AddTail(&borrows, &bbl->bbl_Node);

                                    Remove(&brl->brl_Node);
                                    FreePooled(BluetoothBase->bt_SemaMemPool, brl, sizeof(struct BtReadLock));
                                    bls->bls_SharedLockCount--;
                                    brl2->brl_Count += bbl->bbl_Count;
                                }
                                XPRINTF(10, ("Already locked %p transfer\n", bls));
                                moveowner = FALSE;
                                break;
                            }
                        } while((brl2 = (struct BtReadLock *) brl2->brl_Node.ln_Succ)->brl_Node.ln_Succ);
                    }
                    if(moveowner) {
                        if((bbl = (struct BtBorrowLock *) AllocPooled(BluetoothBase->bt_SemaMemPool, sizeof(struct BtBorrowLock)))) {
                            bbl->bbl_LockSem = bls;
                            bbl->bbl_ReadLock = brl;
                            bbl->bbl_Count = brl->brl_Count;
                            AddTail(&borrows, &bbl->bbl_Node);
                            brl->brl_Task = task;
                            XPRINTF(10, ("Std lock %p transfer\n", bls));
                        }
                    }
                    break;
                }
            } while((brl = (struct BtReadLock *) brl->brl_Node.ln_Succ)->brl_Node.ln_Succ);
        }
        bsi = (struct BtSemaInfo *) bsi->bsi_Node.ln_Succ;
    }

    sigmask = Wait(signals);

    // try to get moved locks back first
    bbl = (struct BtBorrowLock *) borrows.lh_Head;
    while(bbl->bbl_Node.ln_Succ) {
        Remove(&bbl->bbl_Node);
        bls = bbl->bbl_LockSem;
        if(bbl->bbl_ExclLockCount) {
            if(bbl->bbl_ExclLockCount == bls->bls_ExclLockCount) {
                bls->bls_Owner = thistask;
                FreePooled(BluetoothBase->bt_SemaMemPool, bbl, sizeof(struct BtBorrowLock));
            } else {
                bls->bls_ExclLockCount -= bbl->bbl_ExclLockCount;
                AddTail(&reclaims, &bbl->bbl_Node);
            }
        } else {
            if(bls->bls_Owner == task) {
                if(bbl->bbl_Count == bbl->bbl_ReadLock->brl_Count) {
                    Remove(&bbl->bbl_ReadLock->brl_Node);
                    FreePooled(BluetoothBase->bt_SemaMemPool, bbl->bbl_ReadLock, sizeof(struct BtReadLock));
                    bbl->bbl_ReadLock = NULL;
                    bls->bls_SharedLockCount--; // should turn to 0
                } else {
                    bbl->bbl_ReadLock->brl_Count -= bbl->bbl_Count;
                    bbl->bbl_ReadLock = NULL;
                }
                AddHead(&reclaims, &bbl->bbl_Node);
            } else {
                if(bbl->bbl_Count == bbl->bbl_ReadLock->brl_Count) {
                    bbl->bbl_ReadLock->brl_Task = thistask;
                    FreePooled(BluetoothBase->bt_SemaMemPool, bbl, sizeof(struct BtBorrowLock));
                } else {
                    bbl->bbl_ReadLock->brl_Count -= bbl->bbl_Count;
                    bbl->bbl_ReadLock = NULL;
                    AddHead(&reclaims, &bbl->bbl_Node);
                }
            }
        }
        bbl = (struct BtBorrowLock *) borrows.lh_Head;
    }

    // try to reclaim released locks
    bbl = (struct BtBorrowLock *) reclaims.lh_Head;
    while(bbl->bbl_Node.ln_Succ) {
        Remove(&bbl->bbl_Node);
        bls = bbl->bbl_LockSem;
        while(bbl->bbl_Count) {
            bLockSemShared(BluetoothBase, bls);
            --bbl->bbl_Count;
        }
        while(bbl->bbl_ExclLockCount) {
            bLockSemExcl(BluetoothBase, bls);
            --bbl->bbl_ExclLockCount;
        }
        FreePooled(BluetoothBase->bt_SemaMemPool, bbl, sizeof(struct BtBorrowLock));
        bbl = (struct BtBorrowLock *) reclaims.lh_Head;
    }
    Permit();

    return(sigmask);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Support *** */

/* /// "btCopyStr()" */
AROS_LH1(STRPTR, btCopyStr,
         AROS_LHA(CONST_STRPTR, name, A0),
         LIBBASETYPEPTR, BluetoothBase, 10, bt)
{
    AROS_LIBFUNC_INIT
    STRPTR rs = btAllocVec((ULONG) strlen(name)+1);
    KPRINTF(1, ("btCopyStr(%s)\n", name));
    if(rs) {
        strcpy(rs, name);
    }
    return(rs);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btSafeRawDoFmtA()" */
AROS_LH4(void, btSafeRawDoFmtA,
         AROS_LHA(STRPTR, buf, A0),
         AROS_LHA(ULONG, len, D0),
         AROS_LHA(CONST_STRPTR, fmtstr, A1),
         AROS_LHA(RAWARG, fmtdata, A2),
         LIBBASETYPEPTR, BluetoothBase, 40, bt)
{
    AROS_LIBFUNC_INIT
    struct BtRawDoFmt rdf;

    if(len > 0) {
        rdf.rdf_Len = len;
        rdf.rdf_Buf = buf;
        RawDoFmt(fmtstr, fmtdata, (void (*)()) bPutChar, &rdf);
        buf[len-1] = 0;
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "bPutChar()" */
AROS_UFH2(void, bPutChar,
          AROS_UFHA(char, ch, D0),
          AROS_UFHA(struct BtRawDoFmt *, rdf, A3))
{
    AROS_USERFUNC_INIT
    if(rdf->rdf_Len) {
        rdf->rdf_Len--;
        *rdf->rdf_Buf++ = ch;
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "btCopyStrFmtA()" */
AROS_LH2(STRPTR, btCopyStrFmtA,
         AROS_LHA(CONST_STRPTR, fmtstr, A0),
         AROS_LHA(RAWARG, fmtdata, A1),
         LIBBASETYPEPTR, BluetoothBase, 41, bt)
{
    AROS_LIBFUNC_INIT
    ULONG len = 0;
    STRPTR buf;

    RawDoFmt(fmtstr, fmtdata, (void (*)()) bRawFmtLength, &len);
    buf = btAllocVec(len+1);
    if(buf) {
        btSafeRawDoFmtA(buf, len+1, fmtstr, fmtdata);
    }
    return(buf);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "bRawFmtLength()" */
AROS_UFH2(void, bRawFmtLength,
          AROS_UFHA(char, ch, D0),
          AROS_UFHA(ULONG *, len, A3))
{
    AROS_USERFUNC_INIT
    (*len)++;
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "btDelayMS()" */
AROS_LH1(void, btDelayMS,
         AROS_LHA(ULONG, milli, D0),
         LIBBASETYPEPTR, BluetoothBase, 11, bt)
{
    AROS_LIBFUNC_INIT
    struct MsgPort mp;
    struct timerequest tr;

    memset(&mp, 0, sizeof(mp));

    KPRINTF(1, ("btDelayMS(%ld)\n", milli));
    mp.mp_Flags = PA_SIGNAL;
    mp.mp_SigBit = SIGB_SINGLE;
    mp.mp_SigTask = FindTask(NULL);
    NewList(&mp.mp_MsgList);
    CopyMem(&BluetoothBase->bt_TimerIOReq, &tr, sizeof(tr));
    tr.tr_node.io_Message.mn_ReplyPort = &mp;
    tr.tr_time.tv_secs  = milli / 1000;
    tr.tr_time.tv_micro = (milli % 1000) * 1000;
    DoIO((struct IORequest *) &tr);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btSpawnSubTask()" */
AROS_LH3(struct Task *, btSpawnSubTask,
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(APTR, initpc, A1),
         AROS_LHA(APTR, userdata, A2),
         LIBBASETYPEPTR, BluetoothBase, 37, bt)
{
    AROS_LIBFUNC_INIT
#define SUBTASKSTACKSIZE AROS_STACKSIZE
    struct {
        struct MemList mrm_ml;
        struct MemEntry mtm_me[2];
    } memlist;

    struct MemList *newmemlist;
    struct MemEntry *me;
    struct Task *nt;
    struct Process *subtask;

    if(!(name && initpc)) {
        return(NULL);
    }

    /* If there's dos available, create a process instead of a task */
    if(bOpenDOS(BluetoothBase)) {
        subtask = CreateNewProcTags(NP_Entry, initpc,
                                    NP_StackSize, SUBTASKSTACKSIZE,
                                    NP_Priority, BluetoothBase->bt_GlobalCfg->bgc_SubTaskPri,
                                    NP_Name, name,
                                    NP_CopyVars, FALSE,
                                    NP_UserData, userdata,
                                    TAG_END);
        return((struct Task *) subtask);
    }

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

    newmemlist = NewAllocEntry(&memlist.mrm_ml, NULL);
    if (!newmemlist)
    {
        return(NULL);
    }
    me = &newmemlist->ml_ME[0];
    nt = me[0].me_Un.meu_Addr;
    nt->tc_Node.ln_Name = me[2].me_Un.meu_Addr;
    strcpy(nt->tc_Node.ln_Name, name);
    nt->tc_Node.ln_Type = NT_TASK;
    nt->tc_Node.ln_Pri = BluetoothBase->bt_GlobalCfg->bgc_SubTaskPri;
    nt->tc_SPLower = me[1].me_Un.meu_Addr;
    nt->tc_SPUpper = nt->tc_SPReg = (APTR) ((IPTR) nt->tc_SPLower + SUBTASKSTACKSIZE);
    nt->tc_UserData = userdata;
    NewList(&nt->tc_MemEntry);
    AddTail(&nt->tc_MemEntry, (struct Node *) newmemlist);
    if((nt = AddTask(nt, initpc, NULL))) {
        XPRINTF(10, ("Started task %p (%s)\n", nt, name));
        return(nt);
    }
    FreeEntry(newmemlist);
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "bHaveDOS()" */
BOOL bHaveDOS(LIBBASETYPEPTR BluetoothBase)
{
    if(DOSBase) {
        return TRUE;
    }
    return FALSE;
}
/* \\\ */

/* /// "bOpenDOS()" */
BOOL bOpenDOS(LIBBASETYPEPTR BluetoothBase)
{
    if(DOSBase) {
        return TRUE;
    }
    if((DOSBase = OpenLibrary("dos.library", 39))) {
        return TRUE;
    }
    return FALSE;
}
/* \\\ */

/* /// "bFindName()" */
struct Node * bFindName(LIBBASETYPEPTR BluetoothBase, struct List *list, STRPTR name)
{
    struct Node *res = NULL;

    Forbid();
    while(*name) {
        res = FindName(list, name);
        if(res) {
            break;
        }
        do {
            if((*name == '/') || (*name == ':')) {
                ++name;
                break;
            }
        } while(*(++name));
    }
    Permit();
    return(res);
}
/* \\\ */

/* /// "bStripString()" */
void bStripString(LIBBASETYPEPTR BluetoothBase, STRPTR str)
{
    STRPTR srcptr = str;
    STRPTR tarptr = str;
    STRPTR lastgoodchar = str;
    BOOL leadingspaces = TRUE;
    UBYTE ch;
    ULONG len = 0;

    while((ch = *srcptr++)) {
        len++;
        if(ch == ' ') {
            if(!leadingspaces) {
                *tarptr++ = ch;
            }
        } else {
            *tarptr++ = ch;
            lastgoodchar = tarptr;
            leadingspaces = FALSE;
        }
    }
    *lastgoodchar = 0;
    if((str == lastgoodchar) && (len > 6)) {
        strcpy(str, "<empty>");
    }
}
/* \\\ */

/* *** Address / UUID helpers *** */

/* /// "bAddrToStr()" */
void bAddrToStr(const UBYTE *addr, STRPTR buf)
{
    static const char hex[] = "0123456789ABCDEF";
    WORD i;
    /* addresses are stored little endian on the wire; print human order */
    for(i = 5; i >= 0; i--) {
        *buf++ = hex[addr[i] >> 4];
        *buf++ = hex[addr[i] & 15];
        if(i) {
            *buf++ = ':';
        }
    }
    *buf = 0;
}
/* \\\ */

/* /// "bStrToAddr()" */
BOOL bStrToAddr(CONST_STRPTR str, UBYTE *addr)
{
    WORD i;
    for(i = 5; i >= 0; i--) {
        UBYTE v = 0;
        WORD n;
        for(n = 0; n < 2; n++) {
            UBYTE c = *str++;
            v <<= 4;
            if((c >= '0') && (c <= '9')) {
                v |= c - '0';
            } else if((c >= 'a') && (c <= 'f')) {
                v |= c - 'a' + 10;
            } else if((c >= 'A') && (c <= 'F')) {
                v |= c - 'A' + 10;
            } else {
                return(FALSE);
            }
        }
        addr[i] = v;
        if(i) {
            if((*str != ':') && (*str != '-')) {
                return(FALSE);
            }
            str++;
        }
    }
    return(TRUE);
}
/* \\\ */

/* /// "bUUID16To128()" */
/* Bluetooth base UUID 00000000-0000-1000-8000-00805F9B34FB, big endian */
static const UBYTE btBaseUUID[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                                      0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB };

void bUUID16To128(UWORD uuid16, UBYTE *uuid128)
{
    memcpy(uuid128, btBaseUUID, 16);
    uuid128[2] = uuid16 >> 8;
    uuid128[3] = uuid16 & 0xff;
}
/* \\\ */

/* /// "bUUID128To16()" */
BOOL bUUID128To16(const UBYTE *uuid128, UWORD *uuid16)
{
    if((uuid128[0] == 0) && (uuid128[1] == 0) && (memcmp(&uuid128[4], &btBaseUUID[4], 12) == 0)) {
        *uuid16 = ((UWORD) uuid128[2] << 8) | uuid128[3];
        return(TRUE);
    }
    return(FALSE);
}
/* \\\ */

/* /// "bUUIDToStr()" */
void bUUIDToStr(const UBYTE *uuid128, STRPTR buf)
{
    static const char hex[] = "0123456789abcdef";
    UWORD i;
    for(i = 0; i < 16; i++) {
        *buf++ = hex[uuid128[i] >> 4];
        *buf++ = hex[uuid128[i] & 15];
        if((i == 3) || (i == 5) || (i == 7) || (i == 9)) {
            *buf++ = '-';
        }
    }
    *buf = 0;
}
/* \\\ */

/* *** Attributes *** */

/* /// "Packtables for btGetAttrs() and btSetAttrs() " */
/* Pack table for BtBase */
static const ULONG BtBasePT[] = {
    PACK_STARTTABLE(BSA_Dummy),
    PACK_ENTRY(BSA_Dummy, BSA_ConfigRead, BtBase, bt_ConfigRead, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BSA_Dummy, BSA_GlobalConfig, BtBase, bt_GlobalCfg, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSA_Dummy, BSA_MemPoolUsage, BtBase, bt_MemAllocated, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSA_Dummy, BSA_CurrConfigHash, BtBase, bt_ConfigHash, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSA_Dummy, BSA_SavedConfigHash, BtBase, bt_SavedConfigHash, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BSA_Dummy, BSA_ReleaseVersion, BtBase, bt_ReleaseVersion, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSA_Dummy, BSA_OSVersion, BtBase, bt_OSVersion, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

/* Pack table for BtErrorMsg */
static const ULONG BtErrorMsgPT[] = {
    PACK_STARTTABLE(BEMA_Dummy),
    PACK_ENTRY(BEMA_Dummy, BEMA_Level, BtErrorMsg, bem_Level, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEMA_Dummy, BEMA_Origin, BtErrorMsg, bem_Origin, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEMA_Dummy, BEMA_Msg, BtErrorMsg, bem_Msg, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

/* Pack table for BtClass */
static const ULONG BtClassPT[] = {
    PACK_STARTTABLE(BCA_Dummy),
    PACK_ENTRY(BCA_Dummy, BCA_ClassBase, BtClass, bc_ClassBase, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BCA_Dummy, BCA_ClassName, BtClass, bc_ClassName, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BCA_Dummy, BCA_FullPath, BtClass, bc_FullPath, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BCA_Dummy, BCA_UseCount, BtClass, bc_UseCnt, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

/* Pack table for BtHardware */
static const ULONG BtHardwarePT[] = {
    PACK_STARTTABLE(BHA_Dummy),
    PACK_ENTRY(BHA_Dummy, BHA_DeviceName, BtHardware, bth_DevName, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_DeviceUnit, BtHardware, bth_Unit, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_ProductName, BtHardware, bth_ProductName, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_Manufacturer, BtHardware, bth_Manufacturer, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_Description, BtHardware, bth_Description, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_Copyright, BtHardware, bth_Copyright, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_Version, BtHardware, bth_Version, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_Revision, BtHardware, bth_Revision, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_DriverVersion, BtHardware, bth_DriverVers, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_NumDevices, BtHardware, bth_NumDevices, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_State, BtHardware, bth_State, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_LocalName, BtHardware, bth_LocalName, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_ClassOfDevice, BtHardware, bth_ClassOfDevice, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_HCIVersion, BtHardware, bth_HCIVersion, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_HCIRevision, BtHardware, bth_HCIRevision, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_LMPVersion, BtHardware, bth_LMPVersion, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_LMPSubversion, BtHardware, bth_LMPSubversion, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_ManufacturerID, BtHardware, bth_ManufacturerID, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_ACLMaxPktSize, BtHardware, bth_ACLMaxPktSize, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_ACLNumPkts, BtHardware, bth_ACLNumPkts, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_SCOMaxPktSize, BtHardware, bth_SCOMaxPktSize, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_SCONumPkts, BtHardware, bth_SCONumPkts, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_LEACLMaxPktSize, BtHardware, bth_LEACLMaxPktSize, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_LEACLNumPkts, BtHardware, bth_LEACLNumPkts, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_ErrorCount, BtHardware, bth_ErrorCount, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BHA_Dummy, BHA_LastHCIError, BtHardware, bth_LastHCIError, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_WORDBIT(BHA_Dummy, BHA_IsClassic, BtHardware, bth_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BTHF_CLASSIC),
    PACK_WORDBIT(BHA_Dummy, BHA_IsLE, BtHardware, bth_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BTHF_LE),
    PACK_WORDBIT(BHA_Dummy, BHA_IsDiscovering, BtHardware, bth_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BTHF_DISCOVERING),
    PACK_WORDBIT(BHA_Dummy, BHA_Discoverable, BtHardware, bth_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BTHF_DISCOVERABLE),
    PACK_WORDBIT(BHA_Dummy, BHA_Connectable, BtHardware, bth_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BTHF_CONNECTABLE),
    PACK_ENDTABLE
};

/* Pack table for BtDevice */
static const ULONG BtDevicePT[] = {
    PACK_STARTTABLE(BDA_Dummy),
    PACK_ENTRY(BDA_Dummy, BDA_AddressType, BtDevice, bd_AddrType, PKCTRL_UBYTE|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_Name, BtDevice, bd_Name, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_OrigName, BtDevice, bd_OrigName, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_ClassOfDevice, BtDevice, bd_ClassOfDevice, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_Appearance, BtDevice, bd_Appearance, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_RSSI, BtDevice, bd_RSSI, PKCTRL_LONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_ConnHandle, BtDevice, bd_ConnHandle, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_Role, BtDevice, bd_Role, PKCTRL_UBYTE|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_LinkType, BtDevice, bd_LinkType, PKCTRL_UBYTE|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_BondFlags, BtDevice, bd_Keys.bkc_Flags, PKCTRL_UBYTE|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_Hardware, BtDevice, bd_Hardware, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_Binding, BtDevice, bd_DevBinding, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BDA_Dummy, BDA_BindingClass, BtDevice, bd_ClsBinding, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BDA_Dummy, BDA_NumServices, BtDevice, bd_NumServices, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_IDString, BtDevice, bd_IDString, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_VendorID, BtDevice, bd_VendorID, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_ProductID, BtDevice, bd_ProductID, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_ProductVersion, BtDevice, bd_ProductVersion, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_VendorIDSource, BtDevice, bd_VendorIDSource, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_LMPVersion, BtDevice, bd_LMPVersion, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_ManufacturerID, BtDevice, bd_ManufacturerID, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_AdvDataLength, BtDevice, bd_AdvDataLen, PKCTRL_UBYTE|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_PairingState, BtDevice, bd_PairingState, PKCTRL_UBYTE|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_PairingRequest, BtDevice, bd_PairingRequest, PKCTRL_UBYTE|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_PairingPasskey, BtDevice, bd_PairingPasskey, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_IsNewToMe, BtDevice, bd_IsNewToMe, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BDA_Dummy, BDA_InhibitPopup, BtDevice, bd_PoPoCfg.bpc_InhibitPopup, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BDA_Dummy, BDA_InhibitClassBind, BtDevice, bd_PoPoCfg.bpc_NoClassBind, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BDA_Dummy, BDA_AutoConnect, BtDevice, bd_PoPoCfg.bpc_AutoConnect, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BDA_Dummy, BDA_Trusted, BtDevice, bd_PoPoCfg.bpc_Trusted, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_WORDBIT(BDA_Dummy, BDA_IsClassic, BtDevice, bd_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BDFF_CLASSIC),
    PACK_WORDBIT(BDA_Dummy, BDA_IsLE, BtDevice, bd_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BDFF_LE),
    PACK_WORDBIT(BDA_Dummy, BDA_IsDiscovered, BtDevice, bd_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BDFF_DISCOVERED),
    PACK_WORDBIT(BDA_Dummy, BDA_IsRegistered, BtDevice, bd_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BDFF_REGISTERED),
    PACK_WORDBIT(BDA_Dummy, BDA_IsBonded, BtDevice, bd_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BDFF_BONDED),
    PACK_WORDBIT(BDA_Dummy, BDA_IsConnected, BtDevice, bd_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BDFF_CONNECTED),
    PACK_WORDBIT(BDA_Dummy, BDA_IsEncrypted, BtDevice, bd_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BDFF_ENCRYPTED),
    PACK_WORDBIT(BDA_Dummy, BDA_IsDead, BtDevice, bd_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BDFF_DEAD),
    PACK_WORDBIT(BDA_Dummy, BDA_HasAppBinding, BtDevice, bd_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BDFF_APPBINDING),
    PACK_WORDBIT(BDA_Dummy, BDA_ServicesKnown, BtDevice, bd_Flags, PKCTRL_BIT|PKCTRL_UNPACKONLY, BDFF_SERVICESKNOWN),
    PACK_ENDTABLE
};

/* Pack table for BtService */
static const ULONG BtServicePT[] = {
    PACK_STARTTABLE(BSVA_Dummy),
    PACK_ENTRY(BSVA_Dummy, BSVA_Device, BtService, bsv_Device, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_UUID16, BtService, bsv_UUID16, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_Name, BtService, bsv_Name, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_Protocol, BtService, bsv_Protocol, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_PSM, BtService, bsv_PSM, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_RFCOMMChannel, BtService, bsv_RFCOMMChannel, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_RecordHandle, BtService, bsv_RecordHandle, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_StartHandle, BtService, bsv_StartHandle, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_EndHandle, BtService, bsv_EndHandle, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_Version, BtService, bsv_Version, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_IsPrimary, BtService, bsv_IsPrimary, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_Binding, BtService, bsv_SvcBinding, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BSVA_Dummy, BSVA_BindingClass, BtService, bsv_ClsBinding, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BSVA_Dummy, BSVA_IDString, BtService, bsv_IDString, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_NumEndpoints, BtService, bsv_NumEPs, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_ServiceClassIDs, BtService, bsv_ServiceClassIDs, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_HIDDescriptor, BtService, bsv_HidDescriptor, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BSVA_Dummy, BSVA_HIDDescriptorLen, BtService, bsv_HidDescriptorLen, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

/* Pack table for BtEndpoint */
static const ULONG BtEndpointPT[] = {
    PACK_STARTTABLE(BEA_Dummy),
    PACK_ENTRY(BEA_Dummy, BEA_CanRead, BtEndpoint, bep_CanRead, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_CanWrite, BtEndpoint, bep_CanWrite, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_Service, BtEndpoint, bep_Service, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_Type, BtEndpoint, bep_Type, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_PSM, BtEndpoint, bep_PSM, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_CID, BtEndpoint, bep_CID, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_RFCOMMChannel, BtEndpoint, bep_RFCOMMChannel, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_Handle, BtEndpoint, bep_Handle, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_UUID16, BtEndpoint, bep_UUID16, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_Properties, BtEndpoint, bep_Properties, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_MaxPktSize, BtEndpoint, bep_MaxPktSize, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_Name, BtEndpoint, bep_Name, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_ReportID, BtEndpoint, bep_ReportID, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_ReportType, BtEndpoint, bep_ReportType, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_CCCDHandle, BtEndpoint, bep_CCCDHandle, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BEA_Dummy, BEA_ReportRefHandle, BtEndpoint, bep_RefHandle, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

/* Pack table for BtChannel */
static const ULONG BtChannelPT[] = {
    PACK_STARTTABLE(BCHA_Dummy),
    PACK_ENTRY(BCHA_Dummy, BCHA_Endpoint, BtChannel, bch_Endpoint, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BCHA_Dummy, BCHA_Error, BtChannel, bch_Error, PKCTRL_LONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BCHA_Dummy, BCHA_Actual, BtChannel, bch_Actual, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BCHA_Dummy, BCHA_Device, BtChannel, bch_Device, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BCHA_Dummy, BCHA_Timeout, BtChannel, bch_Timeout, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_WORDBIT(BCHA_Dummy, BCHA_AutoConnect, BtChannel, bch_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, BCHF_AUTOCONNECT),
    PACK_WORDBIT(BCHA_Dummy, BCHA_NoWait, BtChannel, bch_Flags, PKCTRL_BIT|PKCTRL_PACKUNPACK, BCHF_NOWAIT),
    PACK_ENDTABLE
};

/* Pack table for BtAppBinding */
static const ULONG BtAppBindingPT[] = {
    PACK_STARTTABLE(BABA_Dummy),
    PACK_ENTRY(BABA_Dummy, BABA_ReleaseHook, BtAppBinding, bab_ReleaseHook, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BABA_Dummy, BABA_Device, BtAppBinding, bab_Device, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BABA_Dummy, BABA_UserData, BtAppBinding, bab_UserData, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BABA_Dummy, BABA_Task, BtAppBinding, bab_Task, PKCTRL_IPTR|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BABA_Dummy, BABA_ForceRelease, BtAppBinding, bab_ForceRelease, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENDTABLE
};

/* Pack table for BtEventNote */
static const ULONG BtEventNotePT[] = {
    PACK_STARTTABLE(BENA_Dummy),
    PACK_ENTRY(BENA_Dummy, BENA_EventID, BtEventNote, ben_Event, PKCTRL_UWORD|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BENA_Dummy, BENA_Param1, BtEventNote, ben_Param1, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENTRY(BENA_Dummy, BENA_Param2, BtEventNote, ben_Param2, PKCTRL_IPTR|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

/* Pack table for BtGlobalCfg */
static const ULONG BtGlobalCfgPT[] = {
    PACK_STARTTABLE(BGCA_Dummy),
    PACK_ENTRY(BGCA_Dummy, BGCA_LogInfo, BtGlobalCfg, bgc_LogInfo, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_LogWarning, BtGlobalCfg, bgc_LogWarning, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_LogError, BtGlobalCfg, bgc_LogError, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_LogFailure, BtGlobalCfg, bgc_LogFailure, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_SubTaskPri, BtGlobalCfg, bgc_SubTaskPri, PKCTRL_WORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_BootDelay, BtGlobalCfg, bgc_BootDelay, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_PopupDeviceNew, BtGlobalCfg, bgc_PopupDeviceNew, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_PopupDeviceGone, BtGlobalCfg, bgc_PopupDeviceGone, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_PopupPairing, BtGlobalCfg, bgc_PopupPairing, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_PopupCloseDelay, BtGlobalCfg, bgc_PopupCloseDelay, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_PopupActivateWin, BtGlobalCfg, bgc_PopupActivateWin, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_PopupWinToFront, BtGlobalCfg, bgc_PopupWinToFront, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_Discoverable, BtGlobalCfg, bgc_Discoverable, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_Connectable, BtGlobalCfg, bgc_Connectable, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_DiscoveryTime, BtGlobalCfg, bgc_DiscoveryTime, PKCTRL_ULONG|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_AutoConnect, BtGlobalCfg, bgc_AutoConnect, PKCTRL_UWORD|PKCTRL_PACKUNPACK),
    PACK_ENTRY(BGCA_Dummy, BGCA_PrefsVersion, BtGlobalCfg, bgc_PrefsVersion, PKCTRL_ULONG|PKCTRL_UNPACKONLY),
    PACK_ENDTABLE
};

static const ULONG *BtPTArray[BGA_LAST+1] = {
    NULL,
    BtBasePT,       /* BGA_STACK */
    BtClassPT,      /* BGA_BTCLASS */
    BtHardwarePT,   /* BGA_HARDWARE */
    BtDevicePT,     /* BGA_DEVICE */
    BtServicePT,    /* BGA_SERVICE */
    BtEndpointPT,   /* BGA_ENDPOINT */
    BtErrorMsgPT,   /* BGA_ERRORMSG */
    BtChannelPT,       /* BGA_CHANNEL */
    BtAppBindingPT, /* BGA_APPBINDING */
    BtEventNotePT,  /* BGA_EVENTNOTE */
    BtGlobalCfgPT   /* BGA_STACKCFG */
};
/* \\\ */

/* /// "btGetAttrsA()" */
AROS_LH3(LONG, btGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, btstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, BluetoothBase, 21, bt)
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti;
    ULONG count = 0;
    ULONG *packtab = NULL;

    KPRINTF(1, ("btGetAttrsA(%ld, %p, %p)\n", type, btstruct, tags));

    if(type <= BGA_LAST) {
        packtab = (ULONG *) BtPTArray[type];
    }

    switch(type) {
    case BGA_STACK:
        btstruct = BluetoothBase;
        if((ti = FindTagItem(BSA_HardwareList, tags))) {
            *((struct List **) ti->ti_Data) = &BluetoothBase->bt_Hardware;
            count++;
        }
        if((ti = FindTagItem(BSA_ClassList, tags))) {
            *((struct List **) ti->ti_Data) = &BluetoothBase->bt_Classes;
            count++;
        }
        if((ti = FindTagItem(BSA_ErrorMsgList, tags))) {
            *((struct List **) ti->ti_Data) = &BluetoothBase->bt_ErrorMsgs;
            count++;
        }
        break;

    case BGA_STACKCFG:
        btstruct = BluetoothBase->bt_GlobalCfg;
        if((ti = FindTagItem(BGCA_LocalName, tags))) {
            *((STRPTR *) ti->ti_Data) = (STRPTR) BluetoothBase->bt_GlobalCfg->bgc_LocalName;
            count++;
        }
        break;

    case BGA_HARDWARE: {
        struct BtHardware *bth = (struct BtHardware *) btstruct;
        if((ti = FindTagItem(BHA_DeviceList, tags))) {
            *((struct List **) ti->ti_Data) = &bth->bth_Devices;
            count++;
        }
        if((ti = FindTagItem(BHA_Address, tags))) {
            *((BD_ADDR **) ti->ti_Data) = &bth->bth_Address;
            count++;
        }
        if((ti = FindTagItem(BHA_AddressString, tags))) {
            *((STRPTR *) ti->ti_Data) = (STRPTR) bth->bth_AddrString;
            count++;
        }
        if((ti = FindTagItem(BHA_Features, tags))) {
            *((UBYTE **) ti->ti_Data) = bth->bth_Features;
            count++;
        }
        if((ti = FindTagItem(BHA_ManufacturerName, tags))) {
            *((STRPTR *) ti->ti_Data) = btNumToStr(BNTS_MANUFACTURER, bth->bth_ManufacturerID, "unknown");
            count++;
        }
        if((ti = FindTagItem(BHA_IsReady, tags))) {
            *((IPTR *) ti->ti_Data) = (bth->bth_State == BHS_READY);
            count++;
        }
        break;
    }

    case BGA_DEVICE: {
        struct BtDevice *bd = (struct BtDevice *) btstruct;
        if((ti = FindTagItem(BDA_ServiceList, tags))) {
            *((struct List **) ti->ti_Data) = &bd->bd_Services;
            count++;
        }
        if((ti = FindTagItem(BDA_Address, tags))) {
            *((BD_ADDR **) ti->ti_Data) = &bd->bd_Address;
            count++;
        }
        if((ti = FindTagItem(BDA_AddressString, tags))) {
            *((STRPTR *) ti->ti_Data) = (STRPTR) bd->bd_AddrString;
            count++;
        }
        if((ti = FindTagItem(BDA_AdvData, tags))) {
            *((UBYTE **) ti->ti_Data) = bd->bd_AdvData;
            count++;
        }
        if((ti = FindTagItem(BDA_LastSeen, tags))) {
            *((struct DateStamp **) ti->ti_Data) = &bd->bd_LastSeen;
            count++;
        }
        if((ti = FindTagItem(BDA_FirstSeen, tags))) {
            *((struct DateStamp **) ti->ti_Data) = &bd->bd_FirstSeen;
            count++;
        }
        break;
    }

    case BGA_SERVICE: {
        struct BtService *bsv = (struct BtService *) btstruct;
        if((ti = FindTagItem(BSVA_EndpointList, tags))) {
            *((struct List **) ti->ti_Data) = &bsv->bsv_Endpoints;
            count++;
        }
        if((ti = FindTagItem(BSVA_UUID, tags))) {
            *((UBYTE **) ti->ti_Data) = bsv->bsv_UUID;
            count++;
        }
        if((ti = FindTagItem(BSVA_UUIDString, tags))) {
            /* returns a static per-service buffer built from the UUID */
            *((STRPTR *) ti->ti_Data) = bsv->bsv_IDString;
            count++;
        }
        break;
    }

    case BGA_ENDPOINT: {
        struct BtEndpoint *bep = (struct BtEndpoint *) btstruct;
        if((ti = FindTagItem(BEA_UUID, tags))) {
            *((UBYTE **) ti->ti_Data) = bep->bep_UUID;
            count++;
        }
        if((ti = FindTagItem(BEA_IsOpen, tags))) {
            *((IPTR *) ti->ti_Data) = (bep->bep_Chan != NULL);
            count++;
        }
        break;
    }

    case BGA_ERRORMSG:
        if((ti = FindTagItem(BEMA_DateStamp, tags))) {
            *((struct DateStamp **) ti->ti_Data) = &(((struct BtErrorMsg *) btstruct)->bem_DateStamp);
            count++;
        }
        break;

    case BGA_CHANNEL: {
        struct BtChannel *bch = (struct BtChannel *) btstruct;
        if((ti = FindTagItem(BCHA_MaxPktSize, tags))) {
            *((IPTR *) ti->ti_Data) = bch->bch_Endpoint ? bch->bch_Endpoint->bep_MaxPktSize : 0;
            count++;
        }
        break;
    }
    }
    if(packtab) {
        return((LONG) (UnpackStructureTags(btstruct, (ULONG *) packtab, tags)+count));
    } else {
        return(-1);
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btSetAttrsA()" */
AROS_LH3(LONG, btSetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, btstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, BluetoothBase, 22, bt)
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti;
    ULONG count = 0;
    ULONG *packtab = NULL;
    BOOL savepopocfg = FALSE;
    BOOL checkcfgupdate = FALSE;
    LONG res;

    KPRINTF(1, ("btSetAttrsA(%ld, %p, %p)\n", type, btstruct, tags));

    if(type <= BGA_LAST) {
        packtab = (ULONG *) BtPTArray[type];
    }

    switch(type) {
    case BGA_DEVICE: {
        struct BtDevice *bd = (struct BtDevice *) btstruct;
        if(FindTagItem(BDA_InhibitPopup, tags) || FindTagItem(BDA_InhibitClassBind, tags) ||
           FindTagItem(BDA_AutoConnect, tags) || FindTagItem(BDA_Trusted, tags)) {
            savepopocfg = TRUE;
        }
        if((ti = FindTagItem(BDA_Name, tags))) {
            STRPTR newname = (STRPTR) ti->ti_Data;
            if(newname && *newname && (!bd->bd_Name || strcmp(bd->bd_Name, newname))) {
                btFreeVec(bd->bd_Name);
                bd->bd_Name = btCopyStr(newname);
                bd->bd_Node.ln_Name = bd->bd_Name;
                savepopocfg = TRUE;
            }
            count++;
        }
        break;
    }

    case BGA_HARDWARE: {
        struct BtHardware *bth = (struct BtHardware *) btstruct;
        LONG err = 0;
        if((ti = FindTagItem(BHA_LocalName, tags))) {
            bSubmitCtrl(BluetoothBase, bth, NULL, BTPRI_SETLOCALNAME, 0, 0, (APTR) ti->ti_Data,
                        ti->ti_Data ? strlen((STRPTR) ti->ti_Data) : 0, &err);
            count++;
        }
        if((ti = FindTagItem(BHA_ClassOfDevice, tags))) {
            ULONG cod = ti->ti_Data;
            bSubmitCtrl(BluetoothBase, bth, NULL, BTPRI_SETCOD, 0, 0, &cod, sizeof(cod), &err);
            count++;
        }
        if(FindTagItem(BHA_Discoverable, tags) || FindTagItem(BHA_Connectable, tags)) {
            UWORD disc = (bth->bth_Flags & BTHF_DISCOVERABLE) ? 1 : 0;
            UWORD conn = (bth->bth_Flags & BTHF_CONNECTABLE) ? 1 : 0;
            if((ti = FindTagItem(BHA_Discoverable, tags))) {
                disc = ti->ti_Data ? 1 : 0;
                count++;
            }
            if((ti = FindTagItem(BHA_Connectable, tags))) {
                conn = ti->ti_Data ? 1 : 0;
                count++;
            }
            bSubmitCtrl(BluetoothBase, bth, NULL, BTPRI_SETSCANMODE, disc, conn, NULL, 0, &err);
        }
        break;
    }

    case BGA_STACK:
        btstruct = BluetoothBase;
        break;

    case BGA_STACKCFG:
        btstruct = BluetoothBase->bt_GlobalCfg;
        if((ti = FindTagItem(BGCA_LocalName, tags))) {
            STRPTR src = (STRPTR) ti->ti_Data;
            if(src) {
                strncpy((char *) BluetoothBase->bt_GlobalCfg->bgc_LocalName, src, sizeof(BluetoothBase->bt_GlobalCfg->bgc_LocalName)-1);
            } else {
                BluetoothBase->bt_GlobalCfg->bgc_LocalName[0] = 0;
            }
            count++;
        }
        checkcfgupdate = TRUE;
        break;
    }
    if(packtab) {
        res = (LONG) (PackStructureTags(btstruct, packtab, tags)+count);
    } else {
        res = -1;
    }
    if(savepopocfg) {
        /* per-device settings (name, popup/bind/connect policy): in memory
           only, the prefs' Save/Use writes them - as Poseidon does */
        bStoreDevConfig(BluetoothBase, (struct BtDevice *) btstruct, FALSE);
    }
    if(checkcfgupdate) {
        struct BtIFFContext *pic;
        bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
        pic = btFindCfgForm(NULL, IFFFORM_BTSTACKCFG);
        if(pic) {
            bAddCfgChunk(BluetoothBase, pic, BluetoothBase->bt_GlobalCfg);
        }
        bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
        bCheckCfgChanged(BluetoothBase);
    }
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */
