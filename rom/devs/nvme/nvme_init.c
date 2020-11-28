/*
    Copyright © 2020, The AROS Development Team. All rights reserved
    $Id$
 */

#include <aros/debug.h>

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

#include "nvme_intern.h"
#include <hardware/pci.h>

#include LC_LIBDEFS_FILE

#if defined(__OOP_NOATTRBASES__)
/* Keep order the same as order of IDs in struct NVMEBase! */
static CONST_STRPTR const attrBaseIDs[] =
{
    IID_HW,
    IID_Hidd,
    IID_Hidd_PCIDevice,
    IID_Hidd_StorageUnit,
    IID_Hidd_NVME,
    IID_Hidd_Bus,
    IID_Hidd_NVMEBus,
    IID_Hidd_NVMEUnit,
    NULL
};
#endif

#if defined(__OOP_NOMETHODBASES__)
static CONST_STRPTR const methBaseIDs[] =
{
    IID_Hidd_PCIDevice,
    IID_Hidd_PCIDriver,
    IID_HW,
    IID_Hidd_Storage,
    IID_Hidd_StorageController,
    NULL
};
#endif

CONST_STRPTR nvmeDeviceName = "nvme.device";
CONST_STRPTR nvmeControllerName = "Non-Volatile Memory Express Controller";

static int NVME_Init(struct NVMEBase *NVMEBase)
{
    struct BootLoaderBase	*BootLoaderBase;

    D(bug("[NVME--] %s: %s Initialization\n", __func__, nvmeDeviceName);)

    NVMEBase->nvme_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!NVMEBase->nvme_UtilityBase)
        return FALSE;

    if ((NVMEBase->nvme_KernelBase = OpenResource("kernel.resource")) == NULL)
    {
        CloseLibrary(NVMEBase->nvme_UtilityBase);
        return FALSE;
    }

    /* Initialize lists */
    NEWLIST(&NVMEBase->nvme_Controllers);
    NEWLIST(&NVMEBase->nvme_Units);
    NVMEBase->nvme_HostCount=0;

    BootLoaderBase = OpenResource("bootloader.resource");
    D(bug("[NVME--] %s: BootloaderBase = %p\n", __func__, BootLoaderBase));
    if (BootLoaderBase != NULL)
    {
        struct List *list;
        struct Node *node;

        list = (struct List *)GetBootInfo(BL_Args);
        if (list)
        {
            ForeachNode(list, node)
            {
                if (strncmp(node->ln_Name, "NVME=", 4) == 0)
                {
                    const char *CmdLine = &node->ln_Name[4];

                    if (strstr(CmdLine, "disable"))
                    {
                        D(bug("[NVME--] %s: Disabling NVME support\n", __func__));
                        return FALSE;
                    }
                }
            }
        }
    }

    /*
     * Alloc everything needed from a pool, so that we avoid memory fragmentation.
     */
    NVMEBase->nvme_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED , 8192, 4096);
    if (NVMEBase->nvme_MemPool == NULL)
        return FALSE;

    D(bug("[NVME--] %s: MemPool @ %p\n", __func__, NVMEBase->nvme_MemPool);)

#if defined(__OOP_NOATTRBASES__)
    /* Get some useful bases */
    if (OOP_ObtainAttrBasesArray(&NVMEBase->nvme_HWAttrBase, attrBaseIDs))
        return FALSE;
#endif
#if defined(__OOP_NOMETHODBASES__)
    if (OOP_ObtainMethodBasesArray(&NVMEBase->nvme_HiddPCIDeviceMethodBase, methBaseIDs))
    {
#if defined(__OOP_NOATTRBASES__)
        OOP_ReleaseAttrBasesArray(&NVMEBase->nvme_HWAttrBase, attrBaseIDs);
#endif
        return FALSE;
    }
#endif

    D(bug("[NVME--] %s: Base NVME Hidd Class @ %p\n", __func__, NVMEBase->nvmeClass);)
    D(bug("[NVME--] %s: NVME PCI Bus Class @ %p\n", __func__, NVMEBase->busClass);)

    NVMEBase->storageRoot = OOP_NewObject(NULL, CLID_Hidd_Storage, NULL);
    if (!NVMEBase->storageRoot)
        NVMEBase->storageRoot = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    if (!NVMEBase->storageRoot)
    {
        return FALSE;
    }
    D(bug("[NVME--] %s: storage root @ %p\n", __func__, NVMEBase->storageRoot);)

    return TRUE;
}

static int NVME_Open
(
    struct NVMEBase *NVMEBase,
    struct IORequest *iorq,
    ULONG unitnum,
    ULONG flags
)
{
    struct nvme_Controller *nvmeNode;
    struct Hook searchHook =
    {
        .h_Entry = Hidd_NVMEBus_Open,
        .h_Data  = iorq
    };

    /* Assume it failed */
    iorq->io_Error  = IOERR_OPENFAIL;
    iorq->io_Device = &NVMEBase->nvme_Device;
    iorq->io_Unit   = (APTR)(IPTR)-1;

    /* Try to find the unit */
    ForeachNode (&NVMEBase->nvme_Controllers, nvmeNode)
    {
        HIDD_StorageController_EnumBuses(nvmeNode->ac_Object, &searchHook, (APTR)(IPTR)unitnum);
    }
    D(bug("[NVME%02d] Open result: %d\n", unitnum, iorq->io_Error));

    /* If found, io_Error will be reset to zero */
    return iorq->io_Error ? FALSE : TRUE;
}

