/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 PCIe host bridge driver, private state.

    The register map is in <hardware/bcm2712_pcie.h>; configuration space
    offsets and capability layouts come from <hardware/pci.h>.
*/

#ifndef PCIE_BCM2712_H
#define PCIE_BCM2712_H

#include <inttypes.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <oop/oop.h>

#include <hardware/bcm2712_pcie.h>

#include LC_LIBDEFS_FILE

/* The x4 bridge's MSI block has 64 messages, and RP1 uses all of them. */
#define MIP_MAX_VECTORS                 64

/*
 * Bring-up tracing. None of this can be exercised under emulation - QEMU's
 * raspi models have no PCIe - so the first run on real hardware is also the
 * first test. Leave this on until a controller has been seen to enumerate.
 */
#define PCIE_BRINGUP                    1

#if PCIE_BRINGUP
#define BRINGUP(x)                      x
#else
#define BRINGUP(x)
#endif

/*
 * A translation window, from either "ranges" (outbound) or "dma-ranges"
 * (inbound). Both properties use the same cell layout on this bridge.
 */
struct pcie_range
{
    uint32_t        flags;
    uint64_t        pci_base;
    uint64_t        cpu_base;
    uint64_t        size;
};

/* Each bridge publishes two of each; leave room for a tree that grows one. */
#define PCIE_MAX_RANGES                 4

/* Which bridge a driver object drives, handed to it via aHidd_DriverData. */
struct pcie_bridge
{
    const char         *node;           /* device tree path */
    uint64_t            reg_base;
    const char         *name;
};

/*
 * Per bridge state, the instance data of one driver object.
 *
 * Everything here is read back out of the registers once the bridge is
 * running, whether we programmed it or the firmware did - see BridgeAdopt().
 * Nothing downstream needs to know which of the two happened.
 */
struct PCIBcm2712Data
{
    const struct pcie_bridge *bridge;

    volatile uint8_t   *regs;
    volatile uint8_t   *rescal;
    volatile uint8_t   *reset;

    /* Which bit of the reset controller holds this bridge, from "resets". */
    uint32_t            reset_cell;

    struct pcie_range   ranges[PCIE_MAX_RANGES];
    uint32_t            nranges;
    struct pcie_range   dmaranges[PCIE_MAX_RANGES];
    uint32_t            ndmaranges;

    /* The MEM32 outbound window, which is where endpoint BARs are placed. */
    uint64_t            mem_pci_base;
    uint64_t            mem_cpu_base;
    uint64_t            mem_size;
    uint64_t            mem_next;       /* bump allocator over the window */

    /* The bridge's secondary bus, read from its own config header rather
       than assumed: on a firmware initialised bridge it is already set. */
    UBYTE               secondary_bus;

    /*
     * The GIC INTID the bridge raises for the endpoint's INTA, from the
     * node's interrupt-map. Config space carries no usable interrupt line
     * on this bridge, so this is substituted when one is read.
     */
    uint32_t            intx_irq;

    /*
     * This bridge's MSI block. msi_first_intid already carries the message
     * offset, so message n of ours raises msi_first_intid + n; msi_used is
     * one bit per message handed out.
     */
    volatile uint8_t   *mip;
    uint64_t            msi_target;     /* the PCI address a device writes */
    uint32_t            msi_first_intid;
    uint32_t            msi_count;
    uint64_t            msi_used;

    BOOL                link_up;
    BOOL                trained;        /* we brought it up, firmware had not */
};

/*
 * Per device vector bookkeeping. firstVector holds the first message plus
 * one, so a freshly created device reads as holding none.
 */
struct PCIBcm2712DevData
{
    ULONG               firstVector;
    ULONG               nvectors;
    ULONG               cap;            /* config offset of the capability */
    BOOL                msix;
};

/* Per module state. Only what genuinely has one instance lives here. */
struct pci_staticdata
{
    OOP_Class          *driverClass;
    OOP_Class          *deviceClass;

    OOP_AttrBase        hiddPCIDriverAB;
    OOP_AttrBase        hiddPCIDeviceAB;
    OOP_AttrBase        hiddAB;

    struct Library     *kernelBase;
};

struct pcibcm2712base
{
    struct Library        lib;
    struct pci_staticdata psd;
};

#define PSD(cl) (&((struct pcibcm2712base *)(cl)->UserData)->psd)

/* Shared between the two source files. */
ULONG PCIE_ReadConfig(struct PCIBcm2712Data *data, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg);
void  PCIE_WriteConfig(struct PCIBcm2712Data *data, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val);
BOOL  PCIE_BridgeSetup(struct pci_staticdata *psd, struct PCIBcm2712Data *data);

#endif /* PCIE_BCM2712_H */
