/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI host bridge driver described by the device tree.
*/

#ifndef _PCIDT_H
#define _PCIDT_H

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/interrupts.h>

#include <oop/oop.h>

#include <fdt.h>

#include LC_LIBDEFS_FILE

/*
 * How this bridge's configuration space is reached.
 *
 * ECAM is the flat window the PCI Firmware specification describes:
 * one megabyte per bus, addressed directly. QEMU's virt machine and
 * anything "pci-host-ecam-generic" works this way.
 *
 * DesignWare controllers instead expose a small window which is a
 * viewport onto one device at a time. Its
 * internal address translation unit has to be pointed at the bus,
 * device and function first, so a config access is a register write
 * followed by the read.
 */
enum pcidt_access
{
    PCIDT_ACCESS_ECAM = 0,
    PCIDT_ACCESS_DWC
};

#define PCIDT_MAX_BRIDGES 8

/* Regions out of a bridge's 64-bit window that are remembered */
#define PCIDT_MAX_REGIONS 32

/*
 * How a device's legacy interrupt pin becomes a controller source.
 *
 * The device tree gives an "interrupt-map" - a list of (child address,
 * child interrupt) to (parent controller, parent interrupt) - and an
 * "interrupt-map-mask" saying which parts of that key matter. Firmware
 * here leaves the interrupt line register unprogrammed, so this is the
 * only description of the wiring there is.
 */
/*
 * A tree's interrupt-map masks the device away and lists four entries,
 * one per pin. An ACPI _PRT names every device separately, so the same
 * four sources arrive as one entry per device and pin.
 */
#define PCIDT_MAX_INTMAP 192

struct pcidt_intmap
{
    ULONG   key[4];     /* masked child address (3) and pin (1) */
    ULONG   irq;        /* the source it comes out on           */
};

/* Where the message controller reports what has arrived, relative
   to the DesignWare register block */
#define DWC_MSI_INTR0_STATUS    0x830

/* The controller has 32, which is also as many as DWC_MSI_INTR0_STATUS
   can report in one word. The sources for them come out of a range
   shared with every other bridge, so ask for the whole set and settle
   for less when several are present - the request is halved until it
   fits. With HW_IRQ_COUNT at 1024 there is now room for three bridges
   to have all 32 rather than 8 apiece. */
#define PCIDT_MSI_VECTORS   32

struct pcidt_bridge
{
    fdt_node_t          node;           /* FDT_NONE when found through ACPI */
    IPTR                cfgBase;        /* ECAM window, or DWC viewport */
    IPTR                cfgSize;
    IPTR                dbiBase;        /* DesignWare register block   */
    IPTR                dbiSize;
    UBYTE               busStart;
    UBYTE               busEnd;
    enum pcidt_access   access;
    /* 1 when the translation unit has a register block per region,
       0 when there is one set behind a viewport selector */
    UBYTE               atuUnroll;

    ULONG               intMask[4];
    struct pcidt_intmap intMap[PCIDT_MAX_INTMAP];
    ULONG               intMapCount;

    /* The 64-bit window, when the tree describes one */
    IPTR                mem64PciBase;
    IPTR                mem64CpuBase;
    IPTR                mem64Size;

    /*
     * Sv39 cannot identity-map a CPU address with bit 38 set, and the
     * Titan's 64-bit window sits at 256GB exactly. Regions assigned out
     * of such a window are mapped linearly into a canonical window
     * instead (the same one the kernel remaps the boot framebuffer
     * into), and the driver's MapPCI/PCItoCPU/CPUtoPCI translate.
     * mem64Va is that window's base, equal to mem64CpuBase when the
     * window is identity-mappable.
     */
    IPTR                mem64Va;

    /* The 32-bit memory and I/O windows, for handing out space
       firmware did not */
    IPTR                mem32PciBase;
    IPTR                mem32CpuBase;
    IPTR                mem32Size;
    IPTR                ioPciBase;
    IPTR                ioCpuBase;
    IPTR                ioSize;

