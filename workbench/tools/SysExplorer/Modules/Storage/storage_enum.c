/*
    Copyright (C) 2015-2020, The AROS Development Team.
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

CONST_STRPTR storageenumfunc_name = "StorageEnum.Func";
CONST_STRPTR devicepageclass_name = "DevicePage.Class";
CONST_STRPTR genericwindowclass_name = "GenericWindow.Class";

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

/*****************************************************************************
    NAME
        BOOL EnumBusUnits(

    SYNOPSIS
        OOP_Object *obj, struct MUI_NListtree_TreeNode *parent, APTR hookFunc, APTR hookData)

    FUNCTION
        Enumerate a storage bus through a caller-supplied hook entry.

    INPUTS
        obj - Live OOP object implementing Hidd_StorageBus.EnumUnits.
        parent - Tree node or parent token forwarded unchanged to the hook.
        hookFunc - Non-NULL AROS hook entry accepting struct Hook * in A0,
            the unit OOP_Object * in A2 and parent in A1. Return FALSE to
            continue, TRUE to request termination of enumeration.
        hookData - Caller data made available as hook->h_Data; may be NULL.

    RESULT
        No defined return value. The public declaration specifies BOOL, but
        the implementation returns void. Call only for its callback effects
        and do not inspect the result.

    NOTES
        The call dispatches synchronously to the bus. Keep obj, hookFunc,
        hookData and parent valid until return. The temporary hook must not
        be retained. Only h_Entry and h_Data are initialized; callbacks must
        not read other Hook fields or rely on an h_SubEntry trampoline.

        Unit objects are borrowed from the bus; this wrapper acquires no
        reference, creates no tree nodes and performs no locking. The bus
        implementation controls enumeration order, locking and termination.
        Do not modify bus membership or dispose enumerated units in a hook.

        Use the initialized Storage.Module base in task context. If the hook
        updates the host's MUI tree, call from the host task. The call has no
        timeout; duration includes bus traversal and every hook invocation.

    SEE ALSO
        RegisterStorageBusHandler()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        void QueryATAStorageFeatures(

    SYNOPSIS
        Object *obj, char *devName, int devUnit)

    FUNCTION
        Reserved entry point for adding ATA feature information to a property group.

    INPUTS
        obj - Intended target MUI group; currently unused.
        devName - Intended Exec device name; currently unused.
        devUnit - Intended device unit number; currently unused.

    RESULT
        None.

    NOTES
        This implementation performs no operation. It does not open a device,
        query SMART or TRIM support, inspect the inputs or modify the group.
        Pointer arguments may be NULL; no storage is retained.

        Absence of added feature information does not mean that the device
        lacks the feature. Use the initialized Storage.Module base.

*****************************************************************************/
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

/*****************************************************************************
    NAME
        BOOL RegisterStorageClassHandler(

    SYNOPSIS
        CONST_STRPTR classid, BYTE pri, struct MUI_CustomClass *customwinclass, CLASS_ENUMFUNC enumfunc, CLASS_VALIDFUNC validfunc)

    FUNCTION
        Register a SysExplorer storage handler for an OOP class.

    INPUTS
        classid - Persistent, non-NULL, NUL-terminated class identifier.
        pri - Signed priority for a newly inserted handler; higher values first.
        customwinclass - Borrowed MUI property-window class, or NULL.
        enumfunc - CLASS_ENUMFUNC child enumerator, or NULL.
        validfunc - CLASS_VALIDFUNC object filter, or NULL to accept all objects.

    RESULT
        TRUE if the storage entry was inserted or updated; FALSE on allocation
        failure or if an existing entry's enumerator differs from HWEnum.Func.
        TRUE does not confirm registration in the host's handler list.

    NOTES
        Use the initialized Storage.Module base obtained through GetBase().
        The identifier, custom class and callbacks are retained without copying.
        Keep them valid while the host uses the handler. Register during module
        startup and serialize all registry access; no lock is taken.

        New entries are ordered by priority, with equal priorities in insertion
        order. A new entry also attempts host registration with the same class,
        priority and callbacks but no property-window class.

        An existing entry is replaceable only when its enumerator equals the
        pointer returned by GetBase("HWEnum.Func"). Unlike RegisterClassHandler(),
        a NULL enumerator normally prevents replacement. An update overwrites
        all fields but neither reorders the list nor updates the host entry.

        The storage enumerator calls validfunc(obj, flags) before inserting an
        object. FALSE omits the object and descendants. flags initially contains
        TNF_LIST|TNF_OPEN when enumfunc is present, otherwise zero. For an accepted
        object, enumfunc(obj, parent) receives the new tree node if TNF_LIST is
        set. Arguments are borrowed; do not retain flags or dispose the object
        or parent. The storage enumeration path does not consult SkipClass().

        No callback runs during registration, and no existing tree node or
        window is refreshed. There is no unregister operation.

    BUGS
        Failure of the accompanying host registration is ignored. Updating
        priority does not restore sorted order, so the stored priority may
        not describe the actual handler-selection order.

    SEE ALSO
        RegisterStorageControllerHandler(), RegisterStorageBusHandler(), FindObjectHandler()

*****************************************************************************/
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


