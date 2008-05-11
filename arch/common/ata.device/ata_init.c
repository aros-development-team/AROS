/*
    Copyright © 2004-2008, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/
/*
 * CHANGELOG:
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
 */

#define DEBUG 0
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
#include <string.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>

#include <oop/oop.h>
#include <hidd/pci.h>
#include <proto/oop.h>


#include "ata.h"
#include LC_LIBDEFS_FILE


typedef struct 
{
    struct ataBase     *ATABase;
    UWORD               CurrentBus;
    UWORD               PredefBus;
} EnumeratorArgs;

/* Add a bootnode using expansion.library */
BOOL AddVolume(ULONG StartCyl, ULONG EndCyl, struct ata_Unit *unit)
{
    struct ExpansionBase *ExpansionBase;
    struct DeviceNode *devnode;
    IPTR *pp;
    static int volnum;
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
                    D(bug("IDE: AddVolume called on unknown devicetype\n"));
            }
            dosdevname[2] += volnum;
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

                    D(bug("-Adding volume %s with SC=%d, EC=%d\n",
                          &(devnode->dn_Ext.dn_AROS.dn_DevName[0]), StartCyl, EndCyl));
                    AddBootNode(pp[DE_BOOTPRI + 4], 0, devnode, 0);
                    D(bug("done\n"));
                    
                    volnum++;

                    return TRUE;
                }
            }
        }

        CloseLibrary((struct Library *)ExpansionBase);
    }

    return FALSE;
}

static void Add_Device(IPTR IOBase, IPTR IOAlt, IPTR INTLine,
		      IPTR DMABase, int x, EnumeratorArgs *a)
{
    /*
     * static list of io/irqs that we can handle
     */
    static struct __bus 
    {
        ULONG port;
        ULONG alt;
        UBYTE irq;
    } Buses[] = 
    {
        {0x1f0, 0x3f4, 14},
        {0x170, 0x374, 15},
        {0x168, 0x36c, 10},
        {0x1e8, 0x3ec, 11},
    };

    /*
     * ata bus - this is going to be created and linked to the master list here
     */
    struct ata_Bus *ab;

    /*
     * see if IO Base is valid. otherwise pick device from static list 
     * (this most likely means the device is right there)
     */
    if (IOBase == 0)
    {
        if (a->PredefBus < (sizeof(Buses) / sizeof(Buses[0])))
        {
            /*
             * collect IOBase and interrupt from the above list
             */
            IOBase  = Buses[a->PredefBus].port;
            IOAlt   = Buses[a->PredefBus].alt;
            INTLine = Buses[a->PredefBus].irq;
            a->PredefBus++;
        }
        else
        {
            IOBase  = 0;
            IOAlt   = 0;
            DMABase = 0;
            INTLine = 0;
            bug("[ATA>>] Found more controllers\n");
            /*
             * we're all done. no idea what else they want from us
             */
            return;
	}
    }

    D(bug("[ATA>>] IO: %x:%x DMA: %x\n", IOBase, IOAlt, DMABase));

    /*
     * initialize structure
     */
    ab = (struct ata_Bus*) AllocVecPooled(a->ATABase->ata_MemPool, sizeof(struct ata_Bus));
    if (ab == NULL)
        return;

    ab->ab_Base         = a->ATABase;
    ab->ab_Port         = IOBase;
    ab->ab_Alt          = IOAlt;
    ab->ab_Irq          = INTLine;
    ab->ab_Dev[0]       = DEV_NONE;
    ab->ab_Dev[1]       = DEV_NONE;
    ab->ab_Flags        = 0;
    ab->ab_SleepySignal = 0;
    ab->ab_BusNum       = a->CurrentBus++;
    ab->ab_Timeout      = 0;
    ab->ab_Units[0]     = 0;
    ab->ab_Units[1]     = 0;
    ab->ab_IntHandler   = (HIDDT_IRQ_Handler *)AllocVecPooled(a->ATABase->ata_MemPool, sizeof(HIDDT_IRQ_Handler));
    ab->ab_Task         = 0;
    ab->ab_HandleIRQ    = 0;

    D(bug("[ATA>>] Analysing bus %d, units %d and %d\n", ab->ab_BusNum, ab->ab_BusNum<<1, (ab->ab_BusNum<<1)+1));

    /*
     * allocate DMA PRD
     */
    ab->ab_PRD          = AllocVecPooled(a->ATABase->ata_MemPool, (PRD_MAX+1) * 2 * sizeof(struct PRDEntry));  
    if ((0x10000 - ((ULONG)ab->ab_PRD & 0xffff)) < PRD_MAX * sizeof(struct PRDEntry))
       ab->ab_PRD      = (void*)((((IPTR)ab->ab_PRD)+0xffff) &~ 0xffff);

    /*
     * scan bus - try to locate all devices (disables irq)
     */
    ata_InitBus(ab);
    if (ab->ab_Dev[0] > DEV_UNKNOWN)
    {
        ab->ab_Units[0] = AllocVecPooled(a->ATABase->ata_MemPool, sizeof(struct ata_Unit));
        ab->ab_Units[0]->au_DMAPort = (DMABase != 0 ? DMABase + (x<<3) : 0);
        ata_init_unit(ab, 0);
    }
    if (ab->ab_Dev[1] > DEV_UNKNOWN)
    {
        ab->ab_Units[1] = AllocVecPooled(a->ATABase->ata_MemPool, sizeof(struct ata_Unit));
        ab->ab_Units[1]->au_DMAPort = (DMABase != 0 ? DMABase + (x<<3) : 0);
        ata_init_unit(ab, 1);
    }

    D(bug("[ATA>>] Bus %ld: Unit 0 - %x, Unit 1 - %x\n", ab->ab_BusNum, ab->ab_Dev[0], ab->ab_Dev[1]));

    /*
     * start things up :)
     * note: this happens no matter there are devices or not 
     * sort of almost-ready-for-hotplug ;)
     */
    AddTail((struct List*)&a->ATABase->ata_Buses, (struct Node*)ab);
}

