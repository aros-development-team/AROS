#include <aros/debug.h>
#include <hardware/ata.h>
#include <hidd/ata.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "bus_class.h"
#include "interface_pio.h"
#include "interface_dma.h"

AROS_INTH1(ata_PCI_Interrupt, struct ATA_BusData *, data)
{
    AROS_INTFUNC_INIT

    UBYTE status;

    /*
     * The DMA status register indicates all interrupt types, not
     * just DMA interrupts. However, if there's no DMA port, we have
     * to rely on the busy flag, which is incompatible with IRQ sharing.
     * We read ATA status register only once, because reading it tells
     * the drive to deassert INTRQ.
     */
    if (data->bus->atapb_DMABase != 0)
    {
        port_t dmaStatusPort = dma_Status + data->bus->atapb_DMABase;
        UBYTE dmastatus = inb(dmaStatusPort);

        if (!(dmastatus & DMAF_Interrupt))
            return FALSE;

        /*
         * Acknowledge interrupt (note that the DMA interrupt bit should be
         * cleared for all interrupt types).
         * Clear DMA interrupt bit before clearing interrupt by reading status
         * register. Otherwise, it seems that the DMA bit could get set again
         * for a new interrupt before we clear it, resulting in a missed interrupt.
         *              Neil
         */
        outb(dmastatus | DMAF_Error | DMAF_Interrupt, dmaStatusPort);
        status = inb(data->bus->atapb_IOBase + ata_Status);
    }
    else
    {
        status = inb(data->bus->atapb_IOBase + ata_Status);

        if (status & ATAF_BUSY)
            return FALSE;
    }

    data->ata_HandleIRQ(status, data->irqData);
    return TRUE;

    AROS_INTFUNC_EXIT
}

void ata_Raw_Interrupt(struct ATA_BusData *data, void *unused)
{
    AROS_INTC1(ata_PCI_Interrupt, data);
}

OOP_Object *PCIATA__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &msg->mID);
    if (o)
    {
        struct ataBase *base = cl->UserData;
        struct ATA_BusData *data = OOP_INST_DATA(cl, o);
        OOP_MethodID mDispose;

        /* No check because we always supply this */
        data->bus = (struct ata_ProbedBus *)GetTagData(aHidd_DriverData, 0, msg->attrList);

        if (data->bus->atapb_DMABase)
        {
            /* We have a DMA controller and will need a buffer */
            OOP_GetAttr(data->bus->atapb_Device->ref_Device,
                        aHidd_PCIDevice_Driver, (IPTR *)&data->pciDriver);
            data->dmaBuf = HIDD_PCIDriver_AllocPCIMem(data->pciDriver,
                                                      (PRD_MAX + 1) * 2 * sizeof(struct PRDEntry));
        }

        if (data->bus->atapb_Node.ln_Type == ATABUSNODEPRI_PROBED)
        {
            /*
             * We have a PCI device, install interrupt using portable PCI API.
             * But do this only if the device is in native mode. In compatibility
             * mode PCI configuration lies about interrupt number. Experienced
             * on my Acer AspireOne.
             * Perhaps this is portability issue but i don't know what to do with
             * this. Amiga(tm) guys, please check/fix. One possibility is to switch
             * to native mode here, but i believe in this case i would need to
             * also set up all I/O regions. On AspireOne only BAR4 is set for IDE
             * controller. So, also can be bad option. The best case would be if
             * Amiga(tm) never uses compatibility mode.
             *                  Pavel Fedin <pavel_fedin@mail.ru>.
             */
            struct Interrupt *pciInt = AllocMem(sizeof(struct Interrupt), MEMF_PUBLIC);

            if (pciInt)
            {
                pciInt->is_Node.ln_Name = ((struct Node *)cl->UserData)->ln_Name;
                pciInt->is_Node.ln_Pri  = 0;
                pciInt->is_Data         = data;
                pciInt->is_Code         = (APTR)ata_PCI_Interrupt;

                data->irqHandle = pciInt;
                if (HIDD_PCIDevice_AddInterrupt(data->bus->atapb_Device->ref_Device, pciInt))
                {
                    /* Signal structure ownership */
                    data->bus->atapb_Node.ln_Succ = data;
                    return o;
                }

                FreeMem(pciInt, sizeof(struct Interrupt));
            }
        }
        else
        {
            /* Legacy device. Use raw system IRQ. */
            data->irqHandle = KrnAddIRQHandler(data->bus->atapb_INTLine, ata_Raw_Interrupt,
                                               data, NULL);
            if (data->irqHandle)
            {
                data->bus->atapb_Node.ln_Succ = data;
                return o;
            }
        }

        mDispose = msg->mID - moRoot_New + moRoot_Dispose;
        OOP_DoSuperMethod(cl, o, &mDispose);
    }
    return NULL;
}

