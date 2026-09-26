/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Desc: CLID_HW_Controller - the game controller subsystem singleton
*/

#define DEBUG 0
#include <aros/debug.h>

#include <hidd/hidd.h>
#include <hidd/input.h>
#include <hidd/controller.h>
#include <oop/oop.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "controller_intern.h"

#define SysBase     ((struct ExecBase *)(CSD(cl)->cs_SysBase))
#define UtilityBase (CSD(cl)->cs_UtilityBase)
#define OOPBase     (CSD(cl)->cs_OOPBase)

/*****************************************************************************************

    NAME
        --background_subsystem--

    LOCATION
        CLID_HW_Controller

    NOTES
        The subsystem object is a singleton: OOP_NewObject(NULL, CLID_HW_Controller, NULL)
        always returns the same object and it must not be disposed.

        Hardware drivers register one object per controller with moHW_AddDriver
        (the driver class must derive from CLID_Hidd_Controller) and remove it with
        moHW_RemoveDriver when the device goes away. The subsystem assigns a device
        id that is never reused, works out the standard layout, assigns a
        lowlevel.library joyport according to the slot policy, and publishes
        DeviceAdded/DeviceRemoved events.

        Consumers enumerate devices with moHW_EnumDrivers (the hook receives the
        device objects), moHW_Controller_GetDeviceIDs or by creating a subsystem
        wide CLID_Hidd_Controller consumer, and look devices up with
        moHW_Controller_FindDevice.

*****************************************************************************************/

/*****************************************************************************************

    NAME
        --hardware_drivers--

    LOCATION
        CLID_HW_Controller

    NOTES
        A hardware driver is a subclass of CLID_Hidd_Controller. One object
        represents one controller. In its Root::New the driver passes its identity
        (aHidd_Name, aHidd_HardwareName, aHidd_Controller_VendorID/ProductID/Version,
        aHidd_Controller_Bus, aHidd_Controller_Serial, ...), its control table
        (aHidd_Controller_ControlTable), output table (aHidd_Controller_OutputTable)
        and optionally a binding table for the standard layout
        (aHidd_Controller_BindingTable) to the superclass. Input is delivered by
        calling the Push methods of IID_Hidd_Controller on the object itself, from any
        context. Output is implemented by overriding the IID_Hidd_Controller
        output methods and calling the superclass afterwards.

        Devices of family vHidd_Controller_Family_XInput must expose their raw
        controls in XINPUT_GAMEPAD order (buttons A B X Y LB RB Back Start LS RS
        Guide DpadUp DpadDown DpadLeft DpadRight; axes LX LY RX RY LT RT) to get
        the built-in layout.

*****************************************************************************************/

struct ControllerDevice *ctrl_FindDevice(OOP_Class *cl, struct ControllerHWData *hw, UWORD id)
{
    struct ControllerDevice *dev, *found = NULL;

    if (!id)
        return NULL;
    Disable();
    ForeachNode(&hw->devices, dev)
    {
        if (dev->id == id)
        {
            found = dev;
            break;
        }
    }
    Enable();
    return found;
}

/* Legacy joyport slots: Amiga order 1, 0, 2, 3 */
static const UBYTE ctrl_slot_order[HIDD_CONTROLLER_LEGACY_PORTS] = { 1, 0, 2, 3 };

static void ctrl_SlotSet(OOP_Class *cl, struct ControllerHWData *hw, UBYTE slot, struct ControllerDevice *dev)
{
    struct ControllerDevice *old = NULL;

    Disable();
    if (hw->slot[slot])
        old = ctrl_FindDevice(cl, hw, hw->slot[slot]);
    hw->slot[slot] = dev ? dev->id : 0;
    if (old && old != dev)
        old->legacy_port = -1;
    if (dev)
    {
        UBYTE s;
        for (s = 0; s < HIDD_CONTROLLER_LEGACY_PORTS; s++)
            if (s != slot && hw->slot[s] == dev->id)
                hw->slot[s] = 0;
        dev->legacy_port = slot;
    }
    Enable();

    if (old && old != dev && (old->flags & CTRL_DF_REGISTERED))
        ctrl_SendDeviceEvent(cl, hw, old, vHidd_Controller_PortChanged, -1);
    if (dev && (dev->flags & CTRL_DF_REGISTERED))
        ctrl_SendDeviceEvent(cl, hw, dev, vHidd_Controller_PortChanged, slot);
}

