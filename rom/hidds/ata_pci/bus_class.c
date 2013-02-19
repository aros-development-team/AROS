#include <aros/debug.h>
#include <hidd/ata.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "bus_class.h"
#include "interface_pio.h"
#include "interface_dma.h"

OOP_Object *PCIATA__ATABus__Root_New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    o = OOP_DoSuperMethod(cl, o, &msg->mID);
    if (o)
    {
        struct ATA_BusData *data = OOP_INST_DATA(cl, o);
 
        data->bus = (struct ata_ProbedBus *)GetTagData(aHidd_DriverData, 0, msg->attrList);
        if (data->bus->atapb_DMABase)
        {
            /* We have a DMA controller and will need a buffer */
            OOP_GetAttr(data->bus->atapb_Device, aHidd_PCIDevice_Driver, (IPTR *)&data->pciDriver);
            data->dmaBuf = HIDD_PCIDriver_AllocPCIMem(data->pciDriver,
                                                      (PRD_MAX + 1) * 2 * sizeof(struct PRDEntry));
        }
    }
    return o;
}

OOP_Object *PCIATA__ATABus__Root_Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);

    if (data->dmaBuf)
        HIDD_PCIDriver_FreePCIMem(data->pciDriver, (PRD_MAX + 1) * 2 * sizeof(struct PRDEntry));

    HIDD_PCIDevice_Release(data->bus->atapb_Device);
    FreeVec(data->bus);

    OOP_DoSuperMethod(cl, o, msg);
}

void PCIATA__ATABus__Root_Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct ataBase *base = cl->cl_UserData;
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    ULONG idx;
    
    if (IS_HIDD_ATABUS_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_ATABus_Use80Wire:
            /*
             * FIXME: Currently we assume that if the user has DMA controller,
             * he has a modern machine and 80-conductor cable. But what if it
             * is wrong ? What if his cable is broken and he temporarily
             * installed old 40-conductor one ? In this case he will get data
             * corruption.
             */
        case aoHidd_ATABus_UseDMA:
            *msg->storage = data->atapb->atapb_DMABase ? TRUE : FALSE;
            return;
        }
    }

    OOP_DoSuperMethod(&msg->mID);
}

void PCIATA__ATABus__Root_Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    struct TagItem *tstate = msg->attrList;
    struct TagItem *tag;
    
    while ((tag = NextTagItem(&tstate)))
    {
        if (tag->ti_Tag == aHidd_ATABus_Use32Bit)
        {
            /* TODO: Change PIO vectors */
        }
        else if (tag->ti_Tag == aHidd_ATABus_DMAMode)
        {
            dma_SetXFerMode(tag->ti_Data);
        }
    }
}

APTR PCIATA__ATA__ATA_GetPIOInterface(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    struct pio_data *pio = (struct pio_data *)OOP_DoSuperMethod(cl, o, msg);
    
    if (pio)
    {
        pio->ioBase = data->bus->atapb_IOBase;
        pio->ioAlt  = data->bus->atapb_IOAlt;
    }

    return pio;
}

APTR PCIATA__ATA__ATA_GetDMAInterface(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    struct pio_data *dma;

    /* If we don't have a DMA buffer, we cannot do DMA */
    if (!data->dmaBuf)
        return NULL;

    dma = (struct pio_data *)OOP_DoSuperMethod(cl, o, msg);
    if (dma)
    {
        dma->au_dmaPort = data->bus->atapb_DMABase;
        dma->ab_PRD     = data->dmaBuf;

        /* Align our buffer */
        if ((0x10000 - ((IPTR)dma->ab_PRD & 0xffff)) < PRD_MAX * sizeof(struct PRDEntry))
       	    dma->ab_PRD = (void*)((((IPTR)dma->ab_PRD)+0xffff) &~ 0xffff);
    }

    return dma;
}

BOOL PCIATA__ATA__ATA_SetXferMode(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    
    if (data->bus->atapb_DMAPort)
    {
        UBYTE type;

        type = inb(dma_Status + unit->au_DMAPort) & 0x60;
        if ((msg->mode >= AB_XFER_MDMA0) && (msg->mode <= AB_XFER_UDMA6))
        {
            type |= 1 << (5 + (msg->UnitNum & 1));
        }
        else
        {
            type &= ~(1 << (5 + (msg->UnitNum & 1)));
        }

        DINIT(bug("[ADMA] SetXferMode: Trying to apply new DMA (%lx) status: %02lx (unit %ld)\n", unit->au_DMAPort, type, unitNum));

        /*
         * Originally we had unit selection operation here.
         * I hope it's really not needed, because otherwise it's
         * a horrible headace because of 400ns delay.
         * Anyway currently the code which uses this method is broken
         * and disabled in ata.device. Please fix all this.
         * ata_SelectUnit(msg->UnitNum);
         */

        ata_outb(type, dma_Status + unit->au_DMAPort);
        if (type != (inb(dma_Status + unit->au_DMAPort) & 0x60))
        {
            D(bug("[ADMA] SetXferMode: Failed to modify DMA state for this device\n"));
            return FALSE;
        }
    }
    else if ((msg->mode >= AB_XFER_MDMA0) && (msg->mode <= AB_XFER_UDMA6))
    {
        /* DMA is not supported, we cannot set DMA modes */
        return FALSE;
    }

    return TRUE;
}