void DeviceUnref(struct PCIDeviceRef *ref, struct ataBase *base)
{
    ULONG count;

    if (!ref)
        return;

    /*
     * Forbid() because dercement and fetch should be atomic.
     * FIXME: We really need new atomics.
     */
    Forbid();
    count = --ref->ref_Count;
    Permit();

    if (!count)
    {
        HIDD_PCIDevice_Release(ref->ref_Device);
        FreeMem(ref, sizeof(struct PCIDeviceRef));
    }
}

void PCIATA__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ataBase *base = cl->UserData;
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);

    if (data->dmaBuf)
        HIDD_PCIDriver_FreePCIMem(data->pciDriver, data->dmaBuf);

    if (data->bus->atapb_Node.ln_Type == ATABUSNODEPRI_PROBED)
    {
        HIDD_PCIDevice_RemoveInterrupt(data->bus->atapb_Device->ref_Device, data->irqHandle);
        FreeMem(data->irqHandle, sizeof(struct Interrupt));
    }
    else
    {
        KrnRemIRQHandler(data->irqHandle);
    }

    DeviceUnref(data->bus->atapb_Device, base);
    FreeVec(data->bus);

    OOP_DoSuperMethod(cl, o, msg);
}

void PCIATA__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct ataBase *base = cl->UserData;
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_ATABus_Switch(msg->attrID, idx)
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
        *msg->storage = data->bus->atapb_DMABase ? TRUE : FALSE;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

void PCIATA__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct ataBase *base = cl->UserData;
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    struct TagItem *tstate = msg->attrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;

        Hidd_ATABus_Switch(tag->ti_Tag, idx)
        {
        case aoHidd_ATABus_IRQHandler:
            data->ata_HandleIRQ = (APTR)tag->ti_Data;
            break;

        case aoHidd_ATABus_IRQData:
            data->irqData = (APTR)tag->ti_Data;
            break;
        }
    }
}

APTR PCIATA__Hidd_ATABus__GetPIOInterface(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
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

APTR PCIATA__Hidd_ATABus__GetDMAInterface(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    struct dma_data *dma;

    /* If we don't have a DMA buffer, we cannot do DMA */
    if (!data->dmaBuf)
        return NULL;

    dma = (struct dma_data *)OOP_DoSuperMethod(cl, o, msg);
    if (dma)
    {
        dma->au_DMAPort = data->bus->atapb_DMABase;
        dma->ab_PRD     = data->dmaBuf;

        /* Align our buffer */
        if ((0x10000 - ((IPTR)dma->ab_PRD & 0xffff)) < PRD_MAX * sizeof(struct PRDEntry))
       	    dma->ab_PRD = (void*)((((IPTR)dma->ab_PRD)+0xffff) &~ 0xffff);
    }

    return dma;
}

BOOL PCIATA__Hidd_ATABus__SetXferMode(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
#if 0
    /*
     * This code was copied from original ata.device code. There
     * it was disabled because it is complete rubbish. According
     * to specifications, these bits in DMA status register are
     * informational only, and they are set by machine's firmware
     * if it has succesfully configured the drive for DMA operations.
     * Actually, we should modify controller's timing registers here.
     * The problem is that these registers are non-standard, and
     * different controllers have them completely different.
     * Or, perhaps we should simply check these registers here.
     * Currently left as it was.
     */
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
#endif

    return TRUE;
}

void PCIATA__Hidd_ATABus__Shutdown(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    port_t dmaBase = data->bus->atapb_DMABase;

    if (dmaBase)
    {
        /* Shut down DMA */
        outb(inb(dma_Command + dmaBase) & ~DMA_START, dma_Command + dmaBase);
        outl(0, dma_PRD + dmaBase);
    }

    OOP_DoSuperMethod(cl, o, msg);
}
