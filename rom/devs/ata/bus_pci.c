/*
    Copyright © 2004-2011, The AROS Development Team. All rights reserved.
    $Id: lowlevel.c 34191 2010-08-17 16:19:51Z neil $

    Desc: PCI bus driver for ata.device
    Lang: English
*/

#define DSATA(x)

/*
 * What is done here is currently a draft.
 * The whole thing is a good candidate to become a HIDD.
 */

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <asm/io.h>
#include <exec/lists.h>
#include <hardware/ahci.h>
#include <hidd/irq.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include <string.h>

#include "ata.h"
#include "pci.h"

typedef struct 
{
    struct ataBase *ATABase;
    struct MinList  legacybuses;
    struct List	    probedbuses;
    ULONG	    ata__buscount;
    OOP_AttrBase    HiddPCIDeviceAttrBase;
    OOP_MethodID    HiddPCIDriverMethodBase;
} EnumeratorArgs;

struct ata_LegacyBus
{
    struct MinNode atalb_Node;
    IPTR 	   atalb_IOBase;
    IPTR 	   atalb_IOAlt;
    IPTR 	   atalb_INTLine;
    UBYTE	   atalb_ControllerID;
    UBYTE	   atalb_BusID;
};

struct ata_ProbedBus
{
    struct Node atapb_Node;
    IPTR 	atapb_IOBase;
    IPTR 	atapb_IOAlt;
    IPTR 	atapb_INTLine;
    IPTR 	atapb_DMABase;
    BOOL	atapb_80wire;
};

#define ATABUSNODEPRI_PROBED		50
#define ATABUSNODEPRI_PROBEDLEGACY	100
#define ATABUSNODEPRI_LEGACY		0

#define RANGESIZE0 8
#define RANGESIZE1 4
#define DMASIZE 16

/* static list of io/irqs that we can handle */
struct ata__legacybus 
{
    ULONG lb_Port;
    ULONG lb_Alt;
    UBYTE lb_IRQ;
    UBYTE lb_ControllerID;
    UBYTE lb_Bus;
};

static const struct ata__legacybus LegacyBuses[] = 
{
    {0x1f0, 0x3f4, 14, 0, 0},
    {0x170, 0x374, 15, 0, 1},
    {0x168, 0x36c, 10, 1, 0},
    {0x1e8, 0x3ec,  11, 1, 1},
    {0, 0,  0, 0, 0},
};

/*
 * Low-level bus I/O functions.
 * They are called directly via array of pointers supplied by our driver.
 * This should be kept this way even if bus driver part is made into a HIDD
 * some day, for speedup.
 */

#if defined(__i386__) || defined(__x86_64__)

/* On x86 ATA devices use I/O space, data is ignored. */

static void ata_out(UBYTE val, UWORD offset, IPTR port, APTR data)
{
    outb(val, offset + port);
}

static UBYTE ata_in(UWORD offset, IPTR port, APTR data)
{
    return inb(offset + port);
}

static void ata_outl(ULONG val, UWORD offset, IPTR port, APTR data)
{
    outl(val, offset + port);
}

static VOID ata_insw(APTR address, UWORD port, ULONG count, APTR data)
{
    insw(port, address, count >> 1);
}

static VOID ata_insl(APTR address, UWORD port, ULONG count, APTR data)
{
    if (count & 2)
        insw(port, address, count >> 1);
    else
        insl(port, address, count >> 2);
}

static VOID ata_outsw(APTR address, UWORD port, ULONG count, APTR data)
{
    outsw(port, address, count >> 1);
}

static VOID ata_outsl(APTR address, UWORD port, ULONG count, APTR data)
{
    if (count & 2)
        outsw(port, address, count >> 1);
    else
        outsl(port, address, count >> 2);
}

#else

/*
 * This came from SAM440 port. Data is a base address of mapped ISA I/O space.
 * I believe this should work fine on all non-x86 architectures.
 */

