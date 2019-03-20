/*
    Copyright (C) 2015-2019, The AROS Development Team.
    $Id$
*/

#include <aros/debug.h>

#define __STORAGE_NOLIBBASE__

#include <proto/sysexp.h>
#include <proto/storage.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <mui/NListtree_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <devices/newstyle.h>
#include <devices/ata.h>

#include "locale.h"
#include "enums.h"

#include "storage_classes.h"
#include "storage_intern.h"

#if (1) // TODO : Move into libbase
extern OOP_AttrBase HiddAttrBase;
extern OOP_AttrBase HWAttrBase;
extern OOP_AttrBase HiddStorageUnitAB;

extern OOP_MethodID HWBase;
extern OOP_MethodID HiddStorageControllerBase;
extern OOP_MethodID HiddStorageBusBase;
extern OOP_MethodID HiddStorageUnitBase;
#endif

CONST_STRPTR storagebuswindowclass_name = "StorageBusWindow.Class";
CONST_STRPTR storageunitwindowclass_name = "StorageUnitWindow.Class";

AROS_UFH3S(BOOL, storageenumFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, obj, A2),
    AROS_UFHA(void *, parent, A1))
{
    AROS_USERFUNC_INIT

    BOOL objValid = TRUE;
    CONST_STRPTR name = NULL;
    struct MUI_NListtree_TreeNode *tn;
    ULONG flags = 0;
    struct SysexpEnum_data *edata = (struct SysexpEnum_data *)h->h_Data;
    struct InsertObjectMsg msg =
    {
        .obj = obj,
        .winClass = NULL
    };
    struct ClassHandlerNode *clHandlers;
    struct SysexpBase *SysexpBase;

    D(bug("[storage.sysexp] %s()\n", __func__));

    if (!edata)
        return TRUE;

    SysexpBase = edata->ed_sysexpbase;
    clHandlers = FindObjectHandler(obj, edata->ed_list);

    D(bug("[storage.sysexp] %s: list @ 0x%p\n", __func__, edata->ed_list));
    D(bug("[storage.sysexp] %s: handler @ 0x%p\n", __func__, clHandlers));

    if (clHandlers)
    {
        if (clHandlers->muiClass)
            msg.winClass = clHandlers->muiClass;
        if (clHandlers->enumFunc)
            flags = TNF_LIST|TNF_OPEN;
        if (clHandlers->validFunc)
            objValid = clHandlers->validFunc(obj, &flags);
    }

    if (objValid)
    {
        int objnum;

        /* This is either HW or HIDD subclass */
        OOP_GetAttr(obj, aHW_ClassName, (IPTR *)&name);
        if (!name)
            OOP_GetAttr(obj, aHidd_HardwareName, (IPTR *)&name);

        D(bug("[storage.sysexp] %s: name = '%s'\n", __func__, name));

        objnum = ++SysexpBase->GlobalCount;

        tn = (APTR)DoMethod(SysexpBase->sesb_Tree, MUIM_NListtree_Insert, name, &msg,
                            parent, MUIV_NListtree_Insert_PrevNode_Tail, flags);
        D(bug("[storage.sysexp] %s: Inserted TreeNode 0x%p <%s> UserData 0x%p\n", __func__, tn, tn->tn_Name, tn->tn_User));

        /* If we have enumerator for this class, call it now */
        if (clHandlers && clHandlers->enumFunc && (flags & TNF_LIST))
            clHandlers->enumFunc(obj, tn);
        
        if (objnum == SysexpBase->GlobalCount)
        {
           tn->tn_Flags &= ~flags;
        }
    }
    return FALSE; /* Continue enumeration */

    AROS_USERFUNC_EXIT
}