    /*
     * The regions assigned out of the 64-bit window, as found when the
     * bridge came up and as moved since. A region is resized and placed
     * by checking against these, which are sized already - sizing a base
     * register again would disturb a device that is in use.
     */
    struct pcidt_region
    {
        UQUAD           addr;           /* on the bus */
        UQUAD           size;
        UBYTE           bus, dev, sub;
        UBYTE           reg;            /* config offset of the register */
    }                   mem64Regions[PCIDT_MAX_REGIONS];
    ULONG               mem64RegionCount;

    /*
     * The controller's own message interrupt. Everything signalled by
     * a message arrives on this one source and is told apart by the
     * status register, so a device asking for a vector is given a bit
     * in that register and this source to be woken on.
     */
    ULONG               index;          /* which entry of psd->bridges */
    ULONG               msiIrq;         /* 0 when the tree names none */
    IPTR                msiTarget;      /* address devices write to   */
    ULONG               msiUsed;        /* one bit per allocated vector */
    UBYTE               msiReady;
    /*
     * The sources the vectors were given, one after another from
     * msiIrqBase. Whoever serves msiIrq fans out to them, so a driver
     * hooking its own is entered only for its own device.
     */
    ULONG               msiIrqBase;
    ULONG               msiVectors;
};

/*
 * Where the firmware describes something on a bridge: a node of the
 * device tree, or one of the ACPI namespace. The objects made for the
 * bridge and for its devices name these as their firmware nodes (see
 * PCIDT_DeclareFirmwareNodes()).
 */
struct pcidt_fwnode
{
    struct MinNode      node;
    APTR                acpi;           /* ACPI_HANDLE, NULL for a tree node */
    fdt_node_t          dt;
    UBYTE               bridge;         /* index into psd->bridges */
    UBYTE               bus, dev, sub;
};

/* In dev: the node describes the bridge itself */
#define PCIDT_FWNODE_HOST   0xff
/* In sub: the node stands for every function of the device */
#define PCIDT_FWNODE_ANYSUB 0xff

struct pcidt_staticdata
{
    struct Library      *OOPBase;
    struct Library      *utilityBase;
    APTR                 kernelBase;
    struct Library      *pciBase;

    OOP_AttrBase         hiddAB;
    OOP_AttrBase         hiddPCIDriverAB;
    OOP_AttrBase         hidd_PCIDeviceAB;

    OOP_MethodID         hidd_PCIDeviceMB;
    OOP_MethodID         hiddMB;

    struct MinList       fwNodes;       /* struct pcidt_fwnode */

    OOP_Class           *pcidtDriverClass;
    OOP_Class           *pcidtDeviceClass;

    /*
     * Bridges found in the device tree. The driver class has no way to
     * be told which one an instance is for - every PCIDriver attribute
     * is read-only - so registration and instantiation walk this list
     * in step, and New() takes the next entry each time. That holds
     * because AddHardwareDriver() creates the instance synchronously,
     * one per call, from our own init.
     */
    struct pcidt_bridge  bridges[PCIDT_MAX_BRIDGES];
    ULONG                bridgeCount;
    ULONG                bridgeNext;
};

/* Per-instance data for a bridge */
struct PCIDTBusData
{
    struct pcidt_bridge  bridge;
};

/* Per-instance data for a device on one of those bridges */
struct PCIDTDeviceData
{
    APTR                 mmconfig;      /* ECAM only: its 4KiB of config */
    UBYTE                bus;
    UBYTE                dev;
    UBYTE                sub;
    ULONG                bridge;        /* index into psd->bridges */
    LONG                 msiVector;     /* -1 until one is obtained */
};

#undef HiddPCIDriverAttrBase
#undef HiddAttrBase
#undef HiddPCIDeviceAttrBase
#undef HiddPCIDeviceBase
#undef HiddBase

#define HiddBase                (PSD(cl)->hiddMB)
#define HiddPCIDriverAttrBase   (PSD(cl)->hiddPCIDriverAB)
#define HiddAttrBase            (PSD(cl)->hiddAB)
#define HiddPCIDeviceAttrBase   (PSD(cl)->hidd_PCIDeviceAB)
#define HiddPCIDeviceBase       (PSD(cl)->hidd_PCIDeviceMB)

/* KernelBase is deliberately not a PSD macro: the discovery code that
   needs it runs outside any method, where there is no `cl` */
