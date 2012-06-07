/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI direct driver for Elbox Mediator series
    Lang: English
*/

#define __OOP_NOATTRBASES__

#define DEBUG 1
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

#define pci_readl(b, d, f, r)       base->cfg_readl(base, b, d, f, r)
#define pci_writel(b, d, f, r, v)   base->cfg_writel(base, b, d, f, r, v)

static ULONG a12k_pci_readl(struct pcibase *base, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    ULONG tmp;
    UWORD offset;

    if (bus >= 1)
        return ~0;

    offset = EMPB_CONF_DEV_STRIDE  * dev +
             EMPB_CONF_FUNC_STRIDE * sub +
             reg;
    offset >>= 2;

    Disable();
    base->setup[EMPB_SETUP_BRIDGE_OFF] = EMPB_BRIDGE_CONF;
    tmp = base->config[offset];
    base->setup[EMPB_SETUP_BRIDGE_OFF] = EMPB_BRIDGE_IO;
    Enable();

    return AROS_LE2LONG(tmp);
}
    
static VOID a12k_pci_writel(struct pcibase *base, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg, ULONG val)
{
    UWORD offset;

    if (bus >= 1)
        return;

    offset = EMPB_CONF_DEV_STRIDE  * dev +
             EMPB_CONF_FUNC_STRIDE * sub +
             reg;
    offset >>= 2;

    val = AROS_LONG2LE(val);

    Disable();
    base->setup[EMPB_SETUP_BRIDGE_OFF] = EMPB_BRIDGE_CONF;
    base->config[offset] = val;
    base->setup[EMPB_SETUP_BRIDGE_OFF] = EMPB_BRIDGE_IO;
    Enable();
}

static BOOL a12k_init(struct pcibase *base)
{
    D(bug("MEDIATOR: Attempting A1200 style init\n"));

    base->setup = base->baseDev->cd_BoardAddr;
    base->config = base->baseDev->cd_BoardAddr + EMPB_BRIDGE_OFF;
    base->cfg_readl = a12k_pci_readl;
    base->cfg_writel = a12k_pci_writel;

    base->io = CreatePCIResourceList((IPTR)base->baseDev->cd_BoardAddr + EMPB_BRIDGE_OFF, EMPB_BRIDGE_SIZE + 1);
    base->mem = CreatePCIResourceList((IPTR)base->memDev->cd_BoardAddr, base->memDev->cd_BoardSize);

    return TRUE;
}

static ULONG a4k_pci_readl(struct pcibase *base, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    ULONG tmp;
    UWORD offset;

    if (bus >= 64)
        return ~0;

    offset = EMZ4_CONF_BUS_STRIDE  * bus +
             EMZ4_CONF_DEV_STRIDE  * dev +
             EMZ4_CONF_FUNC_STRIDE * sub +
             reg;
    offset >>= 2;

    tmp = base->config[offset];

    return AROS_LE2LONG(tmp);
}
    
static VOID a4k_pci_writel(struct pcibase *base, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg, ULONG val)
{
    UWORD offset;

    if (bus >= 64)
        return;

    offset = EMZ4_CONF_BUS_STRIDE  * bus +
             EMZ4_CONF_DEV_STRIDE  * dev +
             EMZ4_CONF_FUNC_STRIDE * sub +
             reg;
    offset >>= 2;

    base->config[offset] = AROS_LONG2LE(val);
}

static BOOL a4k_init(struct pcibase *base)
{
    D(bug("MEDIATOR: Attempting A3000/4000 style init\n"));

    base->setup[EMZ4_SETUP_STATUS_OFF] = 0x00;
    base->setup[EMZ4_SETUP_CONFIG_OFF] = 0x41;

    base->setup = base->baseDev->cd_BoardAddr;
    base->config = base->baseDev->cd_BoardAddr + EMZ4_CONFIG_OFF;

    base->cfg_readl = a4k_pci_readl;
    base->cfg_writel = a4k_pci_writel;

    base->io = CreatePCIResourceList((IPTR)base->baseDev->cd_BoardAddr + EMZ4_IOPORT_OFF, EMZ4_IOPORT_SIZE + 1);
    base->mem = CreatePCIResourceList((IPTR)base->memDev->cd_BoardAddr, base->memDev->cd_BoardSize);

    return TRUE;
}