static VOID ata_out(UBYTE val, UWORD offset, IPTR port, APTR data)
{
    outb(val, (uint8_t *)(port + offset + data));
}

static UBYTE ata_in(UWORD offset, IPTR port, APTR data)
{
    return inb((uint8_t *)(port + offset + data));
}

static VOID ata_outl(ULONG val, UWORD offset, IPTR port, APTR data)
{
    outl_le(val, (uint32_t *)(port + offset + data));
}

static VOID ata_insw(APTR address, UWORD port, ULONG count, APTR data)
{
    UWORD *addr = address;
    UWORD *p = (UWORD*)(port + data);
    
    while(count)
    {
        *addr++ = inw(p);
        count -= 2;
    }
}

static VOID ata_insl(APTR address, UWORD port, ULONG count, APTR data)
{
    if (count & 2)
        ata_insw(address, port, count, data);
    else
    {
        ULONG *addr = address;
        ULONG *p = (ULONG*)(port + data);
        
        while(count)
        {
            *addr++ = inl(p);
            count -= 4;
        }
    }
}

static VOID ata_outsw(APTR address, UWORD port, ULONG count, APTR data)
{
    UWORD *addr = address;
    UWORD *p = (UWORD*)(port + data);
    
    while(count)
    {
        outw(*addr++, p);
        count -= 2;
    }
}

static VOID ata_outsl(APTR address, UWORD port, ULONG count, APTR data)
{
    if (count & 2)
        ata_outsw(address, port, count, data);
    else
    {
        ULONG *addr = address;
        ULONG *p = (ULONG*)(port + data);
        
        while(count)
        {
            outl(*addr++, p);
            count -= 4;
        }
    }
}

#endif

static void ata_Interrupt(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    /*
     * Our interrupt handler should call this function.
     * It's our problem how to store bus pointer. Here we use h_Data for it.
     */
    ata_HandleIRQ(irq->h_Data);
}

/* Actually a quick hack. Proper implementation really needs HIDDizing this code. */
static APTR CreateInterrupt(struct ata_Bus *bus)
{
    HIDDT_IRQ_Handler *IntHandler = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_PUBLIC);

    if (IntHandler)
    {
        OOP_Object *o;

        /*
            Prepare nice interrupt for our bus. Even if interrupt sharing is enabled,
            it should work quite well
        */
        IntHandler->h_Node.ln_Pri = 10;
        IntHandler->h_Node.ln_Name = bus->ab_Task->tc_Node.ln_Name;
        IntHandler->h_Code = ata_Interrupt;
        IntHandler->h_Data = bus;

        o = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
        if (o)
        {
            struct pHidd_IRQ_AddHandler msg =
            {
                mID:            OOP_GetMethodID(IID_Hidd_IRQ, moHidd_IRQ_AddHandler),
                handlerinfo:    IntHandler,
                id:             bus->ab_IRQ,
            };
            int retval = OOP_DoMethod(o, &msg.mID);

            OOP_DisposeObject(o);
            
            if (retval)
            	return IntHandler;
        }
    }

    FreeMem(IntHandler, sizeof(HIDDT_IRQ_Handler));
    return NULL;
}

static const struct ata_BusDriver pci_driver = 
{
    ata_out,
    ata_in,
    ata_outl,
    ata_insw,
    ata_outsw,
    ata_insl,
    ata_outsl,
    CreateInterrupt
};

/*
 * PCI BUS ENUMERATOR
 *   collect ALL ata/ide capable devices (including SATA and other) and
 *   spawn concurrent tasks.
 *
 * This function is growing too large. It will shorten drasticly once this whole mess gets converted into c++
 */

