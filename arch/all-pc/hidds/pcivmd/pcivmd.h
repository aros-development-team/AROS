/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Intel Volume Management Device (VMD) PCI bus driver.
*/

#ifndef _PCIVMD_H
#define _PCIVMD_H

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/interrupts.h>

#include <oop/oop.h>

#include LC_LIBDEFS_FILE

/*
 * A VMD is an ordinary PCI endpoint that hides a whole PCI domain behind
 * itself. Its three base registers are the only way to reach that domain:
 *
 *   CFGBAR  (BAR0) - ECAM style configuration space of the hidden buses
 *   MEMBAR1 (BAR2) - memory window the hidden devices' BARs live in
 *   MEMBAR2 (BAR4) - second memory window; its first 0x2000 bytes hold
 *                    the VMD's own MSI-X table and pending bits
 *
 * Legacy INTx is not forwarded out of the domain. Message signalled
 * interrupts of the hidden devices are captured by the VMD and re-raised
 * as one of its own MSI-X vectors, selected by the destination ID field
 * of the address the hidden device was programmed with.
 */

/* VMD configuration space registers */
#define VMD_REG_VMCAP                   0x40
#define VMD_REG_VMCONFIG                0x44
#define VMD_REG_VMLOCK                  0x70

#define VMCAP_BUS_RESTRICT              (1 << 0)
#define VMCONFIG_MSI_REMAP_OFF          (1 << 1)
#define VMCONFIG_BUS_RESTRICT(x)        (((x) >> 8) & 0x3)
#define VMLOCK_MEMBAR_SHADOW            (1 << 1)

/* Base register indices of the VMD endpoint */
#define VMD_CFGBAR                      0
#define VMD_MEMBAR1                     2
#define VMD_MEMBAR2                     4

/* MEMBAR2 layout */
#define VMD_MB2_MSIX_SIZE               0x2000
#define VMD_MB2_SHADOW_OFFSET           0x2000
#define VMD_MB2_SHADOW_SIZE             16

/* "SHDW" signature of the vendor specific shadow capability */
#define VMD_VSCAP_SHADOW_SIG            0x53484457

/* Per-device feature flags */
#define VMDF_MEMBAR_SHADOW              (1 << 0)    /* shadow registers in MEMBAR2      */
#define VMDF_BUS_RESTRICTIONS           (1 << 1)    /* first bus is set by VMCONFIG     */
#define VMDF_MEMBAR_SHADOW_VSCAP        (1 << 2)    /* shadow registers in a capability */
#define VMDF_OFFSET_FIRST_VECTOR        (1 << 3)    /* vector 0 is the VMD's own        */
#define VMDF_CAN_BYPASS_MSI_REMAP       (1 << 4)    /* remapping can be turned off      */

/* MSI address the hidden devices are given; bits 19:12 select the VMD vector */
#define VMD_MSI_ADDR(vec)               (0xFEE00000u | ((ULONG)(vec) << 12))

/* How many of the VMD's own vectors we ask for, and can track */
#define VMD_MAX_VECTORS                 8
/* How many vectors a single hidden device can be given */
#define VMD_CHILD_MAXVECS               32

/* PCI Express capability: device/port types with one device per link */
#define PCIE_TYPE_ROOT_PORT             0x4
#define PCIE_TYPE_DOWNSTREAM            0x6
#define PCIE_TYPE_PCIE_BRIDGE           0x8

struct VMDVector
{
    ULONG                       irq;            /* system IRQ               */
    ULONG                       native;         /* CPU vector               */
    ULONG                       users;          /* hidden vectors routed    */
};

/* One of the memory windows the VMD forwards into the domain */
struct VMDWindow
{
    UQUAD                       cpubase;        /* where the CPU sees it    */
    UQUAD                       busbase;        /* where the devices see it */
    UQUAD                       size;
    UQUAD                       next;           /* allocation cursor (bus)  */
    UQUAD                       limit;          /* end of allocatable (bus) */
};

struct pcivmd_staticdata;

struct VMDDomain
{
    struct MinNode              vd_Node;
    struct pcivmd_staticdata    *vd_psd;
    OOP_Object                  *vd_Device;     /* the VMD endpoint         */
    OOP_Object                  *vd_ParentDrv;  /* the bus driver it is on  */
    UWORD                       vd_Product;
    ULONG                       vd_Features;

