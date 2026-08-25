/*
 *----------------------------------------------------------------------------
 *       bluetooth.library: classes, bindings, error log, events
 *----------------------------------------------------------------------------
 *
 * Classes are libraries in SYS:Classes/Bluetooth exporting the btclass API
 * (libraries/btclass.h). Binding works as in Poseidon: for every registered
 * device the classes are asked, in priority order, for a service binding
 * per service and, failing that, for a device binding. The class scan of a
 * device is triggered from the event handler task (the "hub task" of this
 * stack) so that class code never runs inside the hardware task.
 */

#include "debug.h"

#include "bluetooth.library.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/btclass.h>
#include <proto/timer.h>

#include <string.h>

#define NewList(list) NEWLIST(list)

#define BtClsBase bc->bc_ClassBase
#define DOSBase BluetoothBase->bt_DosBase
#define TimerBase BluetoothBase->bt_TimerIOReq.tr_node.io_Device

/* *** Classes *** */

/* /// "btAddClass()" */
AROS_LH2(struct BtClass *, btAddClass,
         AROS_LHA(STRPTR, name, A1),
         AROS_LHA(ULONG, vers, D0),
         LIBBASETYPEPTR, BluetoothBase, 33, bt)
{
    AROS_LIBFUNC_INIT
    struct Library *cls = NULL;
    struct BtClass *bc;
    IPTR pri = 0;
    STRPTR desc = "";
    STRPTR origname = name;

    KPRINTF(5, ("btAddClass(%s, %ld)\n", name, vers));

    while(*name) {
        if((cls = OpenLibrary(name, vers))) {
            break;
        }
        do {
            if((*name == '/') || (*name == ':')) {
                ++name;
                break;
            }
        } while(*(++name));
    }
    if(cls) {
        Forbid();
        if(FindName(&BluetoothBase->bt_Classes, cls->lib_Node.ln_Name)) {
            Permit();
            btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Attempted to add class %s twice.",
                           name);
            KPRINTF(20, ("attempt to add class twice!\n"));
            CloseLibrary(cls);
            return(NULL);
        }
        Permit();
        if((bc = btAllocVec(sizeof(struct BtClass)))) {
            bc->bc_Base = BluetoothBase;
            bc->bc_ClassBase = cls;
            bc->bc_Node.ln_Name = bc->bc_ClassName = btCopyStr(cls->lib_Node.ln_Name);
            bc->bc_FullPath = btCopyStr(origname);

            btcGetAttrs(BCGA_CLASS, NULL,
                        BCCA_Priority, &pri,
                        BCCA_Description, &desc,
                        TAG_END);

            bc->bc_Node.ln_Pri = pri;
            btLockWriteBase();
            Enqueue(&BluetoothBase->bt_Classes, &bc->bc_Node);
            btUnlockBase();

            btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                           "Say hello to %s V%ld.%ld (%s).",
                           cls->lib_Node.ln_Name, cls->lib_Version, cls->lib_Revision, desc);
            btSendEvent(BEHMB_ADDCLASS, bc, NULL);
            return(bc);
        }
        CloseLibrary(cls);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btRemClass()" */