static void PCIInitialize(struct pcibase *base, int bus, struct MinList *io, struct MinList *mem)
{
    int dev, sub;

    int i;
    D(bug("MEDIATOR: Dumping config area for device 0:\n"));
    for (i = 0; i < 256/4; i++) {
        if ((i % 4) == 0) {
            D(bug("%p:", i));
        }
        D(bug("%c%08x", (i ==2) ? '-' : ' ',pci_readl(0, 0, 0, i)));
        if ((i % 4) == 3) {
            D(bug("\n"));
        }
    }


    /* Initialize all devices attached to this PCIDriver
     *
     * TODO: Handle PCI bridges
     */
    for (dev = 0; dev < 32; dev++) {
        for (sub = 0; sub < 8; sub++) {
            ULONG tmp, venprod;
            int i;
            UBYTE cmd = PCICMF_BUSMASTER;

            venprod = pci_readl(bus, dev, sub, 0);
            if (venprod == 0 || venprod == ~0)
                continue;

            D(bug("MEDIATOR: %d:%02x.%d %04x:%04x\n", bus, dev, sub, venprod & 0xffff, (venprod >> 16) & 0xffff));

            /* Program the 6 BARs */
            for (i = 0; i < 6; i++) {
                ULONG bar, start, size;

                pci_writel(bus, dev, sub, PCICS_BAR0 + i * 4, ~0);
                bar = pci_readl(bus, dev, sub, PCICS_BAR0 + i * 4);
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

                pci_writel(bus, dev, sub, PCICS_BAR0 + i*4, start);

                if ((bar & PCIBAR_MEMTYPE_MASK) == PCIBAR_MEMTYPE_64BIT) {
                    /* Skip the 2nd part of a 64 bit BAR */
                    i++;
                    pci_writel(bus, dev, sub, PCICS_BAR0 + i*4, 0);
                }
            }

            /* Set up the interrupt */
            tmp = pci_readl(bus, dev, sub, PCICS_INT_LINE);
            tmp &= ~0xff;
            tmp |= EMPB_INT;
            pci_writel(bus, dev, sub, PCICS_INT_LINE, tmp);

            /* Enable the device */
            tmp = pci_readl(bus, dev, sub, PCICS_COMMAND);
            tmp &= ~0xff;
            tmp |= cmd;
            pci_writel(bus, dev, sub, PCICS_COMMAND, tmp);

            /* If not multi-function, skip the rest */
            if (sub == 0) {
                tmp = pci_readl(bus, dev, sub, PCICS_CACHELS);
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

            mymsg.mID = msg->mID;
            mymsg.attrList = (struct TagItem *)&mytags;

            if (msg->attrList)
            {
                mytags[2].ti_Tag = TAG_MORE;
                mytags[2].ti_Data = (IPTR)msg->attrList;
            }
         
            msg = &mymsg;

            /* Set up board, based on whether the baseDev is
             * in Zorro III space or not.
             */
            if (baseDev->cd_Rom.er_Flags & ERFF_EXTENDED) {
                a4k_init(BASE(cl));
            } else {
                a12k_init(BASE(cl));
            }
        
            /* Initialize bus 0 */
            PCIInitialize(BASE(cl), 0, BASE(cl)->io, BASE(cl)->mem);

            /* We don't need these resource lists any longer */
            DeletePCIResourceList(BASE(cl)->io);
            DeletePCIResourceList(BASE(cl)->mem);

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
    BASE(cl)->baseDev->cd_Flags |= CDF_CONFIGME;
    BASE(cl)->baseDev->cd_Driver = NULL;
    BASE(cl)->memDev->cd_Flags |= CDF_CONFIGME;
    BASE(cl)->memDev->cd_Driver = NULL;
}

void PCIMediator__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    BASE(cl)->cfg_writel(BASE(cl), msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
}

ULONG PCIMediator__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, 
                                            struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    return BASE(cl)->cfg_readl(BASE(cl), msg->bus, msg->dev, msg->sub, msg->reg);
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
