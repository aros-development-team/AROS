/*
    Copyright © 2004-2010, The AROS Development Team. All rights reserved
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
 */

#define DEBUG 1
#include <aros/debug.h>

#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <utility/utility.h>
#include <oop/oop.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>

#include <dos/bptr.h>
#include <dos/filehandler.h>
#include <dos/dosextens.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>

#include <oop/oop.h>
#include <hidd/pci.h>
#include <proto/oop.h>

#include "ata.h"
#include "timer.h"

#include LC_LIBDEFS_FILE

#define RANGESIZE0 8
#define RANGESIZE1 4
#define DMASIZE 16

/* Add a bootnode using expansion.library */
BOOL ata_RegisterVolume(ULONG StartCyl, ULONG EndCyl, struct ata_Unit *unit)
{
    struct ExpansionBase *ExpansionBase;
    struct DeviceNode *devnode;
    IPTR *pp;
    TEXT dosdevname[4] = "HD0", *handler;
    UWORD len;

    ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library",
                                                        40L);

    if (ExpansionBase)
    {
        pp = AllocMem(24*sizeof(IPTR), MEMF_PUBLIC | MEMF_CLEAR);

        if (pp)
        {
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
            pp[0] = (IPTR)dosdevname;
            pp[1] = (IPTR)MOD_NAME_STRING;
            pp[2] = unit->au_UnitNum;
            pp[DE_TABLESIZE + 4] = DE_BOOTBLOCKS;
            pp[DE_SIZEBLOCK + 4] = 1 << (unit->au_SectorShift - 2);
            pp[DE_NUMHEADS + 4] = unit->au_Heads;
            pp[DE_SECSPERBLOCK + 4] = 1;
            pp[DE_BLKSPERTRACK + 4] = unit->au_Sectors;
            pp[DE_RESERVEDBLKS + 4] = 2;
            pp[DE_LOWCYL + 4] = StartCyl;
            pp[DE_HIGHCYL + 4] = EndCyl;
            pp[DE_NUMBUFFERS + 4] = 10;
            pp[DE_BUFMEMTYPE + 4] = MEMF_PUBLIC | MEMF_CHIP;
            pp[DE_MAXTRANSFER + 4] = 0x00200000;
            pp[DE_MASK + 4] = 0x7FFFFFFE;
            pp[DE_BOOTPRI + 4] = ((!unit->au_DevType) ? 0 : 10);
            pp[DE_DOSTYPE + 4] = 0x444F5301;
            pp[DE_BOOTBLOCKS + 4] = 2;
            devnode = MakeDosNode(pp);

            if (devnode)
            {
                if(unit->au_DevType == DG_DIRECT_ACCESS)
                    handler = "afs.handler";
                else
                    handler = "cdrom.handler";
                len = strlen(handler);
                if ((devnode->dn_Handler =
                     MKBADDR(AllocMem(AROS_BSTR_MEMSIZE4LEN(len),
                                      MEMF_PUBLIC | MEMF_CLEAR
                             )
                     )
                ))
                {
                    CopyMem(handler, AROS_BSTR_ADDR(devnode->dn_Handler), len);
                    AROS_BSTR_setstrlen(devnode->dn_Handler, len);

                    D(bug("[ATA>>]:-ata_RegisterVolume: '%s' with StartCyl=%d, EndCyl=%d .. ",
                          AROS_DOSDEVNAME(devnode), StartCyl, EndCyl));
                    AddBootNode(pp[DE_BOOTPRI + 4], 0, devnode, 0);
                    D(bug("done\n"));
                    
                    return TRUE;
                }
            }
        }

        CloseLibrary((struct Library *)ExpansionBase);
    }

    return FALSE;
}