void ctrl_SlotAssignAuto(OOP_Class *cl, struct ControllerHWData *hw, struct ControllerDevice *dev)
{
    UWORD i;

    if (hw->slot_policy != vHidd_Controller_SlotPolicy_Auto)
        return;
    for (i = 0; i < HIDD_CONTROLLER_LEGACY_PORTS; i++)
    {
        UBYTE slot = ctrl_slot_order[i];
        if (hw->slot[slot] == 0 && !(hw->slot_flags[slot] & CTRL_SF_PINNED))
        {
            ctrl_SlotSet(cl, hw, slot, dev);
            return;
        }
    }
}

void ctrl_SlotRelease(OOP_Class *cl, struct ControllerHWData *hw, struct ControllerDevice *dev)
{
    UBYTE s;

    Disable();
    for (s = 0; s < HIDD_CONTROLLER_LEGACY_PORTS; s++)
        if (hw->slot[s] == dev->id)
            hw->slot[s] = 0;
    dev->legacy_port = -1;
    Enable();
}

/*****************************************************************************************
    Root
*****************************************************************************************/

OOP_Object *ControllerHW__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct controller_staticdata *csd = CSD(cl);

    if (!csd->hwObject)
    {
        struct TagItem new_tags[] =
        {
            { aHW_ClassName, (IPTR)"Game controllers" },
            { TAG_DONE,      0                        }
        };
        struct pRoot_New new_msg =
        {
            .mID      = msg->mID,
            .attrList = new_tags
        };

        csd->hwObject = (OOP_Object *)OOP_DoSuperMethod(cl, o, &new_msg.mID);
        if (csd->hwObject)
        {
            struct ControllerHWData *hw = OOP_INST_DATA(cl, csd->hwObject);

            NEWLIST(&hw->devices);
            NEWLIST(&hw->consumers);
            hw->next_id = 1;
            hw->devcount = 0;
            hw->slot_policy = vHidd_Controller_SlotPolicy_Auto;
            ctrl_MappingInitDB(cl, hw);
        }
    }
    return csd->hwObject;
}

VOID ControllerHW__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    /* singleton, never disposed */
}

VOID ControllerHW__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct ControllerHWData *hw = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_HWCONTROLLER_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHW_Controller_DeviceCount:    *msg->storage = hw->devcount; return;
        case aoHW_Controller_ClockFrequency: *msg->storage = CSD(cl)->cs_EClockFreq; return;
        case aoHW_Controller_SlotPolicy:     *msg->storage = hw->slot_policy; return;
        case aoHW_Controller_Version:        *msg->storage = HIDD_CONTROLLER_API_VERSION; return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID ControllerHW__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct ControllerHWData *hw = OOP_INST_DATA(cl, o);
    struct TagItem *tag, *tstate = msg->attrList;

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;
        if (IS_HWCONTROLLER_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
            case aoHW_Controller_SlotPolicy:
                hw->slot_policy = (UBYTE)tag->ti_Data;
                break;
            }
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*****************************************************************************************
    HW
*****************************************************************************************/

OOP_Object *ControllerHW__HW__AddDriver(OOP_Class *cl, OOP_Object *o, struct pHW_AddDriver *msg)
{
    D(bug("[Controller:HW] %s()\n", __func__));
    return (OOP_Object *)OOP_DoSuperMethod(cl, o, &msg->mID);
}

BOOL ControllerHW__HW__RemoveDriver(OOP_Class *cl, OOP_Object *o, struct pHW_RemoveDriver *msg)
{
    D(bug("[Controller:HW] %s: 0x%p\n", __func__, msg->driverObject));
    return (BOOL)OOP_DoSuperMethod(cl, o, &msg->mID);
}