/*
 * PCI BUS ENUMERATOR
 *   collect ALL ata/ide capable devices (including SATA and other) and spawn consecutive tasks
 *
 * This function is growing too large. It will shorten drasticly once this whole mess gets converted into c++
 */

static
AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(OOP_Object *,     Device, A2),
    AROS_UFHA(APTR,             message,A1))
{
    AROS_USERFUNC_INIT

    /*
     * parameters we will want to acquire
     */
    IPTR            ProductID, 
                    VendorID,
                    DMABase,
                    INTLine,
                    IOBase,
                    IOAlt;

    /*
     * the PCI Attr Base
     */
    OOP_AttrBase HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    /*
     * new parameters for every device:
     * - allow bus mastering
     */
    struct TagItem attrs[] = 
    {
        { aHidd_PCIDevice_isMaster, TRUE    },
        { TAG_DONE,                 0UL     }
    };

    /*
     * enumerator params
     */
    EnumeratorArgs *a = (EnumeratorArgs*)hook->h_Data;

    /*
     * temporary variables
     */
    int             x;

    /*
     * obtain more or less useful data
     */
    OOP_GetAttr(Device, aHidd_PCIDevice_ProductID,          &ProductID);
    OOP_GetAttr(Device, aHidd_PCIDevice_VendorID,           &VendorID);
    OOP_GetAttr(Device, aHidd_PCIDevice_Base4,              &DMABase);

    if (a->ATABase->ata_NoDMA)
        DMABase = 0;

    /*
     * we can have as many as four ports assigned to this device
     */
    for (x=0; x<2; x++)
    {
        /*
         * obtain base and interrupt
         */
        switch (x)
        {
            case 0: 
               OOP_GetAttr(Device, aHidd_PCIDevice_Base0, &IOBase); 
               OOP_GetAttr(Device, aHidd_PCIDevice_Base1, &IOAlt); 
               break;
            case 1:
               OOP_GetAttr(Device, aHidd_PCIDevice_Base2, &IOBase); 
               OOP_GetAttr(Device, aHidd_PCIDevice_Base3, &IOAlt); 
               break;
        }
        OOP_GetAttr(Device, aHidd_PCIDevice_INTLine, &INTLine);

	D(bug("[ATA.scanbus] IDE device %04x:%04x - IO: %x:%x DMA: %x\n", ProductID, VendorID, IOBase, IOAlt, DMABase));
	Add_Device(IOBase, IOAlt, INTLine, DMABase, x, a);

    }

    /*
     * check dma status
     */
    if (DMABase != 0)
        D(bug("[ATA  ] Bus0 status says %02x, Bus1 status says %02x\n", ata_in(2, DMABase), ata_in(10, DMABase)));
    
    OOP_SetAttrs(Device, attrs);
    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);

    AROS_USERFUNC_EXIT
}


