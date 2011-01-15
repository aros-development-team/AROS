
#include "ata.h"

#if defined(__i386__) || defined(__x86_64__)

/*
 * having an x86 assembly here i dare to assume that this is meant to be
 * an x86[_64] device only.
 *
 * Not anymore.
 *
 * This functions will stay here for some time *IF* and only if the code is compiled for
 * x86 or x86_64 architecture. Otherwise, function declarations will be emitted and the
 * one who ports the driver will be reponsible for adding the missing code.
 */

/*
 * the outsl and insl commands improperly assumed that every transfer is sized to multiple of four
 */

static VOID ata_insw(APTR address, IPTR port, ULONG count)
{
    insw(port, address, count >> 1);
}

static VOID ata_insl(APTR address, IPTR port, ULONG count)
{
    if (count & 2)
        insw(port, address, count >> 1);
    else
        insl(port, address, count >> 2);
}

static VOID ata_outsw(APTR address, IPTR port, ULONG count)
{
    outsw(port, address, count >> 1);
}

static VOID ata_outsl(APTR address, IPTR port, ULONG count)
{
    if (count & 2)
        outsw(port, address, count >> 1);
    else
        outsl(port, address, count >> 2);
}

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

    BOOL	_usablebus = FALSE;

    /*
     * the PCI Attr Base
     */
    OOP_AttrBase HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

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
    OOP_GetAttr(Device, aHidd_PCIDevice_VendorID,           &VendorID);
    OOP_GetAttr(Device, aHidd_PCIDevice_ProductID,          &ProductID);
    OOP_GetAttr(Device, aHidd_PCIDevice_Base4,              &DMABase);
    OOP_GetAttr(Device, aHidd_PCIDevice_Size4,              &DMASize);
    OOP_GetAttr(Device, aHidd_PCIDevice_SubClass,           &SubClass);
    OOP_GetAttr(Device, aHidd_PCIDevice_Interface,          &Interface);

    /*
     * we can have up to two buses assigned to this device
     */
    for (x = 0; SubClass != 0 && SubClass != 7 && x < MAX_DEVICEBUSES; x++)
    {
	struct ata_LegacyBus *_legacyBus = NULL;
        BOOL isLegacy = FALSE;

	if (x == 0)
	{
		bug("[ATA  ] ata_PCIEnumerator_h: Found IDE device %04x:%04x\n", VendorID, ProductID);
	}

        /*
         * obtain I/O bases and interrupt line
         */
        if ((Interface & (1 << (x << 1))) || SubClass != 1)
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
            a->ATABase->ata__legacybuses.lh_Head)->atalb_ControllerID == 0)
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
            bug("[ATA  ] ata_PCIEnumerator_h: Ran out of legacy buses\n");
            IOBase = 0;
        }

        if (IOBase != (IPTR)NULL && IOSize == RANGESIZE0
            && AltSize == RANGESIZE1
            && (DMASize >= DMASIZE || DMABase == NULL || SubClass == 1))
	{
	    struct ata_ProbedBus *probedbus;
	    D(bug("[ATA  ] ata_PCIEnumerator_h: Adding Bus %d - IRQ %d, IO: %x:%x, DMA: %x\n", x, INTLine, IOBase, IOAlt, DMABase));
	    if ((probedbus = AllocMem(sizeof(struct ata_ProbedBus), MEMF_CLEAR | MEMF_PUBLIC)) != (IPTR)NULL)
	    {
		probedbus->atapb_IOBase = IOBase;
		probedbus->atapb_IOAlt = IOAlt;
		probedbus->atapb_INTLine = INTLine;
		if (DMABase != 0)
		    probedbus->atapb_DMABase = DMABase + (x << 3);
		probedbus->atapb_a = a;
		probedbus->atapb_Has80Wire = TRUE;

		if (isLegacy)
		{
		    D(bug("[ATA  ] ata_PCIEnumerator_h: Device using Legacy-Bus IOPorts\n"));
		    probedbus->atapb_Node.ln_Pri = ATABUSNODEPRI_PROBEDLEGACY - (a->ATABase->ata__buscount++);
		}
		else
		    probedbus->atapb_Node.ln_Pri = ATABUSNODEPRI_PROBED - (a->ATABase->ata__buscount++);

		Enqueue((struct List *)&a->ATABase->ata__probedbuses, (struct Node *)probedbus);
		_usablebus = TRUE;
	    }
	}
    }

    if (_usablebus)
    {
	struct TagItem attrs[] = 
	{
	    { aHidd_PCIDevice_isIO,     TRUE },
	    { aHidd_PCIDevice_isMaster, DMABase != 0 },
	    { TAG_DONE,                 0UL     }
	};
	OOP_SetAttrs(Device, attrs);
    }

    /* check dma status if applicable */
    if (DMABase != 0)
        D(bug("[ATA  ] ata_PCIEnumerator_h: Bus0 DMA Status %02x, Bus1 DMA Status %02x\n", ata_in(2, DMABase), ata_in(10, DMABase)));

    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);

    AROS_USERFUNC_EXIT
}