/* Called by the HW base class after the driver object has been created */
BOOL ControllerHW__HW__SetUpDriver(OOP_Class *cl, OOP_Object *o, struct pHW_SetUpDriver *msg)
{
    struct ControllerHWData *hw = OOP_INST_DATA(cl, o);
    struct ControllerDevice *dev = ctrl_Device(cl, msg->driverObject);

    if (!dev)
    {
        D(bug("[Controller:HW] %s: 0x%p is not a controller device\n", __func__, msg->driverObject));
        return FALSE;
    }

    Disable();
    dev->id = hw->next_id++;
    if (hw->next_id == 0)
        hw->next_id = 1;
    dev->rd.device_id = dev->id;
    ADDTAIL(&hw->devices, &dev->node);
    hw->devcount++;
    Enable();

    ctrl_ApplyMapping(cl, dev);
    dev->flags |= CTRL_DF_REGISTERED;

    D(bug("[Controller:HW] %s: device %u '%s' registered\n", __func__, dev->id, dev->hwname ? dev->hwname : (CONST_STRPTR)"?"));

    ctrl_SendDeviceEvent(cl, hw, dev, vHidd_Controller_DeviceAdded, 0);
    ctrl_SlotAssignAuto(cl, hw, dev);

    return TRUE;
}

/* Called by the HW base class before the driver object is disposed */
void ControllerHW__HW__CleanUpDriver(OOP_Class *cl, OOP_Object *o, struct pHW_CleanUpDriver *msg)
{
    struct ControllerHWData *hw = OOP_INST_DATA(cl, o);
    struct ControllerDevice *dev = ctrl_Device(cl, msg->driverObject);

    if (!dev)
        return;

    D(bug("[Controller:HW] %s: device %u\n", __func__, dev->id));

    dev->connected = FALSE;
    Disable();
    dev->rd.flags &= ~vHidd_Controller_RF_Connected;
    Enable();

    if (dev->flags & CTRL_DF_REGISTERED)
    {
        /* subsystem wide consumers first, then the device bound ones (which are unlinked) */
        ctrl_SendDeviceEvent(cl, hw, dev, vHidd_Controller_DeviceRemoved, 0);
        ctrl_DetachDeviceConsumers(cl, dev);
    }
    ctrl_SlotRelease(cl, hw, dev);

    Disable();
    if (dev->flags & CTRL_DF_REGISTERED)
    {
        REMOVE(&dev->node);
        hw->devcount--;
    }
    dev->flags &= ~CTRL_DF_REGISTERED;
    Enable();
}

/*****************************************************************************************
    HW_Input: PushEvent with a struct pHidd_Controller_RawReport
*****************************************************************************************/

void ControllerHW__HW_Input__PushEvent(OOP_Class *cl, OOP_Object *o, struct pHW_Input_PushEvent *msg)
{
    struct ControllerDevice *dev = msg->driver ? ctrl_Device(cl, msg->driver) : NULL;

    if (dev && msg->iedata && (dev->flags & CTRL_DF_REGISTERED))
        ctrl_PushReport(cl, dev, (const struct pHidd_Controller_RawReport *)msg->iedata);
}

/*****************************************************************************************
    HW_Controller
*****************************************************************************************/

OOP_Object *ControllerHW__HW_Controller__FindDevice(OOP_Class *cl, OOP_Object *o, struct pHW_Controller_FindDevice *msg)
{
    struct ControllerHWData *hw = OOP_INST_DATA(cl, o);
    struct ControllerDevice *dev = ctrl_FindDevice(cl, hw, msg->deviceid);

    return dev ? dev->obj : NULL;
}

ULONG ControllerHW__HW_Controller__GetDeviceIDs(OOP_Class *cl, OOP_Object *o, struct pHW_Controller_GetDeviceIDs *msg)
{
    struct ControllerHWData *hw = OOP_INST_DATA(cl, o);
    struct ControllerDevice *dev;
    ULONG n = 0;

    Disable();
    ForeachNode(&hw->devices, dev)
    {
        if (msg->buffer && n < msg->max)
            msg->buffer[n] = dev->id;
        n++;
    }
    Enable();
    return n;
}