static
AROS_UFH3(void, ata_PCIEnumerator_h,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(OOP_Object *,     Device, A2),
    AROS_UFHA(APTR,             message,A1))
{
    AROS_USERFUNC_INIT

    /*
     * parameters we will want to acquire
     */
    IPTR	ProductID,
		VendorID,
		DMABase,
		DMASize,
                INTLine,
                IOBase,
                IOAlt,
                IOSize,
                AltSize,
                SubClass,
                Interface;

    /*
     * enumerator params
     */
    EnumeratorArgs *a = hook->h_Data;

    /*
     * the PCI Attr Base
     */
    OOP_AttrBase HiddPCIDeviceAttrBase = a->HiddPCIDeviceAttrBase;

    /*
     * temporary variables
     */
    int             x;

    /*
     * obtain more or less useful data
     */
    OOP_GetAttr(Device, aHidd_PCIDevice_VendorID,           &VendorID);
    OOP_GetAttr(Device, aHidd_PCIDevice_ProductID,          &ProductID);
    OOP_GetAttr(Device, aHidd_PCIDevice_Base4,              &DMABase);
    OOP_GetAttr(Device, aHidd_PCIDevice_Size4,              &DMASize);
    OOP_GetAttr(Device, aHidd_PCIDevice_SubClass,           &SubClass);
    OOP_GetAttr(Device, aHidd_PCIDevice_Interface,          &Interface);

    D(bug("[PCI-ATA] ata_PCIEnumerator_h: Found IDE device %04x:%04x\n", VendorID, ProductID));

    /*
     * SATA controllers are handled by the AHCI driver.
     */
    if (SubClass == PCI_SUBCLASS_SATA) {
        DSATA(bug("[PCI-ATA] Device %04x:%04x is a SATA device, ignoring\n", VendorID, ProductID));
        return;
    }

    /*
     * we can have up to two buses assigned to this device
     */
    for (x = 0; SubClass != 0 && SubClass != 7 && x < MAX_DEVICEBUSES; x++)
    {
	struct ata_LegacyBus *_legacyBus = NULL;
        BOOL isLegacy = FALSE;

        /*
         * obtain I/O bases and interrupt line
         */
        if ((Interface & (1 << (x << 1))) || SubClass != PCI_SUBCLASS_IDE)
        {
            switch (x)
            {
                case 0:
                    OOP_GetAttr(Device, aHidd_PCIDevice_Base0, &IOBase);
                    OOP_GetAttr(Device, aHidd_PCIDevice_Size0, &IOSize);
                    OOP_GetAttr(Device, aHidd_PCIDevice_Base1, &IOAlt);
                    OOP_GetAttr(Device, aHidd_PCIDevice_Size1, &AltSize);
                    break;
                case 1:
                    OOP_GetAttr(Device, aHidd_PCIDevice_Base2, &IOBase);
                    OOP_GetAttr(Device, aHidd_PCIDevice_Size2, &IOSize);
                    OOP_GetAttr(Device, aHidd_PCIDevice_Base3, &IOAlt);
                    OOP_GetAttr(Device, aHidd_PCIDevice_Size3, &AltSize);
               break;
            }
            OOP_GetAttr(Device, aHidd_PCIDevice_INTLine, &INTLine);
        }
        else if ((_legacyBus = (struct ata_LegacyBus *)
            a->legacybuses.mlh_Head)->atalb_ControllerID == 0)
	{
            Remove((struct Node *)_legacyBus);
            IOBase = _legacyBus->atalb_IOBase;
            IOAlt = _legacyBus->atalb_IOAlt;
            INTLine = _legacyBus->atalb_INTLine;
            FreeMem(_legacyBus, sizeof(struct ata_LegacyBus));
            isLegacy = TRUE;
            IOSize = RANGESIZE0;
            AltSize = RANGESIZE1;
        }
        else
        {
            bug("[PCI-ATA] ata_PCIEnumerator_h: Ran out of legacy buses\n");
            IOBase = 0;
        }

        if (IOBase != 0 && IOSize == RANGESIZE0
            && AltSize == RANGESIZE1
            && (DMASize >= DMASIZE || DMABase == 0 || SubClass == PCI_SUBCLASS_IDE))
	{
	    struct ata_ProbedBus *probedbus;
	    D(bug("[PCI-ATA] ata_PCIEnumerator_h: Adding Bus %d - IRQ %d, IO: %x:%x, DMA: %x\n", x, INTLine, IOBase, IOAlt, DMABase));

	    if ((probedbus = AllocMem(sizeof(struct ata_ProbedBus), MEMF_CLEAR | MEMF_PUBLIC)) != NULL)
	    {
		probedbus->atapb_IOBase = IOBase;
		probedbus->atapb_IOAlt = IOAlt;
		probedbus->atapb_INTLine = INTLine;
		if (DMABase != 0)
		    probedbus->atapb_DMABase = DMABase + (x << 3);
		probedbus->atapb_80wire = TRUE;

		if (isLegacy)
		{
		    D(bug("[PCI-ATA] ata_PCIEnumerator_h: Device using Legacy-Bus IOPorts\n"));
		    probedbus->atapb_Node.ln_Pri = ATABUSNODEPRI_PROBEDLEGACY - (a->ata__buscount++);
		}
		else
		    probedbus->atapb_Node.ln_Pri = ATABUSNODEPRI_PROBED - (a->ata__buscount++);

		Enqueue((struct List *)&a->probedbuses, &probedbus->atapb_Node);
		
		OOP_SetAttrsTags(Device, aHidd_PCIDevice_isIO, TRUE,
					 aHidd_PCIDevice_isMaster, DMABase != 0,
	    				 TAG_DONE);
	    }
	}
    }

    /* check dma status if applicable */
    D(if (DMABase != 0) bug("[PCI-ATA] ata_PCIEnumerator_h: Bus0 DMA Status %02x, Bus1 DMA Status %02x\n", ata_in(2, DMABase, (APTR)0xe8000000), ata_in(10, DMABase, (APTR)0xe8000000));)

    AROS_USERFUNC_EXIT
}