void ata_Scan(struct ataBase *base)
{
    OOP_Object *pci;
    struct ata_ProbedBus *probedbus;

    struct Node* node;
    EnumeratorArgs Args=
    {
        base,
        0
    };

    D(bug("[ATA--] ata_Scan: Enumerating devices\n"));

    if (base->ata_ScanFlags & ATA_SCANPCI) {
	D(bug("[ATA--] ata_Scan: Checking for supported PCI devices ..\n"));
	pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

	if (pci)
	{
	    struct Hook FindHook = {
		h_Entry:    (IPTR (*)())ata_PCIEnumerator_h,
		h_Data:     &Args
	    };

	    struct TagItem Requirements[] = {
		{tHidd_PCI_Class,       0x01},
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
    }
    if (base->ata_ScanFlags & ATA_SCANLEGACY) {
	struct ata_LegacyBus *legacybus;
	D(bug("[ATA--] ata_Scan: Adding Remaining Legacy-Buses\n"));
	while ((legacybus = (struct ata_LegacyBus *)
	    RemHead((struct List *)&base->ata__legacybuses)) != NULL)
	{
	    if ((probedbus = AllocMem(sizeof(struct ata_ProbedBus), MEMF_CLEAR | MEMF_PUBLIC)) != NULL)
	    {
		probedbus->atapb_IOBase = legacybus->atalb_IOBase;
		probedbus->atapb_IOAlt = legacybus->atalb_IOAlt;
		probedbus->atapb_INTLine = legacybus->atalb_INTLine;
		probedbus->atapb_DMABase = (IPTR)NULL;
		probedbus->atapb_Has80Wire = FALSE;
		probedbus->atapb_a = &Args;
		probedbus->atapb_Node.ln_Pri = ATABUSNODEPRI_LEGACY - (base->ata__buscount++);
		D(bug("[ATA--] ata_Scan: Adding Legacy Bus - IO: %x:%x\n",
		    probedbus->atapb_IOBase, probedbus->atapb_IOAlt));
		Enqueue((struct List *)&base->ata__probedbuses, (struct Node *)&probedbus->atapb_Node);
	    }
	}
	FreeMem(legacybus, sizeof(struct ata_LegacyBus));
    }

    D(bug("[ATA--] ata_Scan: Registering Probed Buses..\n"));
    while ((probedbus = (struct ata_ProbedBus *)
	RemHead((struct List *)&base->ata__probedbuses)) != NULL)
    {
	ata_RegisterBus(
		probedbus->atapb_IOBase,
		probedbus->atapb_IOAlt,
		probedbus->atapb_INTLine,
		probedbus->atapb_DMABase,
		probedbus->atapb_Has80Wire,
		probedbus->atapb_a);

	FreeMem(probedbus, sizeof(struct ata_ProbedBus));
    }

    ata_scanstart(base);

}

struct ata_ProbedBus
{
    struct Node 		atapb_Node;
    IPTR 			atapb_IOBase;
    IPTR 			atapb_IOAlt;
    IPTR 			atapb_INTLine;
    IPTR 			atapb_DMABase;
    EnumeratorArgs 		*atapb_a;
    BOOL                        atapb_Has80Wire;
};

struct ata_LegacyBus
{
    struct Node 		atalb_Node;
    IPTR 			atalb_IOBase;
    IPTR 			atalb_IOAlt;
    IPTR 			atalb_INTLine;
    IPTR 			atalb_DMABase;
    UBYTE			atalb_ControllerID;
    UBYTE			atalb_BusID;
};

#define ATABUSNODEPRI_PROBED		50
#define ATABUSNODEPRI_PROBEDLEGACY	100
#define ATABUSNODEPRI_LEGACY		0

/* static list of io/irqs that we can handle */
static struct ata__legacybus 
{
    ULONG lb_Port;
    ULONG lb_Alt;
    UBYTE lb_IRQ;
    UBYTE lb_ControllerID;
    UBYTE lb_Bus;
} LegacyBuses[] = 
{
    {0x1f0, 0x3f4, 14, 0, 0},
    {0x170, 0x374, 15, 0, 1},
    {0x168, 0x36c, 10, 1, 0},
    {0x1e8, 0x3ec,  11, 1, 1},
    {0, 0,  0, 0, 0},
};

void ata_configure(LIBBASETYPEPTR LIBBASE)
{
    struct ata_LegacyBus	*_legacybus;
    int i;

    /* Prepare lists for probed/found ide buses */
    NEWLIST((struct List *)&LIBBASE->ata__legacybuses);
    NEWLIST((struct List *)&LIBBASE->ata__probedbuses);

    /* Build the list of possible legacy-bus ports */
    for (i = 0; LegacyBuses[i].lb_Port != 0 ; i++)
    {
	if ((_legacybus = AllocMem(sizeof(struct ata_LegacyBus), MEMF_CLEAR | MEMF_PUBLIC)) != NULL)
	{
	    D(bug("[ATA--] ata_init: Prepare Legacy Bus %d:%d entry [IOPorts %x:%x IRQ %d]\n", LegacyBuses[i].lb_ControllerID, LegacyBuses[i].lb_Bus, LegacyBuses[i].lb_Port, LegacyBuses[i].lb_Alt, LegacyBuses[i].lb_IRQ));

	    _legacybus->atalb_IOBase = (IPTR)LegacyBuses[i].lb_Port;
	    _legacybus->atalb_IOAlt = (IPTR)LegacyBuses[i].lb_Alt;
	    _legacybus->atalb_INTLine = (IPTR)LegacyBuses[i].lb_IRQ;
	    _legacybus->atalb_ControllerID = (IPTR)LegacyBuses[i].lb_ControllerID;
	    _legacybus->atalb_BusID = (IPTR)LegacyBuses[i].lb_Bus;
	    AddTail(&LIBBASE->ata__legacybuses, &_legacybus->atalb_Node);
	}
    }

    /* Set default ata.device config options */
    LIBBASE->ata_ScanFlags = ATA_SCANPCI | ATA_SCANLEGACY;
    LIBBASE->ata_32bit = FALSE;
    LIBBASE->ata_NoMulti = FALSE;
    LIBBASE->ata_NoDMA = FALSE;
    LIBBASE->ata_Poll = FALSE;
}

static void ata_Interrupt(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct ata_Bus *bus = (struct ata_Bus *)irq->h_Data;

    ata_HandleIRQ(bus);
}

int ata_CreateInterrupt(struct ata_Bus *bus)
{
    struct OOP_Object *o;
    int retval = 0;

    if (bus->ab_IntHandler)
    {
        /*
            Prepare nice interrupt for our bus. Even if interrupt sharing is enabled,
            it should work quite well
        */
        bus->ab_IntHandler->h_Node.ln_Pri = 10;
        bus->ab_IntHandler->h_Node.ln_Name = bus->ab_Task->tc_Node.ln_Name;
        bus->ab_IntHandler->h_Code = ata_Interrupt;
        bus->ab_IntHandler->h_Data = bus;

        o = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
        if (o)
        {
            struct pHidd_IRQ_AddHandler __msg__ = {
                mID:            OOP_GetMethodID(IID_Hidd_IRQ, moHidd_IRQ_AddHandler),
                handlerinfo:    bus->ab_IntHandler,
                id:             bus->ab_IRQ,
            }, *msg = &__msg__;

            if (OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg))
                retval = 1;

            OOP_DisposeObject((OOP_Object *)o);
        }
    }

    return retval;
}

#endif