/* Re-evaluate mappings of connected devices after a database change */
static void ctrl_RemapDevices(OOP_Class *cl, struct ControllerHWData *hw, const UBYTE *guid)
{
    struct ControllerDevice *dev;
    UWORD ids[64];
    ULONG n = 0, i;

    Disable();
    ForeachNode(&hw->devices, dev)
    {
        if (n < 64)
            ids[n++] = dev->id;
    }
    Enable();

    for (i = 0; i < n; i++)
    {
        dev = ctrl_FindDevice(cl, hw, ids[i]);
        if (!dev)
            continue;
        if (guid)
        {
            /* only devices whose vendor/product match the record */
            if (dev->guid[4] != guid[4] || dev->guid[5] != guid[5] ||
                dev->guid[8] != guid[8] || dev->guid[9] != guid[9])
                continue;
        }
        ctrl_ApplyMapping(cl, dev);
        ctrl_SendDeviceEvent(cl, hw, dev, vHidd_Controller_Remapped, dev->mapsource);
    }
}

BOOL ControllerHW__HW_Controller__AddMapping(OOP_Class *cl, OOP_Object *o, struct pHW_Controller_AddMapping *msg)
{
    struct ControllerHWData *hw = OOP_INST_DATA(cl, o);
    UBYTE source = msg->source;

    if (!msg->mapping)
        return FALSE;
    if (source != vHidd_Controller_MapSrc_API && source != vHidd_Controller_MapSrc_User)
        source = vHidd_Controller_MapSrc_API;
    if (!ctrl_MappingAdd(cl, hw, msg->mapping, source))
        return FALSE;
    ctrl_RemapDevices(cl, hw, msg->mapping->guid);
    return TRUE;
}

BOOL ControllerHW__HW_Controller__RemoveMapping(OOP_Class *cl, OOP_Object *o, struct pHW_Controller_RemoveMapping *msg)
{
    struct ControllerHWData *hw = OOP_INST_DATA(cl, o);

    if (!msg->guid)
        return FALSE;
    if (!ctrl_MappingRemove(cl, hw, (const UBYTE *)msg->guid, msg->source))
        return FALSE;
    ctrl_RemapDevices(cl, hw, (const UBYTE *)msg->guid);
    return TRUE;
}

BOOL ControllerHW__HW_Controller__Configure(OOP_Class *cl, OOP_Object *o, struct pHW_Controller_Configure *msg)
{
    struct ControllerHWData *hw = OOP_INST_DATA(cl, o);
    struct TagItem *tag, *tstate = msg->tags;

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;
        if (IS_HWCONTROLLER_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
            case aoHW_Controller_SlotPolicy:
                hw->slot_policy = (UBYTE)tag->ti_Data;
                break;
            }
        }
    }
    return TRUE;
}

BOOL ControllerHW__HW_Controller__AssignSlot(OOP_Class *cl, OOP_Object *o, struct pHW_Controller_AssignSlot *msg)
{
    struct ControllerHWData *hw = OOP_INST_DATA(cl, o);
    struct ControllerDevice *dev = NULL;

    if (msg->slot >= HIDD_CONTROLLER_LEGACY_PORTS)
        return FALSE;
    if (msg->deviceid)
    {
        dev = ctrl_FindDevice(cl, hw, msg->deviceid);
        if (!dev)
            return FALSE;
    }
    ctrl_SlotSet(cl, hw, msg->slot, dev);
    return TRUE;
}

UWORD ControllerHW__HW_Controller__GetSlotDevice(OOP_Class *cl, OOP_Object *o, struct pHW_Controller_GetSlotDevice *msg)
{
    struct ControllerHWData *hw = OOP_INST_DATA(cl, o);

    if (msg->slot >= HIDD_CONTROLLER_LEGACY_PORTS)
        return 0;
    return hw->slot[msg->slot];
}