AROS_LH1(void, btRemClass,
         AROS_LHA(struct BtClass *, bc, A1),
         LIBBASETYPEPTR, BluetoothBase, 34, bt)
{
    AROS_LIBFUNC_INIT
    KPRINTF(5, ("btRemClass(%p)\n", bc));
    btLockWriteBase();
    Remove(&bc->bc_Node);
    btUnlockBase();

    /* Check if there are still bindings remaining */
    while(bc->bc_UseCnt) {
        struct BtDevice *bd;
        struct BtService *bsv;

        KPRINTF(20, ("This should never happen: Class %s still in use (%ld), can't close!\n",
                     bc->bc_ClassBase->lib_Node.ln_Name, bc->bc_UseCnt));

        /* Well, try to release the open bindings in a best effort attempt */
        btLockReadBase();
        bd = NULL;
        while((bd = btGetNextDevice(bd))) {
            if(bd->bd_DevBinding && (bd->bd_ClsBinding == bc) && (!(bd->bd_Flags & BDFF_APPBINDING))) {
                btUnlockBase();
                btReleaseDevBinding(bd);
                btLockReadBase();
                bd = NULL; /* restart */
                continue;
            }
            bsv = (struct BtService *) bd->bd_Services.lh_Head;
            while(bsv->bsv_Node.ln_Succ) {
                if(bsv->bsv_SvcBinding && (bsv->bsv_ClsBinding == bc)) {
                    btUnlockBase();
                    btReleaseSvcBinding(bsv);
                    btLockReadBase();
                    bd = NULL; /* restart */
                    break;
                }
                bsv = (struct BtService *) bsv->bsv_Node.ln_Succ;
            }
        }
        btUnlockBase();
        if(bc->bc_UseCnt) {
            btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                           "This should never happen! Class %s still in use (cnt=%ld). Could not get rid of it!",
                           bc->bc_ClassBase->lib_Node.ln_Name, bc->bc_UseCnt);
            break;
        }
    }
    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "Removed class %s.",
                   bc->bc_ClassBase->lib_Node.ln_Name);
    CloseLibrary(bc->bc_ClassBase);
    btFreeVec(bc->bc_ClassName);
    btFreeVec(bc->bc_FullPath);
    btFreeVec(bc);
    btSendEvent(BEHMB_REMCLASS, bc, NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Error Msgs *** */

/* /// "btAddErrorMsgA()" */
AROS_LH4(struct BtErrorMsg *, btAddErrorMsgA,
         AROS_LHA(UWORD, level, D0),
         AROS_LHA(STRPTR, origin, A0),
         AROS_LHA(STRPTR, fmtstr, A1),
         AROS_LHA(RAWARG, fmtdata, A2),
         LIBBASETYPEPTR, BluetoothBase, 38, bt)
{
    AROS_LIBFUNC_INIT
    struct BtErrorMsg *bem;
    struct BtGlobalCfg *bgc = BluetoothBase->bt_GlobalCfg;
    if(bgc) {
        if(((!bgc->bgc_LogInfo) && (level < RETURN_WARN)) ||
                ((!bgc->bgc_LogWarning) && (level >= RETURN_WARN) && (level < RETURN_ERROR)) ||
                ((!bgc->bgc_LogError) && (level >= RETURN_ERROR) && (level < RETURN_FAIL)) ||
                ((!bgc->bgc_LogFailure) && (level >= RETURN_FAIL))) {
            return(NULL);
        }
    }
    if((bem = btAllocVec(sizeof(struct BtErrorMsg)))) {
        bem->bem_Base = BluetoothBase;
        bem->bem_Level = level;
        if((bem->bem_Origin = btCopyStr(origin))) {
            if((bem->bem_Msg = btCopyStrFmtA(fmtstr, fmtdata))) {
                if (BluetoothBase->bt_Flags & BTF_KLOG) {
                    KPrintF("[%s] %s\n", origin, bem->bem_Msg);
                }
                if(bHaveDOS(BluetoothBase)) {
                    DateStamp(&bem->bem_DateStamp);
                } else if(BluetoothBase->bt_StackInit) {
                    struct timerequest tr = BluetoothBase->bt_TimerIOReq;
                    tr.tr_node.io_Command = TR_GETSYSTIME;
                    DoIO((struct IORequest *) &tr);
                    bem->bem_DateStamp.ds_Days = tr.tr_time.tv_secs / (24*60*60);
                    bem->bem_DateStamp.ds_Minute = (tr.tr_time.tv_secs / 60) % 60;
                    bem->bem_DateStamp.ds_Tick = (tr.tr_time.tv_secs % 60) * 50;
                }
                Forbid();
                AddTail(&BluetoothBase->bt_ErrorMsgs, &bem->bem_Node);
                Permit();
                btSendEvent(BEHMB_ADDERRORMSG, bem, NULL);
                /* with the "btdebug" boot argument also mirror the line to the
                   boot volume, so a board without keyboard/shell still leaves a
                   readable log. Only from a Process (DOS I/O needs one). */
                if((BluetoothBase->bt_Flags & BTF_KLOG) && bHaveDOS(BluetoothBase) &&
                   (FindTask(NULL)->tc_Node.ln_Type == NT_PROCESS)) {
                    BPTR fh = Open("SYS:BluetoothLog.txt", MODE_READWRITE);
                    if(fh) {
                        Seek(fh, 0, OFFSET_END);
                        FPrintf(fh, "%ld %s: %s\n", (LONG) level, origin, bem->bem_Msg);
                        Close(fh);
                    }
                }
                return(bem);
            }
            btFreeVec(bem->bem_Origin);
        }
        btFreeVec(bem);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btRemErrorMsg()" */
AROS_LH1(void, btRemErrorMsg,
         AROS_LHA(struct BtErrorMsg *, bem, A0),
         LIBBASETYPEPTR, BluetoothBase, 39, bt)
{
    AROS_LIBFUNC_INIT
    KPRINTF(1, ("btRemErrorMsg()\n"));
    Forbid();
    Remove(&bem->bem_Node);
    Permit();
    btFreeVec(bem->bem_Origin);
    btFreeVec(bem->bem_Msg);
    btFreeVec(bem);
    btSendEvent(BEHMB_REMERRORMSG, bem, NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Bindings *** */

/* /// "bDeviceClassScan()" */
/*
 * Offers a registered device to the classes: forced bindings first, then
 * a service binding for every unbound service, then -- if no service could
 * be bound -- a device binding. Called with the library base read locked.
 */
void bDeviceClassScan(LIBBASETYPEPTR BluetoothBase, struct BtDevice *bd)
{
    struct BtClass *bc;
    struct BtService *bsv;
    APTR binding;
    BOOL hassvcbinding;
    STRPTR owner;

    if(!(bd->bd_Flags & BDFF_REGISTERED)) {
        return;
    }
    /* Only offer a device to the classes once its services are known. A
     * DEVICECONNECTED scan can otherwise run while the hardware task is still
     * enumerating services (GATT/SDP); the binding attempt takes device locks
     * and spawns binding subtasks, stalling that enumeration until it times
     * out. Services becoming known raises BEHMB_SERVICESCHG, which drives a
     * second scan that does the real binding. */
    if(!(bd->bd_Flags & BDFF_SERVICESKNOWN)) {
        return;
    }
    if(bd->bd_PoPoCfg.bpc_NoClassBind || bd->bd_DevBinding || (bd->bd_Flags & BDFF_APPBINDING)) {
        return;
    }
    KPRINTF(5, ("Doing ClassScan on Device: %s\n", bd->bd_Name));

    /* A class's service binding (btcDoMethod(BCM_*ServiceBinding)) can spawn a
     * subtask that does blocking GATT/SDP I/O, which the hardware task must
     * service - and the hardware task also needs the device lock (pairing,
     * disconnect, ...). So we must NOT hold the device lock across a binding
     * attempt. Instead: snapshot the unbound services under the lock and mark
     * them "binding in progress" (bClearServices/bFreeService keep those), drop
     * the lock for the attempts, then reacquire it briefly to record results. */
    {
        struct BtService *svcs[16];
        UWORD nsvc = 0, i;

        btLockWriteDevice(bd);
        for(bsv = (struct BtService *) bd->bd_Services.lh_Head; bsv->bsv_Node.ln_Succ;
            bsv = (struct BtService *) bsv->bsv_Node.ln_Succ) {
            if(bsv->bsv_SvcBinding) {
                hassvcbinding = TRUE;
            } else if(!bsv->bsv_BindingInProgress && (nsvc < 16)) {
                bsv->bsv_BindingInProgress = TRUE;
                svcs[nsvc++] = bsv;
            }
        }
        btUnlockDevice(bd);

        /* forced device binding wins over service bindings, as before */
        owner = btGetForcedBinding(bd->bd_IDString, NULL);
        if((!hassvcbinding) && owner) {
            bc = (struct BtClass *) BluetoothBase->bt_Classes.lh_Head;
            while(bc->bc_Node.ln_Succ) {
                if(!strcmp(owner, bc->bc_ClassName)) {
                    binding = (APTR) btcDoMethod(BCM_ForceDeviceBinding, bd);
                    if(binding) {
                        btLockWriteDevice(bd);
                        bd->bd_DevBinding = binding;
                        bd->bd_ClsBinding = bc;
                        btUnlockDevice(bd);
                        bc->bc_UseCnt++;
                        btSendEvent(BEHMB_ADDBINDING, bd, NULL);
                    } else {
                        btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                       "Forced device binding of %s to %s failed.", bd->bd_Name, owner);
                    }
                    break;
                }
                bc = (struct BtClass *) bc->bc_Node.ln_Succ;
            }
            btLockWriteDevice(bd);
            for(i = 0; i < nsvc; i++) {
                svcs[i]->bsv_BindingInProgress = FALSE;
            }
            btUnlockDevice(bd);
            return;
        }

        /* Service bindings, one snapshot service at a time, WITHOUT the lock. */
        for(i = 0; i < nsvc; i++) {
            bsv = svcs[i];
            binding = NULL;
            owner = btGetForcedBinding(bd->bd_IDString, bsv->bsv_IDString);
            bc = (struct BtClass *) BluetoothBase->bt_Classes.lh_Head;
            while(bc->bc_Node.ln_Succ) {
                if(owner) {
                    if(!strcmp(owner, bc->bc_ClassName)) {
                        binding = (APTR) btcDoMethod(BCM_ForceServiceBinding, bsv);
                        if(!binding) {
                            btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                           "Forced service binding of %s to %s failed.", bd->bd_Name, owner);
                        }
                    }
                    if(!binding) {
                        bc = (struct BtClass *) bc->bc_Node.ln_Succ;
                        continue;
                    }
                } else {
                    binding = (APTR) btcDoMethod(BCM_AttemptServiceBinding, bsv);
                }
                if(binding) {
                    btLockWriteDevice(bd);
                    bsv->bsv_SvcBinding = binding;
                    bsv->bsv_ClsBinding = bc;
                    btUnlockDevice(bd);
                    hassvcbinding = TRUE;
                    bc->bc_UseCnt++;
                    btSendEvent(BEHMB_ADDBINDING, bd, NULL);
                    break;
                }
                bc = (struct BtClass *) bc->bc_Node.ln_Succ;
            }
        }

        /* done with the snapshot: clear the in-progress markers */
        btLockWriteDevice(bd);
        for(i = 0; i < nsvc; i++) {
            svcs[i]->bsv_BindingInProgress = FALSE;
        }
        btUnlockDevice(bd);
    }

    /* Could not establish a service binding: try a device binding then. Device
     * bindings do not do blocking I/O for our classes, so the lock is fine. */
    if(!hassvcbinding && !bd->bd_DevBinding) {
        btLockWriteDevice(bd);
        owner = btGetForcedBinding(bd->bd_IDString, NULL);
        bc = (struct BtClass *) BluetoothBase->bt_Classes.lh_Head;
        while(bc->bc_Node.ln_Succ) {
            binding = NULL;
            if(owner) {
                if(!strcmp(owner, bc->bc_ClassName)) {
                    binding = (APTR) btcDoMethod(BCM_ForceDeviceBinding, bd);
                    if(!binding) {
                        btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                       "Forced device binding of %s to %s failed.", bd->bd_Name, owner);
                    }
                }
                if(!binding) {
                    bc = (struct BtClass *) bc->bc_Node.ln_Succ;
                    continue;
                }
            } else {
                binding = (APTR) btcDoMethod(BCM_AttemptDeviceBinding, bd);
            }
            if(binding) {
                bd->bd_DevBinding = binding;
                bd->bd_ClsBinding = bc;
                bc->bc_UseCnt++;
                btSendEvent(BEHMB_ADDBINDING, bd, NULL);
                break;
            }
            bc = (struct BtClass *) bc->bc_Node.ln_Succ;
        }
        btUnlockDevice(bd);
    }
}
/* \\\ */

/* /// "btClassScan()" */
AROS_LH0(void, btClassScan,
         LIBBASETYPEPTR, BluetoothBase, 35, bt)
{
    AROS_LIBFUNC_INIT
    struct BtHardware *bth;
    struct BtDevice *bd;
    struct BtClass *bc;

    btLockReadBase();

    if((FindTask(NULL)->tc_Node.ln_Type != NT_PROCESS) && (!BluetoothBase->bt_ConfigRead)) {
        BluetoothBase->bt_StartedAsTask = TRUE;
    }

    bc = (struct BtClass *) BluetoothBase->bt_Classes.lh_Head;
    if(!bc->bc_Node.ln_Succ) {
        btAddErrorMsg0(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "ClassScan attempted with no classes installed!");
        btUnlockBase();
        return;
    }

    bth = (struct BtHardware *) BluetoothBase->bt_Hardware.lh_Head;
    while(bth->bth_Node.ln_Succ) {
        bd = (struct BtDevice *) bth->bth_Devices.lh_Head;
        while (bd->bd_Node.ln_Succ) {
            bDeviceClassScan(BluetoothBase, bd);
            bd = (struct BtDevice *) bd->bd_Node.ln_Succ;
        }
        bth = (struct BtHardware *) bth->bth_Node.ln_Succ;
    }
    btUnlockBase();
    KPRINTF(5, ("************ Scanning finished!\n"));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btClaimAppBindingA()" */
AROS_LH1(struct BtAppBinding *, btClaimAppBindingA,
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, BluetoothBase, 45, bt)
{
    AROS_LIBFUNC_INIT
    struct BtDevice *bd;
    struct BtService *bsv;
    struct BtAppBinding tmpbab;
    struct BtAppBinding *bab = NULL;

    KPRINTF(2, ("btClaimAppBindingA(%p)\n", tags));

    tmpbab.bab_Device = NULL;
    tmpbab.bab_ReleaseHook = NULL;
    tmpbab.bab_Task = NULL;
    tmpbab.bab_ForceRelease = FALSE;
    btSetAttrsA(BGA_APPBINDING, &tmpbab, tags);
    if(tmpbab.bab_Device && tmpbab.bab_ReleaseHook) {
        bd = tmpbab.bab_Device;

        if(tmpbab.bab_ForceRelease) {
            if(bd->bd_DevBinding) {
                btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "%s really wants to bind to %s, so I'm letting the old binding go.",
                               FindTask(NULL)->tc_Node.ln_Name,
                               bd->bd_Name);
                btReleaseDevBinding(bd);
            } else {
                bsv = (struct BtService *) bd->bd_Services.lh_Head;
                while(bsv->bsv_Node.ln_Succ) {
                    if(bsv->bsv_SvcBinding) {
                        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "%s really wants to bind to %s, so I'm letting the old binding go.",
                                       FindTask(NULL)->tc_Node.ln_Name,
                                       bd->bd_Name);
                        btReleaseSvcBinding(bsv);
                    }
                    bsv = (struct BtService *) bsv->bsv_Node.ln_Succ;
                }
            }
        }
        btLockWriteDevice(bd);
        if(!bd->bd_DevBinding) {
            BOOL hassvc = FALSE;
            bsv = (struct BtService *) bd->bd_Services.lh_Head;
            while(bsv->bsv_Node.ln_Succ) {
                if(bsv->bsv_SvcBinding) {
                    hassvc = TRUE;
                    break;
                }
                bsv = (struct BtService *) bsv->bsv_Node.ln_Succ;
            }
            if(!hassvc) {
                if((bab = btAllocVec(sizeof(struct BtAppBinding)))) {
                    *bab = tmpbab;
                    bab->bab_Task = FindTask(NULL);
                    bab->bab_Node.ln_Name = bab->bab_Task->tc_Node.ln_Name;
                    bd->bd_DevBinding = bab;
                    bd->bd_ClsBinding = NULL;
                    bd->bd_Flags |= BDFF_APPBINDING;
                }
            }
        }
        btUnlockDevice(bd);
        if(bab) {
            btSendEvent(BEHMB_ADDBINDING, bd, NULL);
            return(bab);
        }
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btReleaseAppBinding()" */
AROS_LH1(void, btReleaseAppBinding,
         AROS_LHA(struct BtAppBinding *, bab, A0),
         LIBBASETYPEPTR, BluetoothBase, 46, bt)
{
    AROS_LIBFUNC_INIT
    KPRINTF(2, ("btReleaseAppBinding(%p)\n", bab));
    if(bab) {
        btReleaseDevBinding(bab->bab_Device);
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btReleaseDevBinding()" */
AROS_LH1(void, btReleaseDevBinding,
         AROS_LHA(struct BtDevice *, bd, A0),
         LIBBASETYPEPTR, BluetoothBase, 50, bt)
{
    AROS_LIBFUNC_INIT
    struct BtClass *bc;
    APTR binding;
    struct BtAppBinding *bab;

    KPRINTF(5, ("btReleaseDevBinding(%p)\n", bd));
    if(bd) {
        btLockWriteDevice(bd);
        if((binding = bd->bd_DevBinding)) {
            bd->bd_DevBinding = NULL;
            if(bd->bd_Flags & BDFF_APPBINDING) {
                bab = (struct BtAppBinding *) binding;
                CallHookPkt(bab->bab_ReleaseHook, bab, (APTR) bab->bab_UserData);
                bd->bd_ClsBinding = NULL;
                bd->bd_Flags &= ~BDFF_APPBINDING;
                btFreeVec(bab);
                btSendEvent(BEHMB_REMBINDING, bd, NULL);
            } else {
                bc = bd->bd_ClsBinding;
                if(bc) {
                    bd->bd_ClsBinding = NULL;
                    btcDoMethod(BCM_ReleaseDeviceBinding, binding);
                    bc->bc_UseCnt--;
                    btSendEvent(BEHMB_REMBINDING, bd, NULL);
                }
            }
        }
        btUnlockDevice(bd);
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btReleaseSvcBinding()" */
AROS_LH1(void, btReleaseSvcBinding,
         AROS_LHA(struct BtService *, bsv, A0),
         LIBBASETYPEPTR, BluetoothBase, 51, bt)
{
    AROS_LIBFUNC_INIT
    struct BtClass *bc;
    struct BtDevice *bd;
    APTR binding;

    KPRINTF(5, ("btReleaseSvcBinding(%p)\n", bsv));

    if(bsv) {
        bd = bsv->bsv_Device;
        btLockWriteDevice(bd);
        if((binding = bsv->bsv_SvcBinding)) {
            bsv->bsv_SvcBinding = NULL;
            bc = bsv->bsv_ClsBinding;
            if(bc) {
                bsv->bsv_ClsBinding = NULL;
                btcDoMethod(BCM_ReleaseServiceBinding, binding);
                bc->bc_UseCnt--;
            }
            btSendEvent(BEHMB_REMBINDING, bd, NULL);
        }
        btUnlockDevice(bd);
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btUnbindAll()" */
AROS_LH0(void, btUnbindAll,
         LIBBASETYPEPTR, BluetoothBase, 52, bt)
{
    AROS_LIBFUNC_INIT
    struct BtHardware *bth;
    struct BtDevice *bd;
    struct BtService *bsv;
    BOOL restart;

    KPRINTF(10, ("btUnbindAll()\n"));
    btLockReadBase();
    do {
        restart = FALSE;
        bth = (struct BtHardware *) BluetoothBase->bt_Hardware.lh_Head;
        while(bth->bth_Node.ln_Succ) {
            bd = (struct BtDevice *) bth->bth_Devices.lh_Head;
            while(bd->bd_Node.ln_Succ) {
                if(bd->bd_DevBinding) {
                    btUnlockBase();
                    btReleaseDevBinding(bd);
                    btLockReadBase();
                    restart = TRUE;
                    break;
                }
                bsv = (struct BtService *) bd->bd_Services.lh_Head;
                while(bsv->bsv_Node.ln_Succ) {
                    if(bsv->bsv_SvcBinding) {
                        btUnlockBase();
                        btReleaseSvcBinding(bsv);
                        btLockReadBase();
                        restart = TRUE;
                        break;
                    }
                    bsv = (struct BtService *) bsv->bsv_Node.ln_Succ;
                }
                if(restart) {
                    break;
                }
                bd = (struct BtDevice *) bd->bd_Node.ln_Succ;
            }
            if(restart) {
                break;
            }
            bth = (struct BtHardware *) bth->bth_Node.ln_Succ;
        }
    } while(restart);
    btUnlockBase();
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Events *** */

/* /// "btAddEventHandler()" */
AROS_LH2(struct BtEventHook *, btAddEventHandler,
         AROS_LHA(struct MsgPort *, mp, A1),
         AROS_LHA(ULONG, msgmask, D0),
         LIBBASETYPEPTR, BluetoothBase, 47, bt)
{
    AROS_LIBFUNC_INIT
    struct BtEventHook *beh = NULL;

    KPRINTF(5, ("btAddEventHandler(%p, %p)\n", mp, msgmask));

    if(mp) {
        ObtainSemaphore(&BluetoothBase->bt_ReentrantLock);
        if((beh = btAllocVec(sizeof(struct BtEventHook)))) {
            beh->beh_MsgPort = mp;
            beh->beh_MsgMask = msgmask;
            AddTail(&BluetoothBase->bt_EventHooks, &beh->beh_Node);
        }
        ReleaseSemaphore(&BluetoothBase->bt_ReentrantLock);
    }
    return(beh);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btRemEventHandler()" */
AROS_LH1(void, btRemEventHandler,
         AROS_LHA(struct BtEventHook *, beh, A0),
         LIBBASETYPEPTR, BluetoothBase, 48, bt)
{
    AROS_LIBFUNC_INIT
    struct Message *msg;

    KPRINTF(5, ("btRemEventHandler(%p)\n", beh));
    if(!beh) {
        return;
    }
    ObtainSemaphore(&BluetoothBase->bt_ReentrantLock);
    Remove(&beh->beh_Node);
    while((msg = GetMsg(beh->beh_MsgPort))) {
        ReplyMsg(msg);
    }
    ReleaseSemaphore(&BluetoothBase->bt_ReentrantLock);
    bGarbageCollectEvents(BluetoothBase);
    btFreeVec(beh);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btSendEvent()" */
AROS_LH3(void, btSendEvent,
         AROS_LHA(ULONG, behmt, D0),
         AROS_LHA(APTR, param1, A0),
         AROS_LHA(APTR, param2, A1),
         LIBBASETYPEPTR, BluetoothBase, 49, bt)
{
    AROS_LIBFUNC_INIT
    struct BtEventNote *ben;
    struct BtEventHook *beh;
    ULONG msgmask = (1L<<behmt);

    KPRINTF(1, ("btSendEvent(%p, %p, %p)\n", behmt, param1, param2));

    bGarbageCollectEvents(BluetoothBase);
    ObtainSemaphore(&BluetoothBase->bt_ReentrantLock);
    beh = (struct BtEventHook *) BluetoothBase->bt_EventHooks.lh_Head;
    while(beh->beh_Node.ln_Succ) {
        if(beh->beh_MsgMask & msgmask) {
            if((ben = btAllocVec(sizeof(struct BtEventNote)))) {
                ben->ben_Msg.mn_ReplyPort = &BluetoothBase->bt_EventReplyPort;
                ben->ben_Msg.mn_Length = sizeof(struct BtEventNote);
                ben->ben_Event = behmt;
                ben->ben_Param1 = param1;
                ben->ben_Param2 = param2;
                PutMsg(beh->beh_MsgPort, &ben->ben_Msg);
            }
        }
        beh = (struct BtEventHook *) beh->beh_Node.ln_Succ;
    }
    ReleaseSemaphore(&BluetoothBase->bt_ReentrantLock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "bGarbageCollectEvents()" */
void bGarbageCollectEvents(LIBBASETYPEPTR BluetoothBase)
{
    struct BtEventNote *ben;
    while((ben = (struct BtEventNote *) GetMsg(&BluetoothBase->bt_EventReplyPort))) {
        btFreeVec(ben);
    }
}
/* \\\ */

/* *** Event handler task *** */

/* /// "bStartEventHandler()" */
BOOL bStartEventHandler(LIBBASETYPEPTR BluetoothBase)
{
    struct BtHandlerTask *bh = &BluetoothBase->bt_EventHandler;

    /* Already running? */
    if(bh->bh_Task) {
        return(TRUE);
    }

    bh->bh_ReadySignal = SIGB_SINGLE;
    bh->bh_ReadySigTask = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE); // clear single bit

    /*
     * Note: we must not hold bt_ReentrantLock here. The event broadcaster
     * registers itself with btAddEventHandler() - which takes bt_ReentrantLock -
     * before it signals readiness, so holding the lock would deadlock the sub
     * task against us. libOpen() (our only caller) is already serialised by the
     * library loader, so no additional guard is needed.
     */
    if(btSpawnSubTask("Bluetooth Event Broadcast", bEventHandlerTask, BluetoothBase)) {
        Wait(1UL<<bh->bh_ReadySignal);
    }
    bh->bh_ReadySigTask = NULL;

    if(bh->bh_Task) {
        btAddErrorMsg0(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Event broadcaster started.");
        return(TRUE);
    }
    return(FALSE);
}
/* \\\ */

/* /// "bEventHandlerTask()" */
/*
 * Besides forwarding config changes to the classes (like Poseidon's event
 * broadcaster) this task performs the class scans: whenever a device becomes
 * registered, connected or gets its services enumerated, and after every
 * class addition, the classes are offered the registered devices. Doing
 * this here keeps class code out of the hardware tasks.
 */
AROS_UFH0(void, bEventHandlerTask)
{
    AROS_USERFUNC_INIT
    LIBBASETYPEPTR BluetoothBase;
    struct Task *thistask;
    ULONG sigs;
    ULONG sigmask;
    struct BtClass *bc;
    struct BtHandlerTask *bh;
    struct BtEventNote *ben;
    ULONG counter;
    ULONG cfgchanged;
    BOOL classscan;

    thistask = FindTask(NULL);
    BluetoothBase = thistask->tc_UserData;
    bh = &BluetoothBase->bt_EventHandler;
    SetTaskPri(thistask, 0);

    if((bh->bh_MsgPort = CreateMsgPort())) {
        if((bh->bh_TimerMsgPort = CreateMsgPort())) {
            if((bh->bh_TimerIOReq = (struct timerequest *) CreateIORequest(bh->bh_TimerMsgPort, sizeof(struct timerequest)))) {
                if(!(OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *) bh->bh_TimerIOReq, 0))) {
                    bh->bh_TimerIOReq->tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
                    bh->bh_TimerIOReq->tr_node.io_Message.mn_Node.ln_Name = "EventHandler";
                    bh->bh_TimerIOReq->tr_node.io_Command = TR_ADDREQUEST;

                    bh->bh_EventHandler = btAddEventHandler(bh->bh_MsgPort,
                                                            BEHMF_CONFIGCHG|BEHMF_DEVICEREGISTERED|
                                                            BEHMF_DEVICEUNREGISTERED|
                                                            BEHMF_DEVICECONNECTED|BEHMF_SERVICESCHG|
                                                            BEHMF_ADDCLASS|BEHMF_ADDHARDWARE);
                    if(bh->bh_EventHandler) {
                        bh->bh_Task = thistask;
                        Forbid();
                        if(bh->bh_ReadySigTask) {
                            Signal(bh->bh_ReadySigTask, 1L<<bh->bh_ReadySignal);
                        }
                        Permit();

                        bh->bh_TimerIOReq->tr_time.tv_micro = 500*1000;
                        SendIO(&bh->bh_TimerIOReq->tr_node);
                        sigmask = (1UL<<bh->bh_MsgPort->mp_SigBit)|(1UL<<bh->bh_TimerMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
                        counter = 0;
                        cfgchanged = 0;
                        classscan = FALSE;
                        do {
                            if(BluetoothBase->bt_CheckConfigReq) {
                                bCheckCfgChanged(BluetoothBase);
                            }
                            if(BluetoothBase->bt_SaveConfigReq) {
                                /* a registration or bond changed: persist it. Only
                                   once a config has been loaded from (or written to)
                                   disk - before BTStackLoader ran, the in-memory
                                   config is just the defaults and must not clobber
                                   the user's prefs file. */
                                BluetoothBase->bt_SaveConfigReq = FALSE;
                                if(BluetoothBase->bt_ConfigRead && bOpenDOS(BluetoothBase)) {
                                    if(btSaveCfgToDisk(NULL, FALSE)) {
                                        btAddErrorMsg0(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                                       "Device registrations saved to Sys/bluetooth.prefs.");
                                    }
                                }
                            }
                            while((ben = (struct BtEventNote *) GetMsg(bh->bh_MsgPort))) {
                                switch(ben->ben_Event) {
                                case BEHMB_CONFIGCHG:
                                    if(!cfgchanged) {
                                        cfgchanged = counter;
                                    }
                                    break;

                                case BEHMB_DEVICEREGISTERED:
                                case BEHMB_DEVICECONNECTED:
                                case BEHMB_SERVICESCHG:
                                case BEHMB_ADDCLASS:
                                case BEHMB_ADDHARDWARE:
                                    classscan = TRUE;
                                    break;

                                case BEHMB_DEVICEUNREGISTERED: {
                                    /* the hardware task cannot release bindings itself */
                                    struct BtDevice *bd = (struct BtDevice *) ben->ben_Param1;
                                    struct BtService *bsv;
                                    btLockReadBase();
                                    if(bd && !(bd->bd_Flags & BDFF_REGISTERED)) {
                                        btReleaseDevBinding(bd);
                                        bsv = (struct BtService *) bd->bd_Services.lh_Head;
                                        while(bsv->bsv_Node.ln_Succ) {
                                            btReleaseSvcBinding(bsv);
                                            bsv = (struct BtService *) bsv->bsv_Node.ln_Succ;
                                        }
                                    }
                                    btUnlockBase();
                                    break;
                                }
                                }
                                ReplyMsg(&ben->ben_Msg);
                            }
                            if(classscan) {
                                classscan = FALSE;
                                btClassScan();
                            }
                            if(CheckIO(&bh->bh_TimerIOReq->tr_node)) {
                                WaitIO(&bh->bh_TimerIOReq->tr_node);
                                bh->bh_TimerIOReq->tr_time.tv_micro = 500*1000;
                                SendIO(&bh->bh_TimerIOReq->tr_node);
                                counter++;
                                if((cfgchanged + 2) == counter) {
                                    KPRINTF(10, ("Sending information about config changed to all classes.\n"));
                                    btLockReadBase();
                                    bc = (struct BtClass *) BluetoothBase->bt_Classes.lh_Head;
                                    while(bc->bc_Node.ln_Succ) {
                                        btcDoMethod(BCM_ConfigChangedEvent);
                                        bc = (struct BtClass *) bc->bc_Node.ln_Succ;
                                    }
                                    btUnlockBase();
                                    cfgchanged = 0;
                                }
                            }
                            sigs = Wait(sigmask);
                        } while(!(sigs & SIGBREAKF_CTRL_C));
                        btRemEventHandler(bh->bh_EventHandler);
                        bh->bh_EventHandler = NULL;
                        AbortIO(&bh->bh_TimerIOReq->tr_node);
                        WaitIO(&bh->bh_TimerIOReq->tr_node);
                    }
                    CloseDevice((struct IORequest *) bh->bh_TimerIOReq);
                }
                DeleteIORequest((struct IORequest *) bh->bh_TimerIOReq);
            }
            DeleteMsgPort(bh->bh_TimerMsgPort);
        }
        DeleteMsgPort(bh->bh_MsgPort);
        bh->bh_MsgPort = NULL;
    }
    Forbid();
    bh->bh_Task = NULL;
    if(bh->bh_ReadySigTask) {
        Signal(bh->bh_ReadySigTask, 1L<<bh->bh_ReadySignal);
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */
