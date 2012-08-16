/*
    Copyright © 2004-2011, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/
/*
 * PARTIAL CHANGELOG:
 * DATE        NAME                ENTRY
 * ----------  ------------------  -------------------------------------------------------------------
 * 2008-04-25  P. Fedin		   Brought back device discovery for old machines without PCI IDE controllers
 * 2008-01-25  T. Wiszkowski       Rebuilt, rearranged and partially fixed 60% of the code here
 *                                 Enabled implementation to scan for other PCI IDE controllers
 *                                 Implemented ATAPI Packet Support for both read and write
 *                                 Corrected ATAPI DMA handling                            
 *                                 Fixed major IDE enumeration bugs severely handicapping transfers with more than one controller
 *                                 Compacted source and implemented major ATA support procedure
 *                                 Improved DMA and Interrupt management
 *                                 Removed obsolete code
 * 2008-01-26  T. Wiszkowski       Added 'nodma' flag for ata driver
 *                                 Moved variables out of global scope
 *                                 Replaced static variables
 * 2008-02-08  T. Wiszkowski       Fixed DMA accesses for direct scsi devices,
 *                                 Corrected IO Areas to allow ATA to talk to PCI controllers
 * 2008-02-24  T. Wiszkowski       Corrected unit open function
 * 2008-03-03  T. Wiszkowski       Added drive reselection + setup delay on Init
 * 2008-03-23  T. Wiszkowski       Corrected Alternative Command block position
 * 2008-03-30  T. Wiszkowski       Added workaround for interrupt collision handling; fixed SATA in LEGACY mode.
 *                                 nForce and Intel SATA chipsets should now be operational.
 * 2008-04-03  T. Wiszkowski       Fixed IRQ flood issue, eliminated and reduced obsolete / redundant code                                 
 * 2008-04-07  T. Wiszkowski       Changed bus timeout mechanism
 * 2008-04-07  M. Schulz           The SiL3114 chip yields Class 0x01 and SubClass 0x80. Therefore it will 
 *                                 not be find with the generic enumeration. Do an explicit search after it 
 *                                 since ata.device may handle it in legacy mode without any issues.
 * 2008-05-11  T. Wiszkowski       Remade the ata trannsfers altogether, corrected the pio/irq handling 
 *                                 medium removal, device detection, bus management and much more
 * 2008-05-18  T. Wiszkowski       corrected device naming to handle cases where more than 10 physical units may be available
 * 2008-06-24  P. Fedin            Added 'NoMulti' flag to disable multisector transfers
 * 2009-03-05  T. Wiszkowski       remade timeouts, added timer-based and benchmark-based delays.
 * 2011-04-05  P. Fedin            Addid basic SATA handling, needed for Mac.
 * 2011-05-19  P. Fedin            The Big Rework. Separated bus-specific code. Made 64-bit-friendly.
 * 2012-02-12  T. Wilen            ata_RegisterBus() Flags parameter introduced and optional early interrupt setup.
 */

#include <aros/bootloader.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <utility/utility.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>

#include <string.h>

#include "ata.h"
#include "timer.h"

#include LC_LIBDEFS_FILE

/* Add a bootnode using expansion.library */
BOOL ata_RegisterVolume(ULONG StartCyl, ULONG EndCyl, struct ata_Unit *unit)
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
        switch (unit->au_DevType)
        {
            case DG_DIRECT_ACCESS:
                break;
            case DG_CDROM:
                dosdevname[0] = 'C';
                break;
            default:
                D(bug("[ATA>>]:-ata_RegisterVolume called on unknown devicetype\n"));
        }

        if (unit->au_UnitNum < 10)
            dosdevname[2] += unit->au_UnitNum % 10;
        else
            dosdevname[2] = 'A' - 10 + unit->au_UnitNum;
    
        pp[0] 		    = (IPTR)dosdevname;
        pp[1]		    = (IPTR)MOD_NAME_STRING;
        pp[2]		    = unit->au_UnitNum;
        pp[DE_TABLESIZE    + 4] = DE_BOOTBLOCKS;
        pp[DE_SIZEBLOCK    + 4] = 1 << (unit->au_SectorShift - 2);
        pp[DE_NUMHEADS     + 4] = unit->au_Heads;
        pp[DE_SECSPERBLOCK + 4] = 1;
        pp[DE_BLKSPERTRACK + 4] = unit->au_Sectors;
        pp[DE_RESERVEDBLKS + 4] = 2;
        pp[DE_LOWCYL       + 4] = StartCyl;
        pp[DE_HIGHCYL      + 4] = EndCyl;
        pp[DE_NUMBUFFERS   + 4] = 10;
        pp[DE_BUFMEMTYPE   + 4] = MEMF_PUBLIC | MEMF_31BIT;
        pp[DE_MAXTRANSFER  + 4] = 0x00200000;
        pp[DE_MASK         + 4] = 0x7FFFFFFE;
        pp[DE_BOOTPRI      + 4] = ((unit->au_DevType == DG_DIRECT_ACCESS) ? 0 : 10);
        pp[DE_DOSTYPE      + 4] = ((unit->au_DevType == DG_DIRECT_ACCESS) ? IdDOS : IdCDVD);
        pp[DE_CONTROL      + 4] = 0;
        pp[DE_BOOTBLOCKS   + 4] = 2;
    
        devnode = MakeDosNode(pp);

        if (devnode)
        {
            D(bug("[ATA>>]:-ata_RegisterVolume: '%b', type=0x%08lx with StartCyl=%d, EndCyl=%d .. ",
                  devnode->dn_Name, pp[DE_DOSTYPE + 4], StartCyl, EndCyl));

            AddBootNode(pp[DE_BOOTPRI + 4], ADNF_STARTPROC, devnode, NULL);
            D(bug("done\n"));
            
            return TRUE;
        }

        CloseLibrary((struct Library *)ExpansionBase);
    }

    return FALSE;
}

static AROS_INTH1(ATAResetHandler,struct ata_Bus *, bus)
{
    AROS_INTFUNC_INIT

    struct ata_Unit *unit;
    UWORD i;

    /* Stop DMA */
    for (i = 0; i < MAX_BUSUNITS; i++)
    {
        unit = bus->ab_Units[i];
        if (unit != NULL)
        {
            if(unit->au_DMAPort != 0)
            {
                dma_StopDMA(unit);
                BUS_OUTL(0, dma_PRD, unit->au_DMAPort);
            }
        }
    }

    /* Disable interrupts */
    BUS_OUT(0x2, ata_AltControl, bus->ab_Alt);

    return FALSE;

    AROS_INTFUNC_EXIT
}

/*
 * This routine needs to be called by bus probe code in order to register a device.
 * IOBase     - base address of primary I/O registers on your bus.
 * IOAlt      - base address of secondary I/O register bank. Zero if no secondary bank
 *              is present. (IDE splitter on Amiga(tm), for example).
 * DMABase    - base address of DMA controller on your bus. Zero if DMA is not supported.
 * Flags      - Misc flags
 * driver     - structure holding pointers to I/O functions (for speedup)
 * driverData - driver-specific data, whatever it needs.
 *
 * Flags:     - ARBF_80Wire
 *              Set if your drive is connected using 80-wire cable. Enables high-speed
 *              UDMA modes (where appropriate).
 *            - ARBF_EarlyInterrupt
 *              Setup interrupt handler before IDE bus probe to catch possible spurious
 *              interrupts (IDE splitter disables access to ata_devcon register)
 *
 * When a HIDD subsystem is implemented, these parameters will become HIDD attributes.
 */
