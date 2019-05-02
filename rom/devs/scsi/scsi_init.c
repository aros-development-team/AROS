/*
    Copyright © 2004-2019, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>
#include <proto/oop.h>

#include <aros/bootloader.h>
#include <aros/symbolsets.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <hidd/hidd.h>
#include <hidd/bus.h>
#include <hidd/storage.h>
#include <utility/utility.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <string.h>

#include "scsi.h"
#include "timer.h"

#include LC_LIBDEFS_FILE

/* Add a bootnode using expansion.library */
BOOL scsi_RegisterVolume(ULONG StartCyl, ULONG EndCyl, struct scsi_Unit *unit)
{
    struct ExpansionBase *ExpansionBase;
    struct DeviceNode *devnode;
    TEXT dosdevname[4] = "HD0";
    const ULONG IdDOS = AROS_MAKE_ID('D','O','S','\001');
    const ULONG IdCDVD = AROS_MAKE_ID('C','D','V','D');

    ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library",
                                                        40L);

    if (ExpansionBase)
    {
        IPTR pp[24];

        /* This should be dealt with using some sort of volume manager or such. */
        switch (unit->su_DevType)
        {
            case DG_DIRECT_ACCESS:
                break;
            case DG_CDROM:
                dosdevname[0] = 'C';
                break;
            default:
                D(bug("[SCSI>>]:-scsi_RegisterVolume called on unknown devicetype\n"));
        }

        if (unit->su_UnitNum < 10)
            dosdevname[2] += unit->su_UnitNum % 10;
        else
            dosdevname[2] = 'A' - 10 + unit->su_UnitNum;
    
        pp[0] 		    = (IPTR)dosdevname;
        pp[1]		    = (IPTR)MOD_NAME_STRING;
        pp[2]		    = unit->su_UnitNum;
        pp[DE_TABLESIZE    + 4] = DE_BOOTBLOCKS;
        pp[DE_SIZEBLOCK    + 4] = 1 << (unit->su_SectorShift - 2);
        pp[DE_NUMHEADS     + 4] = unit->su_Heads;
        pp[DE_SECSPERBLOCK + 4] = 1;
        pp[DE_BLKSPERTRACK + 4] = unit->su_Sectors;
        pp[DE_RESERVEDBLKS + 4] = 2;
        pp[DE_LOWCYL       + 4] = StartCyl;
        pp[DE_HIGHCYL      + 4] = EndCyl;
        pp[DE_NUMBUFFERS   + 4] = 10;
        pp[DE_BUFMEMTYPE   + 4] = MEMF_PUBLIC | MEMF_31BIT;
        pp[DE_MAXTRANSFER  + 4] = 0x00200000;
        pp[DE_MASK         + 4] = 0x7FFFFFFE;
        pp[DE_BOOTPRI      + 4] = ((unit->su_DevType == DG_DIRECT_ACCESS) ? 0 : 10);
        pp[DE_DOSTYPE      + 4] = ((unit->su_DevType == DG_DIRECT_ACCESS) ? IdDOS : IdCDVD);
        pp[DE_CONTROL      + 4] = 0;
        pp[DE_BOOTBLOCKS   + 4] = 2;
    
        devnode = MakeDosNode(pp);

        if (devnode)
        {
            D(bug("[SCSI>>]:-scsi_RegisterVolume: '%b', type=0x%08lx with StartCyl=%d, EndCyl=%d .. ",
                  devnode->dn_Name, pp[DE_DOSTYPE + 4], StartCyl, EndCyl));

            AddBootNode(pp[DE_BOOTPRI + 4], ADNF_STARTPROC, devnode, NULL);
            D(bug("done\n"));
            
            return TRUE;
        }

        CloseLibrary((struct Library *)ExpansionBase);
    }

    return FALSE;
}

#if defined(__OOP_NOATTRBASES__)
/* Keep order the same as order of IDs in struct scsiBase! */
static CONST_STRPTR const attrBaseIDs[] =
{
    IID_Hidd_SCSIUnit,
    IID_HW,
    IID_Hidd_Bus,
    IID_Hidd_SCSIBus,
    IID_Hidd_StorageUnit,
    NULL
};
#endif

#if defined(__OOP_NOMETHODBASES__)
static CONST_STRPTR const methBaseIDs[] =
{
    IID_HW,
    IID_Hidd_SCSIBus,
    IID_Hidd_StorageController,
    NULL
};
#endif