static const struct TagItem Requirements[] =
{
    {tHidd_PCI_Class, PCI_CLASS_MASSSTORAGE},
    {TAG_DONE,        0x00		   }
};

static int ata_pci_Scan(struct ataBase *base)
{
    OOP_Object *pci;
    struct ata_LegacyBus *_legacybus;
    struct ata_ProbedBus *probedbus;
    BOOL scanpci = TRUE;
    BOOL scanlegacy = TRUE;
    EnumeratorArgs Args;
    int i;

    /* Prepare lists for probed/found ide buses */
    NEWLIST(&Args.legacybuses);
    NEWLIST(&Args.probedbuses);
    Args.ata__buscount = 0;

    /* Build the list of possible legacy-bus ports */
    for (i = 0; LegacyBuses[i].lb_Port != 0 ; i++)
    {
	if ((_legacybus = AllocMem(sizeof(struct ata_LegacyBus), MEMF_CLEAR | MEMF_PUBLIC)) != NULL)
	{
	    D(bug("[PCI-ATA] ata_init: Prepare Legacy Bus %d:%d entry [IOPorts %x:%x IRQ %d]\n", LegacyBuses[i].lb_ControllerID, LegacyBuses[i].lb_Bus, LegacyBuses[i].lb_Port, LegacyBuses[i].lb_Alt, LegacyBuses[i].lb_IRQ));

	    _legacybus->atalb_IOBase = (IPTR)LegacyBuses[i].lb_Port;
	    _legacybus->atalb_IOAlt = (IPTR)LegacyBuses[i].lb_Alt;
	    _legacybus->atalb_INTLine = (IPTR)LegacyBuses[i].lb_IRQ;
	    _legacybus->atalb_ControllerID = (IPTR)LegacyBuses[i].lb_ControllerID;
	    _legacybus->atalb_BusID = (IPTR)LegacyBuses[i].lb_Bus;

	    AddTail((struct List *)&Args.legacybuses, (struct Node *)_legacybus);
	}
    }

    /* Obtain additional parameters */
    if (base->ata_CmdLine)
    {
        if (strstr(base->ata_CmdLine, "nopci"))
        {
            D(bug("[PCI-ATA] ata_init: Disabling PCI device scan\n"));
            scanpci = FALSE;
        }
        if (strstr(base->ata_CmdLine, "nolegacy"))
        {
            D(bug("[PCI-ATA] ata_init: Disabling Legacy ports\n"));
            scanlegacy = FALSE;
        }
    }

    D(bug("[PCI-ATA] ata_Scan: Enumerating devices\n"));

    if (scanpci)
    {
	D(bug("[PCI-ATA] ata_Scan: Checking for supported PCI devices ..\n"));

    	Args.ATABase                 = base;
    	Args.HiddPCIDriverMethodBase = 0;
	Args.HiddPCIDeviceAttrBase   = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

	if (Args.HiddPCIDeviceAttrBase)
	{
	    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

	    if (pci)
	    {
	    	struct Hook FindHook =
	    	{
		    h_Entry:    (IPTR (*)())ata_PCIEnumerator_h,
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

	    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
	}
    }

    if (scanlegacy)
    {
	struct ata_LegacyBus *legacybus;

	D(bug("[PCI-ATA] ata_Scan: Adding Remaining Legacy-Buses\n"));
	while ((legacybus = (struct ata_LegacyBus *)
	    RemHead((struct List *)&Args.legacybuses)) != NULL)
	{
	    if ((probedbus = AllocMem(sizeof(struct ata_ProbedBus), MEMF_CLEAR | MEMF_PUBLIC)) != NULL)
	    {
		probedbus->atapb_IOBase  = legacybus->atalb_IOBase;
		probedbus->atapb_IOAlt   = legacybus->atalb_IOAlt;
		probedbus->atapb_INTLine = legacybus->atalb_INTLine;
		probedbus->atapb_DMABase = 0;
		probedbus->atapb_80wire  = FALSE;
		probedbus->atapb_Node.ln_Pri = ATABUSNODEPRI_LEGACY - (Args.ata__buscount++);
		D(bug("[PCI-ATA] ata_Scan: Adding Legacy Bus - IO: %x:%x\n",
		    probedbus->atapb_IOBase, probedbus->atapb_IOAlt));

		Enqueue(&Args.probedbuses, &probedbus->atapb_Node);
	    }
	}
	FreeMem(legacybus, sizeof(struct ata_LegacyBus));
    }

    D(bug("[PCI-ATA] ata_Scan: Registering Probed Buses..\n"));

    while ((probedbus = (struct ata_ProbedBus *)RemHead(&Args.probedbuses)) != NULL)
    {
	/*
	 * 0xe8000000 here is a temporary kludge for SAM440 port. It's base address
	 * of memory-mapped ISA I/O space.
	 * In fact our PCI subsystem needs an attribute to be able to query this value.
	 * We don't use definition from asm/amcc440.h because this file is available
	 * only when doing SAM440 build.
	 * On x86 this value is ignored, see I/O functions.
	 */
	ata_RegisterBus(probedbus->atapb_IOBase, probedbus->atapb_IOAlt, probedbus->atapb_INTLine,
			probedbus->atapb_DMABase, probedbus->atapb_80wire ? ARBF_80Wire : 0,
			&pci_driver, (APTR)0xe8000000, base);

	FreeMem(probedbus, sizeof(struct ata_ProbedBus));
    }

    return TRUE;
}

/*
 * ata.device main code has two init routines with 0 and 127 priorities.
 * All bus scanners must run between them.
 */
ADD2INITLIB(ata_pci_Scan, 30)
ADD2LIBS("irq.hidd", 0, static struct Library *, __irqhidd)