/* Close given device */
static int NVME_Close
(
    struct NVMEBase *NVMEBase,
    struct IORequest *iorq
)
{
    struct nvme_Unit *unit = (struct nvme_Unit *)iorq->io_Unit;
    
    /* First of all make the important fields of struct IORequest invalid! */
    iorq->io_Unit = (struct Unit *)~0;
    
    /* Decrease use counters of unit */
    AROS_ATOMIC_DEC(((struct Unit *)unit)->unit_OpenCnt);

    return TRUE;
}


/*
 * PCI BUS ENUMERATOR
 *
 * nvme.device unit numbers are as follows:
 *   First NVME device:  unit   0..31
 *   Second device:      units 32..63
 *   etc..
 */

typedef struct 
{
    struct NVMEBase *NVMEBase;
    struct List	    devices;
} EnumeratorArgs;

static
AROS_UFH3(void, nvme_PCIEnumerator_h,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(OOP_Object *,     Device, A2),
    AROS_UFHA(APTR,             message,A1))
{
    AROS_USERFUNC_INIT

    device_t dev;

    EnumeratorArgs *a = hook->h_Data;
    struct NVMEBase *NVMEBase = a->NVMEBase;
    CONST_STRPTR owner;

    dev = AllocPooled(NVMEBase->nvme_MemPool, sizeof(*dev));
    if (dev == NULL)
        return;

    dev->dev_NVMEBase = NVMEBase;
    dev->dev_Object   = Device;
    dev->dev_HostID   = NVMEBase->nvme_HostCount;

    D(bug("[NVME:PCI] %s: %s PCI device @ 0x%p\n", __func__, nvmeControllerName, Device));

    owner = HIDD_PCIDevice_Obtain(Device, NVMEBase->nvme_Device.dd_Library.lib_Node.ln_Name);
    if (owner)
    {
        D(bug("[NVME:PCI] Device is already in use by %s\n", __func__, owner));
        FreePooled(NVMEBase->nvme_MemPool, dev, sizeof(*dev));
        return;
    }        

    NVMEBase->nvme_HostCount++;
	
    AddTail(&a->devices, (struct Node *)dev);

    return;
    AROS_USERFUNC_EXIT
}

static const struct TagItem Requirements[] =
{
    {tHidd_PCI_Class,     PCI_CLASS_MASSSTORAGE},
    {tHidd_PCI_SubClass,  PCI_SUBCLASS_NVM},
    {tHidd_PCI_Interface, 2},
    {TAG_DONE }
};

static int NVME_Probe(struct NVMEBase *NVMEBase)
{
    OOP_Object *pci;
    EnumeratorArgs Args;
    device_t dev;
    struct TagItem nvme_tags[] =
    {
        {aHidd_Name             , (IPTR)nvmeDeviceName          },
        {aHidd_HardwareName     , (IPTR)nvmeControllerName      },
        {aHidd_Producer		, 0                             },
#define NVME_TAG_VEND 2
        {aHidd_Product		, 0                             },
#define NVME_TAG_PROD 3
        {aHidd_DriverData	, 0                             },
#define NVME_TAG_DATA 4
        {TAG_DONE               , 0                             }
    };

    D(bug("[NVME:PCI] %s: Enumerating PCI Devices\n", __func__));

    Args.NVMEBase                 = NVMEBase;
    NEWLIST(&Args.devices);

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

    if (pci)
    {
        struct Hook FindHook =
        {
            h_Entry:    (IPTR (*)())nvme_PCIEnumerator_h,
            h_Data:     &Args
        };

        struct pHidd_PCI_EnumDevices enummsg =
        {
            mID:            OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
            callback:       &FindHook,
            requirements:   Requirements,
        };

        OOP_DoMethod(pci, &enummsg.mID);
        OOP_DisposeObject(pci);
    }

    D(bug("[NVME:PCI] %s: Registering Detected Hosts..\n", __func__));
	
    while ((dev = (device_t)RemHead(&Args.devices)) != NULL) {
        OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_VendorID , &nvme_tags[NVME_TAG_VEND].ti_Data);
        OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_ProductID, &nvme_tags[NVME_TAG_PROD].ti_Data);
        nvme_tags[NVME_TAG_DATA].ti_Data = (IPTR)dev;
        OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_Driver, (IPTR *) &dev->dev_PCIDriverObject);
        HW_AddDriver(NVMEBase->storageRoot, NVMEBase->nvmeClass, nvme_tags);
        D(
            if (dev->dev_Controller)
            {
                bug("[NVME:PCI] %s: NVME Controller Object @ 0x%p\n", __func__, dev->dev_Controller);
            }
        )
    }

    return TRUE;
}

/*
 * nvme.device main code has two init routines with 0 and 127 priorities.
 * All bus scanners must run between them.
 */
ADD2INITLIB(NVME_Probe, 30)

ADD2INITLIB(NVME_Init, 0)
ADD2OPENDEV(NVME_Open, 0)
ADD2CLOSEDEV(NVME_Close, 0)