#define UtilityBase             (PSD(cl)->utilityBase)
#define OOPBase                 (PSD(cl)->OOPBase)

struct PCIDTBase
{
    struct Library          LibNode;
    struct pcidt_staticdata psd;
};

#define PSD(cl) (&((struct PCIDTBase *)cl->UserData)->psd)

/*
 * Map a region so the CPU can reach it - the boot-time page tables
 * cover RAM only, so nothing device-side answers until this is done.
 */
BOOL  PCIDT_Map(struct pcidt_staticdata *psd, IPTR base, IPTR size);
BOOL  PCIDT_MapAt(struct pcidt_staticdata *psd, IPTR va, IPTR pa, IPTR size);

/*
 * Sv39 identity mapping only reaches the lower 256GB; addresses above
 * it are remapped linearly from this canonical base, the same rule the
 * kernel applies to the boot framebuffer (see krnFBAddr() in
 * kernel_startup.c), so both name one address for the same aperture.
 */
#define PCIDT_SV39_IDENTITY_LIMIT   (1UL << 38)
#define PCIDT_HIGH_WINDOW           0x2000000000UL

/*
 * The other way a machine can describe its host bridges. Reached only
 * when the device tree named none: a board described by a tree never
 * gets here, and one without ACPI tables finds nothing.
 */
ULONG PCIDT_DiscoverACPI(struct pcidt_staticdata *psd);

/*
 * The firmware's description of the bridges and of what sits on them.
 * PCIDT_ScanACPINodes() collects the ACPI side of it for every bridge
 * the MCFG table knows, however the bridge was found; PCIDT_ACPIPath()
 * gives the full name of a node it recorded.
 */
void  PCIDT_RecordFirmwareNode(struct pcidt_staticdata *psd, APTR acpi,
                               fdt_node_t dt, ULONG bridge, UBYTE bus,
                               UBYTE dev, UBYTE sub);
void  PCIDT_ScanACPINodes(struct pcidt_staticdata *psd);
BOOL  PCIDT_ACPIPath(APTR handle, STRPTR buf, ULONG size);
void  PCIDT_DeclareFirmwareNodes(OOP_Class *cl, OOP_Object *o, ULONG bridge,
                                 UBYTE bus, UBYTE dev, UBYTE sub);

/* Config access, shared by the driver class methods */
ULONG PCIDT_ReadConfig(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                       UBYTE sub, UWORD reg);
void  PCIDT_WriteConfig(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                        UBYTE sub, UWORD reg, ULONG val);
void  PCIDT_DropBootCfgWindows(struct pcidt_bridge *b);
void  PCIDT_SetupOutboundWindows(struct pcidt_bridge *b);
void  PCIDT_EnableDMA(struct pcidt_bridge *b, IPTR base, IPTR size);
/*
 * Make sure the controller forwards a span of the 64-bit window, adding
 * outbound windows where none covers it. Returns the regions it enabled,
 * as a mask for PCIDT_DropOutbound(), or ~0 if the span cannot be
 * forwarded (nothing is left enabled then). Bridges that need no address
 * translation forward everything and return 0.
 */
ULONG PCIDT_ForwardOutbound(struct pcidt_bridge *b, IPTR cpu, IPTR pci,
                            IPTR size);
void  PCIDT_DropOutbound(struct pcidt_bridge *b, ULONG regions);
void  PCIDT_DumpLinkState(struct pcidt_bridge *b);
void  PCIDT_DisableMSI(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                       UBYTE sub);
void  PCIDT_DumpIntStatus(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                          UBYTE sub);
void  PCIDT_SetINTx(struct pcidt_bridge *b, UBYTE bus, UBYTE dev, UBYTE sub,
                    BOOL enable);
void  PCIDT_DumpInbound(struct pcidt_bridge *b);
BOOL  PCIDT_MSIInit(struct pcidt_bridge *b, IPTR target);
ULONG PCIDT_MSIAck(struct pcidt_bridge *b);
LONG  PCIDT_MSIEnable(struct pcidt_bridge *b, UBYTE bus, UBYTE dev, UBYTE sub);

#endif /* _PCIDT_H */