AROS_UFH3S(BOOL, storageunitenumFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, unit, A2),
    AROS_UFHA(void *, parent, A1))
{
    AROS_USERFUNC_INIT

    struct SysexpEnum_data *edata = (struct SysexpEnum_data *)h->h_Data;
    struct InsertObjectMsg msg =
    {
        .obj            = unit,
        .winClass       = StorageUnitWindow_CLASS
    };
    CONST_STRPTR name;
    struct SysexpBase *SysexpBase;

    D(bug("[storage.sysexp] %s()\n", __func__));

    if (!edata)
        return TRUE;

    SysexpBase = edata->ed_sysexpbase;

    D(bug("[storage.sysexp] %s: StorageUnit @ %p\n", __func__, unit));
    if (!unit)
        return TRUE;

    SysexpBase->GlobalCount++;

    OOP_GetAttr(unit, aHidd_StorageUnit_Model, (IPTR *)&name);
    DoMethod(SysexpBase->sesb_Tree, MUIM_NListtree_Insert, name, &msg,
             parent, MUIV_NListtree_Insert_PrevNode_Tail, 0);

    return FALSE; /* Continue enumeration */

    AROS_USERFUNC_EXIT
}


static struct SysexpEnum_data privatehookdata =
{
    NULL,
    NULL
};

struct Hook storageenum_hook =
{
    .h_Entry = storageenumFunc,
    .h_Data = &privatehookdata
};

struct Hook storageunitenum_hook =
{
    .h_Entry = storageunitenumFunc,
    .h_Data = &privatehookdata
};

static void storageHWEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *tn)
{
    D(bug("[storage.sysexp] HWEnum: enumerating..\n"));
    D(bug("[storage.sysexp] HWEnum: storage class list @ 0x%p\n", storageenum_hook.h_Data));
    HW_EnumDrivers(obj, &storageenum_hook, tn);
}

static void storageControllerEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *tn)
{
    D(bug("[storage.sysexp] ControllerEnum: enumerating..\n"));
    D(bug("[storage.sysexp] ControllerEnum: storage class list @ 0x%p\n", storageenum_hook.h_Data));
    HIDD_StorageController_EnumBuses(obj, &storageenum_hook, tn);
}

void storageBusEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *parent)
{
    D(bug("[storage.sysexp] BusEnum: enumerating..\n"));
    D(bug("[storage.sysexp] BusEnum: storage class list @ 0x%p\n", storageenum_hook.h_Data));
    HIDD_StorageBus_EnumUnits(obj, &storageunitenum_hook, parent);
}

AROS_LH4(void, EnumBusUnits,
         AROS_LHA(OOP_Object *, obj, A0),
         AROS_LHA(struct MUI_NListtree_TreeNode *, parent, A1),
         AROS_LHA(APTR, hookFunc, A2),
         AROS_LHA(APTR, hookData, A3),
         struct SysexpStorageBase *, StorageBase, 15, Storage)
{
    AROS_LIBFUNC_INIT

    struct Hook busunitenum_hook;
    D(bug("[storage.sysexp] %s: StorageBus @ %p\n", __func__, obj));

    busunitenum_hook.h_Entry = hookFunc;
    busunitenum_hook.h_Data = hookData;

    HIDD_StorageBus_EnumUnits(obj, &busunitenum_hook, parent);

    AROS_LIBFUNC_EXIT
}

/*
    Query a units device to see if it supports ATA device features, and if they are enebaled
    for the unit.
    Add objects to the passed in colgroup(2) if supported features are found.
*/
AROS_LH3(void, QueryATAStorageFeatures,
         AROS_LHA(Object *, obj, A0),
         AROS_LHA(char *, devName, A1),
         AROS_LHA(int, devUnit, D0),
         struct SysexpStorageBase *, StorageBase, 16, Storage)
{
    AROS_LIBFUNC_INIT
#if (0)
    struct IOStdReq *io;
    struct MsgPort * ioReplyPort;
    struct NSDeviceQueryResult nsdqr;
    LONG error;

    ioReplyPort = CreateMsgPort();
    if (!ioReplyPort)
        return;

