/*
 *----------------------------------------------------------------------------
 *          bluetooth.library: hardware, devices, services, channels
 *----------------------------------------------------------------------------
 *
 * The object model mirrors poseidon.library: BtHardware (radio) owns
 * BtDevices (remote devices), a BtDevice owns BtServices, a BtService owns
 * BtEndpoints, and BtChannels are the transfer objects. All I/O and control
 * requests travel as messages to the hardware task (hwtask.c), exactly like
 * PsdPipes travel to Poseidon's pDeviceTask.
 */

#include "debug.h"

#include "bluetooth.library.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/btclass.h>
#include <proto/timer.h>

#include <string.h>
#include <btcore/buffer.h>
#include <btcore/sdp.h>

#define NewList(list) NEWLIST(list)
#define min(x,y) (((x) < (y)) ? (x) : (y))

#define BtClsBase bc->bc_ClassBase
#define DOSBase BluetoothBase->bt_DosBase
#define TimerBase BluetoothBase->bt_TimerIOReq.tr_node.io_Device

/* *** Hardware *** */

/* /// "bFindHardware()" */
struct BtHardware * bFindHardware(LIBBASETYPEPTR BluetoothBase, STRPTR name, ULONG unit)
{
    struct BtHardware *bth;
    Forbid();
    while(*name) {
        bth = (struct BtHardware *) BluetoothBase->bt_Hardware.lh_Head;
        while(bth->bth_Node.ln_Succ) {
            if((bth->bth_Unit == unit) && (!strcmp(bth->bth_DevName, name))) {
                Permit();
                return(bth);
            }
            bth = (struct BtHardware *) bth->bth_Node.ln_Succ;
        }
        do {
            if((*name == '/') || (*name == ':')) {
                ++name;
                break;
            }
        } while(*(++name));
    }
    Permit();
    return(NULL);
}
/* \\\ */

/* /// "btAddHardware()" */
AROS_LH2(struct BtHardware *, btAddHardware,
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(ULONG, unit, D0),
         LIBBASETYPEPTR, BluetoothBase, 12, bt)
{
    AROS_LIBFUNC_INIT
    struct BtHardware *bth;
    char buf[64];
    struct Task *tmptask;
    KPRINTF(5, ("btAddHardware(%s, %ld)\n", name, unit));

    if((bth = bFindHardware(BluetoothBase, name, unit))) {
        /* normal since a saved config mounts its radios at BTStackLoader
           time: an AddBTHardware that follows finds its work done */
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "Hardware %s/%ld is already online.", name, unit);
        return(bth);
    }

    if((bth = btAllocVec(sizeof(struct BtHardware)))) {
        NewList(&bth->bth_Devices);
        NewList(&bth->bth_DeadDevices);
        bth->bth_Unit = unit;
        bth->bth_Base = BluetoothBase;
        bth->bth_State = BHS_OFFLINE;
        if((bth->bth_Node.ln_Name = bth->bth_DevName = btCopyStr(name))) {
            btSafeRawDoFmt(buf, 64, "bthw<%s/%ld>", name, unit);
            bth->bth_ReadySignal = SIGB_SINGLE;
            bth->bth_ReadySigTask = FindTask(NULL);
            SetSignal(0, SIGF_SINGLE); // clear single bit
            if((tmptask = btSpawnSubTask(buf, bHWTask, bth))) {
                btBorrowLocksWait(tmptask, 1UL<<bth->bth_ReadySignal);
                if(bth->bth_Task) {
                    bth->bth_ReadySigTask = NULL;
                    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "New hardware %s/%ld added (%s, %s).",
                                   bth->bth_DevName,
                                   bth->bth_Unit,
                                   bth->bth_ProductName,
                                   bth->bth_AddrString);
                    btSendEvent(BEHMB_ADDHARDWARE, bth, NULL);
                    return(bth);
                }
            }
            bth->bth_ReadySigTask = NULL;
            btFreeVec(bth->bth_DevName);
        }
        btFreeVec(bth);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btRemHardware()" */
AROS_LH1(void, btRemHardware,
         AROS_LHA(struct BtHardware *, bth, A0),
         LIBBASETYPEPTR, BluetoothBase, 13, bt)
{
    AROS_LIBFUNC_INIT
    struct BtDevice *bd;
    ULONG cnt;

    KPRINTF(5, ("FreeHardware(%p)\n", bth));

    bd = (struct BtDevice *) bth->bth_Devices.lh_Head;
    while(bd->bd_Node.ln_Succ) {
        bFreeBindings(BluetoothBase, bd);
        btSendEvent(BEHMB_REMDEVICE, bd, NULL);
        bd = (struct BtDevice *) bth->bth_Devices.lh_Head;
    }
    /* Connections still refer to device services and endpoints. Stop the
       hardware task before freeing those records; bindings have already
       drained their channels while the task could still service them. */
    Forbid();
    /* Note that the subtask unlinks the hardware! */
    bth->bth_ReadySignal = SIGB_SINGLE;
    bth->bth_ReadySigTask = FindTask(NULL);
    if(bth->bth_Task) {
        Signal(bth->bth_Task, SIGBREAKF_CTRL_C);
    }
    Permit();
    while(bth->bth_Task) {
        Wait(1L<<bth->bth_ReadySignal);
    }
    cnt = 0;
    bd = (struct BtDevice *) bth->bth_DeadDevices.lh_Head;
    while(bd->bd_Node.ln_Succ) {
        if(bd->bd_UseCnt) {
            KPRINTF(20, ("Can't remove device, usecnt %ld\n", bd->bd_UseCnt));
            if(++cnt == 5) {
                btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "Can't remove device '%s', there are still %ld channels in use...",
                               bd->bd_Name, bd->bd_UseCnt);
            }
            if(++cnt == 30) {
                btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "Okay, going down with device '%s' anyway, maybe the driver crashed?",
                               bd->bd_Name);
                bd->bd_UseCnt = 0;
                cnt--;
            } else {
                btDelayMS(1000);
            }
        } else {
            bFreeDevice(BluetoothBase, bd);
        }
        bd = (struct BtDevice *) bth->bth_DeadDevices.lh_Head;
    }

    KPRINTF(1, ("FreeHardware(%p) freevec name\n", bth));
    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "Removed hardware %s/%ld. Bye bye!",
                   bth->bth_DevName, bth->bth_Unit);
    btFreeVec(bth->bth_DevName);
    btFreeVec(bth->bth_ProductName);
    btFreeVec(bth->bth_Manufacturer);
    btFreeVec(bth->bth_Description);
    btFreeVec(bth->bth_Copyright);
    btFreeVec(bth->bth_LocalName);
    btFreeVec(bth);
    btSendEvent(BEHMB_REMHARDWARE, bth, NULL);
    KPRINTF(1, ("FreeHardware(%p) done\n", bth));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btEnumerateHardware()" */
/*
 * The Poseidon counterpart enumerates the root hub. A radio has no root
 * device; instead the registered (paired) devices stored in the config are
 * instantiated for this hardware, the configured scan modes are applied and
 * a class scan is scheduled. Returns bth on success.
 */
AROS_LH1(APTR, btEnumerateHardware,
         AROS_LHA(struct BtHardware *, bth, A0),
         LIBBASETYPEPTR, BluetoothBase, 14, bt)
{
    AROS_LIBFUNC_INIT
    ULONG count;
    LONG err;

    KPRINTF(5, ("btEnumerateHardware(%p)\n", bth));
    if(!bth || (bth->bth_State != BHS_READY)) {
        return(NULL);
    }

    count = bRestoreDevices(BluetoothBase, bth);

    /* apply default scan modes */
    bSubmitCtrl(BluetoothBase, bth, NULL, BTPRI_SETSCANMODE,
                BluetoothBase->bt_GlobalCfg->bgc_Discoverable ? 1 : 0,
                BluetoothBase->bt_GlobalCfg->bgc_Connectable ? 1 : 0,
                NULL, 0, &err);
    if(BluetoothBase->bt_GlobalCfg->bgc_LocalName[0]) {
        bSubmitCtrl(BluetoothBase, bth, NULL, BTPRI_SETLOCALNAME, 0, 0,
                    BluetoothBase->bt_GlobalCfg->bgc_LocalName,
                    strlen((char *) BluetoothBase->bt_GlobalCfg->bgc_LocalName), &err);
    } else {
        /* No name configured: never leave the controller's firmware default
           ("RTK_BT_5.0" and the like) on the air - present the machine.
           The prefs' Options page overrides this (bgc_LocalName above). */
        char defname[64];
        defname[0] = 0;
        if(bHaveDOS(BluetoothBase) &&
           (FindTask(NULL)->tc_Node.ln_Type == NT_PROCESS)) {
            if(GetVar((CONST_STRPTR) "HOSTNAME", (STRPTR) defname,
                      sizeof(defname), 0) <= 0) {
                defname[0] = 0;
            }
        }
        if(!defname[0]) {
            strcpy(defname, "AROS");
        }
        bSubmitCtrl(BluetoothBase, bth, NULL, BTPRI_SETLOCALNAME, 0, 0,
                    (UBYTE *) defname, strlen(defname), &err);
    }

    if(count) {
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "%s/%ld: %ld registered device(s) restored.",
                       bth->bth_DevName, bth->bth_Unit, count);
    }
    return(bth);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Devices *** */

