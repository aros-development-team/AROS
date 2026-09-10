/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    RP1 Southbridge driver for Raspberry Pi 5
*/

/* Bring-up diagnostics: window and peripheral addresses. */
#define DEBUG 1

#define __OOP_NOATTRBASES__

#include <exec/types.h>
#include <inttypes.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include "rp1.h"

#include LC_LIBDEFS_FILE

/*
 * RP1 is an endpoint on the x4 root complex, which pcibcm2712.hidd has
 * already adopted and pci.hidd has enumerated. Everything about the link -
 * config space, the outbound window BAR1 lives in, inbound DMA windows, the
 * MSI block and the MSI-X table - belongs to that driver. What is left here
 * is RP1's own: which of its interrupts go out as which message.
 */

/* Named for proto/oop.h and the aHidd_* macros. */
struct Library *OOPBase;
static OOP_AttrBase HiddPCIDeviceAttrBase;

/* RP1 interrupt-to-message block in BAR1; REG_SET is an atomic set alias.
   Datasheet 6.2, register layout from edk2-platforms Rp1BusDxe. */
#define RP1_PCIE_BASE           0x00108000
#define RP1_PCIE_REG_SET        (RP1_PCIE_BASE + 0x800)
#define RP1_PCIE_MSIX_CFG(irq)  (0x008 + ((irq) * 4))
#define  RP1_MSIX_CFG_ENABLE    (1 << 0)
#define RP1_INT_USBHOST0_0      31
#define RP1_INT_USBHOST1_0      36

/* Interrupt n leaves as message n, so entries 0 through the last USB
   interrupt must exist; the table may hold fewer than the block's 64. */
#define RP1_MSI_MIN             (RP1_INT_USBHOST1_0 + 1)
#define RP1_MSI_MAX             64

AROS_UFH3(static void, rp1_enum,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, dev, A2),
    AROS_UFHA(APTR, unused, A1))
{
    AROS_USERFUNC_INIT

    OOP_Object **found = hook->h_Data;

    if (!*found)
        *found = dev;

    AROS_USERFUNC_EXIT
}

static OOP_Object *find_rp1(void)
{
    OOP_Object *pci, *dev = NULL;
    struct Hook hook = {
        .h_Entry = (IPTR (*)())rp1_enum,
        .h_Data  = &dev,
    };
    struct TagItem req[] = {
        { tHidd_PCI_VendorID,  RP1_PCIE_VENDOR_ID },
        { tHidd_PCI_ProductID, RP1_PCIE_DEVICE_ID },
        { TAG_DONE,            0                  }
    };
    struct pHidd_PCI_EnumDevices msg = {
        .mID          = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
        .callback     = &hook,
        .requirements = req,
    };

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (!pci)
        return NULL;

    OOP_DoMethod(pci, (OOP_Msg)&msg);
    OOP_DisposeObject(pci);

    return dev;
}

static ULONG vector_intid(OOP_Object *dev, ULONG vector)
{
    struct TagItem attr[] = {
        { tHidd_PCIVector_Int, 0 },
        { TAG_DONE,            0 }
    };
    struct pHidd_PCIDevice_GetVectorAttribs msg = {
        .mID      = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_GetVectorAttribs),
        .vectorno = vector,
        .attribs  = attr,
    };

    OOP_DoMethod(dev, (OOP_Msg)&msg);

    return (attr[0].ti_Data == (IPTR)-1) ? 0 : (ULONG)attr[0].ti_Data;
}

/* The bridge fills the MSI-X table with message i in entry i; RP1 is then
   told which of its interrupts to send, and we learn where they land. */