void ata_Scan(struct ataBase *base)
{
    OOP_Object *pci;
    struct SignalSemaphore ssem;

    struct Node* node;
    int i;
    EnumeratorArgs Args=
    {
        base,
        0,
        0
    };

    D(bug("[ATA--] Enumerating devices\n"));

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

    if (pci)
    {
        struct Hook FindHook = {
            h_Entry:    (IPTR (*)())Enumerator,
            h_Data:     &Args
        };

        struct TagItem Requirements[] = {
            {tHidd_PCI_Class,       0x01},
            {tHidd_PCI_SubClass,    0x01}, 
            {TAG_DONE,              0x00}
        };

        struct pHidd_PCI_EnumDevices enummsg = {
            mID:            OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
            callback:       &FindHook,
            requirements:   (struct TagItem *)&Requirements,
        }, *msg = &enummsg;
        
        OOP_DoMethod(pci, (OOP_Msg)msg);

        /* 
         * The SiL3114 chip yields Class 0x01 and SubClass 0x80. Therefore it will not be find
         * with the enumeration above. Do an explicit search now since ata.device may handle it
         * in legacy mode without any issues.
         * 
         * Note: This chip is used on Sam440 board.
         */
        Requirements[0].ti_Tag = tHidd_PCI_VendorID;
        Requirements[0].ti_Data = 0x1095;
        Requirements[1].ti_Tag = tHidd_PCI_ProductID;
        Requirements[1].ti_Data = 0x3114;

        OOP_DoMethod(pci, (OOP_Msg)msg);
        
        OOP_DisposeObject(pci);
    }
    if (!Args.CurrentBus) {
	D(bug("[ATA--] No PCI devices found, attempting defaults\n"));
	for (i=0; i<4; i++)
	    Add_Device(0, 0, 0, 0, i & 1, &Args);
    }


    InitSemaphore(&ssem);
    ForeachNode(&base->ata_Buses, node)
    {
        ata_InitBusTask((struct ata_Bus*)node, &ssem);
    }

    /*
     * wait for all buses to complete their init
     */
    ObtainSemaphore(&ssem);

    /*
     * and leave.
     */
    ReleaseSemaphore(&ssem);
}

/*
    Here shall we start. Make function static as it shouldn't be visible from
    outside.
*/
static int ata_init(LIBBASETYPEPTR LIBBASE)
{
    struct BootLoaderBase *BootLoaderBase;

    /*
     * I've decided to use memory pools again. Alloc everything needed from 
     * a pool, so that we avoid memory fragmentation.
     */
    LIBBASE->ata_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED , 8192, 4096);
    if (LIBBASE->ata_MemPool == NULL)
        return FALSE;

    /*
     * store library pointer so we can use it later
     */
    LIBBASE->ata_32bit = FALSE;
    LIBBASE->ata_NoDMA = FALSE;

    /*
     * start initialization: 
     * obtain kernel parameters
     */
    D(bug("[ATA--] ata.device initialization\n"));
    BootLoaderBase = OpenResource("bootloader.resource");
    D(bug("[ATA--] BootloaderBase = %p\n", BootLoaderBase));
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
                    if (strstr(node->ln_Name, "32bit"))
                    {
                        D(bug("[ATA  ] Using 32-bit IO transfers\n"));
                        LIBBASE->ata_32bit = TRUE;
                    }
                    if (strstr(node->ln_Name, "nodma"))
                    {
                        D(bug("[ATA  ] Disabled DMA transfers\n"));
                        LIBBASE->ata_NoDMA = TRUE;
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
ADD2LIBS("irq.hidd", 0, static struct Library *, __irqhidd)
/* vim: set ts=8 sts=4 et : */
