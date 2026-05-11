/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved
*/

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>
#include <proto/oop.h>

#include <aros/atomic.h>
#include <aros/symbolsets.h>
#include <aros/bootloader.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <utility/utility.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>

#include <hidd/pci.h>
#include <hidd/bus.h>
#include <hidd/storage.h>

#include <string.h>

#include "virtio_debug.h"
#include "virtio_intern.h"
#include <hardware/pci.h>

#include LC_LIBDEFS_FILE

#if defined(__OOP_NOATTRBASES__)
static CONST_STRPTR const attrBaseIDs[] = {
    IID_HW,
    IID_Hidd,
    IID_Hidd_PCIDevice,
    IID_Hidd_StorageUnit,
    IID_Hidd_VirtIO,
    IID_Hidd_Bus,
    IID_Hidd_VirtIOBus,
    IID_Hidd_VirtIOUnit,
    NULL
};
#endif

#if defined(__OOP_NOMETHODBASES__)
static CONST_STRPTR const methBaseIDs[] = {
    IID_Hidd_PCIDevice,
    IID_Hidd_PCIDriver,
    IID_HW,
    IID_Hidd_Storage,
    IID_Hidd_StorageController,
    NULL
};
#endif

extern const char GM_UNIQUENAME(LibName)[];
CONST_STRPTR virtioControllerName = "VirtIO Block Controller";

static int VIRTIO_Init(struct VirtIOBase *VirtIOBase)
{
    struct BootLoaderBase *BootLoaderBase;
    BOOL enabled = TRUE;

    D(bug("[VIRTIO--] %s: %s Initialization\n", __func__, GM_UNIQUENAME(LibName));)

    VirtIOBase->virtio_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!VirtIOBase->virtio_UtilityBase)
        return FALSE;

    if ((VirtIOBase->virtio_KernelBase = OpenResource("kernel.resource")) == NULL) {
        CloseLibrary(VirtIOBase->virtio_UtilityBase);
        return FALSE;
    }

    NEWLIST(&VirtIOBase->virtio_Controllers);
    NEWLIST(&VirtIOBase->virtio_Units);
    VirtIOBase->virtio_HostCount = 0;

    BootLoaderBase = OpenResource("bootloader.resource");
    if (BootLoaderBase != NULL) {
        struct List *list;
        struct Node *node;

        list = (struct List *)GetBootInfo(BL_Args);
        if (list) {
            ForeachNode(list, node) {
                if (strncmp(node->ln_Name, "VIRTIO=", 7) == 0) {
                    const char *CmdLine = &node->ln_Name[7];
                    if (strstr(CmdLine, "disable")) {
                        D(bug("[VIRTIO--] %s: Disabling VirtIO support\n", __func__));
                        enabled = FALSE;
                    }
                    if (strstr(CmdLine, "nomsi")) {
                        VirtIOBase->virtio_Flags |= VIRTIOF_FLAG_NOMSI;
                    }
                }
            }
        }
    }

    if (enabled) {
        VirtIOBase->virtio_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED, 8192, 4096);
        if (VirtIOBase->virtio_MemPool == NULL)
            return FALSE;

#if defined(__OOP_NOATTRBASES__)
        if (OOP_ObtainAttrBasesArray(&VirtIOBase->virtio_HWAttrBase, attrBaseIDs)) {
            DeletePool(VirtIOBase->virtio_MemPool);
            VirtIOBase->virtio_MemPool = NULL;
            return FALSE;
        }
#endif
#if defined(__OOP_NOMETHODBASES__)
        if (OOP_ObtainMethodBasesArray(&VirtIOBase->virtio_HiddPCIDeviceMethodBase, methBaseIDs)) {
#if defined(__OOP_NOATTRBASES__)
            OOP_ReleaseAttrBasesArray(&VirtIOBase->virtio_HWAttrBase, attrBaseIDs);
#endif
            DeletePool(VirtIOBase->virtio_MemPool);
            VirtIOBase->virtio_MemPool = NULL;
            return FALSE;
        }