static int SCSI_init(struct scsiBase *SCSIBase)
{
    struct BootLoaderBase	*BootLoaderBase;

    D(bug("[SCSI--] %s: scsi.device Initialization\n", __PRETTY_FUNCTION__));

    /* Prepare the list of detected controllers */
    NEWLIST(&SCSIBase->scsi_Controllers);

    /* Set default scsi.device config options */
    SCSIBase->scsi_32bit   = FALSE;
    SCSIBase->scsi_NoMulti = FALSE;
    SCSIBase->scsi_NoDMA   = FALSE;
    SCSIBase->scsi_Poll    = FALSE;

    /*
     * start initialization: 
     * obtain kernel parameters
     */
    BootLoaderBase = OpenResource("bootloader.resource");
    D(bug("[SCSI--] %s: BootloaderBase = %p\n", __PRETTY_FUNCTION__, BootLoaderBase));
    if (BootLoaderBase != NULL)
    {
        struct List *list;
        struct Node *node;

        list = (struct List *)GetBootInfo(BL_Args);
        if (list)
        {
            ForeachNode(list, node)
            {
                if (strncmp(node->ln_Name, "SCSI=", 4) == 0)
                {
                    const char *CmdLine = &node->ln_Name[4];

                    if (strstr(CmdLine, "disable"))
                    {
                        D(bug("[SCSI  ] %s: Disabling SCSI support\n", __PRETTY_FUNCTION__));
                        return FALSE;
                    }
                    if (strstr(CmdLine, "32bit"))
                    {
                        D(bug("[SCSI  ] %s: Using 32-bit IO transfers\n", __PRETTY_FUNCTION__));
                        SCSIBase->scsi_32bit = TRUE;
                    }
                    if (strstr(CmdLine, "nomulti"))
                    {
                        D(bug("[SCSI  ] %s: Disabled multisector transfers\n", __PRETTY_FUNCTION__));
                        SCSIBase->scsi_NoMulti = TRUE;
                    }
                    if (strstr(CmdLine, "nodma"))
                    {
                        D(bug("[SCSI  ] %s: Disabled DMA transfers\n", __PRETTY_FUNCTION__));
                        SCSIBase->scsi_NoDMA = TRUE;
                    }
                    if (strstr(CmdLine, "poll"))
                    {
                        D(bug("[SCSI  ] %s: Using polling to detect end of busy state\n", __PRETTY_FUNCTION__));
                        SCSIBase->scsi_Poll = TRUE;
                    }
                }
            }
        }
    }

    SCSIBase->scsi_UtilityBase = OpenLibrary("utility.library", 36);
    if (!SCSIBase->scsi_UtilityBase)
    {
        bug("[SCSI--] %s: Failed to open utility.library v36\n", __PRETTY_FUNCTION__);
        return FALSE;
    }
    /*
     * I've decided to use memory pools again. Alloc everything needed from 
     * a pool, so that we avoid memory fragmentation.
     */
    SCSIBase->scsi_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED , 8192, 4096);
    if (SCSIBase->scsi_MemPool == NULL)
    {
        bug("[SCSI--] %s: Failed to Allocate MemPool!\n", __PRETTY_FUNCTION__);
        return FALSE;
    }

    D(bug("[SCSI--] %s: MemPool @ %p\n", __PRETTY_FUNCTION__, SCSIBase->scsi_MemPool));

#if defined(__OOP_NOATTRBASES__)
    if (OOP_ObtainAttrBasesArray(&SCSIBase->unitAttrBase, attrBaseIDs))
    {
        bug("[SCSI--] %s: Failed to obtain AttrBases!\n", __PRETTY_FUNCTION__);
        return FALSE;
    }
    D(
      bug("[SCSI--] %s: HiddBusAB %x @ 0x%p\n", __func__, HiddBusAB, &HiddBusAB);
      bug("[SCSI--] %s: HiddSCSIBusAB %x @ 0x%p\n", __func__, HiddSCSIBusAB, &HiddSCSIBusAB);
    )
#endif

#if defined(__OOP_NOMETHODBASES__)
    if (OOP_ObtainMethodBasesArray(&SCSIBase->hwMethodBase, methBaseIDs))
    {
        bug("[SCSI--] %s: Failed to obtain MethodBases!\n", __PRETTY_FUNCTION__);
        bug("[SCSI--] %s:     %s = %p\n", __PRETTY_FUNCTION__, methBaseIDs[0], SCSIBase->hwMethodBase);
        bug("[SCSI--] %s:     %s = %p\n", __PRETTY_FUNCTION__, methBaseIDs[1], SCSIBase->busMethodBase);
        bug("[SCSI--] %s:     %s = %p\n", __PRETTY_FUNCTION__, methBaseIDs[2], SCSIBase->HiddSCMethodBase);
#if defined(__OOP_NOATTRBASES__)
         OOP_ReleaseAttrBasesArray(&SCSIBase->unitAttrBase, attrBaseIDs);
#endif
        return FALSE;
    }
