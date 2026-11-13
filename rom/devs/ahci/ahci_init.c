/*
    Copyright (C) 2004-2023, The AROS Development Team. All rights reserved

    Desc:
*/

/* Maintainer: Jason S. McMullan <jason.mcmullan@gmail.com>
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

#include "ahci.h"
#include "timer.h"

u_int32_t AhciForceGen = 0;
u_int32_t AhciNoFeatures = 0;
u_int32_t AhciStartDelay = 25;

#include LC_LIBDEFS_FILE

#if defined(__OOP_NOATTRBASES__)
/* Keep order the same as order of IDs in struct AHCIBase! */
static CONST_STRPTR const attrBaseIDs[] =
{
    IID_HW,
    IID_Hidd,
    IID_Hidd_PCIDevice,
    IID_Hidd_StorageUnit,
    IID_Hidd_AHCI,
    IID_Hidd_Bus,
    IID_Hidd_AHCIBus,
    IID_Hidd_AHCIUnit,
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
    NULL
};
#endif

static int AHCI_Init(struct AHCIBase *AHCIBase)
{
    struct BootLoaderBase       *BootLoaderBase;

    AHCIBase->ahci_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!AHCIBase->ahci_UtilityBase)
        return FALSE;
#if defined(AROS_USE_LOGRES)
    AHCIBase->ahci_LogResBase = OpenResource("log.resource");
    if (AHCIBase->ahci_LogResBase)
    {
        APTR LogResBase = AHCIBase->ahci_LogResBase;
        AHCIBase->ahci_LogHandle = logInitialise(&AHCIBase->ahci_Device.dd_Library.lib_Node);
    }
#endif
    ahciDebug("[AHCI--] %s()\n", __func__);

    /* Initialize lists */
    NEWLIST(&AHCIBase->ahci_Controllers);
    NEWLIST(&AHCIBase->ahci_Units);
    AHCIBase->ahci_HostCount=0;

    BootLoaderBase = OpenResource("bootloader.resource");
    ahciDebug("[AHCI--] %s: BootloaderBase = %p\n", __func__, BootLoaderBase);
    if (BootLoaderBase != NULL)
    {
        struct List *list;
        struct Node *node;

        list = (struct List *)GetBootInfo(BL_Args);
        if (list)
        {
            ForeachNode(list, node)
            {
                if (strncmp(node->ln_Name, "AHCI=", 4) == 0)
                {
                    const char *CmdLine = &node->ln_Name[4];

                    if (strstr(CmdLine, "disable"))
                    {
                        ahciWarn("[AHCI--] %s: Disabling AHCI support\n", __func__);
                        CloseLibrary(AHCIBase->ahci_UtilityBase);
                        AHCIBase->ahci_UtilityBase = NULL;
                        return FALSE;
                    }
                    if (strstr(CmdLine, "force150"))
                    {
                        ahciWarn("[AHCI--] %s: Forcing AHCI Gen1\n", __func__);
                        AhciForceGen = 1;
                    }
                    if (strstr(CmdLine, "force300"))
                    {
                        ahciWarn("[AHCI--] %s: Forcing AHCI Gen2\n", __func__);
                        AhciForceGen = 2;
                    }
                    if (strstr(CmdLine, "force600"))
                    {
                        ahciWarn("[AHCI--] %s: Forcing AHCI Gen3\n", __func__);
                        AhciForceGen = 3;
                    }
                    if (strstr(CmdLine, "nofeatures"))
                    {
                        ahciWarn("[AHCI--] %s: Disabling AHCI features\n", __func__);
                        AhciNoFeatures = -1;
                    }
                    if (strstr(CmdLine, "slowerstart"))
                    {
                        ahciWarn("[AHCI--] %s: Using original start delays\n", __func__);
                        AhciStartDelay = 250;
                    }
                }
            }
        }
    }

    /*
     * Alloc everything needed from a pool, so that we avoid memory fragmentation.
     */
    AHCIBase->ahci_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED , 8192, 4096);
    if (AHCIBase->ahci_MemPool != NULL)
    {
        ahciDebug("[AHCI--] %s: MemPool @ %p\n", __func__, AHCIBase->ahci_MemPool);

#if defined(__OOP_NOATTRBASES__)
        /* Get some useful bases */
        if (!OOP_ObtainAttrBasesArray(&AHCIBase->ahci_HWAttrBase, attrBaseIDs))
        {
#endif
#if defined(__OOP_NOMETHODBASES__)
        if (!OOP_ObtainMethodBasesArray(&AHCIBase->ahci_HiddPCIDeviceMethodBase, methBaseIDs))
        {
#endif

        ahciDebug("[AHCI--] %s: Base AHCI Hidd Class @ %p\n", __func__, AHCIBase->ahciClass);
        ahciDebug("[AHCI--] %s: AHCI PCI Bus Class @ %p\n", __func__, AHCIBase->busClass);

        AHCIBase->storageRoot = OOP_NewObject(NULL, CLID_Hidd_Storage, NULL);
        if (!AHCIBase->storageRoot)
            AHCIBase->storageRoot = OOP_NewObject(NULL, CLID_HW_Root, NULL);
        if (AHCIBase->storageRoot)
        {
            ahciDebug("[AHCI--] %s: storage root @ %p\n", __func__, AHCIBase->storageRoot);
            return TRUE;
        }
#if defined(__OOP_NOMETHODBASES__)
        }
#endif
#if defined(__OOP_NOATTRBASES__)
        OOP_ReleaseAttrBasesArray(&AHCIBase->ahci_HWAttrBase, attrBaseIDs);
        }
#endif
        DeletePool(AHCIBase->ahci_MemPool);
        AHCIBase->ahci_MemPool = NULL;
    }
    CloseLibrary(AHCIBase->ahci_UtilityBase);
    AHCIBase->ahci_UtilityBase = NULL;

    return FALSE;
}

static int AHCI_Open
(
    struct AHCIBase *AHCIBase,
    struct IORequest *iorq,
    ULONG unitnum,
    ULONG flags
)
{
    struct cam_sim *tmp, *unit = NULL;

    /*
     * Assume it failed
     */
    iorq->io_Error = IOERR_OPENFAIL;

    ForeachNode(&AHCIBase->ahci_Units, tmp) {
        if (tmp->sim_Unit == unitnum) {
            unit = tmp;
            AROS_ATOMIC_INC(unit->sim_UseCount);
            break;
        }
    }

    if (unit == NULL)
        return FALSE;

    /*
     * set up iorequest
     */
    iorq->io_Device     = &AHCIBase->ahci_Device;
    iorq->io_Unit       = (struct Unit *)unit;
    iorq->io_Error      = 0;

    return TRUE;
}

/* Close given device */
static int AHCI_Close
(
    struct AHCIBase *AHCIBase,
    struct IORequest *iorq
)
{
    struct cam_sim *unit = (struct cam_sim *)iorq->io_Unit;

    /* First of all make the important fields of struct IORequest invalid! */
    iorq->io_Unit = (struct Unit *)~0;
    
    /* Decrease use counters of unit */
    AROS_ATOMIC_DEC(unit->sim_UseCount);

    return TRUE;
}

ADD2INITLIB(AHCI_Init, 0)
ADD2OPENDEV(AHCI_Open, 0)
ADD2CLOSEDEV(AHCI_Close, 0)