#endif

        VirtIOBase->storageRoot = OOP_NewObject(NULL, CLID_Hidd_Storage, NULL);
        if (!VirtIOBase->storageRoot)
            VirtIOBase->storageRoot = OOP_NewObject(NULL, CLID_HW_Root, NULL);
        if (!VirtIOBase->storageRoot)
            return FALSE;
    }
    return enabled;
}

static int VIRTIO_Open(struct VirtIOBase *VirtIOBase,
                      struct IORequest *iorq, ULONG unitnum, ULONG flags)
{
    struct virtio_Controller *cnode;
    struct Hook searchHook = {
        .h_Entry = Hidd_VirtIOBus_Open,
        .h_Data  = iorq,
    };

    D(bug("[VIRTIO%02d] %s(0x%p, %08x)\n", unitnum, __func__, iorq, flags));

    iorq->io_Error  = IOERR_OPENFAIL;
    iorq->io_Device = &VirtIOBase->virtio_Device;
    iorq->io_Unit   = (APTR)(IPTR)-1;

    ForeachNode(&VirtIOBase->virtio_Controllers, cnode) {
        HIDD_StorageController_EnumBuses(cnode->ac_Object, &searchHook, (APTR)(IPTR)unitnum);
    }

    return iorq->io_Error ? FALSE : TRUE;
}

static int VIRTIO_Close(struct VirtIOBase *VirtIOBase, struct IORequest *iorq)
{
    struct virtio_Unit *unit = (struct virtio_Unit *)iorq->io_Unit;

    iorq->io_Unit = (struct Unit *)~0;

    AROS_ATOMIC_DEC(((struct Unit *)unit)->unit_OpenCnt);

    return TRUE;
}

/*
 * PCI BUS ENUMERATOR
 */

typedef struct {
    struct VirtIOBase *VirtIOBase;
    struct List       devices;
} EnumeratorArgs;

static
AROS_UFH3(void, virtio_PCIEnumerator_h,
          AROS_UFHA(struct Hook *,    hook,    A0),
          AROS_UFHA(OOP_Object *,     Device,  A2),
          AROS_UFHA(APTR,             message, A1))
{
    AROS_USERFUNC_INIT

    device_t dev;
    EnumeratorArgs *a = hook->h_Data;
    struct VirtIOBase *VirtIOBase = a->VirtIOBase;
    CONST_STRPTR owner;
    IPTR vendor = 0, product = 0, sub_ven = 0, sub_dev = 0;

    /* Filter to virtio-blk subtype only. */
    OOP_GetAttr(Device, aHidd_PCIDevice_VendorID,         &vendor);
    OOP_GetAttr(Device, aHidd_PCIDevice_ProductID,        &product);
    OOP_GetAttr(Device, aHidd_PCIDevice_SubsystemVendorID, &sub_ven);
    OOP_GetAttr(Device, aHidd_PCIDevice_SubsystemID,      &sub_dev);

    if ((UWORD)vendor != VIRTIO_PCI_VENDOR_ID)
        return;

    /*
     * Modern: device id 0x1040..0x107F. Subsystem ID indicates the device
     *         class. We want only block (subsys = 2).
     * Legacy: device id 0x1001 is virtio-blk.
     */
    if (((UWORD)product) == VIRTIO_PCI_DEVICEID_BLK_LEGACY) {
        /* legacy virtio-blk */
    } else if (((UWORD)product) >= VIRTIO_PCI_DEVICEID_MIN
            && ((UWORD)product) <= VIRTIO_PCI_DEVICEID_MAX) {
        if ((UWORD)sub_dev != VIRTIO_DEVICE_BLOCK)
            return;
    } else {
        return;
    }

    dev = AllocPooled(VirtIOBase->virtio_MemPool, sizeof(*dev));
    if (dev == NULL)
        return;

    memset(dev, 0, sizeof(*dev));
    dev->dev_VirtIOBase = VirtIOBase;
    dev->dev_Object     = Device;
    dev->dev_HostID     = VirtIOBase->virtio_HostCount;
    dev->dev_VendorID   = (UWORD)vendor;
    dev->dev_DeviceID   = (UWORD)product;
    dev->dev_SubVendorID = (UWORD)sub_ven;
    dev->dev_SubsystemID = (UWORD)sub_dev;

    D(bug("[VIRTIO:PCI] %s: virtio-blk PCI device @ 0x%p (vid %04x did %04x)\n",
          __func__, Device, dev->dev_VendorID, dev->dev_DeviceID));

    owner = HIDD_PCIDevice_Obtain(Device, VirtIOBase->virtio_Device.dd_Library.lib_Node.ln_Name);
    if (owner) {
        D(bug("[VIRTIO:PCI] %s: device already owned by %s\n", __func__, owner));
        FreePooled(VirtIOBase->virtio_MemPool, dev, sizeof(*dev));
        return;
    }

    VirtIOBase->virtio_HostCount++;

    AddTail(&a->devices, (struct Node *)dev);
    return;

    AROS_USERFUNC_EXIT
}