#endif

    D(bug("[SCSI  ] %s: Base SCSI Hidd Class @ 0x%p\n", __PRETTY_FUNCTION__, SCSIBase->scsiClass));

    /* Try to setup daemon task looking for diskchanges */
    NEWLIST(&SCSIBase->Daemon_ios);
    InitSemaphore(&SCSIBase->DaemonSem);
    InitSemaphore(&SCSIBase->DetectionSem);
    SCSIBase->daemonParent = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);

    if (!NewCreateTask(TASKTAG_PC, DaemonCode,
                       TASKTAG_NAME       , "SCSI.daemon",
                       TASKTAG_STACKSIZE  , STACK_SIZE,
                       TASKTAG_TASKMSGPORT, &SCSIBase->DaemonPort,
                       TASKTAG_PRI        , TASK_PRI - 1,	/* The daemon should have a little bit lower Pri than handler tasks */
                       TASKTAG_ARG1       , SCSIBase,
                       TAG_DONE))
    {
        bug("[SCSI  ] %s: Failed to start up daemon!\n", __PRETTY_FUNCTION__);
        return FALSE;
    }

    /* Wait for handshake */
    Wait(SIGF_SINGLE);
    D(bug("[SCSI  ] %s: Daemon task set to 0x%p\n", __PRETTY_FUNCTION__, SCSIBase->scsi_Daemon));

    return SCSIBase->scsi_Daemon ? TRUE : FALSE;
}

static int scsi_expunge(struct scsiBase *SCSIBase)
{
    struct scsi_Controller *scsiNode, *tmpNode;
    ForeachNodeSafe (&SCSIBase->scsi_Controllers, scsiNode, tmpNode)
    {
        OOP_Object *storageRoot;
        /*
         * CLID_Hidd_Storage is a singletone, you can get it as many times as
         * you want. Here we save up some space in struct scsiBase by
         * obtaining storageRoot object only when we need it. This happens
         * rarely, so small performance loss is OK here.
         */
        storageRoot = OOP_NewObject(NULL, CLID_Hidd_Storage, NULL);
        if (!storageRoot)
            storageRoot = OOP_NewObject(NULL, CLID_HW_Root, NULL);
        if (storageRoot && HW_RemoveDriver(storageRoot, scsiNode->sc_Object))
        {
            Remove(&scsiNode->sc_Node);
            /* Destroy our singletone */
            OOP_MethodID disp_msg = OOP_GetMethodID(IID_Root, moRoot_Dispose);

            D(bug("[SCSI  ] scsi_expunge: Stopping Daemon...\n"));
            SCSIBase->daemonParent = FindTask(NULL);
            SetSignal(0, SIGF_SINGLE);
            Signal(SCSIBase->scsi_Daemon, SIGBREAKF_CTRL_C);
            Wait(SIGF_SINGLE);

            D(bug("[SCSI  ] scsi_expunge: Done, destroying subystem object\n"));
            OOP_DoSuperMethod(scsiNode->sc_Class, scsiNode->sc_Object, &disp_msg);
            FreeMem(scsiNode, sizeof(struct scsi_Controller));
        }
        else
        {
            /* Our subsystem is in use, we have some bus driver(s) around. */
            D(bug("[SCSI  ] scsi_expunge: SCSI subsystem is in use\n"));
            return FALSE;
        }
    }

#if defined(__OOP_NOATTRBASES__)
    D(bug("[SCSI  ] scsi_expunge: Releasing attribute bases\n"));
    OOP_ReleaseAttrBasesArray(&SCSIBase->unitAttrBase, attrBaseIDs);
#endif

    if (SCSIBase->scsi_UtilityBase)
        CloseLibrary(SCSIBase->scsi_UtilityBase);

    D(bug("[SCSI  ] scsi_expunge: Exiting\n"));
    return TRUE;
}

static int open(struct scsiBase *SCSIBase, struct IORequest *iorq,
                ULONG unitnum, ULONG flags)
{
    struct scsi_Controller *scsiNode;
    struct Hook searchHook =
    {
        .h_Entry = Hidd_SCSIBus_Open,
        .h_Data  = iorq
    };

    /* Assume it failed */
    iorq->io_Error  = IOERR_OPENFAIL;
    iorq->io_Device = &SCSIBase->scsi_Device;
    iorq->io_Unit   = (APTR)(IPTR)-1;

    /* Try to find the unit */
    ForeachNode (&SCSIBase->scsi_Controllers, scsiNode)
    {
        HIDD_StorageController_EnumBuses(scsiNode->sc_Object, &searchHook, (APTR)(IPTR)unitnum);
    }
    D(bug("[SCSI%02d] Open result: %d\n", unitnum, iorq->io_Error));

    /* If found, io_Error will be reset to zero */
    return iorq->io_Error ? FALSE : TRUE;
}

/* Close given device */
static int close
(
    LIBBASETYPEPTR LIBBASE,
    struct IORequest *iorq
)
{
    struct scsi_Unit *unit = (struct scsi_Unit *)iorq->io_Unit;

    /* First of all make the important fields of struct IORequest invalid! */
    iorq->io_Unit = (struct Unit *)~0;
    
    /* Decrease use counters of unit */
    unit->su_Unit.unit_OpenCnt--;

    return TRUE;
}

ADD2INITLIB(SCSI_init, 0)
ADD2EXPUNGELIB(scsi_expunge, 0)
ADD2OPENDEV(open, 0)
ADD2CLOSEDEV(close, 0)