void ata_RegisterBus(IPTR IOBase, IPTR IOAlt, IPTR INTLine, IPTR DMABase, ULONG Flags,
		     const struct ata_BusDriver *driver, APTR driverData, struct ataBase *ATABase)
{
    /*
     * ata bus - this is going to be created and linked to the master list here
     */
    struct ata_Bus *ab;

    UWORD i;

    /*
     * initialize structure
     */
    ab = (struct ata_Bus*) AllocVecPooled(ATABase->ata_MemPool, sizeof(struct ata_Bus));
    if (ab == NULL)
        return;

    ab->ab_Base         = ATABase;
    ab->ab_Port         = IOBase;
    ab->ab_Alt          = IOAlt;
    ab->ab_IRQ          = INTLine;
    ab->ab_Dev[0]       = DEV_NONE;
    ab->ab_Dev[1]       = DEV_NONE;
    ab->ab_Flags        = 0;
    ab->ab_SleepySignal = 0;
    ab->ab_BusNum       = ATABase->ata__buscount++;
    ab->ab_Timeout      = 0;
    ab->ab_Units[0]     = NULL;
    ab->ab_Units[1]     = NULL;
    ab->ab_Task         = NULL;
    ab->ab_HandleIRQ    = NULL;
    ab->ab_Driver       = driver;
    ab->ab_DriverData	= driverData;

    D(bug("[ATA>>] ata_RegisterBus: Analysing bus %d, units %d and %d\n", ab->ab_BusNum, ab->ab_BusNum<<1, (ab->ab_BusNum<<1)+1));
    D(bug("[ATA>>] ata_RegisterBus: IRQ %d, IO: %x:%x, DMA: %x\n", INTLine, IOBase, IOAlt, DMABase));

    /*
     * DMABase is also used for reporting interrupt status, so NoDMA == TRUE
     * is not equal to DMABase == 0.
     */
    if (DMABase && (!ATABase->ata_NoDMA))
    {
    	/* Allocate DMA PRD. Due to the nature of PCI bus it must be in 32-bit memory. */
    	ab->ab_PRD = AllocMem((PRD_MAX + 1) * 2 * sizeof(struct PRDEntry), MEMF_PUBLIC|MEMF_CLEAR|MEMF_31BIT);

    	if (ab->ab_PRD)
    	{
    	    if ((0x10000 - ((IPTR)ab->ab_PRD & 0xffff)) < PRD_MAX * sizeof(struct PRDEntry))
       	    	ab->ab_PRD = (void*)((((IPTR)ab->ab_PRD)+0xffff) &~ 0xffff);
       	}
       	else
       	{
       	    D(bug("[ATA>>] Failed to allocate DMA PRD! Disabling DMA for the bus.\n"));
       	    DMABase = 0;
       	}
    }

    /*
     * add reset handler for this bus
     */
    ab->ab_ResetInt.is_Code = (VOID_FUNC)ATAResetHandler;
    ab->ab_ResetInt.is_Data = ab;
    AddResetCallback(&ab->ab_ResetInt);

    /* catch possible spurious interrupts */
    if (Flags & ARBF_EarlyInterrupt)
        ab->ab_Driver->CreateInterrupt(ab);

    /*
     * scan bus - try to locate all devices (disables irq)
     */
    ata_InitBus(ab);
    for (i = 0; i < MAX_BUSUNITS; i++)
    {
        if (ab->ab_Dev[i] > DEV_UNKNOWN)
        {
            ab->ab_Units[i] = AllocVecPooled(ATABase->ata_MemPool,
                sizeof(struct ata_Unit));
            ab->ab_Units[i]->au_DMAPort = DMABase;
            ab->ab_Units[i]->au_Flags = (Flags & ARBF_80Wire) ? AF_80Wire : 0;
            ata_init_unit(ab, i);
        }
    }

    D(bug("[ATA>>] ata_RegisterBus: Bus %ld: Unit 0 - %x, Unit 1 - %x\n", ab->ab_BusNum, ab->ab_Dev[0], ab->ab_Dev[1]));

    /*
     * start things up :)
     * note: this happens no matter there are devices or not 
     * sort of almost-ready-for-hotplug ;)
     */
    AddTail((struct List*)&ATABase->ata_Buses, (struct Node*)ab);
}

/*
 * This init routine has +127 priority, so it runs after all
 * bus scanners have done their job.
 * It initializes all discovered units.
 */
static int ata_Scan(struct ataBase *base)
{
    struct SignalSemaphore ssem;
    struct ata_Bus* node;
    struct Task *parent = FindTask(NULL);

    D(bug("[ATA--] ata_Scan: Initialising Bus Tasks..\n"));
    InitSemaphore(&ssem);
    ForeachNode(&base->ata_Buses, node)
    {
    	NewCreateTask(TASKTAG_PC	 , BusTaskCode,
    		      TASKTAG_NAME	 , "ATA[PI] Subsystem",
    		      TASKTAG_STACKSIZE  , STACK_SIZE,
    		      TASKTAG_PRI	 , TASK_PRI,
    		      TASKTAG_TASKMSGPORT, &node->ab_MsgPort,
    		      TASKTAG_ARG1 	 , node,
        	      TASKTAG_ARG2	 , parent,
        	      TASKTAG_ARG3	 , &ssem,
        	      TAG_DONE);

	/* Initial handshake */
        Wait(SIGBREAKF_CTRL_C);
    }

    /*
     * wait for all buses to complete their init
     */
    D(bug("[ATA--] ata_Scan: Waiting for Buses to finish Initialising\n"));
    ObtainSemaphore(&ssem);

    /*
     * and leave.
     */
    ReleaseSemaphore(&ssem);
    D(bug("[ATA--] ata_Scan: Finished\n"));

    /* Try to setup daemon task looking for diskchanges */
    NewCreateTask(TASKTAG_PC       , DaemonCode,
                  TASKTAG_NAME     , "ATA.daemon",
                  TASKTAG_STACKSIZE, STACK_SIZE,
                  TASKTAG_PRI      , TASK_PRI - 1,	/* The daemon should have a little bit lower Pri as handler tasks */
                  TASKTAG_ARG1     , base,
                  TAG_DONE);

    return TRUE;
}