static void setup_msi(OOP_Object *dev, IPTR win, LIBBASETYPEPTR LIBBASE)
{
    struct TagItem req[] = {
        { tHidd_PCIVector_Min, RP1_MSI_MIN },
        { tHidd_PCIVector_Max, RP1_MSI_MAX },
        { TAG_DONE,            0             }
    };
    struct pHidd_PCIDevice_ObtainVectors msg = {
        .mID          = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ObtainVectors),
        .requirements = req,
    };
    volatile uint32_t *set0, *set1;

    if (!OOP_DoMethod(dev, (OOP_Msg)&msg))
    {
        D(bug("[RP1MSI] could not get %u messages for RP1 - no MSI\n", RP1_MSI_MIN));
        return;
    }

    LIBBASE->rp1_USBIrq0 = vector_intid(dev, RP1_INT_USBHOST0_0);
    LIBBASE->rp1_USBIrq1 = vector_intid(dev, RP1_INT_USBHOST1_0);

    set0 = (volatile uint32_t *)(win + RP1_PCIE_REG_SET + RP1_PCIE_MSIX_CFG(RP1_INT_USBHOST0_0));
    set1 = (volatile uint32_t *)(win + RP1_PCIE_REG_SET + RP1_PCIE_MSIX_CFG(RP1_INT_USBHOST1_0));
    *set0 = RP1_MSIX_CFG_ENABLE;
    *set1 = RP1_MSIX_CFG_ENABLE;

    D(bug("[RP1MSI] usb irqs: host0 INTID %u, host1 INTID %u\n",
          (unsigned)LIBBASE->rp1_USBIrq0, (unsigned)LIBBASE->rp1_USBIrq1));

    /* MSIX_CFG.TEST is avoided: it latches through the set alias and
       leaves no rising edge for real interrupts. */
}

static int RP1_Init(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *dev, *drv = NULL;
    IPTR bar1 = 0, win;
    struct TagItem enable[] = {
        { aHidd_PCIDevice_isMEM,    TRUE },
        { aHidd_PCIDevice_isMaster, TRUE },
        { TAG_DONE,                 0    }
    };

    OOPBase = OpenLibrary("oop.library", 0);
    if (!OOPBase)
        return TRUE;

    HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    if (!HiddPCIDeviceAttrBase)
        return TRUE;

    dev = find_rp1();
    if (!dev)
        return TRUE;                    /* not an RP1 board */

    OOP_GetAttr(dev, aHidd_PCIDevice_Driver, (IPTR *)&drv);
    OOP_GetAttr(dev, aHidd_PCIDevice_Base1, &bar1);
    if (!drv || !bar1)
    {
        D(bug("[RP1] endpoint found but BAR1 is unset\n"));
        return TRUE;
    }

    /* BAR1 is a bus address; the bridge maps it and says where it landed. */
    {
        struct pHidd_PCIDriver_MapPCI map = {
            .mID        = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI),
            .PCIAddress = (APTR)bar1,
            .Length     = RP1_BAR1_SIZE,
        };

        win = (IPTR)OOP_DoMethod(drv, (OOP_Msg)&map);
    }

    if (!win)
    {
        D(bug("[RP1] could not map the peripheral window at bus 0x%p\n", (APTR)bar1));
        return TRUE;
    }

    OOP_SetAttrs(dev, enable);

    setup_msi(dev, win, LIBBASE);

    LIBBASE->rp1_BAR1 = win;
    LIBBASE->rp1_PCIDevice = dev;
    LIBBASE->rp1_PCIDriver = drv;
    LIBBASE->rp1_Present = TRUE;

    /* Export peripheral addresses */
    LIBBASE->rp1_USB0  = win + RP1_USB0_OFFSET;
    LIBBASE->rp1_USB1  = win + RP1_USB1_OFFSET;
    LIBBASE->rp1_ETH   = win + RP1_ETH_OFFSET;
    LIBBASE->rp1_GPIO  = win + RP1_GPIO_OFFSET;
    LIBBASE->rp1_I2C0  = win + RP1_I2C0_OFFSET;
    LIBBASE->rp1_I2C1  = win + RP1_I2C1_OFFSET;
    LIBBASE->rp1_UART0 = win + RP1_UART0_OFFSET;

    D(bug("[RP1] window 0x%p (bus 0x%p) USB0=0x%p USB1=0x%p ETH=0x%p GPIO=0x%p I2C0=0x%p I2C1=0x%p UART0=0x%p\n",
          (APTR)win, (APTR)bar1,
          (APTR)LIBBASE->rp1_USB0, (APTR)LIBBASE->rp1_USB1, (APTR)LIBBASE->rp1_ETH,
          (APTR)LIBBASE->rp1_GPIO, (APTR)LIBBASE->rp1_I2C0, (APTR)LIBBASE->rp1_I2C1,
          (APTR)LIBBASE->rp1_UART0));

    return TRUE;
}

ADD2INITLIB(RP1_Init, 0)
