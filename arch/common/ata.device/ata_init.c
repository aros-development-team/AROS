/*
    Copyright © 2004-2007, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/
/*
 * CHANGELOG:
 * DATE        NAME                ENTRY
 * ----------  ------------------  -------------------------------------------------------------------
 * 2008-01-25  T. Wiszkowski       Rebuilt, rearranged and partially fixed 60% of the code here
 *                                 Enabled implementation to scan for other PCI IDE controllers
 *                                 Implemented ATAPI Packet Support for both read and write
 *                                 Corrected ATAPI DMA handling                            
 *                                 Fixed major IDE enumeration bugs severely handicapping transfers with more than one controller
 *                                 Compacted source and implemented major ATA support procedure
 *                                 Improved DMA and Interrupt management
 *                                 Removed obsolete code
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

static struct ataBase *__ATABase;
static OOP_AttrBase __IHidd_PCIDev;

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

/*
 * PCI BUS ENUMERATOR
 *   collect ALL ata/ide capable devices (including SATA and other) and spawn consecutive tasks
 */

static
AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(OOP_Object *,     Device, A2),
    AROS_UFHA(APTR,             message,A1))
{
    AROS_USERFUNC_INIT

    /*
     * static list of io/irqs that we can handle
     */
    static struct __bus 
    {
        ULONG port;
        UBYTE irq;
    } Buses[] = 
    {
        {0x1f0, 14},
        {0x170, 15},
        {0x168, 10},
        {0x1e8, 11},
    };

    /*
     * current bus number - used to calculate ATA unit number
     */
    static int      current_bus = 0;

    /*
     * predefined bus number - i'm still not sure if that's the right way to handle this
     * however it should work well. the regular IDE controllers send back no IO/IRQ data
     */
    static int      predef_bus = 0; 

    /*
     * ata bus - this is going to be created and linked to the master list here
     */
    struct ata_Bus *ab;

    /*
     * parameters we will want to acquire
     */
    IPTR            ProductID, 
                    VendorID,
                    DMABase,
                    INTLine,
                    IOBase;

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
     * temporary variables
     */
    int             x;

    /*
     * obtain more or less useful data
     */
    OOP_GetAttr(Device, aHidd_PCIDevice_ProductID, &ProductID);
    OOP_GetAttr(Device, aHidd_PCIDevice_VendorID,  &VendorID);
    OOP_GetAttr(Device, aHidd_PCIDevice_Base4,     &DMABase);

    D(bug("[ATA.scanbus] IDE device %04x:%04x - IO: %x DMA: %x\n", ProductID, VendorID, IOBase, DMABase));

    /*
     * we can have as many as four ports assigned to this device
     */
    for (x=0; x<4; x++)
    {
        /*
         * obtain base and interrupt
         */
        switch (x)
        {
            case 0: OOP_GetAttr(Device, aHidd_PCIDevice_Base0, &IOBase); break;
            case 1: OOP_GetAttr(Device, aHidd_PCIDevice_Base1, &IOBase); break;
            case 2: OOP_GetAttr(Device, aHidd_PCIDevice_Base2, &IOBase); break;
            case 3: OOP_GetAttr(Device, aHidd_PCIDevice_Base3, &IOBase); break;
        }
        OOP_GetAttr(Device, aHidd_PCIDevice_INTLine, &INTLine);

        /*
         * see if IO Base is valid. otherwise pick device from static list 
         * (this most likely means the device is right there)
         */
        if (IOBase == 0)
        {
            if (predef_bus < (sizeof(Buses) / sizeof(Buses[0])))
            {
                /*
                 * collect IOBase and interrupt from the above list
                 */
                IOBase  = Buses[predef_bus].port;
                INTLine = Buses[predef_bus].irq;
                predef_bus++;
            }
            else
            {
                /*
                 * we're all done. no idea what else they want from us
                 */
                break;
            }
        }

        /*
         * initialize structure
         */
        ab = (struct ata_Bus*) AllocVecPooled(__ATABase->ata_MemPool, sizeof(struct ata_Bus));
        if (ab == NULL)
            return;

        ab->ab_Base         = __ATABase;
        ab->ab_Port         = IOBase;
        ab->ab_Irq          = INTLine;
        ab->ab_Dev[0]       = DEV_NONE;
        ab->ab_Dev[1]       = DEV_NONE;
        ab->ab_Flags        = 0;
        ab->ab_SleepySignal = 0;
        ab->ab_BusNum       = current_bus++;
        ab->ab_Waiting      = 0;
        ab->ab_Timeout      = 0;
        ab->ab_Units[0]     = 0;
        ab->ab_Units[1]     = 0;
        ab->ab_IntHandler   = (HIDDT_IRQ_Handler *)AllocVecPooled(__ATABase->ata_MemPool, sizeof(HIDDT_IRQ_Handler));
        D(bug("[ATA  ] Analysing bus %d, units %d and %d\n", ab->ab_BusNum, ab->ab_BusNum<<1, (ab->ab_BusNum<<1)+1));

        /*
         * allocate DMA PRD
         */
        ab->ab_PRD          = AllocVecPooled(__ATABase->ata_MemPool, (PRD_MAX+1) * 2 * sizeof(struct PRDEntry));  
        if ((0x10000 - ((ULONG)ab->ab_PRD & 0xffff)) < PRD_MAX * sizeof(struct PRDEntry))
            ab->ab_PRD      = (void*)((((IPTR)ab->ab_PRD)+0xffff) &~ 0xffff);

        InitSemaphore(&ab->ab_Lock);

        /*
         * scan bus - try to locate all devices
         */
        ata_ScanBus(ab);
        if (ab->ab_Dev[0] > DEV_UNKNOWN)
        {
            ab->ab_Units[0] = AllocVecPooled(__ATABase->ata_MemPool, sizeof(struct ata_Unit));
            ab->ab_Units[0]->au_DMAPort = (APTR)(DMABase != 0 ? DMABase + (x<<3) : 0);
        }
        if (ab->ab_Dev[1] > DEV_UNKNOWN)
        {
            ab->ab_Units[1] = AllocVecPooled(__ATABase->ata_MemPool, sizeof(struct ata_Unit));
            ab->ab_Units[1]->au_DMAPort = (APTR)(DMABase != 0 ? DMABase + (x<<3) : 0);
        }

        D(bug("[ATA  ] Bus %ld: Unit 0 - %x, Unit 1 - %x\n", ab->ab_BusNum, ab->ab_Dev[0], ab->ab_Dev[1]));

        /*
         * start things up :)
         * note: this happens no matter there are devices or not 
         * sort of almost-ready-for-hotplug ;)
         */
        AddTail((struct List*)&__ATABase->ata_Buses, (struct Node*)ab);

        ata_InitBusTask(ab);
    }

    /*
     * check dma status
     */
    if (DMABase != 0)
        D(bug("[ATA  ] Bus0 status says %02x, Bus1 status says %02x\n", inb(DMABase + 2), inb(DMABase + 10)));


    OOP_SetAttrs(Device, attrs);
    AROS_USERFUNC_EXIT
}


void ata_Scan(void)
{
    OOP_Object *pci;

    D(bug("[ATA--] Enumerating devices\n"));

    /*
     * obtain attr base of pci devices i guess ;]
     */
    __IHidd_PCIDev = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

    if (pci)
    {
        struct Hook FindHook = {
            h_Entry:    (IPTR (*)())Enumerator,
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
        
        OOP_DisposeObject(pci);
    }
    
    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
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
    __ATABase = LIBBASE;

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
                
    ata_Scan();
    
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
        if (b->ab_Node.mln_Succ == NULL)
            return FALSE;

        b = (struct ata_Bus*)b->ab_Node.mln_Succ;
    }

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
