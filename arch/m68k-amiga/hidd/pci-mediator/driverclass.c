/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI direct driver for Elbox Mediator series
    Lang: English
*/

#define __OOP_NOATTRBASES__

#include <aros/debug.h>

#include <exec/types.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <libraries/configvars.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/expansion.h>

#include <aros/symbolsets.h>

#include "pci.h"
#include "pci_resource.h"
#include "empbreg.h"

#undef HiddPCIDriverAttrBase
#undef HiddAttrBase

#define HiddPCIDriverAttrBase   (PSD(cl)->hiddPCIDriverAB)
#define HiddAttrBase            (PSD(cl)->hiddAB)

static ULONG pcicfg_readl(struct pcibase *base, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    ULONG tmp;
    UWORD offset;

    offset = EMPB_CONF_DEV_STRIDE  * dev +
             EMPB_CONF_FUNC_STRIDE * sub +
             reg;
    offset >>= 2;

    Disable();
    base->setup[EMPB_SETUP_BRIDGE_OFF] = BRIDGE_CONF;
    tmp = base->config[offset];
    base->setup[EMPB_SETUP_BRIDGE_OFF] = BRIDGE_IO;
    Enable();

    return AROS_LE2LONG(tmp);
}
    
static void pcicfg_writel(struct pcibase *base, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg, ULONG val)
{
    UWORD offset;

    offset = EMPB_CONF_DEV_STRIDE  * dev +
             EMPB_CONF_FUNC_STRIDE * sub +
             reg;
    offset >>= 2;

    val = AROS_LONG2LE(val);

    Disable();
    base->setup[EMPB_SETUP_BRIDGE_OFF] = BRIDGE_CONF;
    base->config[offset] = val;
    base->setup[EMPB_SETUP_BRIDGE_OFF] = BRIDGE_IO;
    Enable();
}

static void PCIInitialize(struct pcibase *base)
{
    int dev, sub;
    const int bus = 0;

    /* Initialize all devices attached to this PCIDriver
     *
     * Since the Mediator can't do Type 1 config cycles,
     * we only need to scan Bus 0.
     */
    for (dev = 0; dev < 32; dev++) {
        for (sub = 0; sub < 8; sub++) {
            ULONG tmp, venprod;
            int i;
            UBYTE cmd = PCICMF_BUSMASTER;

            venprod = pcicfg_readl(base, bus, dev, sub, 0);
            if (venprod == 0 || venprod == ~0)
                continue;

            D(bug("MEDIATOR: %d:%02x.%d %04x:%04x\n", bus, dev, sub, venprod & 0xffff, (venprod >> 16) & 0xffff));

            /* Program the 6 BARs */
            for (i = 0; i < 6; i++) {
                ULONG bar, start, size;

                pcicfg_writel(base, bus, dev, sub, PCICS_BAR0 + i * 4, ~0);
                bar = pcicfg_readl(base, bus, dev, sub, PCICS_BAR0 + i * 4);
                size = bar & ((bar & PCIBAR_MASK_TYPE) ? PCIBAR_MASK_IO : PCIBAR_MASK_MEM);
                size = (~size + 1);
                if (size == 0)
                    continue;

                if ((bar & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_IO) {
                    start = AllocPCIResource(base->io, size);
                    cmd |= PCICMF_IODECODE;
                } else {
                    start = AllocPCIResource(base->mem, size);
                    cmd |= PCICMF_MEMDECODE;
                }

                if (start == ~0) {
                    D(bug("\tBAR%d: %s - %d bytes unallocated\n", i, (bar & PCIBAR_MASK_TYPE) ? "IO" : "Mem", size));
                    cmd = 0;

                    break;
                }

                D(bug("\tBAR%d: %s 0x%x-0x%x\n", i, (bar & PCIBAR_MASK_TYPE) ? "IO" : "Mem", start, start + size - 1));

                pcicfg_writel(base, bus, dev, sub, PCICS_BAR0 + i*4, start);

                if ((bar & PCIBAR_MEMTYPE_MASK) == PCIBAR_MEMTYPE_64BIT) {
                    /* Skip the 2nd part of a 64 bit BAR */
                    i++;
                    pcicfg_writel(base, bus, dev, sub, PCICS_BAR0 + i*4, 0);
                }
            }

            /* Set up the interrupt */
            tmp = pcicfg_readl(base, bus, dev, sub, PCICS_INT_LINE);
            tmp &= ~0xff;
            tmp |= EMPB_INT;
            pcicfg_writel(base, bus, dev, sub, PCICS_INT_LINE, tmp);

            /* Enable the device */
            tmp = pcicfg_readl(base, bus, dev, sub, PCICS_COMMAND);
            tmp &= ~0xff;
            tmp |= cmd;
            pcicfg_writel(base, bus, dev, sub, PCICS_COMMAND, tmp);

            /* If not multi-function, skip the rest */
            if (sub == 0) {
                tmp = pcicfg_readl(base, bus, dev, sub, PCICS_CACHELS);
                tmp >>= 16;
                tmp &= 0xff;
                if (!(tmp & PCIHT_MULTIFUNC))
                    break;
            }
        }
    }
}

/*
    We overload the New method in order to introduce the Hidd Name and
    HardwareName attributes.
*/
OOP_Object *PCIMediator__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New mymsg;
    struct ConfigDev *baseDev, *memDev;
    struct CurrentBinding cb;
    struct Library *ExpansionBase;
    
    struct TagItem mytags[] = {
        { aHidd_Name, (IPTR)"PCIMediator" },
        { aHidd_HardwareName, (IPTR)"Elbox Mediator PCI driver" },
        { TAG_DONE, 0 }
    };

    if (!(ExpansionBase = TaggedOpenLibrary(TAGGEDOPEN_EXPANSION))) {
        OOP_DisposeObject(o);
        return NULL;
    }

    if (GetCurrentBinding(&cb, sizeof(cb)) == sizeof(cb)) {
        baseDev = cb.cb_ConfigDev;
        D(bug("MEDIATOR: Configuring for %d/%d @%p\n", baseDev->cd_Rom.er_Manufacturer, baseDev->cd_Rom.er_Product, baseDev->cd_BoardAddr));
        if ((memDev = FindConfigDev(baseDev, baseDev->cd_Rom.er_Manufacturer, baseDev->cd_Rom.er_Product | 0x80))) {
            D(bug("MEDIATOR: Found companion board\n"));

            baseDev->cd_Flags &= ~CDF_CONFIGME;
            baseDev->cd_Driver = BASE(cl);

            memDev->cd_Flags &= ~CDF_CONFIGME;
            memDev->cd_Driver = BASE(cl);

            BASE(cl)->baseDev = baseDev;
            BASE(cl)->memDev = memDev;

            BASE(cl)->setup = baseDev->cd_BoardAddr + EMPB_SETUP_OFF;
            BASE(cl)->config = baseDev->cd_BoardAddr + EMPB_BRIDGE_OFF;
            BASE(cl)->io = CreatePCIResourceList((IPTR)baseDev->cd_BoardAddr + EMPB_BRIDGE_OFF, EMPB_BRIDGE_SIZE + 1);
            BASE(cl)->mem = CreatePCIResourceList((IPTR)memDev->cd_BoardAddr, memDev->cd_BoardSize);

            mymsg.mID = msg->mID;
            mymsg.attrList = (struct TagItem *)&mytags;

            if (msg->attrList)
            {
                mytags[2].ti_Tag = TAG_MORE;
                mytags[2].ti_Data = (IPTR)msg->attrList;
            }
         
            msg = &mymsg;
         
            PCIInitialize(BASE(cl));

            o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        } else {
            OOP_DisposeObject(o);
        }
    } else {
        OOP_DisposeObject(o);
    }

    CloseLibrary(ExpansionBase);

    return o;
}