/* /// "btAllocDevice()" */
AROS_LH1(struct BtDevice *, btAllocDevice,
         AROS_LHA(struct BtHardware *, bth, A0),
         LIBBASETYPEPTR, BluetoothBase, 15, bt)
{
    AROS_LIBFUNC_INIT
    struct BtDevice *bd;
    KPRINTF(2, ("btAllocDevice(%p)\n", bth));
    if((bd = btAllocVec(sizeof(struct BtDevice)))) {
        bd->bd_Hardware = bth;
        bd->bd_RSSI = 127;
        bd->bd_IsNewToMe = TRUE;

        bInitSem(BluetoothBase, &bd->bd_Lock, "Device");

        NewList(&bd->bd_Services);

        // init prefs
        bd->bd_PoPoCfg.bpc_ChunkID = AROS_LONG2BE(IFFCHNK_POPUP);
        bd->bd_PoPoCfg.bpc_Length = AROS_LONG2BE(sizeof(struct BtPoPoCfg) - 8);
        bd->bd_PoPoCfg.bpc_InhibitPopup = FALSE;
        bd->bd_PoPoCfg.bpc_NoClassBind = FALSE;
        bd->bd_PoPoCfg.bpc_AutoConnect = BluetoothBase->bt_GlobalCfg->bgc_AutoConnect;
        bd->bd_PoPoCfg.bpc_Trusted = FALSE;

        if(bHaveDOS(BluetoothBase)) {
            DateStamp(&bd->bd_FirstSeen);
        }
        bd->bd_LastSeen = bd->bd_FirstSeen;

        btLockWriteBase();
        AddTail(&bth->bth_Devices, &bd->bd_Node);
        bth->bth_NumDevices++;
        btUnlockBase();
        return(bd);
    } else {
        KPRINTF(20, ("btAllocDevice(): out of memory!\n"));
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "bFreeEndpoint()" */
void bFreeEndpoint(LIBBASETYPEPTR BluetoothBase, struct BtEndpoint *bep)
{
    Remove(&bep->bep_Node);
    btFreeVec(bep->bep_Name);
    btFreeVec(bep);
}
/* \\\ */

/* /// "bAllocEndpoint()" */
struct BtEndpoint * bAllocEndpoint(LIBBASETYPEPTR BluetoothBase, struct BtService *bsv)
{
    struct BtEndpoint *bep;
    if((bep = btAllocVec(sizeof(struct BtEndpoint)))) {
        bep->bep_Service = bsv;
        AddTail(&bsv->bsv_Endpoints, &bep->bep_Node);
        bsv->bsv_NumEPs++;
    }
    return(bep);
}
/* \\\ */

/* /// "bFreeService()" */
void bFreeService(LIBBASETYPEPTR BluetoothBase, struct BtService *bsv)
{
    struct BtEndpoint *bep;
    Remove(&bsv->bsv_Node);
    bep = (struct BtEndpoint *) bsv->bsv_Endpoints.lh_Head;
    while(bep->bep_Node.ln_Succ) {
        bFreeEndpoint(BluetoothBase, bep);
        bep = (struct BtEndpoint *) bsv->bsv_Endpoints.lh_Head;
    }
    btFreeVec(bsv->bsv_Name);
    btFreeVec(bsv->bsv_IDString);
    btFreeVec(bsv->bsv_ServiceClassIDs);
    btFreeVec(bsv->bsv_HidDescriptor);
    btFreeVec(bsv);
}
/* \\\ */

/* /// "bAllocService()" */
struct BtService * bAllocService(LIBBASETYPEPTR BluetoothBase, struct BtDevice *bd)
{
    struct BtService *bsv;
    if((bsv = btAllocVec(sizeof(struct BtService)))) {
        bsv->bsv_Device = bd;
        NewList(&bsv->bsv_Endpoints);
        AddTail(&bd->bd_Services, &bsv->bsv_Node);
        bd->bd_NumServices++;
    }
    return(bsv);
}
/* \\\ */

/* /// "bFreeBindings()" */
void bFreeBindings(LIBBASETYPEPTR BluetoothBase, struct BtDevice *bd)
{
    struct BtHardware *bth = bd->bd_Hardware;
    struct BtService *bsv;
    KPRINTF(3, (" FreeBindings(%p)\n", bd));

    /* move device to list of dead devices first */
    btLockWriteBase();
    Remove(&bd->bd_Node);
    AddTail(&bth->bth_DeadDevices, &bd->bd_Node);
    if(bth->bth_NumDevices) {
        bth->bth_NumDevices--;
    }
    btUnlockBase();

    /* If there are bindings, get rid of them. */
    btLockWriteDevice(bd);
    btReleaseDevBinding(bd);

    bsv = (struct BtService *) bd->bd_Services.lh_Head;
    while(bsv->bsv_Node.ln_Succ) {
        btReleaseSvcBinding(bsv);
        bsv = (struct BtService *) bsv->bsv_Node.ln_Succ;
    }
    btUnlockDevice(bd);
}
/* \\\ */

/* /// "bFreeDevice()" */
void bFreeDevice(LIBBASETYPEPTR BluetoothBase, struct BtDevice *bd)
{
    struct BtService *bsv;

    btLockWriteDevice(bd);
    if(bd->bd_UseCnt) {
        KPRINTF(20, ("Couldn't free device, use cnt %ld\n", bd->bd_UseCnt));
        bd->bd_Flags &= ~BDFF_CONNECTED;
        bd->bd_Flags |= BDFF_DELEXPUNGE;
        btUnlockDevice(bd);
    } else {
        bsv = (struct BtService *) bd->bd_Services.lh_Head;
        while(bsv->bsv_Node.ln_Succ) {
            bFreeService(BluetoothBase, bsv);
            bsv = (struct BtService *) bd->bd_Services.lh_Head;
        }
        btFreeVec(bd->bd_OrigName);
        bd->bd_OrigName = NULL;
        btFreeVec(bd->bd_IDString);
        bd->bd_IDString = NULL;
        btUnlockDevice(bd);
        btLockWriteBase();
        Remove(&bd->bd_Node);
        btUnlockBase();
        bDeleteSem(BluetoothBase, &bd->bd_Lock);
        /* cannot free this vector -- tasks might still call LockDevice */
    }
    KPRINTF(3, ("FreeDevice done\n"));
}
/* \\\ */

/* /// "btFreeDevice()" */
AROS_LH1(void, btFreeDevice,
         AROS_LHA(struct BtDevice *, bd, A0),
         LIBBASETYPEPTR, BluetoothBase, 16, bt)
{
    AROS_LIBFUNC_INIT
    struct BtHardware *bth = bd->bd_Hardware;
    struct BtService *bsv;

    KPRINTF(3, (" FreeDevice(%p)\n", bd));

    btLockWriteBase();
    Remove(&bd->bd_Node);
    AddTail(&bth->bth_DeadDevices, &bd->bd_Node);
    if(bth->bth_NumDevices) {
        bth->bth_NumDevices--;
    }
    bd->bd_Flags &= ~BDFF_DELEXPUNGE;
    btUnlockBase();

    btLockWriteDevice(bd);

    /* If there are bindings, get rid of them. */
    btReleaseDevBinding(bd);

    bsv = (struct BtService *) bd->bd_Services.lh_Head;
    while(bsv->bsv_Node.ln_Succ) {
        btReleaseSvcBinding(bsv);
        bsv = (struct BtService *) bsv->bsv_Node.ln_Succ;
    }
    btUnlockDevice(bd);

    bFreeDevice(BluetoothBase, bd);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btLockReadDevice()" */
/*****************************************************************************
    NAME
        VOID btLockReadDevice(

    SYNOPSIS
        APTR bd)

    FUNCTION
        Acquire shared access to a Bluetooth device. The calling task
        may nest acquisitions.

    INPUTS
        bd - Live device object. NULL is invalid.

    RESULT
        None. Return indicates that access has been acquired.

    NOTES
        Call from task context. The call may wait indefinitely and uses
        SIGB_SINGLE while waiting. Balance every acquisition with one
        btUnlockDevice() in the same task. A read acquisition made by the
        exclusive owner increases its exclusive nesting count.

        If a reader record cannot be allocated, the call acquires exclusive
        access instead. Do not depend on another reader entering while this
        lock is held. No reader/writer fairness guarantee is provided.
        Keep the device alive until the matching unlock; acquiring a lock
        does not validate an expired device pointer.

    SEE ALSO
        btLockWriteDevice(), btUnlockDevice(), btBorrowLocksWait()

*****************************************************************************/
AROS_LH1(void, btLockReadDevice,
         AROS_LHA(struct BtDevice *, bd, A0),
         LIBBASETYPEPTR, BluetoothBase, 17, bt)
{
    AROS_LIBFUNC_INIT
    KPRINTF(2, ("btLockReadDevice(%p)\n", bd));
    bLockSemShared(BluetoothBase, &bd->bd_Lock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btLockWriteDevice()" */
/*****************************************************************************
    NAME
        VOID btLockWriteDevice(

    SYNOPSIS
        APTR bd)

    FUNCTION
        Acquire exclusive access to a Bluetooth device. Repeated
        acquisitions by the owner increase the nesting count.

    INPUTS
        bd - Live device object. NULL is invalid.

    RESULT
        None. Return indicates that exclusive access has been acquired.

    NOTES
        Call from task context. The call may wait indefinitely and uses
        SIGB_SINGLE while waiting. Balance every acquisition with one
        btUnlockDevice() in the same task.

        An existing read lock can be promoted when this task is the only
        reader. The read acquisition remains and needs its own unlock after
        the exclusive acquisition is released. Do not attempt concurrent
        read-to-write upgrades: waiting while retaining shared access can
        deadlock with another upgrading reader.
        Keep the device alive until the matching unlock; acquiring a lock
        does not validate an expired device pointer.

    SEE ALSO
        btLockReadDevice(), btUnlockDevice(), btBorrowLocksWait()

*****************************************************************************/
AROS_LH1(void, btLockWriteDevice,
         AROS_LHA(struct BtDevice *, bd, A0),
         LIBBASETYPEPTR, BluetoothBase, 18, bt)
{
    AROS_LIBFUNC_INIT
    KPRINTF(2, ("btLockWriteDevice(%p)\n", bd));
    bLockSemExcl(BluetoothBase, &bd->bd_Lock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btUnlockDevice()" */
/*****************************************************************************
    NAME
        VOID btUnlockDevice(

    SYNOPSIS
        APTR bd)

    FUNCTION
        Release one lock acquisition held by the calling task on a Bluetooth device.

    INPUTS
        bd - Live device object. NULL is invalid.

    RESULT
        None. Ownership errors are logged, not returned.

    NOTES
        Release exclusive acquisitions before any underlying shared
        acquisitions. The last acquisition of a mode releases that mode and
        wakes waiting tasks; it does not guarantee which waiter proceeds.
        An unlock by a non-owner or without an acquisition leaves the lock
        unchanged and can add diagnostic messages. Do not use this call
        as an ownership test. Call from task context.

    SEE ALSO
        btLockReadDevice(), btLockWriteDevice()

*****************************************************************************/
AROS_LH1(void, btUnlockDevice,
         AROS_LHA(struct BtDevice *, bd, A0),
         LIBBASETYPEPTR, BluetoothBase, 19, bt)
{
    AROS_LIBFUNC_INIT
    KPRINTF(2, ("btUnlockDevice(%p)\n", bd));
    bUnlockSem(BluetoothBase, &bd->bd_Lock);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btGetNextDevice()" */
/*****************************************************************************
    NAME
        APTR btGetNextDevice(

    SYNOPSIS
        APTR bd)

    FUNCTION
        Return the next device in hardware-list order, skipping empty radios.

    INPUTS
        bd - Previous device in the active device list, or NULL to start.

    RESULT
        Borrowed device handle, or NULL at the end of the list.

    NOTES
        This enumerates known devices, including disconnected devices; it
        does not perform discovery. No lock is acquired and no reference is
        retained. Exclude list changes throughout traversal and keep each
        handle valid while using it. Do not pass a removed device as the
        cursor or free a returned handle merely to end iteration.

    SEE ALSO
        btFindDeviceA(), btLockReadBase(), btUnlockBase()

*****************************************************************************/
AROS_LH1(struct BtDevice *, btGetNextDevice,
         AROS_LHA(struct BtDevice *, bd, A0),
         LIBBASETYPEPTR, BluetoothBase, 20, bt)
{
    AROS_LIBFUNC_INIT
    struct BtHardware *bth;

    KPRINTF(1, ("btGetNextDevice(%p)\n", bd));
    if(bd) {
        if(bd->bd_Node.ln_Succ->ln_Succ) {
            return((struct BtDevice *) bd->bd_Node.ln_Succ);
        }
        bth = (struct BtHardware *) bd->bd_Hardware->bth_Node.ln_Succ;
    } else {
        bth = (struct BtHardware *) BluetoothBase->bt_Hardware.lh_Head;
    }
    while(bth->bth_Node.ln_Succ) {
        bd = (struct BtDevice *) bth->bth_Devices.lh_Head;
        if(bd->bd_Node.ln_Succ) {
            return(bd);
        }
        bth = (struct BtHardware *) bth->bth_Node.ln_Succ;
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btFindDeviceA()" */
/*****************************************************************************
    NAME
        APTR btFindDeviceA(

    SYNOPSIS
        APTR bd, struct TagItem *taglist)

    FUNCTION
        Find the next known device that satisfies all supplied supported filters.

    INPUTS
        bd - Previous active device, excluded from the search; NULL starts
            at the first device across all radios.
        taglist - Filter tags, or NULL for no filtering.

    TAGS
        BDA_Address - Pointer to six address bytes in BD_ADDR order.
        BDA_AddressString - Address text in XX:XX:XX:XX:XX:XX form;
            comparison ignores case.
        BDA_AddressType, BDA_ClassOfDevice, BDA_VendorID, BDA_ProductID -
            Exact numeric values.
        BDA_Hardware, BDA_Binding - Exact handle values; NULL can match
            an absent binding.
        BDA_Name, BDA_IDString - NUL-terminated, case-sensitive strings.
            A missing device string does not match.
        BDA_IsRegistered, BDA_IsConnected - Boolean tests; nonzero means TRUE.
        BDA_ConnHandle - Exact handle; the device must also be connected.

    RESULT
        Borrowed device handle, or NULL if no later device matches.

    NOTES
        This searches existing objects, not the air. Unknown filter tags
        are ignored, including device attribute tags not listed above.
        Only the first occurrence of each filter is used; standard TagItem
        control tags are honored. Pointer-valued filter data must remain
        valid throughout the call and is not retained.

        No lock is acquired or lifetime extended. Exclude list and attribute
        changes during the search and preserve the returned object's
        lifetime while using it. Continue with the returned handle to find
        later matches.

    SEE ALSO
        btGetNextDevice(), btFindServiceA(), btGetAttrsA()

*****************************************************************************/
AROS_LH2(struct BtDevice *, btFindDeviceA,
         AROS_LHA(struct BtDevice *, bd, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, BluetoothBase, 42, bt)
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti;
    BOOL takeit;
    KPRINTF(2, ("btFindDeviceA(%p, %p)\n", bd, tags));
    while((bd = btGetNextDevice(bd))) {
        takeit = TRUE;
        if((ti = FindTagItem(BDA_Address, tags))) {
            if(memcmp((APTR) ti->ti_Data, bd->bd_Address.bd_Addr, 6)) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BDA_AddressString, tags))) {
            if(stricmp((STRPTR) ti->ti_Data, (STRPTR) bd->bd_AddrString)) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BDA_AddressType, tags))) {
            if(ti->ti_Data != bd->bd_AddrType) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BDA_Hardware, tags))) {
            if(ti->ti_Data != (IPTR) bd->bd_Hardware) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BDA_ClassOfDevice, tags))) {
            if(ti->ti_Data != bd->bd_ClassOfDevice) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BDA_VendorID, tags))) {
            if(ti->ti_Data != bd->bd_VendorID) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BDA_ProductID, tags))) {
            if(ti->ti_Data != bd->bd_ProductID) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BDA_Name, tags))) {
            if(!bd->bd_Name || strcmp((STRPTR) ti->ti_Data, bd->bd_Name)) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BDA_IDString, tags))) {
            if(!bd->bd_IDString || strcmp((STRPTR) ti->ti_Data, bd->bd_IDString)) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BDA_Binding, tags))) {
            if(ti->ti_Data != (IPTR) bd->bd_DevBinding) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BDA_IsRegistered, tags))) {
            if((ti->ti_Data ? TRUE : FALSE) != ((bd->bd_Flags & BDFF_REGISTERED) ? TRUE : FALSE)) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BDA_IsConnected, tags))) {
            if((ti->ti_Data ? TRUE : FALSE) != ((bd->bd_Flags & BDFF_CONNECTED) ? TRUE : FALSE)) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BDA_ConnHandle, tags))) {
            if((ti->ti_Data != bd->bd_ConnHandle) || !(bd->bd_Flags & BDFF_CONNECTED)) {
                takeit = FALSE;
            }
        }
        if(takeit) {
            return(bd);
        }
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btFindServiceA()" */
/*****************************************************************************
    NAME
        APTR btFindServiceA(

    SYNOPSIS
        APTR bd, APTR bsv, struct TagItem *taglist)

    FUNCTION
        Find the next matching service already known for a device.

    INPUTS
        bd - Device whose service list is searched; NULL returns NULL.
        bsv - Previous service in that device's list, excluded from the
            search; NULL starts at the first service.
        taglist - Filter tags, or NULL for no filtering.

    TAGS
        BSVA_UUID16 - Exact primary 16-bit UUID or an entry in the service's
            zero-terminated service-class ID list.
        BSVA_UUID - Pointer to 16 UUID bytes in big-endian order; exact match
            against the primary UUID, without service-class ID expansion.
        BSVA_Protocol, BSVA_PSM, BSVA_RFCOMMChannel, BSVA_RecordHandle -
            Exact numeric values.
        BSVA_Name, BSVA_IDString - NUL-terminated, case-sensitive strings;
            a missing service string does not match.
        BSVA_Binding - Exact binding handle; NULL matches no binding.

    RESULT
        Borrowed service handle, or NULL if no later service matches.

    NOTES
        All supplied supported filters must match. Unknown tags are ignored;
        only the first occurrence of each filter is used. Standard TagItem
        control tags are honored. Filter data is not retained.

        This does not enumerate remote services or acquire a lock. Keep the
        device and cursor valid and exclude service-list changes during
        traversal. A device read lock protects traversal against service
        replacement; it does not retain a service after unlocking. Service
        refresh or device removal can invalidate returned handles.

    SEE ALSO
        btFindEndpointA(), btEnumerateServices(), btLockReadDevice()

*****************************************************************************/
AROS_LH3(struct BtService *, btFindServiceA,
         AROS_LHA(struct BtDevice *, bd, A0),
         AROS_LHA(struct BtService *, bsv, A2),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, BluetoothBase, 43, bt)
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti;
    BOOL takeit;
    KPRINTF(2, ("btFindServiceA(%p, %p, %p)\n", bd, bsv, tags));
    if(!bd) {
        return(NULL);
    }
    if(bsv) {
        bsv = (struct BtService *) bsv->bsv_Node.ln_Succ;
    } else {
        bsv = (struct BtService *) bd->bd_Services.lh_Head;
    }
    while(bsv->bsv_Node.ln_Succ) {
        takeit = TRUE;
        if((ti = FindTagItem(BSVA_UUID16, tags))) {
            BOOL match = (ti->ti_Data == bsv->bsv_UUID16);
            if(!match && bsv->bsv_ServiceClassIDs) {
                UWORD *ids = bsv->bsv_ServiceClassIDs;
                while(*ids) {
                    if(*ids == ti->ti_Data) {
                        match = TRUE;
                        break;
                    }
                    ids++;
                }
            }
            if(!match) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BSVA_UUID, tags))) {
            if(memcmp((APTR) ti->ti_Data, bsv->bsv_UUID, 16)) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BSVA_Protocol, tags))) {
            if(ti->ti_Data != bsv->bsv_Protocol) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BSVA_PSM, tags))) {
            if(ti->ti_Data != bsv->bsv_PSM) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BSVA_RFCOMMChannel, tags))) {
            if(ti->ti_Data != bsv->bsv_RFCOMMChannel) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BSVA_RecordHandle, tags))) {
            if(ti->ti_Data != bsv->bsv_RecordHandle) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BSVA_Name, tags))) {
            if(!bsv->bsv_Name || strcmp((STRPTR) ti->ti_Data, bsv->bsv_Name)) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BSVA_IDString, tags))) {
            if(!bsv->bsv_IDString || strcmp((STRPTR) ti->ti_Data, bsv->bsv_IDString)) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BSVA_Binding, tags))) {
            if(ti->ti_Data != (IPTR) bsv->bsv_SvcBinding) {
                takeit = FALSE;
            }
        }
        if(takeit) {
            return(bsv);
        }
        bsv = (struct BtService *) bsv->bsv_Node.ln_Succ;
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btFindEndpointA()" */
/*****************************************************************************
    NAME
        APTR btFindEndpointA(

    SYNOPSIS
        APTR bsv, APTR bep, struct TagItem *taglist)

    FUNCTION
        Find the next matching endpoint in a known service.

    INPUTS
        bsv - Service whose endpoints are searched; NULL returns NULL.
        bep - Previous endpoint in that service, excluded from the search;
            NULL starts at the first endpoint.
        taglist - Filter tags, or NULL for no filtering.

    TAGS
        BEA_CanRead, BEA_CanWrite - Boolean tests; nonzero means TRUE.
        BEA_Type, BEA_PSM, BEA_CID, BEA_RFCOMMChannel, BEA_Handle,
        BEA_UUID16, BEA_ReportType, BEA_ReportID - Exact numeric values.
        BEA_UUID - Pointer to 16 UUID bytes in big-endian order; exact match.
        BEA_Properties - Required property bits. All supplied bits must be
            present; extra endpoint bits are allowed. Zero matches any value.

    RESULT
        Borrowed endpoint handle, or NULL if no later endpoint matches.

    NOTES
        All supplied supported filters must match. Unknown tags are ignored;
        only the first occurrence of each filter is used. Standard TagItem
        control tags are honored. Filter data is not retained.

        No channel is opened and no lock is acquired. Keep the service and
        cursor valid and exclude endpoint changes during traversal, using
        the owning device's read lock where required. The result is not a
        retained reference: service refresh can free unbound endpoints.
        A capability match does not establish a connection or guarantee I/O.

    SEE ALSO
        btFindServiceA(), btAllocChannel(), btLockReadDevice()

*****************************************************************************/
AROS_LH3(struct BtEndpoint *, btFindEndpointA,
         AROS_LHA(struct BtService *, bsv, A0),
         AROS_LHA(struct BtEndpoint *, bep, A2),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, BluetoothBase, 44, bt)
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti;
    BOOL takeit;
    KPRINTF(2, ("btFindEndpointA(%p, %p, %p)\n", bsv, bep, tags));
    if(!bsv) {
        return(NULL);
    }
    if(bep) {
        bep = (struct BtEndpoint *) bep->bep_Node.ln_Succ;
    } else {
        bep = (struct BtEndpoint *) bsv->bsv_Endpoints.lh_Head;
    }
    while(bep->bep_Node.ln_Succ) {
        takeit = TRUE;
        if((ti = FindTagItem(BEA_CanRead, tags))) {
            if((ti->ti_Data ? TRUE : FALSE) != (bep->bep_CanRead ? TRUE : FALSE)) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BEA_CanWrite, tags))) {
            if((ti->ti_Data ? TRUE : FALSE) != (bep->bep_CanWrite ? TRUE : FALSE)) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BEA_Type, tags))) {
            if(ti->ti_Data != bep->bep_Type) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BEA_PSM, tags))) {
            if(ti->ti_Data != bep->bep_PSM) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BEA_CID, tags))) {
            if(ti->ti_Data != bep->bep_CID) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BEA_RFCOMMChannel, tags))) {
            if(ti->ti_Data != bep->bep_RFCOMMChannel) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BEA_Handle, tags))) {
            if(ti->ti_Data != bep->bep_Handle) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BEA_UUID16, tags))) {
            if(ti->ti_Data != bep->bep_UUID16) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BEA_UUID, tags))) {
            if(memcmp((APTR) ti->ti_Data, bep->bep_UUID, 16)) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BEA_ReportType, tags))) {
            if(ti->ti_Data != bep->bep_ReportType) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BEA_ReportID, tags))) {
            if(ti->ti_Data != bep->bep_ReportID) {
                takeit = FALSE;
            }
        }
        if((ti = FindTagItem(BEA_Properties, tags))) {
            if((ti->ti_Data & bep->bep_Properties) != ti->ti_Data) {
                takeit = FALSE;
            }
        }
        if(takeit) {
            return(bep);
        }
        bep = (struct BtEndpoint *) bep->bep_Node.ln_Succ;
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "bApplyDevConfig()" */
/* Reads the DEVC form for the device: custom name and popup/policy config.
   Called with the device write locked. */
