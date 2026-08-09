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

struct pcidt_bridge
{
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

    /* The 32-bit memory and I/O windows, for handing out space
       firmware did not */
    IPTR                mem32PciBase;
    IPTR                mem32CpuBase;
    IPTR                mem32Size;
    IPTR                ioPciBase;
    IPTR                ioCpuBase;
    IPTR                ioSize;

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
    struct Interrupt    msiAck;
};

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

/*
 * The other way a machine can describe its host bridges. Reached only
 * when the device tree named none: a board described by a tree never
 * gets here, and one without ACPI tables finds nothing.
 */
ULONG PCIDT_DiscoverACPI(struct pcidt_staticdata *psd);

/* Config access, shared by the driver class methods */
ULONG PCIDT_ReadConfig(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                       UBYTE sub, UWORD reg);
void  PCIDT_WriteConfig(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                        UBYTE sub, UWORD reg, ULONG val);
void  PCIDT_DropBootCfgWindows(struct pcidt_bridge *b);
void  PCIDT_EnableDMA(struct pcidt_bridge *b, IPTR base, IPTR size);
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