void ata_RegisterBus(IPTR IOBase, IPTR IOAlt, IPTR INTLine,
    IPTR DMABase, BOOL has80Wire, EnumeratorArgs *a)
{
    /*
     * ata bus - this is going to be created and linked to the master list here
     */
    struct ata_Bus *ab;

    UWORD i;

    /*
     * initialize structure
     */
    ab = (struct ata_Bus*) AllocVecPooled(a->ATABase->ata_MemPool, sizeof(struct ata_Bus));
    if (ab == NULL)
        return;

    ab->ab_Base         = a->ATABase;
    ab->ab_Port         = IOBase;
    ab->ab_Alt          = IOAlt;
    ab->ab_IRQ          = INTLine;
    ab->ab_Dev[0]       = DEV_NONE;
    ab->ab_Dev[1]       = DEV_NONE;
    ab->ab_Flags        = 0;
    ab->ab_SleepySignal = 0;
    ab->ab_BusNum       = a->CurrentBus++;
    ab->ab_Timeout      = 0;
    ab->ab_Units[0]     = NULL;
    ab->ab_Units[1]     = NULL;
#ifdef IDE_HIDDIRQ
    ab->ab_IntHandler   = (HIDDT_IRQ_Handler *)AllocVecPooled(a->ATABase->ata_MemPool, sizeof(HIDDT_IRQ_Handler));
#endif
    ab->ab_Task         = NULL;
    ab->ab_HandleIRQ    = NULL;

    D(bug("[ATA>>] ata_RegisterBus: Analysing bus %d, units %d and %d\n", ab->ab_BusNum, ab->ab_BusNum<<1, (ab->ab_BusNum<<1)+1));
    D(bug("[ATA>>] ata_RegisterBus: IRQ %d, IO: %x:%x, DMA: %x\n", INTLine, IOBase, IOAlt, DMABase));

    /*
     * allocate DMA PRD
     */
#ifdef IDE_DMA
    ab->ab_PRD          = AllocVecPooled(a->ATABase->ata_MemPool, (PRD_MAX+1) * 2 * sizeof(struct PRDEntry));  
    if ((0x10000 - ((ULONG)ab->ab_PRD & 0xffff)) < PRD_MAX * sizeof(struct PRDEntry))
       ab->ab_PRD      = (void*)((((IPTR)ab->ab_PRD)+0xffff) &~ 0xffff);
#endif

    /*
     * scan bus - try to locate all devices (disables irq)
     */
    ata_InitBus(ab);
    for (i = 0; i < MAX_BUSUNITS; i++)
    {
        if (ab->ab_Dev[i] > DEV_UNKNOWN)
        {
            ab->ab_Units[i] = AllocVecPooled(a->ATABase->ata_MemPool,
                sizeof(struct ata_Unit));
#ifdef IDE_DMA
            ab->ab_Units[i]->au_DMAPort = DMABase;
#endif
            ab->ab_Units[i]->au_Flags = has80Wire ? AF_80Wire : 0;
            ata_init_unit(ab, i);
        }
    }

    D(bug("[ATA>>] ata_RegisterBus: Bus %ld: Unit 0 - %x, Unit 1 - %x\n", ab->ab_BusNum, ab->ab_Dev[0], ab->ab_Dev[1]));

    /*
     * start things up :)
     * note: this happens no matter there are devices or not 
     * sort of almost-ready-for-hotplug ;)
     */
    AddTail((struct List*)&a->ATABase->ata_Buses, (struct Node*)ab);
}

void ata_scanstart(struct ataBase *base)
{
    struct SignalSemaphore ssem;
    struct Node* node;

    D(bug("[ATA--] ata_Scan: Initialising Bus Tasks..\n"));
    InitSemaphore(&ssem);
    ForeachNode(&base->ata_Buses, node)
    {
        ata_InitBusTask((struct ata_Bus*)node, &ssem);
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
}

/*
    Here shall we start. Make function static as it shouldn't be visible from
    outside.
*/
static int ata_init(LIBBASETYPEPTR LIBBASE)
{
    struct BootLoaderBase	*BootLoaderBase;

    D(bug("[ATA--] ata_init: ata.device Initialization\n"));

    if (!ata_ishardware())
    	return FALSE;

    /*
     * I've decided to use memory pools again. Alloc everything needed from 
     * a pool, so that we avoid memory fragmentation.
     */
    LIBBASE->ata_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED , 8192, 4096);
    if (LIBBASE->ata_MemPool == NULL)
        return FALSE;

    D(bug("[ATA--] ata_init: MemPool @ %p\n", LIBBASE->ata_MemPool));

    ata_configure(LIBBASE);

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
                    if (strstr(node->ln_Name, "nopci"))
                    {
                        D(bug("[ATA  ] ata_init: Disabling PCI device scan\n"));
                        LIBBASE->ata_ScanFlags &= ~ATA_SCANPCI;
                    }
                    if (strstr(node->ln_Name, "nolegacy"))
                    {
                        D(bug("[ATA  ] ata_init: Disabling Legacy ports\n"));
                        LIBBASE->ata_ScanFlags &= ~ATA_SCANLEGACY;
                    }
#ifdef IDE_32BIT
                    if (strstr(node->ln_Name, "32bit"))
                    {
                        D(bug("[ATA  ] ata_init: Using 32-bit IO transfers\n"));
                        LIBBASE->ata_32bit = TRUE;
                    }
#endif
		    if (strstr(node->ln_Name, "nomulti"))
		    {
			D(bug("[ATA  ] ata_init: Disabled multisector transfers\n"));
			LIBBASE->ata_NoMulti = TRUE;
		    }
#ifdef IDE_DMA
                    if (strstr(node->ln_Name, "nodma"))
                    {
                        D(bug("[ATA  ] ata_init: Disabled DMA transfers\n"));
                        LIBBASE->ata_NoDMA = TRUE;
                    }
#endif
                    if (strstr(node->ln_Name, "poll"))
                    {
                        D(bug("[ATA  ] ata_init: Using polling to detect end of busy state\n"));
                        LIBBASE->ata_Poll = TRUE;
                    }
                }
            }
        }
    }

    /*
     * Initialize BUS list
     */
    LIBBASE->ata_Buses.mlh_Head     = (struct MinNode*) &LIBBASE->ata_Buses.mlh_Tail;
    LIBBASE->ata_Buses.mlh_Tail     = NULL;
    LIBBASE->ata_Buses.mlh_TailPred = (struct MinNode*) &LIBBASE->ata_Buses.mlh_Head;

    /*
     * Find all suitable devices ..
     */
    ata_Scan(LIBBASE);

    /* Try to setup daemon task looking for diskchanges */
    ata_InitDaemonTask(LIBBASE);
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
ADD2OPENDEV(open, 0)
ADD2CLOSEDEV(close, 0)

#ifdef IDE_HIDDIRQ
ADD2LIBS("irq.hidd", 0, static struct Library *, __irqhidd)
#endif

/* vim: set ts=8 sts=4 et : */