VOID PCIMediator__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    DeletePCIResourceList(BASE(cl)->io);
    DeletePCIResourceList(BASE(cl)->mem);
    BASE(cl)->baseDev->cd_Flags |= CDF_CONFIGME;
    BASE(cl)->baseDev->cd_Driver = NULL;
    BASE(cl)->memDev->cd_Flags |= CDF_CONFIGME;
    BASE(cl)->memDev->cd_Driver = NULL;
}

void PCIMediator__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    pcicfg_writel(BASE(cl), msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
}

ULONG PCIMediator__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, 
                                            struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    return pcicfg_readl(BASE(cl), msg->bus, msg->dev, msg->sub, msg->reg);
}

/* Class initialization and destruction */
static int PCIMediator_InitClass(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *pci;
    
    struct pHidd_PCI_AddHardwareDriver msg,*pmsg=&msg;

    D(bug("PCIMediator: Driver initialization\n"));

    LIBBASE->psd.hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    LIBBASE->psd.hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    if (LIBBASE->psd.hiddPCIDriverAB == 0 || LIBBASE->psd.hiddAB == 0)
    {
        D(bug("PCIMediator: ObtainAttrBases failed\n"));
        return FALSE;
    }

    msg.driverClass = LIBBASE->psd.driverClass;
    msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);
    D(bug("PCIMediator: Adding Driver to main the class OK\n"));

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    OOP_DoMethod(pci, (OOP_Msg)pmsg);
    OOP_DisposeObject(pci);

    D(bug("PCIMediator: All OK\n"));

    return TRUE;
}

static int PCIMediator_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("PCIMediator: Class destruction\n"));
    
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);
    
    return TRUE;
}
        
ADD2INITLIB(PCIMediator_InitClass, 0)
ADD2EXPUNGELIB(PCIMediator_ExpungeClass, 0)