/*
    Here shall we start. Make function static as it shouldn't be visible from
    outside.
*/
static int ata_init(LIBBASETYPEPTR LIBBASE)
{
    struct BootLoaderBase	*BootLoaderBase;

    D(bug("[ATA--] ata_init: ata.device Initialization\n"));

    /*
     * I've decided to use memory pools again. Alloc everything needed from 
     * a pool, so that we avoid memory fragmentation.
     */
    LIBBASE->ata_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED , 8192, 4096);
    if (LIBBASE->ata_MemPool == NULL)
        return FALSE;

    D(bug("[ATA--] ata_init: MemPool @ %p\n", LIBBASE->ata_MemPool));

    /* Set default ata.device config options */
    LIBBASE->ata_32bit   = FALSE;
    LIBBASE->ata_NoMulti = FALSE;
    LIBBASE->ata_NoDMA   = FALSE;
    LIBBASE->ata_Poll    = FALSE;
    LIBBASE->ata_CmdLine = NULL;

    /*
     * start initialization: 
     * obtain kernel parameters
     */
    BootLoaderBase = OpenResource("bootloader.resource");
    D(bug("[ATA--] ata_init: BootloaderBase = %p\n", BootLoaderBase));
    if (BootLoaderBase != NULL)
    {
        struct List *list;
        struct Node *node;

        list = (struct List *)GetBootInfo(BL_Args);
        if (list)
        {
            ForeachNode(list, node)
            {
                if (strncmp(node->ln_Name, "ATA=", 4) == 0)
                {
                    /*
                     * Remember the entire command line.
                     * Bus drivers (for example PCI one) may want it.
                     */
                    LIBBASE->ata_CmdLine = &node->ln_Name[4];

                    if (strstr(LIBBASE->ata_CmdLine, "32bit"))
                    {
                        D(bug("[ATA  ] ata_init: Using 32-bit IO transfers\n"));
                        LIBBASE->ata_32bit = TRUE;
                    }
		    if (strstr(LIBBASE->ata_CmdLine, "nomulti"))
		    {
			D(bug("[ATA  ] ata_init: Disabled multisector transfers\n"));
			LIBBASE->ata_NoMulti = TRUE;
		    }
                    if (strstr(LIBBASE->ata_CmdLine, "nodma"))
                    {
                        D(bug("[ATA  ] ata_init: Disabled DMA transfers\n"));
                        LIBBASE->ata_NoDMA = TRUE;
                    }
                    if (strstr(LIBBASE->ata_CmdLine, "poll"))
                    {
                        D(bug("[ATA  ] ata_init: Using polling to detect end of busy state\n"));
                        LIBBASE->ata_Poll = TRUE;
                    }
                }
            }
        }
    }

    /* Initialize BUS list */
    NEWLIST(&LIBBASE->ata_Buses);

    return TRUE;
}

static int open
(
    LIBBASETYPEPTR LIBBASE,
    struct IORequest *iorq,
    ULONG unitnum,
    ULONG flags
)
{
    /*
     * device location
     */
    ULONG bus, dev;
    
    /* 
     * Assume it failed 
     */
    iorq->io_Error = IOERR_OPENFAIL;

    /*
     * actual bus
     */
    struct ata_Bus *b = (struct ata_Bus*)LIBBASE->ata_Buses.mlh_Head;

    /* 
     * Extract bus and device numbers
     */
    bus = unitnum >> 1;                 // 0xff00 >> 8
    dev = (unitnum & 0x1);              // 0x00ff

    /*
     * locate bus
     */
    while (bus--)
    {
        b = (struct ata_Bus*)b->ab_Node.mln_Succ;
        if (b == NULL)
            return FALSE;
    }

    if (b->ab_Node.mln_Succ == NULL)
        return FALSE;

    /*
     * locate unit
     */
    if (b->ab_Units[dev] == NULL)
        return FALSE;

    /*
     * set up iorequest
     */
    iorq->io_Device     = &LIBBASE->ata_Device;
    iorq->io_Unit       = &b->ab_Units[dev]->au_Unit;
    iorq->io_Error      = 0;

    b->ab_Units[dev]->au_Unit.unit_OpenCnt++;

    return TRUE;
}

/* Close given device */
static int close
(
    LIBBASETYPEPTR LIBBASE,
    struct IORequest *iorq
)
{
    struct ata_Unit *unit = (struct ata_Unit *)iorq->io_Unit;

    /* First of all make the important fields of struct IORequest invalid! */
    iorq->io_Unit = (struct Unit *)~0;
    
    /* Decrease use counters of unit */
    unit->au_Unit.unit_OpenCnt--;

    return TRUE;
}

ADD2INITLIB(ata_init, 0)
ADD2INITLIB(ata_Scan, 127)
ADD2OPENDEV(open, 0)
ADD2CLOSEDEV(close, 0)
/* vim: set ts=8 sts=4 et : */