void bApplyDevConfig(LIBBASETYPEPTR BluetoothBase, struct BtDevice *bd)
{
    struct BtIFFContext *pic;
    ULONG *chnk;

    if(!bd->bd_IDString) {
        return;
    }
    bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    pic = btFindCfgForm(NULL, IFFFORM_BTDEVICECFG);
    while(pic) {
        if(bMatchStringChunk(BluetoothBase, pic, IFFCHNK_DEVID, bd->bd_IDString)) {
            STRPTR name;
            bd->bd_IsNewToMe = FALSE;
            if((name = bGetStringChunk(BluetoothBase, pic, IFFCHNK_DEVNAME))) {
                /* the name the device reported last time */
                btFreeVec(bd->bd_OrigName);
                bd->bd_OrigName = name;
                if(!bd->bd_Name || !strcmp(bd->bd_Name, (char *) bd->bd_AddrString)) {
                    btFreeVec(bd->bd_Name);
                    bd->bd_Name = btCopyStr(name);
                    bd->bd_Node.ln_Name = bd->bd_Name;
                }
            }
            if((name = bGetStringChunk(BluetoothBase, pic, IFFCHNK_NAME))) {
                btFreeVec(bd->bd_Name);
                bd->bd_Name = name;
                bd->bd_Node.ln_Name = bd->bd_Name;
            }
            if((chnk = bFindCfgChunk(BluetoothBase, pic, IFFCHNK_POPUP))) {
                struct BtPoPoCfg *bpc = (struct BtPoPoCfg *) chnk;
                CopyMem(((UBYTE *) bpc) + 8, ((UBYTE *) &bd->bd_PoPoCfg) + 8,
                        min(AROS_LONG2BE(bpc->bpc_Length), AROS_LONG2BE(bd->bd_PoPoCfg.bpc_Length)));
            }
            if((chnk = bFindCfgChunk(BluetoothBase, pic, IFFCHNK_KEYS))) {
                struct BtKeyCfg *bkc = (struct BtKeyCfg *) chnk;
                bd->bd_Keys.bkc_ChunkID = AROS_LONG2BE(IFFCHNK_KEYS);
                bd->bd_Keys.bkc_Length = AROS_LONG2BE(sizeof(struct BtKeyCfg) - 8);
                CopyMem(((UBYTE *) bkc) + 8, ((UBYTE *) &bd->bd_Keys) + 8,
                        min(AROS_LONG2BE(bkc->bkc_Length), sizeof(struct BtKeyCfg) - 8));
                if(bd->bd_Keys.bkc_Flags) {
                    bd->bd_Flags |= BDFF_BONDED;
                }
            }
            break;
        }
        pic = btNextCfgForm(pic);
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
}
/* \\\ */

/* /// "bRestoreDevices()" */
/* Instantiates the registered (paired) devices recorded in the config for a
   radio - the persistent part of the device list. Runs from
   btEnumerateHardware() and again from btParseCfg(): at boot the radio is
   usually brought up (by the USB class) before BTStackLoader has loaded the
   config, so the first pass finds nothing. Returns the number created. */
ULONG bRestoreDevices(LIBBASETYPEPTR BluetoothBase, struct BtHardware *bth)
{
    struct BtIFFContext *pic;
    struct BtDevice *bd;
    ULONG count = 0;

    /* exclusive: bApplyDevConfig() below needs the write lock and a shared
       lock cannot always be promoted */
    bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    pic = btFindCfgForm(NULL, IFFFORM_BTDEVICECFG);
    while(pic) {
        struct BtRegDevCfg *brd = bFindCfgChunk(BluetoothBase, pic, IFFCHNK_REGDEVICE);
        if(brd) {
            BD_ADDR addr;
            CopyMem(brd->brd_Address, addr.bd_Addr, 6);
            btLockReadBase();
            bd = btFindDevice(NULL, BDA_Address, (IPTR) &addr, BDA_Hardware, (IPTR) bth, TAG_END);
            btUnlockBase();
            if(!bd) {
                bd = btAllocDevice(bth);
                if(bd) {
                    btLockWriteDevice(bd);
                    CopyMem(brd->brd_Address, bd->bd_Address.bd_Addr, 6);
                    bd->bd_AddrType = brd->brd_AddressType;
                    bAddrToStr(bd->bd_Address.bd_Addr, (STRPTR) bd->bd_AddrString);
                    bd->bd_IDString = btCopyStrFmt("BT:%s", bd->bd_AddrString);
                    bd->bd_ClassOfDevice = brd->brd_ClassOfDevice;
                    bd->bd_Appearance = brd->brd_Appearance;
                    bd->bd_VendorIDSource = brd->brd_VendorIDSource;
                    bd->bd_VendorID = brd->brd_VendorID;
                    bd->bd_ProductID = brd->brd_ProductID;
                    bd->bd_ProductVersion = brd->brd_ProductVersion;
                    bd->bd_Flags |= BDFF_REGISTERED;
                    if(brd->brd_Flags & BRDF_CLASSIC) {
                        bd->bd_Flags |= BDFF_CLASSIC;
                    }
                    if(brd->brd_Flags & BRDF_LE) {
                        bd->bd_Flags |= BDFF_LE;
                    }
                    if(brd->brd_Flags & BRDF_BONDED) {
                        bd->bd_Flags |= BDFF_BONDED;
                    }
                    bApplyDevConfig(BluetoothBase, bd);
                    if(!bd->bd_Name) {
                        bd->bd_Name = btCopyStr(bd->bd_AddrString);
                    }
                    bd->bd_Node.ln_Name = bd->bd_Name;
                    btUnlockDevice(bd);
                    count++;
                    btSendEvent(BEHMB_ADDDEVICE, bd, NULL);
                    btSendEvent(BEHMB_DEVICEREGISTERED, bd, NULL);
                }
            } else {
                /* already known (discovered or paired before the config was
                   loaded): pick up the stored name/policy/keys */
                btLockWriteDevice(bd);
                if(!(bd->bd_Flags & BDFF_REGISTERED)) {
                    bd->bd_Flags |= BDFF_REGISTERED;
                    bApplyDevConfig(BluetoothBase, bd);
                    btUnlockDevice(bd);
                    count++;
                    btSendEvent(BEHMB_DEVICEREGISTERED, bd, NULL);
                } else {
                    btUnlockDevice(bd);
                }
            }
        }
        pic = btNextCfgForm(pic);
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    if(count) {
        bth->bth_LEListsDirty = TRUE;   /* the controller's accept/resolving lists follow the bonds */
    }
    return(count);
}
/* \\\ */

/* /// "bRekeyDevice()" */
/* A bonded LE peer told us its identity address (it had been talking to us
   from a resolvable private address): from now on the device is identified
   and stored under the identity, while the link that is up keeps using the
   private address (bd_CurAddr). Called with the device write locked. */
void bRekeyDevice(LIBBASETYPEPTR BluetoothBase, struct BtDevice *bd, const UBYTE *addr, UBYTE addrtype)
{
    struct BtIFFContext *pic;
    STRPTR oldid = bd->bd_IDString;
    STRPTR newid;
    BOOL addrname;

    if(!memcmp(bd->bd_Address.bd_Addr, addr, 6)) {
        return;
    }
    CopyMem(bd->bd_Address.bd_Addr, bd->bd_CurAddr, 6);
    bd->bd_CurAddrType = bd->bd_AddrType;
    bd->bd_CurAddrValid = TRUE;
    addrname = bd->bd_Name && !strcmp(bd->bd_Name, (char *) bd->bd_AddrString);
    CopyMem((APTR) addr, bd->bd_Address.bd_Addr, 6);
    bd->bd_AddrType = addrtype;
    bAddrToStr(bd->bd_Address.bd_Addr, (STRPTR) bd->bd_AddrString);
    newid = btCopyStrFmt("BT:%s", bd->bd_AddrString);
    if(!newid) {
        return;
    }
    bd->bd_IDString = newid;
    if(addrname) {
        btFreeVec(bd->bd_Name);
        bd->bd_Name = btCopyStr(bd->bd_AddrString);
        bd->bd_Node.ln_Name = bd->bd_Name;
    }
    /* move the config form (custom name, popup settings) to the new id */
    if(oldid) {
        bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
        pic = btFindCfgForm(NULL, IFFFORM_BTDEVICECFG);
        while(pic) {
            if(bMatchStringChunk(BluetoothBase, pic, IFFCHNK_DEVID, oldid)) {
                bAddStringChunk(BluetoothBase, pic, IFFCHNK_DEVID, newid);
                break;
            }
            pic = btNextCfgForm(pic);
        }
        bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
        btFreeVec(oldid);
    }
}
/* \\\ */

/* /// "bStoreDevConfig()" */
/* Writes custom name, popup config and (if registered) the registration
   record of a device into its DEVC form. With persist set the config is also
   written to disk (ENVARC:/ENV:Sys/bluetooth.prefs) by the event handler
   process - that is how registrations and bond keys survive a reboot without
   the user having to press Save in the prefs. */
void bStoreDevConfig(LIBBASETYPEPTR BluetoothBase, struct BtDevice *bd, BOOL persist)
{
    struct BtIFFContext *pic;
    struct BtIFFContext *root;

    if(!bd->bd_IDString) {
        return;
    }
    bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    pic = btFindCfgForm(NULL, IFFFORM_BTDEVICECFG);
    while(pic) {
        if(bMatchStringChunk(BluetoothBase, pic, IFFCHNK_DEVID, bd->bd_IDString)) {
            break;
        }
        pic = btNextCfgForm(pic);
    }
    if(!pic) {
        root = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
        if(root->bic_Node.ln_Succ) {
            pic = bAllocForm(BluetoothBase, root, IFFFORM_BTDEVICECFG);
            if(pic) {
                bAddStringChunk(BluetoothBase, pic, IFFCHNK_DEVID, bd->bd_IDString);
            }
        }
    }
    if(pic) {
        /* NAME: a custom name given by the user; DNAM: the name the device
           reported, so a device restored at boot is listed by name until it
           is heard from again */
        if(bd->bd_Name && (!bd->bd_OrigName || strcmp(bd->bd_Name, bd->bd_OrigName))) {
            bAddStringChunk(BluetoothBase, pic, IFFCHNK_NAME, bd->bd_Name);
        } else {
            bRemCfgChunk(BluetoothBase, pic, IFFCHNK_NAME);
        }
        if(bd->bd_OrigName) {
            bAddStringChunk(BluetoothBase, pic, IFFCHNK_DEVNAME, bd->bd_OrigName);
        }
        bAddCfgChunk(BluetoothBase, pic, &bd->bd_PoPoCfg);
        if(bd->bd_Flags & BDFF_REGISTERED) {
            struct BtRegDevCfg brd;
            memset(&brd, 0, sizeof(brd));
            brd.brd_ChunkID = AROS_LONG2BE(IFFCHNK_REGDEVICE);
            brd.brd_Length = AROS_LONG2BE(sizeof(struct BtRegDevCfg) - 8);
            CopyMem(bd->bd_Address.bd_Addr, brd.brd_Address, 6);
            brd.brd_AddressType = bd->bd_AddrType;
            brd.brd_Flags = ((bd->bd_Flags & BDFF_CLASSIC) ? BRDF_CLASSIC : 0) |
                            ((bd->bd_Flags & BDFF_LE) ? BRDF_LE : 0) |
                            ((bd->bd_Flags & BDFF_BONDED) ? BRDF_BONDED : 0);
            brd.brd_ClassOfDevice = bd->bd_ClassOfDevice;
            brd.brd_Appearance = bd->bd_Appearance;
            brd.brd_VendorIDSource = bd->bd_VendorIDSource;
            brd.brd_VendorID = bd->bd_VendorID;
            brd.brd_ProductID = bd->bd_ProductID;
            brd.brd_ProductVersion = bd->bd_ProductVersion;
            bAddCfgChunk(BluetoothBase, pic, &brd);
            if((bd->bd_Flags & BDFF_BONDED) && bd->bd_Keys.bkc_Flags) {
                bd->bd_Keys.bkc_ChunkID = AROS_LONG2BE(IFFCHNK_KEYS);
                bd->bd_Keys.bkc_Length = AROS_LONG2BE(sizeof(struct BtKeyCfg) - 8);
                bAddCfgChunk(BluetoothBase, pic, &bd->bd_Keys);
            } else {
                bRemCfgChunk(BluetoothBase, pic, IFFCHNK_KEYS);
            }
        } else {
            bRemCfgChunk(BluetoothBase, pic, IFFCHNK_REGDEVICE);
            bRemCfgChunk(BluetoothBase, pic, IFFCHNK_KEYS);
        }
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    bCheckCfgChanged(BluetoothBase);
    if(persist) {
        BluetoothBase->bt_SaveConfigReq = TRUE;
        Forbid();
        if(BluetoothBase->bt_EventHandler.bh_Task && BluetoothBase->bt_EventHandler.bh_TimerMsgPort) {
            Signal(BluetoothBase->bt_EventHandler.bh_Task,
                   1UL << BluetoothBase->bt_EventHandler.bh_TimerMsgPort->mp_SigBit);
        }
        Permit();
    }
}
/* \\\ */

/* *** Channels *** */

/* /// "btAllocChannel()" */
/*****************************************************************************
    NAME
        APTR btAllocChannel(

    SYNOPSIS
        APTR bd, struct MsgPort *mp, APTR bep)

    FUNCTION
        Allocate a reusable request channel for a device or one of its endpoints.

    INPUTS
        bd - Live device handle; NULL returns NULL.
        mp - Initialized reply port; NULL returns NULL. Use a signal port
            owned by the task that will call the channel wait functions.
        bep - Endpoint belonging to bd, or NULL for device control requests.

    RESULT
        Channel handle, or NULL for invalid arguments or allocation failure.

    NOTES
        Call from task context. Keep the library, reply port, hardware and
        endpoint valid until btFreeChannel(). The channel holds a device
        usage count, but does not independently retain its endpoint across
        service refresh. Coordinate refresh and removal with the channel
        owner. Do not free the channel with btFreeVec().

        An endpoint channel starts with BTPR_READ. Configure a control
        channel with btChannelSetup() before submission. Auto-connect is
        copied from the device policy; it is not a live link to that policy.
        Allocation may wait for the hardware task using SIGB_SINGLE. A
        returned handle does not establish that a remote channel is open;
        the internal open request's error is not returned.

    SEE ALSO
        btChannelSetup(), btSendChannel(), btFreeChannel()

*****************************************************************************/
AROS_LH3(struct BtChannel *, btAllocChannel,
         AROS_LHA(struct BtDevice *, bd, A0),
         AROS_LHA(struct MsgPort *, mp, A1),
         AROS_LHA(struct BtEndpoint *, bep, A2),
         LIBBASETYPEPTR, BluetoothBase, 23, bt)
{
    AROS_LIBFUNC_INIT
    struct BtChannel *bch;

    KPRINTF(2, ("btAllocChannel(%p, %p, %p)\n", bd, mp, bep));
    if(!bd || !mp) {
        return(NULL);
    }
    if(bep && (bep->bep_Service->bsv_Device != bd)) {
        btAddErrorMsg0(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Endpoint does not belong to the device given!");
        return(NULL);
    }
    if((bch = btAllocVec(sizeof(struct BtChannel)))) {
        bch->bch_Msg.mn_Node.ln_Type = NT_FREEMSG;
        bch->bch_Msg.mn_ReplyPort = mp;
        bch->bch_Msg.mn_Length = sizeof(struct BtChannel);
        bch->bch_Device = bd;
        bch->bch_Hardware = bd->bd_Hardware;
        bch->bch_Endpoint = bep;
        bch->bch_MsgPort = mp;
        bch->bch_Timeout = 0;
        if(bd->bd_PoPoCfg.bpc_AutoConnect) {
            bch->bch_Flags |= BCHF_AUTOCONNECT;
        }
        Forbid();
        bd->bd_UseCnt++;
        Permit();
        if(bep) {
            LONG err;
            bSubmitCtrl(BluetoothBase, bd->bd_Hardware, bd, BTPRI_OPENCHANNEL, 0, 0, bep, 0, &err);
        }
        return(bch);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btFreeChannel()" */
/*****************************************************************************
    NAME
        APTR btFreeChannel(

    SYNOPSIS
        APTR pp)

    FUNCTION
        Release a channel and its device usage count.

    INPUTS
        pp - Channel handle, or NULL for no operation.

    RESULT
        No defined return value. Ignore the public APTR result.

    NOTES
        Call from the reply-port owner task. Complete and consume the last
        reply before freeing the channel. Keep the library, port and hardware
        valid through the call. No other task may use the channel afterward.
        The reply port and caller's data buffer are not freed.

        A pending request triggers an abort followed by a wait. Closing an
        endpoint channel can also wait for the hardware task. There is no
        bounded completion guarantee. Releasing the last device usage count
        can finish deferred device cleanup.

    BUGS
        The public declaration returns APTR, but the implementation returns
        void. It does not produce a valid pointer result. A reply already
        queued on the port is not removed when freeing a completed channel;
        consume it first to avoid leaving a freed message in the queue.

    SEE ALSO
        btAllocChannel(), btAbortChannel(), btWaitChannel()

*****************************************************************************/
AROS_LH1(void, btFreeChannel,
         AROS_LHA(struct BtChannel *, bch, A1),
         LIBBASETYPEPTR, BluetoothBase, 24, bt)
{
    AROS_LIBFUNC_INIT
    struct BtDevice *bd;
    if(!bch) {
        return;
    }
    KPRINTF(2, ("btFreeChannel(%p)\n", bch));
    bd = bch->bch_Device;

    if(bch->bch_Msg.mn_Node.ln_Type == NT_MESSAGE) {
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "Tried to free channel on %s that was still pending!", bd->bd_Name);
        btAbortChannel(bch);
        btWaitChannel(bch);
    }

    if(bch->bch_Endpoint) {
        LONG err;
        /* tell the hardware task the channel is gone (channel reference drop) */
        bSubmitCtrl(BluetoothBase, bd->bd_Hardware, bd, BTPRI_CLOSECHANNEL, 0, 0, bch->bch_Endpoint, 0, &err);
    }

    Forbid();
    if(!(--bd->bd_UseCnt) && (bd->bd_Flags & BDFF_DELEXPUNGE)) {
        Permit();
        KPRINTF(20, ("Finally getting rid of device %s\n", bd->bd_Name));
        bFreeDevice(BluetoothBase, bd);
    } else {
        Permit();
    }

    btFreeVec(bch);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btChannelSetup()" */
/*****************************************************************************
    NAME
        VOID btChannelSetup(

    SYNOPSIS
        APTR pp, UWORD rq, UWORD val, UWORD idx)

    FUNCTION
        Set the request and its two 16-bit parameters for the next submission.

    INPUTS
        pp - Idle channel whose preceding reply has been consumed.
        rq - BTPR_* request from libraries/bluetooth.h.
        val, idx - Request parameters described below; use zero when unused.

    NOTES
        Endpoint channels use BTPR_READ to receive one SDU or notification,
        or BTPR_WRITE to send data. val and idx are unused. A GATT endpoint
        read waits for a notification or indication; it is not an ATT read.

        Control channels have no endpoint and support these requests:
        BTPR_REMOTENAME - Receive a NUL-terminated remote name; buffer
            capacity includes the terminator. Actual excludes that terminator.
        BTPR_SDPSEARCH - val is the 16-bit service UUID. Receive consecutive
            four-byte, big-endian record handles; Actual is a byte count.
        BTPR_SDPATTRIBUTES - val is the low half and idx the high half of
            a record handle. Receive the attribute sequence's nested bytes.
        BTPR_GATTREAD - val is the remote attribute handle; receive its value.
        BTPR_GATTWRITE - val is the handle; send a value with a response.
        BTPR_GATTWRITENORSP - val is the handle; send without a response.
        BTPR_READRSSI - Receive one signed BYTE from the cached inquiry or
            advertising result; 127 means unknown.

        Setup performs no I/O or validation. The settings remain until
        changed. Keep the channel idle while changing them. Payload buffers
        are supplied to btSendChannel() or btDoChannel().

    RESULT
        None.

    BUGS
        BTPR_L2CAPECHO is declared but is not handled; it returns BTIOERR_NOTSUPPORTED.

    SEE ALSO
        btDoChannel(), btSendChannel(), btGetChannelActual()

*****************************************************************************/
AROS_LH4(void, btChannelSetup,
         AROS_LHA(struct BtChannel *, bch, A1),
         AROS_LHA(UWORD, rq, D0),
         AROS_LHA(UWORD, val, D1),
         AROS_LHA(UWORD, idx, D2),
         LIBBASETYPEPTR, BluetoothBase, 25, bt)
{
    AROS_LIBFUNC_INIT
    KPRINTF(1, ("btChannelSetup(%p, (%04lx %04lx %04lx))\n", bch, rq, val, idx));
    bch->bch_Request = rq;
    bch->bch_Value = val;
    bch->bch_Index = idx;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btSendChannel()" */
/*****************************************************************************
    NAME
        VOID btSendChannel(

    SYNOPSIS
        APTR pp, APTR data, ULONG len)

    FUNCTION
        Submit a channel request and return without waiting for its reply.

    INPUTS
        pp - Idle channel with no unconsumed previous reply.
        data - Request buffer: writable for reads, readable for writes.
            Supply storage for len bytes when the request transfers data.
        len - Read capacity or write length in bytes.

    RESULT
        None. Obtain completion status with btWaitChannel() or after receiving the reply.

    NOTES
        Call from task context. The buffer is borrowed until completion;
        do not modify a write buffer or inspect a read buffer while pending.
        Keep the channel, endpoint, hardware, port and library valid. Submit
        only one request at a time per channel. Submission clears Actual
        and Error; neither zero value proves completion.

        Completion replies the channel message to its allocation port, even
        when submission immediately detects unavailable hardware. Consume
        each reply once: use btWaitChannel(), or receive it with GetMsg()
        and query the result. Do not call btWaitChannel() after GetMsg() has
        removed that reply. Do not ReplyMsg() the returned channel message.

        Endpoint reads consume one queued SDU or notification. An undersized
        buffer receives a prefix with BTIOERR_OVERFLOW; the remainder is
        discarded. Successful writes need not imply remote application
        receipt. BCHA_NoWait does not bypass connection setup and is not
        implemented for GATT notification reads.

    BUGS
        BCHA_Timeout is stored but not enforced by channel dispatch. Do not
        rely on it to bound a pending request.

    SEE ALSO
        btChannelSetup(), btWaitChannel(), btCheckChannel(), btAbortChannel()

*****************************************************************************/
AROS_LH3(void, btSendChannel,
         AROS_LHA(struct BtChannel *, bch, A1),
         AROS_LHA(APTR, data, A0),
         AROS_LHA(ULONG, len, D0),
         LIBBASETYPEPTR, BluetoothBase, 27, bt)
{
    AROS_LIBFUNC_INIT
    struct BtHardware *bth = bch->bch_Hardware;

    KPRINTF(200, ("btSendChannel(%p, %p, %ld)\n", bch, data, len));
    bch->bch_Data = data;
    bch->bch_Length = len;
    bch->bch_Actual = 0;
    bch->bch_Error = 0;
    bch->bch_AbortChannel = NULL;
    bch->bch_Msg.mn_Node.ln_Type = NT_MESSAGE;
    bch->bch_Msg.mn_ReplyPort = bch->bch_MsgPort;
    if(!bth->bth_Task || (bth->bth_State == BHS_OFFLINE)) {
        bch->bch_Error = BTIOERR_NOTREADY;
        ReplyMsg(&bch->bch_Msg);
        return;
    }
    if(bHaveDOS(BluetoothBase) && bch->bch_Device) {
        DateStamp(&bch->bch_Device->bd_LastSeen);
    }
    PutMsg(&bth->bth_TaskMsgPort, &bch->bch_Msg);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btDoChannel()" */
/*****************************************************************************
    NAME
        LONG btDoChannel(

    SYNOPSIS
        APTR pp, APTR data, ULONG len)

    FUNCTION
        Submit a channel request and wait for its reply.

    INPUTS
        pp - Idle channel with no unconsumed previous reply.
        data - Request buffer, with the direction and lifetime required by
            btSendChannel().
        len - Read capacity or write length in bytes.

    RESULT
        Zero on success, or an IOERR_* or BTIOERR_* completion code.

    NOTES
        This combines btSendChannel() and btWaitChannel(). Call from the
        reply-port owner task, never the hardware task servicing the channel.
        The reply is consumed before return; do not wait for it again.
        Use btGetChannelActual() for the byte count or request-specific
        error detail. Keep all request storage valid until return.

        No overall timeout is enforced. The call can wait indefinitely;
        holding a lock needed by the hardware task can deadlock it.

    SEE ALSO
        btSendChannel(), btWaitChannel(), btGetChannelActual()

*****************************************************************************/
AROS_LH3(LONG, btDoChannel,
         AROS_LHA(struct BtChannel *, bch, A1),
         AROS_LHA(APTR, data, A0),
         AROS_LHA(ULONG, len, D0),
         LIBBASETYPEPTR, BluetoothBase, 26, bt)
{
    AROS_LIBFUNC_INIT
    KPRINTF(200, ("btDoChannel(%p, %p, %ld)\n", bch, data, len));
    btSendChannel(bch, data, len);
    return(btWaitChannel(bch));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btAbortChannel()" */
/*****************************************************************************
    NAME
        VOID btAbortChannel(

    SYNOPSIS
        APTR pp)

    FUNCTION
        Ask the hardware task to cancel a pending channel request.

    INPUTS
        pp - Live channel. A channel that is not pending needs no action.

    RESULT
        None. The call does not report whether cancellation was requested or succeeded.

    NOTES
        Call from the reply-port owner task, never the hardware task.
        The call waits for a separate cancellation reply, using the same
        port, but does not consume the original request's reply. Finish
        with btWaitChannel(pp), or receive that original reply through the
        existing message loop, before reusing its buffer or channel.

        Completion can win the race with cancellation; do not require
        IOERR_ABORTED as the result. Cancellation does not undo remote
        effects already performed. Keep the hardware task and port alive
        through this operation; there is no timeout.

    BUGS
        If allocation of the cancellation message fails, the call returns
        without requesting cancellation and without reporting failure.

    SEE ALSO
        btWaitChannel(), btCheckChannel(), btGetChannelError()

*****************************************************************************/
AROS_LH1(void, btAbortChannel,
         AROS_LHA(struct BtChannel *, bch, A1),
         LIBBASETYPEPTR, BluetoothBase, 28, bt)
{
    AROS_LIBFUNC_INIT
    struct BtChannel *nbch;

    KPRINTF(5, ("btAbortChannel(%p)\n", bch));
    if(bch->bch_Msg.mn_Node.ln_Type != NT_MESSAGE) {
        return;
    }
    if((nbch = btAllocVec(sizeof(struct BtChannel)))) {
        nbch->bch_Device = bch->bch_Device;
        nbch->bch_Hardware = bch->bch_Hardware;
        nbch->bch_MsgPort = nbch->bch_Msg.mn_ReplyPort = bch->bch_MsgPort;
        nbch->bch_Msg.mn_Length = sizeof(struct BtChannel);
        nbch->bch_Msg.mn_Node.ln_Type = NT_MESSAGE;
        nbch->bch_AbortChannel = bch;
        PutMsg(&bch->bch_Hardware->bth_TaskMsgPort, &nbch->bch_Msg);
        btWaitChannel(nbch);
        btFreeVec(nbch);
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btWaitChannel()" */
/*****************************************************************************
    NAME
        LONG btWaitChannel(

    SYNOPSIS
        APTR pp)

    FUNCTION
        Wait until a channel is no longer pending and remove its queued reply.

    INPUTS
        pp - Live channel whose reply has not been removed by GetMsg()
            or another waiter.

    RESULT
        Zero on success, or the stored IOERR_* or BTIOERR_* completion code.

    NOTES
        Call from the task signaled by the reply port. The call waits on
        that port's signal and leaves other queued messages in place.
        Signals observed while waiting are restored for other port users.
        It neither frees the channel nor releases the caller's buffer.

        No overall timeout or break-signal cancellation is provided. Do not
        call from the hardware task or while holding a lock it needs.
        For a newly allocated channel this returns its initial zero status
        without submitting anything. A nonpending channel returns its
        stored status; that is not evidence of a new operation.

    BUGS
        Every call updates the device's reachability history from the stored
        error, including repeated calls after completion. Wait once per
        submitted request; use btGetChannelError() for later inspection.

    SEE ALSO
        btSendChannel(), btAbortChannel(), btGetChannelActual(), btGetChannelError()

*****************************************************************************/
AROS_LH1(LONG, btWaitChannel,
         AROS_LHA(struct BtChannel *, bch, A1),
         LIBBASETYPEPTR, BluetoothBase, 29, bt)
{
    AROS_LIBFUNC_INIT
    ULONG sigs = 0;
    struct BtDevice *bd = bch->bch_Device;
    LONG ioerr;
    KPRINTF(5, ("btWaitChannel(%p)\n", bch));
    while(bch->bch_Msg.mn_Node.ln_Type == NT_MESSAGE) {
        sigs |= Wait(1L<<bch->bch_MsgPort->mp_SigBit);
    }
    Forbid();
    if(bch->bch_Msg.mn_Node.ln_Type == NT_REPLYMSG) {
        bch->bch_Msg.mn_Node.ln_Type = NT_FREEMSG;
        Remove(&bch->bch_Msg.mn_Node);
    }
    // avoid signals getting lost for other messages arriving.
    SetSignal(sigs, sigs);
    Permit();
    ioerr = bch->bch_Error;
    if(bd) {
        switch(ioerr) {
        case BTIOERR_TIMEOUT:
        case BTIOERR_CONNFAILED:
            bd->bd_DeadCount += 3;
            break;
        case BTIOERR_HOSTERROR:
            bd->bd_DeadCount++;
            break;
        default:
            if(bd->bd_DeadCount) {
                bd->bd_DeadCount >>= 1;
            }
        }
        if(bd->bd_DeadCount > 14) {
            if(!(bd->bd_Flags & BDFF_DEAD)) {
                bd->bd_Flags |= BDFF_DEAD;
                btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "Device %s probably out of range or switched off!", bd->bd_Name);
                btSendEvent(BEHMB_DEVICEDEAD, bd, NULL);
            }
        } else {
            if((!bd->bd_DeadCount) && (bd->bd_Flags & BDFF_DEAD)) {
                bd->bd_Flags &= ~BDFF_DEAD;
                btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "Device %s is reachable again.", bd->bd_Name);
            }
        }
    }
    KPRINTF(200, ("btWaitChannel(%p)=%ld\n", bch, ioerr));
    return(ioerr);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btCheckChannel()" */
/*****************************************************************************
    NAME
        APTR btCheckChannel(

    SYNOPSIS
        APTR pp)

    FUNCTION
        Check whether a channel is still pending without waiting.

    INPUTS
        pp - Live channel handle; NULL is invalid.

    RESULT
        NULL while pending; otherwise pp, including for a newly allocated channel.

    NOTES
        A non-NULL result is neither a success status nor proof that a
        request was submitted. It does not remove a reply from the port.
        Consume the reply before resubmitting or freeing the channel, then
        inspect Error and Actual. Serialize access with submitters and
        consumers; this call does not lock the channel.

    SEE ALSO
        btWaitChannel(), btGetChannelError(), btGetChannelActual()

*****************************************************************************/
AROS_LH1(struct BtChannel *, btCheckChannel,
         AROS_LHA(struct BtChannel *, bch, A1),
         LIBBASETYPEPTR, BluetoothBase, 32, bt)
{
    AROS_LIBFUNC_INIT
    KPRINTF(5, ("btCheckChannel(%p)\n", bch));
    if(bch->bch_Msg.mn_Node.ln_Type == NT_MESSAGE) {
        return(NULL);
    }
    return(bch);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btGetChannelActual()" */
/*****************************************************************************
    NAME
        ULONG btGetChannelActual(

    SYNOPSIS
        APTR pp)

    FUNCTION
        Read the channel's stored transfer count or request-specific error detail.

    INPUTS
        pp - Live channel handle; NULL is invalid.

    RESULT
        On successful data transfer, the number of bytes transferred.
        For BTIOERR_OVERFLOW, the number of bytes copied, not the full
        required capacity. For BTIOERR_REMOTEERROR, this may instead be
        an HCI status, ATT error code or SDP client result.

    NOTES
        Check completion and Error before interpreting this value. A new
        channel and each submission start with zero. This call neither
        waits nor consumes a reply; a value read while pending is not a
        completion indication. Remote-name counts exclude the terminating
        NUL; SDP search counts are bytes, not record counts.

    SEE ALSO
        btGetChannelError(), btCheckChannel(), btChannelSetup()

*****************************************************************************/
AROS_LH1(ULONG, btGetChannelActual,
         AROS_LHA(struct BtChannel *, bch, A1),
         LIBBASETYPEPTR, BluetoothBase, 30, bt)
{
    AROS_LIBFUNC_INIT
    return(bch->bch_Actual);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btGetChannelError()" */
/*****************************************************************************
    NAME
        LONG btGetChannelError(

    SYNOPSIS
        APTR pp)

    FUNCTION
        Read the channel's stored error code without waiting or consuming a reply.

    INPUTS
        pp - Live channel handle; NULL is invalid.

    RESULT
        Zero for a successful completed request; otherwise an IOERR_* or
        BTIOERR_* code from exec/errors.h, devices/bluetoothhci.h or
        libraries/bluetooth.h.

    NOTES
        A new channel and each submission start with zero. Establish
        completion before treating zero as success. btGetChannelActual()
        can contain a partial byte count or remote error detail. Reading
        the status does not update device reachability history.

    SEE ALSO
        btGetChannelActual(), btCheckChannel(), btWaitChannel()

*****************************************************************************/
AROS_LH1(LONG, btGetChannelError,
         AROS_LHA(struct BtChannel *, bch, A1),
         LIBBASETYPEPTR, BluetoothBase, 31, bt)
{
    AROS_LIBFUNC_INIT
    return(bch->bch_Error);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Control requests (synchronous messages to the hardware task) *** */

/* /// "bSubmitCtrl()" */
/*
 * Sends a control request to the hardware task and waits for the reply,
 * using a channel and a message port on the stack. When called from within the
 * hardware task itself the request is executed directly (the task cannot
 * wait for itself).
 */
BOOL bSubmitCtrl(LIBBASETYPEPTR BluetoothBase, struct BtHardware *bth, struct BtDevice *bd,
                 UWORD request, UWORD val, UWORD idx, APTR data, ULONG len, LONG *error)
{
    struct BtChannel bch;
    struct MsgPort mp;

    if(!bth || !bth->bth_Task) {
        if(error) {
            *error = BTIOERR_NOTREADY;
        }
        return(FALSE);
    }
    memset(&bch, 0, sizeof(bch));
    memset(&mp, 0, sizeof(mp));
    bch.bch_Device = bd;
    bch.bch_Hardware = bth;
    bch.bch_Request = request;
    bch.bch_Value = val;
    bch.bch_Index = idx;
    bch.bch_Data = data;
    bch.bch_Length = len;
    bch.bch_Msg.mn_Length = sizeof(struct BtChannel);

    if(FindTask(NULL) == bth->bth_Task) {
        bHandleChannel(BluetoothBase, bth, &bch, TRUE);
    } else {
        mp.mp_Flags = PA_SIGNAL;
        mp.mp_SigBit = SIGB_SINGLE;
        mp.mp_SigTask = FindTask(NULL);
        NewList(&mp.mp_MsgList);
        bch.bch_MsgPort = &mp;
        bch.bch_Msg.mn_ReplyPort = &mp;
        bch.bch_Msg.mn_Node.ln_Type = NT_MESSAGE;
        SetSignal(0, SIGF_SINGLE);
        PutMsg(&bth->bth_TaskMsgPort, &bch.bch_Msg);
        while(bch.bch_Msg.mn_Node.ln_Type == NT_MESSAGE) {
            Wait(SIGF_SINGLE);
        }
        Forbid();
        if(bch.bch_Msg.mn_Node.ln_Type == NT_REPLYMSG) {
            Remove(&bch.bch_Msg.mn_Node);
        }
        Permit();
    }
    if(error) {
        *error = bch.bch_Error;
    }
    return(bch.bch_Error ? FALSE : TRUE);
}
/* \\\ */

/* /// "btStartDiscoveryA()" */
AROS_LH2(BOOL, btStartDiscoveryA,
         AROS_LHA(struct BtHardware *, bth, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, BluetoothBase, 75, bt)
{
    AROS_LIBFUNC_INIT
    struct BtDiscoveryParams bdp;
    LONG err;

    if(!bth) {
        return(FALSE);
    }
    bdp.bdp_Duration = GetTagData(BDSA_Duration, BluetoothBase->bt_GlobalCfg->bgc_DiscoveryTime, tags);
    bdp.bdp_Classic = GetTagData(BDSA_Classic, TRUE, tags) ? TRUE : FALSE;
    bdp.bdp_LE = GetTagData(BDSA_LE, TRUE, tags) ? TRUE : FALSE;
    bdp.bdp_ResolveNames = GetTagData(BDSA_ResolveNames, TRUE, tags) ? TRUE : FALSE;
    bdp.bdp_ClearOld = GetTagData(BDSA_ClearOld, FALSE, tags) ? TRUE : FALSE;
    if(!bdp.bdp_Duration) {
        bdp.bdp_Duration = 10;
    }
    return(bSubmitCtrl(BluetoothBase, bth, NULL, BTPRI_DISCOVERY, 0, 0, &bdp, sizeof(bdp), &err));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btStopDiscovery()" */
AROS_LH1(BOOL, btStopDiscovery,
         AROS_LHA(struct BtHardware *, bth, A0),
         LIBBASETYPEPTR, BluetoothBase, 76, bt)
{
    AROS_LIBFUNC_INIT
    LONG err;
    if(!bth) {
        return(FALSE);
    }
    return(bSubmitCtrl(BluetoothBase, bth, NULL, BTPRI_STOPDISCOVERY, 0, 0, NULL, 0, &err));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btConnectDevice()" */
AROS_LH1(BOOL, btConnectDevice,
         AROS_LHA(struct BtDevice *, bd, A0),
         LIBBASETYPEPTR, BluetoothBase, 77, bt)
{
    AROS_LIBFUNC_INIT
    LONG err;
    if(!bd) {
        return(FALSE);
    }
    return(bSubmitCtrl(BluetoothBase, bd->bd_Hardware, bd, BTPRI_CONNECT, 0, 0, NULL, 0, &err));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btDisconnectDevice()" */
AROS_LH1(BOOL, btDisconnectDevice,
         AROS_LHA(struct BtDevice *, bd, A0),
         LIBBASETYPEPTR, BluetoothBase, 78, bt)
{
    AROS_LIBFUNC_INIT
    LONG err;
    if(!bd) {
        return(FALSE);
    }
    return(bSubmitCtrl(BluetoothBase, bd->bd_Hardware, bd, BTPRI_DISCONNECT, 0, 0, NULL, 0, &err));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "bFillPairParams()" */
static void bFillPairParams(LIBBASETYPEPTR BluetoothBase, struct BtPairParams *bpp, struct TagItem *tags)
{
    struct TagItem *ti;
    memset(bpp, 0, sizeof(*bpp));
    bpp->bpp_Bond = GetTagData(BPRA_Bond, TRUE, tags) ? TRUE : FALSE;
    bpp->bpp_MITM = GetTagData(BPRA_MITM, FALSE, tags) ? TRUE : FALSE;
    bpp->bpp_IOCapability = GetTagData(BPRA_IOCapability, BPIO_DISPLAYYESNO, tags);
    bpp->bpp_PINCode = (STRPTR) GetTagData(BPRA_PINCode, 0, tags);
    if((ti = FindTagItem(BPRA_Passkey, tags))) {
        bpp->bpp_Passkey = ti->ti_Data;
        bpp->bpp_HavePasskey = TRUE;
    }
    if((ti = FindTagItem(BPRA_Confirm, tags))) {
        bpp->bpp_Confirm = ti->ti_Data ? TRUE : FALSE;
        bpp->bpp_HaveConfirm = TRUE;
    }
}
/* \\\ */

/* /// "btPairDeviceA()" */
AROS_LH2(BOOL, btPairDeviceA,
         AROS_LHA(struct BtDevice *, bd, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, BluetoothBase, 79, bt)
{
    AROS_LIBFUNC_INIT
    struct BtPairParams bpp;
    LONG err;
    if(!bd) {
        return(FALSE);
    }
    bFillPairParams(BluetoothBase, &bpp, tags);
    return(bSubmitCtrl(BluetoothBase, bd->bd_Hardware, bd, BTPRI_PAIR, 0, 0, &bpp, sizeof(bpp), &err));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btPairingReplyA()" */
AROS_LH2(BOOL, btPairingReplyA,
         AROS_LHA(struct BtDevice *, bd, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, BluetoothBase, 80, bt)
{
    AROS_LIBFUNC_INIT
    struct BtPairParams bpp;
    LONG err;
    if(!bd) {
        return(FALSE);
    }
    bFillPairParams(BluetoothBase, &bpp, tags);
    return(bSubmitCtrl(BluetoothBase, bd->bd_Hardware, bd, BTPRI_PAIRREPLY, 0, 0, &bpp, sizeof(bpp), &err));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btUnpairDevice()" */
AROS_LH1(BOOL, btUnpairDevice,
         AROS_LHA(struct BtDevice *, bd, A0),
         LIBBASETYPEPTR, BluetoothBase, 81, bt)
{
    AROS_LIBFUNC_INIT
    LONG err;
    if(!bd) {
        return(FALSE);
    }
    return(bSubmitCtrl(BluetoothBase, bd->bd_Hardware, bd, BTPRI_UNPAIR, 0, 0, NULL, 0, &err));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btRegisterDevice()" */
AROS_LH1(BOOL, btRegisterDevice,
         AROS_LHA(struct BtDevice *, bd, A0),
         LIBBASETYPEPTR, BluetoothBase, 82, bt)
{
    AROS_LIBFUNC_INIT
    LONG err;
    if(!bd) {
        return(FALSE);
    }
    return(bSubmitCtrl(BluetoothBase, bd->bd_Hardware, bd, BTPRI_REGISTER, 0, 0, NULL, 0, &err));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btUnregisterDevice()" */
AROS_LH1(BOOL, btUnregisterDevice,
         AROS_LHA(struct BtDevice *, bd, A0),
         LIBBASETYPEPTR, BluetoothBase, 83, bt)
{
    AROS_LIBFUNC_INIT
    LONG err;
    if(!bd) {
        return(FALSE);
    }
    return(bSubmitCtrl(BluetoothBase, bd->bd_Hardware, bd, BTPRI_UNREGISTER, 0, 0, NULL, 0, &err));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btEnumerateServices()" */
AROS_LH1(BOOL, btEnumerateServices,
         AROS_LHA(struct BtDevice *, bd, A0),
         LIBBASETYPEPTR, BluetoothBase, 84, bt)
{
    AROS_LIBFUNC_INIT
    LONG err;
    if(!bd) {
        return(FALSE);
    }
    return(bSubmitCtrl(BluetoothBase, bd->bd_Hardware, bd, BTPRI_ENUMSERVICES, 0, 0, NULL, 0, &err));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** Service records (our SDP server) *** */

/* /// "bServiceRecordEvent()" */
/* Tell the classes (the GATT and SDP server classes decide whether the
   record is offered) and whoever listens for events. */
static void bServiceRecordEvent(LIBBASETYPEPTR BluetoothBase, struct BtServiceRecord *bsr, BOOL added)
{
    struct BtClass *bc;

    btLockReadBase();
    for(bc = (struct BtClass *) BluetoothBase->bt_Classes.lh_Head; bc->bc_Node.ln_Succ;
        bc = (struct BtClass *) bc->bc_Node.ln_Succ) {
        btcDoMethod(BCM_ServiceRecordEvent, bsr, (IPTR) added);
    }
    btUnlockBase();
    btSendEvent(added ? BEHMB_ADDSERVICEREC : BEHMB_REMSERVICEREC, bsr, NULL);
}
/* \\\ */

/* /// "bFreeServiceRecord()" */
static void bFreeServiceRecord(LIBBASETYPEPTR BluetoothBase, struct BtServiceRecord *bsr)
{
    ULONG n;

    btFreeVec(bsr->bsr_Attrs);
    if(bsr->bsr_Chars) {
        for(n = 0; n < bsr->bsr_NumChars; n++) {
            btFreeVec(bsr->bsr_Chars[n].bgc_Value);
        }
        btFreeVec(bsr->bsr_Chars);
    }
    btFreeVec(bsr->bsr_Owner);
    btFreeVec(bsr->bsr_Name);
    btFreeVec(bsr);
}
/* \\\ */

/* /// "bSetGattUUID()" */
/* uuid128 is most significant byte first, the wire wants the reverse */
static UWORD bSetGattUUID(UBYTE *le, UWORD uuid16, CONST UBYTE *uuid128)
{
    ULONG n;

    if(uuid16 || !uuid128) {
        le[0] = uuid16 & 0xff;
        le[1] = uuid16 >> 8;
        return(2);
    }
    for(n = 0; n < 16; n++) {
        le[n] = uuid128[15 - n];
    }
    return(16);
}
/* \\\ */

/* /// "bAddGattRecord()" */
/*
 * A GATT service: its declaration and its characteristics become attributes
 * of the stack's GATT server. The record starts disabled; the GATT server
 * class enables what its user wants offered.
 */
static struct BtServiceRecord *bAddGattRecord(LIBBASETYPEPTR BluetoothBase, struct TagItem *tags)
{
    struct BtServiceRecord *bsr;
    ULONG uuid16 = GetTagData(BSRA_UUID16, 0, tags);
    CONST UBYTE *uuid128 = (CONST UBYTE *) GetTagData(BSRA_UUID128, 0, tags);
    struct BtGattCharDef *defs = (struct BtGattCharDef *) GetTagData(BSRA_Characteristics, 0, tags);
    ULONG num = GetTagData(BSRA_NumCharacteristics, 0, tags);
    STRPTR name = (STRPTR) GetTagData(BSRA_Name, 0, tags);
    STRPTR owner = (STRPTR) GetTagData(BSRA_Owner, 0, tags);
    ULONG interval = GetTagData(BSRA_LEConnInterval, 0, tags);
    ULONG n;

    if((!uuid16 && !uuid128) || (num && !defs) || (num > 64)) {
        return(NULL);
    }
    for(n = 0; n < num; n++) {
        if((!defs[n].bgd_UUID16 && !defs[n].bgd_UUID128) || (defs[n].bgd_Len > BGATT_MAXVALUE) ||
           (defs[n].bgd_MaxLen > BGATT_MAXVALUE) || (defs[n].bgd_Len && !defs[n].bgd_Value)) {
            return(NULL);
        }
    }
    if(!(bsr = btAllocVec(sizeof(struct BtServiceRecord)))) {
        return(NULL);
    }
    memset(bsr, 0, sizeof(struct BtServiceRecord));
    bsr->bsr_Protocol = BSVP_ATT;
    bsr->bsr_UUID16 = uuid128 ? 0 : uuid16;
    bsr->bsr_UUIDLen = bSetGattUUID(bsr->bsr_UUID, bsr->bsr_UUID16, uuid128);
    if(bsr->bsr_UUIDLen == 16) {
        CopyMem((APTR) uuid128, bsr->bsr_UUID128, 16);
    }
    bsr->bsr_Name = name ? btCopyStr(name) : NULL;
    bsr->bsr_Node.ln_Name = bsr->bsr_Name;
    if(!owner) {
        owner = (STRPTR) FindTask(NULL)->tc_Node.ln_Name;
    }
    bsr->bsr_Owner = owner ? btCopyStr(owner) : NULL;
    bsr->bsr_NumChars = num;
    bsr->bsr_LEConnInterval = (interval >= 1200) ? 1199 : interval;
    if(num) {
        if(!(bsr->bsr_Chars = btAllocVec(num * sizeof(struct BtGattChar)))) {
            bFreeServiceRecord(BluetoothBase, bsr);
            return(NULL);
        }
        memset(bsr->bsr_Chars, 0, num * sizeof(struct BtGattChar));
        for(n = 0; n < num; n++) {
            struct BtGattChar *bgc = &bsr->bsr_Chars[n];
            bgc->bgc_UUIDLen = bSetGattUUID(bgc->bgc_UUID, defs[n].bgd_UUID16,
                                            defs[n].bgd_UUID16 ? NULL : defs[n].bgd_UUID128);
            bgc->bgc_Properties = defs[n].bgd_Properties;
            bgc->bgc_MaxLen = (defs[n].bgd_MaxLen > defs[n].bgd_Len) ? defs[n].bgd_MaxLen : defs[n].bgd_Len;
            bgc->bgc_Len = defs[n].bgd_Len;
            if(bgc->bgc_MaxLen) {
                if(!(bgc->bgc_Value = btAllocVec(bgc->bgc_MaxLen))) {
                    bFreeServiceRecord(BluetoothBase, bsr);
                    return(NULL);
                }
                memset(bgc->bgc_Value, 0, bgc->bgc_MaxLen);
                if(bgc->bgc_Len) {
                    CopyMem((APTR) defs[n].bgd_Value, bgc->bgc_Value, bgc->bgc_Len);
                }
            }
        }
    }

    /* the handles are never used again, so that a device which remembers
       them cannot end up at another service */
    btLockWriteBase();
    if((ULONG) BluetoothBase->bt_NextGattHandle + 1 + 3 * num > 0xff00) {
        btUnlockBase();
        bFreeServiceRecord(BluetoothBase, bsr);
        return(NULL);
    }
    bsr->bsr_Handle = BluetoothBase->bt_NextRecordHandle++;
    bsr->bsr_FirstHandle = BluetoothBase->bt_NextGattHandle;
    BluetoothBase->bt_NextGattHandle += 1 + 3 * num;
    bsr->bsr_LastHandle = bsr->bsr_FirstHandle + 3 * num;
    if(num && !(bsr->bsr_Chars[num - 1].bgc_Properties & (BGDP_NOTIFY|BGDP_INDICATE))) {
        bsr->bsr_LastHandle--; /* the last characteristic has no client configuration */
    }
    AddTail(&BluetoothBase->bt_ServiceRecords, &bsr->bsr_Node);
    btUnlockBase();
    KPRINTF(5, ("GATT service %04lx: handles %04lx-%04lx\n", uuid16, bsr->bsr_FirstHandle, bsr->bsr_LastHandle));
    bServiceRecordEvent(BluetoothBase, bsr, TRUE);
    return(bsr);
}
/* \\\ */

/* Cache the shortest interval requested by an enabled GATT service. */
void bUpdateLEConnInterval(LIBBASETYPEPTR BluetoothBase)
{
    struct BtServiceRecord *bsr;
    ULONG interval = BluetoothBase->bt_LEConnInterval;

    btLockReadBase();
    for(bsr = (struct BtServiceRecord *) BluetoothBase->bt_ServiceRecords.lh_Head;
        bsr->bsr_Node.ln_Succ; bsr = (struct BtServiceRecord *) bsr->bsr_Node.ln_Succ) {
        if(bsr->bsr_Enabled && (bsr->bsr_Protocol == BSVP_ATT) &&
           bsr->bsr_LEConnInterval &&
           (!interval || (bsr->bsr_LEConnInterval < interval))) {
            interval = bsr->bsr_LEConnInterval;
        }
    }
    /* only a change is worth a new request on every peripheral link */
    if(BluetoothBase->bt_LEServiceInterval != interval) {
        BluetoothBase->bt_LEServiceInterval = interval;
        BluetoothBase->bt_LEConnSeq++;
    }
    btUnlockBase();
}

/* /// "btAddServiceRecordA()" */
/*
 * A record the SDP server advertises: ServiceRecordHandle, ServiceClassIDList,
 * ProtocolDescriptorList (L2CAP [+PSM] [, RFCOMM channel]), BrowseGroupList
 * (public browse root), BluetoothProfileDescriptorList and ServiceName. The
 * attribute list content is encoded once; the record handle is unique per
 * stack lifetime.
 */
AROS_LH1(APTR, btAddServiceRecordA,
         AROS_LHA(struct TagItem *, tags, A0),
         LIBBASETYPEPTR, BluetoothBase, 88, bt)
{
    AROS_LIBFUNC_INIT
    struct BtServiceRecord *bsr;
    ULONG uuid16 = GetTagData(BSRA_UUID16, 0, tags);
    ULONG proto = GetTagData(BSRA_Protocol, BSVP_RFCOMM, tags);
    ULONG channel = GetTagData(BSRA_RFCOMMChannel, 0, tags);
    ULONG psm = GetTagData(BSRA_PSM, 0, tags);
    ULONG profile = GetTagData(BSRA_ProfileUUID16, uuid16, tags);
    ULONG version = GetTagData(BSRA_ProfileVersion, 0x0102, tags);
    STRPTR name = (STRPTR) GetTagData(BSRA_Name, 0, tags);
    STRPTR owner = (STRPTR) GetTagData(BSRA_Owner, 0, tags);
    UBYTE buf[256];
    struct bt_buf_writer w;
    ULONG nlen = name ? strlen((char *) name) : 0;
    ULONG protolen;

    KPRINTF(5, ("btAddServiceRecordA(%04lx, %ld)\n", uuid16, proto));
    if(proto == BSVP_ATT) {
        return(bAddGattRecord(BluetoothBase, tags));
    }
    if(!uuid16 || ((proto == BSVP_RFCOMM) && ((channel < 1) || (channel > 30))) ||
       ((proto == BSVP_L2CAP) && !psm) || ((proto != BSVP_RFCOMM) && (proto != BSVP_L2CAP))) {
        return(NULL);
    }
    if(nlen > 64) {
        nlen = 64;
    }
    if(!(bsr = btAllocVec(sizeof(struct BtServiceRecord)))) {
        return(NULL);
    }
    memset(bsr, 0, sizeof(struct BtServiceRecord));
    bsr->bsr_Enabled = TRUE;
    bsr->bsr_UUID16 = uuid16;
    bsr->bsr_Protocol = proto;
    bsr->bsr_Channel = channel;
    bsr->bsr_PSM = psm;
    bsr->bsr_Name = name ? btCopyStr(name) : NULL;
    if(!owner) {
        owner = (STRPTR) FindTask(NULL)->tc_Node.ln_Name;
    }
    bsr->bsr_Owner = owner ? btCopyStr(owner) : NULL;

    btLockWriteBase();
    bsr->bsr_Handle = BluetoothBase->bt_NextRecordHandle++;
    btUnlockBase();

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_sdp_encode_uint(&w, 0x0000, 2);
    bt_sdp_encode_uint(&w, bsr->bsr_Handle, 4);
    bt_sdp_encode_uint(&w, 0x0001, 2);
    bt_sdp_encode_sequence_header(&w, 3);
    bt_sdp_encode_uuid16(&w, uuid16);
    bt_sdp_encode_uint(&w, 0x0004, 2);
    /* L2CAP descriptor: 5 bytes with a PSM, 3 without; RFCOMM descriptor: 5 */
    protolen = ((proto == BSVP_L2CAP) ? 7 : 5) + ((proto == BSVP_RFCOMM) ? 7 : 0);
    bt_sdp_encode_sequence_header(&w, protolen);
    if(proto == BSVP_L2CAP) {
        bt_sdp_encode_sequence_header(&w, 5);
        bt_sdp_encode_uuid16(&w, 0x0100);
        bt_sdp_encode_uint(&w, psm, 2);
    } else {
        bt_sdp_encode_sequence_header(&w, 3);
        bt_sdp_encode_uuid16(&w, 0x0100);
        bt_sdp_encode_sequence_header(&w, 5);
        bt_sdp_encode_uuid16(&w, 0x0003);
        bt_sdp_encode_uint(&w, channel, 1);
    }
    bt_sdp_encode_uint(&w, 0x0005, 2);
    bt_sdp_encode_sequence_header(&w, 3);
    bt_sdp_encode_uuid16(&w, 0x1002);           /* PublicBrowseRoot */
    bt_sdp_encode_uint(&w, 0x0009, 2);
    bt_sdp_encode_sequence_header(&w, 8);
    bt_sdp_encode_sequence_header(&w, 6);
    bt_sdp_encode_uuid16(&w, profile);
    bt_sdp_encode_uint(&w, version, 2);
    if(nlen) {
        bt_sdp_encode_uint(&w, 0x0100, 2);
        bt_buf_writer_write_u8(&w, 0x25);        /* text, 8 bit length */
        bt_buf_writer_write_u8(&w, nlen);
        bt_buf_writer_write_bytes(&w, (const uint8_t *) name, nlen);
    }
    bsr->bsr_AttrsLen = bt_buf_writer_len(&w);
    if(!(bsr->bsr_Attrs = btAllocVec(bsr->bsr_AttrsLen))) {
        bFreeServiceRecord(BluetoothBase, bsr);
        return(NULL);
    }
    CopyMem(buf, bsr->bsr_Attrs, bsr->bsr_AttrsLen);
    bsr->bsr_Node.ln_Name = bsr->bsr_Name;

    btLockWriteBase();
    AddTail(&BluetoothBase->bt_ServiceRecords, &bsr->bsr_Node);
    btUnlockBase();
    KPRINTF(5, ("service record %08lx: class %04lx, %ld bytes\n", bsr->bsr_Handle, uuid16, bsr->bsr_AttrsLen));
    BluetoothBase->bt_EIRSeq++;
    bServiceRecordEvent(BluetoothBase, bsr, TRUE);
    return(bsr);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btRemServiceRecord()" */
AROS_LH1(void, btRemServiceRecord,
         AROS_LHA(APTR, record, A0),
         LIBBASETYPEPTR, BluetoothBase, 89, bt)
{
    AROS_LIBFUNC_INIT
    struct BtServiceRecord *bsr = record;
    struct BtGattNotification *bgn, *next;
    if(!bsr) {
        return;
    }
    /* the radio tasks only look at the records with the base locked */
    btLockWriteBase();
    Remove(&bsr->bsr_Node);
    ObtainSemaphore(&BluetoothBase->bt_GattLock);
    bgn = (struct BtGattNotification *) BluetoothBase->bt_GattNotifications.lh_Head;
    while(bgn->bgn_Node.ln_Succ) {
        next = (struct BtGattNotification *) bgn->bgn_Node.ln_Succ;
        if(bgn->bgn_Record == bsr) {
            Remove(&bgn->bgn_Node);
            BluetoothBase->bt_GattNotificationCount--;
            btFreeVec(bgn);
        }
        bgn = next;
    }
    ReleaseSemaphore(&BluetoothBase->bt_GattLock);
    btUnlockBase();
    bUpdateLEConnInterval(BluetoothBase);
    BluetoothBase->bt_LEAdvSeq++;
    BluetoothBase->bt_EIRSeq++;
    bServiceRecordEvent(BluetoothBase, bsr, FALSE);
    bFreeServiceRecord(BluetoothBase, bsr);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btSetServiceValue()" */
/*
 * Change the value of a characteristic of one of our GATT services. Devices
 * that subscribed to it are notified by the radio tasks shortly after.
 * Returns the length stored or -1.
 */
AROS_LH4(LONG, btSetServiceValue,
         AROS_LHA(APTR, record, A0),
         AROS_LHA(ULONG, index, D0),
         AROS_LHA(APTR, data, A1),
         AROS_LHA(ULONG, len, D1),
         LIBBASETYPEPTR, BluetoothBase, 90, bt)
{
    AROS_LIBFUNC_INIT
    struct BtServiceRecord *bsr = record;
    struct BtGattChar *bgc;
    struct BtGattNotification *bgn = NULL;
    struct BtHardware *bth;
    ULONG seq;

    if(!bsr || (bsr->bsr_Protocol != BSVP_ATT) || (index >= bsr->bsr_NumChars) || (len && !data)) {
        return(-1);
    }
    bgc = &bsr->bsr_Chars[index];
    if(len > bgc->bgc_MaxLen) {
        return(-1);
    }
    if(bgc->bgc_Properties & (BGDP_NOTIFY|BGDP_INDICATE)) {
        if(!(bgn = btAllocVec(sizeof(struct BtGattNotification) + len))) {
            return(-1);
        }
        bgn->bgn_Record = bsr;
        bgn->bgn_Index = index;
        bgn->bgn_Length = len;
        if(len) {
            CopyMem(data, bgn->bgn_Data, len);
        }
    }
    /* what every radio has sent (or none is left to send) goes first */
    bGattSrvCollect(BluetoothBase);
    ObtainSemaphore(&BluetoothBase->bt_GattLock);
    if(bgn && !bgc->bgc_Subscribers) {
        /* nobody follows this value: there is nothing to send */
        btFreeVec(bgn);
        bgn = NULL;
    }
    if(bgn && (BluetoothBase->bt_GattNotificationCount >= 256)) {
        ReleaseSemaphore(&BluetoothBase->bt_GattLock);
        btFreeVec(bgn);
        return(-1);             /* explicit backpressure: no silent data loss */
    }
    if(len) {
        CopyMem(data, bgc->bgc_Value, len);
    }
    bgc->bgc_Len = len;
    seq = ++BluetoothBase->bt_GattSeq;
    bgc->bgc_Seq = seq;
    if(bgn) {
        bgn->bgn_Seq = seq;
        AddTail(&BluetoothBase->bt_GattNotifications, &bgn->bgn_Node);
        BluetoothBase->bt_GattNotificationCount++;
    }
    ReleaseSemaphore(&BluetoothBase->bt_GattLock);

    /* Do not quantise notifications to the radio's 100 ms maintenance tick. */
    btLockReadBase();
    for(bth = (struct BtHardware *) BluetoothBase->bt_Hardware.lh_Head;
        bth->bth_Node.ln_Succ; bth = (struct BtHardware *) bth->bth_Node.ln_Succ) {
        if(bth->bth_Task)
            Signal(bth->bth_Task, SIGBREAKF_CTRL_E);
    }
    btUnlockBase();
    return((LONG) len);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btGetServiceValue()" */
/* The current value (a device may have written it: BEHMB_SERVICEWRITE).
   Returns its length, of which at most len bytes were copied, or -1. */
AROS_LH4(LONG, btGetServiceValue,
         AROS_LHA(APTR, record, A0),
         AROS_LHA(ULONG, index, D0),
         AROS_LHA(APTR, buf, A1),
         AROS_LHA(ULONG, len, D1),
         LIBBASETYPEPTR, BluetoothBase, 91, bt)
{
    AROS_LIBFUNC_INIT
    struct BtServiceRecord *bsr = record;
    struct BtGattChar *bgc;
    LONG actual;

    if(!bsr || (bsr->bsr_Protocol != BSVP_ATT) || (index >= bsr->bsr_NumChars)) {
        return(-1);
    }
    bgc = &bsr->bsr_Chars[index];
    ObtainSemaphore(&BluetoothBase->bt_GattLock);
    actual = bgc->bgc_Len;
    if(buf && len && actual) {
        CopyMem(bgc->bgc_Value, buf, min((ULONG) actual, len));
    }
    ReleaseSemaphore(&BluetoothBase->bt_GattLock);
    return(actual);
    AROS_LIBFUNC_EXIT
}
/* \\\ */