    volatile UBYTE              *vd_CfgBar;
    UQUAD                       vd_CfgSize;
    UBYTE                       vd_BusStart;    /* real number of bus 0     */
    UWORD                       vd_BusCount;
    UWORD                       vd_LastBus;     /* highest bus handed out   */
    UBYTE                       vd_OneDev[32];  /* buses with only device 0 */

    struct VMDWindow            vd_Win[2];
    struct VMDWindow            *vd_Lo;         /* below 4GB                */
    struct VMDWindow            *vd_Hi;         /* anywhere                 */

    struct VMDVector            vd_Vec[VMD_MAX_VECTORS];
    UWORD                       vd_VecCount;
    UWORD                       vd_VecFirst;
#if defined(__AROSEXEC_SMP__)
    spinlock_t                  vd_CfgLock;
#endif
};

struct pcivmd_staticdata
{
    struct Library              *OOPBase;
    struct Library              *utilityBase;
    APTR                        kernelBase;

    OOP_AttrBase                hiddAB;
    OOP_AttrBase                hiddPCIDriverAB;
    OOP_AttrBase                hiddPCIDeviceAB;

    OOP_MethodID                hiddPCIMB;
    OOP_MethodID                hiddPCIDriverMB;
    OOP_MethodID                hiddPCIDeviceMB;
    OOP_MethodID                hwMB;

    OOP_Class                   *vmdDriverClass;
    OOP_Class                   *vmdDeviceClass;

    struct MinList              domains;
};

struct PCIVMDBase
{
    struct Library              LibNode;
    struct pcivmd_staticdata    psd;
};

#define PSD(cl)                 (&((struct PCIVMDBase *)cl->UserData)->psd)

/* Everything is reached through a local "psd" */
#undef HiddAttrBase
#undef HiddPCIDriverAttrBase
#undef HiddPCIDeviceAttrBase
#undef HiddPCIBase
#undef HiddPCIDriverBase
#undef HiddPCIDeviceBase
#undef HWBase

#define HiddAttrBase            (psd->hiddAB)
#define HiddPCIDriverAttrBase   (psd->hiddPCIDriverAB)
#define HiddPCIDeviceAttrBase   (psd->hiddPCIDeviceAB)
#define HiddPCIBase             (psd->hiddPCIMB)
#define HiddPCIDriverBase       (psd->hiddPCIDriverMB)
#define HiddPCIDeviceBase       (psd->hiddPCIDeviceMB)
#define HWBase                  (psd->hwMB)

#define KernelBase              (psd->kernelBase)
#define UtilityBase             (psd->utilityBase)
#define OOPBase                 (psd->OOPBase)

struct VMDBusData
{
    struct VMDDomain            *domain;
};

#define VMDVEC_NONE             0
#define VMDVEC_MSI              1
#define VMDVEC_MSIX             2

struct VMDDeviceData
{
    struct VMDDomain            *domain;
    UBYTE                       bus, dev, sub;
    UBYTE                       vecMode;
    UBYTE                       vecCount;
    UBYTE                       vecMap[VMD_CHILD_MAXVECS];
};

/* MSI-X table entry */
struct vmd_msix_entry
{
    ULONG                       msg_addr_lo;
    ULONG                       msg_addr_hi;
    ULONG                       msg_data;
    ULONG                       vector_ctrl;
} __attribute__((packed));

/* pcivmd_hw.c */
ULONG VMD_RawRead(struct VMDDomain *dom, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, UBYTE len);
void VMD_RawWrite(struct VMDDomain *dom, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, UBYTE len, ULONG val);
ULONG VMD_ReadConfig(struct VMDDomain *dom, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, UBYTE len);
void VMD_WriteConfig(struct VMDDomain *dom, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, UBYTE len, ULONG val);
APTR VMD_ConfigAddr(struct VMDDomain *dom, UBYTE bus, UBYTE dev, UBYTE sub);
UBYTE VMD_FindCapability(struct VMDDomain *dom, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE capid);
APTR VMD_MapBus(struct VMDDomain *dom, UQUAD busaddr, ULONG length);
struct VMDDomain *VMD_CreateDomain(struct pcivmd_staticdata *psd, OOP_Object *device, ULONG features);

/* pcivmd_deviceclass.c */
BOOL VMD_ObtainVectors(struct VMDDeviceData *data, UWORD vectmin, UWORD vectmax);
void VMD_ReleaseVectors(struct VMDDeviceData *data);

#endif /* _PCIVMD_H */