/*****************************************************************************
    NAME
        BOOL RegisterStorageControllerHandler(

    SYNOPSIS
        CONST_STRPTR classid, BYTE pri, struct MUI_CustomClass *customwinclass, CLASS_ENUMFUNC enumfunc, CLASS_VALIDFUNC validfunc)

    FUNCTION
        Register a SysExplorer storage-controller handler with a default enumerator.

    INPUTS
        classid - Persistent, non-NULL, NUL-terminated controller class identifier.
        pri - Signed handler priority; higher values are considered first.
        customwinclass - Borrowed MUI property-window class, or NULL.
        enumfunc - Custom CLASS_ENUMFUNC callback, or NULL to use the default
            controller enumerator.
        validfunc - CLASS_VALIDFUNC object filter, or NULL to accept all objects.

    RESULT
        Result of RegisterStorageClassHandler(). TRUE confirms the storage
        entry only; it does not confirm host registration or enumeration.

    NOTES
        With a NULL enumfunc, the registered callback calls
        HIDD_StorageController_EnumBuses() when the tree is enumerated.
        Buses are processed through storage class-handler lookup and filtering.
        The controller holds its bus-list read lock during callbacks; do not
        add or remove buses from those callbacks.

        Registration itself does not enumerate devices. Use the initialized
        Storage.Module base during host setup. Input lifetimes, serialization,
        replacement restrictions and priority-update limitations are those of
        RegisterStorageClassHandler().

    SEE ALSO
        RegisterStorageClassHandler(), EnumBusUnits()

*****************************************************************************/
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

/*****************************************************************************
    NAME
        BOOL RegisterStorageBusHandler(

    SYNOPSIS
        CONST_STRPTR classid, BYTE pri, struct MUI_CustomClass *customwinclass, CLASS_ENUMFUNC enumfunc, CLASS_VALIDFUNC validfunc)

    FUNCTION
        Register a SysExplorer storage-bus handler with a default enumerator.

    INPUTS
        classid - Persistent, non-NULL, NUL-terminated bus class identifier.
        pri - Signed handler priority; higher values are considered first.
        customwinclass - Borrowed MUI property-window class, or NULL.
        enumfunc - Custom CLASS_ENUMFUNC callback, or NULL to use the default
            bus enumerator.
        validfunc - CLASS_VALIDFUNC object filter, or NULL to accept all objects.

    RESULT
        Result of RegisterStorageClassHandler(). TRUE confirms the storage
        entry only; it does not confirm host registration or enumeration.

    NOTES
        With a NULL enumfunc, the registered callback calls
        HIDD_StorageBus_EnumUnits() when the tree is enumerated.
        Units are inserted with the standard StorageUnitWindow class. This
        default unit callback does not perform per-unit handler lookup or
        invoke per-unit validation callbacks.

        Registration itself does not enumerate devices. Use the initialized
        Storage.Module base during host setup. Input lifetimes, serialization,
        replacement restrictions and priority-update limitations are those of
        RegisterStorageClassHandler().

    SEE ALSO
        RegisterStorageClassHandler(), EnumBusUnits()

*****************************************************************************/
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

    StorageBase->sesb_GenericWindowCLASS = GetBase(genericwindowclass_name);
    StorageBase->sesb_DevicePageCLASS = GetBase(devicepageclass_name);
    StorageBusWindow_CLASS->mcc_Class->cl_UserData = (IPTR)StorageBase;

    RegisterBase(storageenumfunc_name, storageHWEnum);
    RegisterBase(storagebuswindowclass_name, StorageBusWindow_CLASS);
    RegisterBase(storageunitwindowclass_name, StorageUnitWindow_CLASS);

    RegisterClassHandler(CLID_Hidd_Storage, 90, NULL, storageHWEnum, NULL);
    RegisterStorageControllerHandler(CLID_Hidd_StorageController, 90, StorageBase->sesb_GenericWindowCLASS, NULL, NULL);
    RegisterStorageBusHandler(CLID_Hidd_StorageBus, 0, StorageBusWindow_CLASS, NULL, NULL);
}