    io = CreateIORequest(ioReplyPort, sizeof(struct IOStdReq));
    if (!io)
    {
        DeleteMsgPort(ioReplyPort);
        return;
    }

    if (!OpenDevice(devName, devUnit,(struct IORequest *)io,0))
    {
        io->io_Command = NSCMD_DEVICEQUERY;
        io->io_Length  = sizeof(nsdqr);
        io->io_Data    = (APTR)&nsdqr;

        error = DoIO((struct IORequest *)io);

        if((!error) &&
           (io->io_Actual >= 16) &&
           (io->io_Actual <= sizeof(nsdqr)) &&
           (nsdqr.SizeAvailable == io->io_Actual))
        {
            UWORD *cmdcheck;
            for(cmdcheck = nsdqr.SupportedCommands;
                *cmdcheck;
                cmdcheck++)
            {
                ULONG queryres = 0;

                // Does the device understand the SMART Cmd?
                if(*cmdcheck == HD_SMARTCMD)
                {
                    // Check if the unit Supports SMART
                    io->io_Command = HD_SMARTCMD;
                    io->io_Reserved1 = io->io_Reserved2 = ATAFEATURE_TEST_AVAIL;
                    io->io_Length  = sizeof(queryres);
                    io->io_Data    = (APTR)&queryres;
                    error = DoIO((struct IORequest *)io);
                    if ((!error) && (io->io_Actual >= 4) && (queryres == SMART_MAGIC_ID))
                    {
                        IPTR smartSpacer = (IPTR)HVSpace;
                        IPTR smartLabel = (IPTR)Label("SMART Supported");
                        if (DoMethod(obj, MUIM_Group_InitChange))
                        {
                            DoMethod(obj, OM_ADDMEMBER, smartSpacer);
                            DoMethod(obj, OM_ADDMEMBER, smartLabel);
                            DoMethod(obj, MUIM_Group_ExitChange);
                        }
                    }
                }

                // Does the device understand the TRIM Cmd?
                if(*cmdcheck == HD_TRIMCMD)
                {
                    // Check if the unit Supports TRIM
                    io->io_Command = HD_TRIMCMD;
                    io->io_Reserved1 = io->io_Reserved2 = ATAFEATURE_TEST_AVAIL;
                    io->io_Length  = sizeof(queryres);
                    io->io_Data    = (APTR)&queryres;
                    error = DoIO((struct IORequest *)io);
                    if ((!error) && (io->io_Actual >= 4) && (queryres == TRIM_MAGIC_ID))
                    {
                        IPTR trimSpacer = (IPTR)HVSpace;
                        IPTR trimLabel = (IPTR)Label("TRIM Supported");
                        if (DoMethod(obj, MUIM_Group_InitChange))
                        {
                            DoMethod(obj, OM_ADDMEMBER, trimSpacer);
                            DoMethod(obj, OM_ADDMEMBER, trimLabel);
                            DoMethod(obj, MUIM_Group_ExitChange);
                        }
                    }
                }
            }
        }
        CloseDevice((struct IORequest *)io);
    }
    DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(ioReplyPort);
#endif
    AROS_LIBFUNC_EXIT
}

AROS_LH5(BOOL, RegisterStorageClassHandler,
         AROS_LHA(CONST_STRPTR, classid, A0),
         AROS_LHA(BYTE, pri, D0),
         AROS_LHA(struct MUI_CustomClass *, customwinclass, A1),
         AROS_LHA(CLASS_ENUMFUNC, enumfunc, A2),
         AROS_LHA(CLASS_VALIDFUNC, validfunc, A3),
         struct SysexpStorageBase *, StorageBase, 9, Storage)
{
    AROS_LIBFUNC_INIT

    struct SysexpBase *SysexpBase = StorageBase->sesb_SysexpBase;
    struct ClassHandlerNode *newClass;
    BOOL add = TRUE;

