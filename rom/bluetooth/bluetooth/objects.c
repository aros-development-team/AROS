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

    if(bFindHardware(BluetoothBase, name, unit)) {
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "Hardware %s/%ld is already in use.", name, unit);
        return(NULL);
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
        bFreeDevice(BluetoothBase, bd);
        btSendEvent(BEHMB_REMDEVICE, bd, NULL);
        bd = (struct BtDevice *) bth->bth_Devices.lh_Head;
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
    }

    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "%s/%ld: %ld registered device(s) restored.",
                   bth->bth_DevName, bth->bth_Unit, count);
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