/* Match Red Hat / virtio vendor; subtype filtering is done in the hook. */
static const struct TagItem Requirements[] = {
    {tHidd_PCI_VendorID,    VIRTIO_PCI_VENDOR_ID},
    {TAG_DONE}
};

static int VIRTIO_Probe(struct VirtIOBase *VirtIOBase)
{
    OOP_Object *pci;
    EnumeratorArgs Args;
    device_t dev;
    struct TagItem virtio_tags[] = {
        {aHidd_Name,            (IPTR)GM_UNIQUENAME(LibName)    },
        {aHidd_HardwareName,    (IPTR)virtioControllerName      },
        {aHidd_Producer,        0                               },
#define VIRTIO_TAG_VEND 2
        {aHidd_Product,         0                               },
#define VIRTIO_TAG_PROD 3
        {aHidd_DriverData,      0                               },
#define VIRTIO_TAG_DATA 4
        {TAG_DONE,              0                               }
    };

    D(bug("[VIRTIO:PCI] %s: Enumerating PCI Devices\n", __func__));

    Args.VirtIOBase = VirtIOBase;
    NEWLIST(&Args.devices);

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (pci) {
        struct Hook FindHook = {
            .h_Entry = (IPTR (*)())virtio_PCIEnumerator_h,
            .h_Data  = &Args,
        };
        struct pHidd_PCI_EnumDevices enummsg = {
            .mID            = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
            .callback       = &FindHook,
            .requirements   = Requirements,
        };

        OOP_DoMethod(pci, &enummsg.mID);
        OOP_DisposeObject(pci);
    }

    while ((dev = (device_t)RemHead(&Args.devices)) != NULL) {
        OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_VendorID,  &virtio_tags[VIRTIO_TAG_VEND].ti_Data);
        OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_ProductID, &virtio_tags[VIRTIO_TAG_PROD].ti_Data);
        virtio_tags[VIRTIO_TAG_DATA].ti_Data = (IPTR)dev;
        OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_Driver,    (IPTR *)&dev->dev_PCIDriverObject);

        HW_AddDriver(VirtIOBase->storageRoot, VirtIOBase->virtioClass, virtio_tags);
        D(
            if (dev->dev_Controller) {
                bug("[VIRTIO:PCI] %s: VirtIO Controller Object @ 0x%p\n", __func__, dev->dev_Controller);
            }
        )
    }

    return TRUE;
}

ADD2INITLIB(VIRTIO_Probe, 30)
ADD2INITLIB(VIRTIO_Init, 0)
ADD2OPENDEV(VIRTIO_Open, 0)
ADD2CLOSEDEV(VIRTIO_Close, 0)