    if ((newClass = FindClassHandler(classid, &StorageBase->sesb_HandlerList)))
    {
        if (newClass->enumFunc != GetBase("HWEnum.Func"))
            return FALSE;

        D(bug("[storage.sysexp] %s: Updating '%s'..\n", __func__, classid));
        add = FALSE;
    }

    if (add)
    {
        D(bug("[storage.sysexp] %s: Registering '%s'..\n", __func__, classid));
        newClass = AllocMem(sizeof(struct ClassHandlerNode), MEMF_CLEAR);
    }

    if (newClass)
    {
        newClass->ch_Node.ln_Name = (char *)classid;
        newClass->ch_Node.ln_Pri = pri;
        newClass->muiClass = customwinclass;
	newClass->enumFunc = enumfunc;
        newClass->validFunc = validfunc;

        if (add)
        {
            RegisterClassHandler(classid, pri, NULL, newClass->enumFunc, newClass->validFunc);
            Enqueue(&StorageBase->sesb_HandlerList, &newClass->ch_Node);
        }

        return TRUE;
    }
    return FALSE;

    AROS_LIBFUNC_EXIT
}


AROS_LH5(BOOL, RegisterStorageControllerHandler,
         AROS_LHA(CONST_STRPTR, classid, A0),
         AROS_LHA(BYTE, pri, D0),
         AROS_LHA(struct MUI_CustomClass *, customwinclass, A1),
         AROS_LHA(CLASS_ENUMFUNC, enumfunc, A2),
         AROS_LHA(CLASS_VALIDFUNC, validfunc, A3),
         struct SysexpStorageBase *, StorageBase, 10, Storage)
{
    AROS_LIBFUNC_INIT

    CLASS_ENUMFUNC enumFunc;

    if (enumfunc)
	enumFunc = enumfunc;
    else
	enumFunc = storageControllerEnum;

    return RegisterStorageClassHandler(classid, pri, customwinclass, enumFunc, validfunc);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(BOOL, RegisterStorageBusHandler,
         AROS_LHA(CONST_STRPTR, classid, A0),
         AROS_LHA(BYTE, pri, D0),
         AROS_LHA(struct MUI_CustomClass *, customwinclass, A1),
         AROS_LHA(CLASS_ENUMFUNC, enumfunc, A2),
         AROS_LHA(CLASS_VALIDFUNC, validfunc, A3),
         struct SysexpStorageBase *, StorageBase, 11, Storage)
{
    AROS_LIBFUNC_INIT

     CLASS_ENUMFUNC enumFunc;

    if (enumfunc)
	enumFunc = enumfunc;
    else
	enumFunc = storageBusEnum;

    return RegisterStorageClassHandler(classid, pri, customwinclass, enumFunc, validfunc);

    AROS_LIBFUNC_EXIT
}

void StorageStartup(struct SysexpBase *SysexpBase)
{
    struct SysexpStorageBase *StorageBase;

    D(bug("[storage.sysexp] %s(%p)\n", __func__, SysexpBase));

    StorageBase = GetBase("Storage.Module");

    D(bug("[storage.sysexp] %s: StorageBase @ %p\n", __func__, StorageBase));

    privatehookdata.ed_sysexpbase = SysexpBase;
    privatehookdata.ed_list = &StorageBase->sesb_HandlerList;

    StorageBase->sesb_DevicePageCLASS = GetBase("DevicePage.Class");
    StorageBusWindow_CLASS->mcc_Class->cl_UserData = (IPTR)StorageBase;

    RegisterBase(storagebuswindowclass_name, StorageBusWindow_CLASS);
    RegisterBase(storageunitwindowclass_name, StorageUnitWindow_CLASS);

    RegisterClassHandler(CLID_Hidd_Storage, 90, NULL, storageHWEnum, NULL);
    RegisterStorageBusHandler(CLID_Hidd_StorageBus, 0, StorageBusWindow_CLASS, NULL, NULL);
}
