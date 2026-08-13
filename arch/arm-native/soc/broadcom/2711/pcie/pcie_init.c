/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2711 PCIe host bridge bring-up and driver registration.
*/

#define __OOP_NOATTRBASES__

#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/mbox.h>
#include <proto/openfirmware.h>
#include <proto/bootloader.h>

#include <aros/bootloader.h>

#define __NOLIBBASE__
#include <proto/kernel.h>

IPTR __arm_periiobase __attribute__((used)) = 0;
#define ARM_PERIIOBASE __arm_periiobase

#include <hardware/bcm2708.h>
#include <hardware/videocore.h>

#include <aros/macros.h>

#include <string.h>

#include "pcie.h"

#include <aros/debug.h>

APTR MBoxBase;
void *OpenFirmwareBase;
APTR BootLoaderBase;

/* "PCIE=disable" on the kernel command line keeps the bridge untouched. */
static BOOL PCIeEnabled(void)
{
    struct List *list;
    struct Node *node;

    BootLoaderBase = OpenResource("bootloader.resource");
    if (!BootLoaderBase)
        return TRUE;

    list = (struct List *)GetBootInfo(BL_Args);
    if (!list)
        return TRUE;

    ForeachNode(list, node)
    {
        if (strncmp(node->ln_Name, "PCIE=", 5) == 0 &&
            strstr(&node->ln_Name[5], "disable"))
        {
            D(bug("[PCIBcm2711] disabled on the command line\n"));
            return FALSE;
        }
    }

    return TRUE;
}

/*
 * The bridge registers raise an external abort when the block is absent
 * (emulators mark the device tree node disabled instead of modelling
 * it), so consult the tree before the first register access.
 */
static BOOL PCIeNodeUsable(void)
{
    void *key, *prop;
    const char *val;

    OpenFirmwareBase = OpenResource("openfirmware.resource");
    if (!OpenFirmwareBase)
        return FALSE;

    key = OF_OpenKey("/scb/pcie@7d500000");
    if (!key)
    {
        D(bug("[PCIBcm2711] no PCIe node in the device tree\n"));
        return FALSE;
    }

    /* OF_OpenKey falls back to the last resolved node; make sure this
     * really is the PCIe controller. */
    prop = OF_FindProperty(key, "compatible");
    if (!prop)
    {
        D(bug("[PCIBcm2711] PCIe node has no compatible property\n"));
        return FALSE;
    }
    val = OF_GetPropValue(prop);
    if (!val || !strstr(val, "2711-pcie"))
    {
        D(bug("[PCIBcm2711] node is not the PCIe controller (%s)\n", val ? val : "?"));
        return FALSE;
    }

    prop = OF_FindProperty(key, "status");
    if (prop)
    {
        val = OF_GetPropValue(prop);
        if (val && strcmp(val, "okay") != 0)
        {
            D(bug("[PCIBcm2711] node status '%s', leaving the bridge alone\n", val));
            return FALSE;
        }
    }

    return TRUE;
}

/*
 * Read the inbound mapping the firmware published: dma-ranges gives the
 * bus address that system memory appears at, which is not the system
 * address itself on a board whose memory does not fit below the window.
 */
/*
 * The firmware picks both the bus-address offset and how much of memory a
 * bus master may reach, to suit the RAM fitted, and publishes them in
 * dma-ranges. Read both rather than assuming: the inbound window has to be
 * a power of two covering the reachable span, and a board with a different
 * split needs a different one.
 */
static void DMARangesFromDT(uint64_t *offset, uint32_t *exp)
{
    void *key, *prop;
    const uint32_t *r;
    uint64_t pci, cpu, size;
    uint32_t shift;

    *offset = 0;
    *exp = BCM2711_DMA_EXP;

    if (!OpenFirmwareBase)
        return;

    key = OF_FindNodeByCompatible(NULL, "brcm,bcm2711-pcie");
    if (!key)
        return;

    prop = OF_FindProperty(key, "dma-ranges");
    if (!prop || OF_GetPropLen(prop) < 7 * 4)
        return;

    /* <pci flags, pci hi, pci lo, cpu hi, cpu lo, size hi, size lo> */
    r = (const uint32_t *)OF_GetPropValue(prop);
    pci  = ((uint64_t)AROS_BE2LONG(r[1]) << 32) | AROS_BE2LONG(r[2]);
    cpu  = ((uint64_t)AROS_BE2LONG(r[3]) << 32) | AROS_BE2LONG(r[4]);
    size = ((uint64_t)AROS_BE2LONG(r[5]) << 32) | AROS_BE2LONG(r[6]);

    *offset = pci - cpu;

    for (shift = 0; shift < 64 && ((uint64_t)1 << shift) < size; shift++)
        ;

    /* RC_BAR_SIZE() encodes 64KB..64GB; anything else means we misread the
       property, so keep the value that suits the boards we know. */
    if (shift >= 16 && shift <= 36)
        *exp = shift;

    D(bug("[PCIBcm2711] dma-ranges: bus %08x%08x <- cpu %08x%08x, size %08x%08x (2^%u)\n",
          (unsigned)(pci >> 32), (unsigned)pci,
          (unsigned)(cpu >> 32), (unsigned)cpu,
          (unsigned)(size >> 32), (unsigned)size, (unsigned)*exp));
}

/*
 * Which of the node's interrupt outputs is the MSI one. The "interrupts"
 * property lists GIC specifiers of three cells each - <type, number, flags>,
 * SPIs as type 0 - and "interrupt-names" names them in the same order.
 */
static void MSIIrqFromDT(struct pci_staticdata *psd)
{
    void *key, *prop;
    const uint32_t *ints;
    const char *names, *n;
    uint32_t nints, idx, len;

    psd->msi_irq = BCM2711_PCIE_MSI;

    if (!OpenFirmwareBase)
        return;

    key = OF_FindNodeByCompatible(NULL, "brcm,bcm2711-pcie");
    if (!key)
        return;

    prop = OF_FindProperty(key, "interrupts");
    if (!prop)
        return;
    ints = (const uint32_t *)OF_GetPropValue(prop);
    nints = OF_GetPropLen(prop) / (3 * 4);

    prop = OF_FindProperty(key, "interrupt-names");
    if (!prop)
        return;
    names = (const char *)OF_GetPropValue(prop);
    len = OF_GetPropLen(prop);

    /*
     * The names sit back to back, each NUL terminated - but the property
     * comes from firmware, so the terminator is looked for inside it rather
     * than trusted to be there.
     */
    for (idx = 0, n = names; (idx < nints) && (n < names + len); idx++)
    {
        const char *end = memchr(n, '\0', (names + len) - n);

        if (!end)
            break;      /* unterminated: the rest of the property is not names */

        if (strcmp(n, "msi") == 0)
        {
            psd->msi_irq = GIC_SPI_BASE + AROS_BE2LONG(ints[idx * 3 + 1]);
            BRINGUP(bug("[PCIBcm2711] MSI output -> SPI %u (INTID %u)\n",
                        (unsigned)AROS_BE2LONG(ints[idx * 3 + 1]),
                        (unsigned)psd->msi_irq));
            return;
        }

        n = end + 1;
    }

    BRINGUP(bug("[PCIBcm2711] no \"msi\" entry in interrupt-names, assuming INTID %u\n",
                (unsigned)psd->msi_irq));
}

static inline uint32_t rd32(volatile uint8_t *regs, uint32_t reg)
{
    return *(volatile uint32_t *)(regs + reg);
}

static inline void wr32(volatile uint8_t *regs, uint32_t reg, uint32_t val)
{
    *(volatile uint32_t *)(regs + reg) = val;
}

/*
 * All 32 vectors funnel into this one interrupt; what distinguishes them
 * is the bit the bridge set in INTR2_INT_STATUS. The status is cleared
 * here - the endpoint handlers hang off the same interrupt and run after
 * us, and a level output nobody clears never falls.
 */
static void PCIeMSIHandler(void *data, void *unused)
{
#if PCIE_BRINGUP
    static uint32_t seen = 0;
#endif
    struct pci_staticdata *psd = data;
    uint32_t status = rd32(psd->regs, PCIE_MSI_INTR2_INT_STATUS);

    if (status)
    {
        wr32(psd->regs, PCIE_MSI_INTR2_INT_CLR, status);
#if PCIE_BRINGUP
        /* The first few prove delivery; after that they are just traffic. */
        if (seen < 4)
        {
            seen++;
            bug("[PCIBcm2711] MSI vectors %08x (%u)\n", (unsigned)status, (unsigned)seen);
        }
#endif
    }
}

/* Busy wait on the generic counter; init runs before timer.device. */
static void delay_us(uint32_t us)
{
    uint64_t freq, now, end;

    asm volatile("mrs %0, CNTFRQ_EL0" : "=r"(freq));
    asm volatile("mrs %0, CNTPCT_EL0" : "=r"(now));
    end = now + (freq * us) / 1000000;

    do {
        asm volatile("mrs %0, CNTPCT_EL0" : "=r"(now));
    } while (now < end);
}

/*
 * Ask the firmware to load the VL805 xHCI controller firmware. On
 * boards whose bootloader EEPROM already carries it this returns
 * without effect. The argument encodes the controller's PCI address.
 */
static void NotifyXHCIReset(void)
{
    unsigned int *msg_, *msg;

    if ((MBoxBase = OpenResource("mbox.resource")) == NULL)
        return;

    /* Own our cache lines; see <proto/mbox.h>. */
    msg_ = AllocMem(MBOX_MSG_ALIGN + (MBOX_MSG_ALIGN - 1), MEMF_CLEAR);
    if (!msg_)
        return;
    msg = (unsigned int *)(((uintptr_t)msg_ + (MBOX_MSG_ALIGN - 1)) & ~(uintptr_t)(MBOX_MSG_ALIGN - 1));

    msg[0] = AROS_LONG2LE(7 * 4);
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(VCTAG_NOTIFYXHCI);
    msg[3] = AROS_LONG2LE(4);
    msg[4] = AROS_LONG2LE(4);
    msg[5] = AROS_LONG2LE(EXT_CFG_ADDR(1, 0, 0));
    msg[6] = 0;

    MBoxWrite((APTR)VCMB_BASE, VCMB_PROPCHAN, msg);
    if (MBoxRead((APTR)VCMB_BASE, VCMB_PROPCHAN) == msg)
        BRINGUP(bug("[PCIBcm2711] VL805 firmware notify: status %08x, tag %08x, value %08x\n",
              AROS_LE2LONG(msg[1]), AROS_LE2LONG(msg[4]), AROS_LE2LONG(msg[5])));
    else
        BRINGUP(bug("[PCIBcm2711] VL805 firmware notify got no reply\n"));

    FreeMem(msg_, MBOX_MSG_ALIGN + (MBOX_MSG_ALIGN - 1));
}

static BOOL BridgeInit(struct pci_staticdata *psd)
{
    volatile uint8_t *regs = psd->regs;
    uint32_t tmp;
    int i;

    /*
     * The register core may be held in SW_INIT out of power-on; while it
     * is, everything but the reset block answers with a bus error, so the
     * reset block is the only thing that may be read first.
     *
     * If the firmware has already trained the link, leave it alone. Taking
     * the bus through reset costs the attached controller the firmware the
     * bootloader loaded into it, and with it the power it supplies to the
     * ports.
     */
    tmp = rd32(regs, PCIE_RGR1_SW_INIT_1);
    if (!(tmp & PCIE_RGR1_SW_INIT_1_INIT))
    {
        uint32_t status = rd32(regs, PCIE_MISC_PCIE_STATUS);

        D(bug("[PCIBcm2711] bridge is already running, status %08x\n", status));

        if ((status & (PCIE_MISC_PCIE_STATUS_PCIE_PHYLINKUP | PCIE_MISC_PCIE_STATUS_PCIE_DL_ACTIVE)) ==
            (PCIE_MISC_PCIE_STATUS_PCIE_PHYLINKUP | PCIE_MISC_PCIE_STATUS_PCIE_DL_ACTIVE))
        {
            psd->preinitialised = TRUE;
            D(bug("[PCIBcm2711] link already trained, keeping it\n"));
        }
    }

    if (!psd->preinitialised)
    {
        D(bug("[PCIBcm2711] resetting the bridge\n"));
        wr32(regs, PCIE_RGR1_SW_INIT_1, PCIE_RGR1_SW_INIT_1_PERST | PCIE_RGR1_SW_INIT_1_INIT);
        delay_us(200);

        /* Release the bridge core, keep PERST# asserted */
        wr32(regs, PCIE_RGR1_SW_INIT_1, PCIE_RGR1_SW_INIT_1_PERST);
        delay_us(200);

        D(bug("[PCIBcm2711] bridge revision %08x\n", rd32(regs, PCIE_MISC_REVISION)));

        /* Power up the PHY */
        tmp = rd32(regs, PCIE_HARD_DEBUG);
        wr32(regs, PCIE_HARD_DEBUG, tmp & ~PCIE_HARD_DEBUG_SERDES_IDDQ);
        delay_us(200);
    }

    /*
     * SCB/inbound setup: one region at PCI address 0. The size follows
     * the DMA range the SoC allows a bus master to reach, rounded up to
     * a power of two, not the memory that happens to be fitted: on this
     * SoC that range is 3GB and the window is therefore 4GB.
     */
    wr32(regs, PCIE_MISC_MISC_CTRL,
         PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN | PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE |
         PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_128 | MISC_CTRL_SCB0_SIZE(psd->dma_exp));

    wr32(regs, PCIE_MISC_RC_BAR2_CONFIG_LO,
         (uint32_t)psd->dma_offset | RC_BAR_SIZE(psd->dma_exp));
    wr32(regs, PCIE_MISC_RC_BAR2_CONFIG_HI, (uint32_t)(psd->dma_offset >> 32));

    /*
     * Present the inbound window in the byte order the rest of the system
     * uses. Left as it comes out of reset, everything a bus master reads
     * from memory arrives swapped.
     */
    tmp = rd32(regs, PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1);
    tmp = (tmp & ~PCIE_RC_CFG_VENDOR_ENDIAN_BAR2_MASK) |
          PCIE_RC_CFG_VENDOR_ENDIAN_BAR2_LITTLE;
    wr32(regs, PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1, tmp);


    wr32(regs, PCIE_MISC_RC_BAR1_CONFIG_LO, 0);
    wr32(regs, PCIE_MISC_RC_BAR3_CONFIG_LO, 0);

    /*
     * Aim the MSI matcher and open all 32 vectors. Nothing signals yet -
     * a device only writes here once ObtainVectors has programmed its MSI
     * capability - but the target has to be standing before that happens.
     */
    wr32(regs, PCIE_MISC_MSI_BAR_CONFIG_LO,
         (uint32_t)BCM2711_MSI_TARGET | PCIE_MISC_MSI_BAR_CONFIG_LO_EN);
    wr32(regs, PCIE_MISC_MSI_BAR_CONFIG_HI, (uint32_t)(BCM2711_MSI_TARGET >> 32));
    wr32(regs, PCIE_MISC_MSI_DATA_CONFIG, PCIE_MISC_MSI_DATA_CONFIG_32);
    wr32(regs, PCIE_MSI_INTR2_INT_CLR, 0xffffffff);
    wr32(regs, PCIE_MSI_INTR2_INT_MASK_CLR, 0xffffffff);

    /* Outbound window: CPU 0x6_0000_0000 -> PCI 0xC0000000, 64MB */
    wr32(regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO, BCM2711_PCIE_PCI_WIN);
    wr32(regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI, 0);
    {
        uint32_t base_mb  = BCM2711_PCIE_CPU_WIN >> 20;
        uint32_t limit_mb = (BCM2711_PCIE_CPU_WIN + BCM2711_PCIE_WIN_SIZE - 1) >> 20;

        wr32(regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT,
             ((base_mb & 0xfff) << 4) | ((limit_mb & 0xfff) << 20));
        wr32(regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI, base_mb >> 12);
        wr32(regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI, limit_mb >> 12);
    }

    /* Deassert PERST# and wait for the link */
    if (!psd->preinitialised)
    {
        D(bug("[PCIBcm2711] releasing PERST#\n"));
        wr32(regs, PCIE_RGR1_SW_INIT_1, 0);
    }

    for (i = 0; i < 100; i++)
    {
        tmp = rd32(regs, PCIE_MISC_PCIE_STATUS);
        if ((tmp & (PCIE_MISC_PCIE_STATUS_PCIE_PHYLINKUP | PCIE_MISC_PCIE_STATUS_PCIE_DL_ACTIVE)) ==
            (PCIE_MISC_PCIE_STATUS_PCIE_PHYLINKUP | PCIE_MISC_PCIE_STATUS_PCIE_DL_ACTIVE))
            break;
        delay_us(5000);
    }

    if ((tmp & (PCIE_MISC_PCIE_STATUS_PCIE_PHYLINKUP | PCIE_MISC_PCIE_STATUS_PCIE_DL_ACTIVE)) !=
        (PCIE_MISC_PCIE_STATUS_PCIE_PHYLINKUP | PCIE_MISC_PCIE_STATUS_PCIE_DL_ACTIVE))
    {
        bug("[PCIBcm2711] link did not come up (status %08x)\n", tmp);
        return FALSE;
    }

    BRINGUP(bug("[PCIBcm2711] link up after %dms (status %08x)\n", i * 5, tmp));


#if PCIE_BRINGUP
    /*
     * Everything the bring-up depends on, in one place: if a controller
     * fails to appear on real hardware, this says whether the fault is the
     * bridge, the windows, the endpoint, or the routing.
     */
    bug("[PCIBcm2711] --- bring-up state ---\n");
    bug("[PCIBcm2711]   misc_ctrl=%08x (scb0_size field %u)\n",
        rd32(regs, PCIE_MISC_MISC_CTRL),
        (unsigned)((rd32(regs, PCIE_MISC_MISC_CTRL) >> 27) & 0x1f));
    bug("[PCIBcm2711]   inbound  bar2=%08x:%08x (offset %08x%08x, 2^%u)\n",
        rd32(regs, PCIE_MISC_RC_BAR2_CONFIG_HI),
        rd32(regs, PCIE_MISC_RC_BAR2_CONFIG_LO),
        (unsigned)(psd->dma_offset >> 32), (unsigned)psd->dma_offset,
        (unsigned)psd->dma_exp);
    bug("[PCIBcm2711]   outbound win0 lo=%08x hi=%08x baselimit=%08x\n",
        rd32(regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO),
        rd32(regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI),
        rd32(regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT));
    {
        ULONG id, cls;

        Disable();
        wr32(regs, PCIE_EXT_CFG_INDEX, EXT_CFG_ADDR(1, 0, 0));
        id  = rd32(regs, PCIE_EXT_CFG_DATA + 0x00);
        cls = rd32(regs, PCIE_EXT_CFG_DATA + 0x08);
        Enable();

        if (id == 0xffffffff)
            bug("[PCIBcm2711]   endpoint 1:0.0: no response yet (its firmware "
                "loads when a driver enables it)\n");
        else
            bug("[PCIBcm2711]   endpoint 1:0.0: %04x:%04x class %06x\n",
                (unsigned)(id & 0xffff), (unsigned)(id >> 16),
                (unsigned)(cls >> 8));
    }
    bug("[PCIBcm2711] ----------------------\n");
#endif

    /* Present the root port as a PCI-PCI bridge */
    tmp = rd32(regs, PCIE_RC_CFG_PRIV1_ID_VAL3);
    wr32(regs, PCIE_RC_CFG_PRIV1_ID_VAL3, (tmp & ~0xffffffUL) | 0x060400);

    /* Bus numbers: primary 0, secondary 1, subordinate 1 */
    tmp = rd32(regs, 0x18);
    wr32(regs, 0x18, (tmp & 0xff000000) | 0x00010100);

    /* Forward the memory window through the bridge */
    wr32(regs, 0x20, ((BCM2711_PCIE_PCI_WIN + BCM2711_PCIE_WIN_SIZE - 0x100000) & 0xfff00000)
                     | (BCM2711_PCIE_PCI_WIN >> 16));

    /* Enable memory decode and bus mastering on the root port. Devices
       signal by message; the legacy pin block stays as reset left it,
       which is masked. */
    tmp = rd32(regs, PCI_CMD);
    wr32(regs, PCI_CMD, tmp | PCI_CMD_MEMORY | PCI_CMD_MASTER);

    return TRUE;
}

/*
 * The endpoint has no firmware-assigned resources; place its BAR at
 * the bottom of the window and route its interrupt.
 */
static void SetupEndpoint(struct pci_staticdata *psd)
{
    volatile uint8_t *regs = psd->regs;
    uint32_t id, tmp;

    Disable();
    wr32(regs, PCIE_EXT_CFG_INDEX, EXT_CFG_ADDR(1, 0, 0));
    id = rd32(regs, PCIE_EXT_CFG_DATA + 0x00);

    if (id != 0xffffffff && id != 0)
    {
        /* 64-bit memory BAR0. Keep whatever the firmware placed there:
           moving a controller it has already set up loses us its work. */
        uint32_t bar = rd32(regs, PCIE_EXT_CFG_DATA + 0x10) & ~0xfUL;

        if (!psd->preinitialised || bar == 0)
        {
            wr32(regs, PCIE_EXT_CFG_DATA + 0x10, BCM2711_PCIE_PCI_WIN);
            wr32(regs, PCIE_EXT_CFG_DATA + 0x14, 0);
        }
        else
            D(bug("[PCIBcm2711] keeping firmware BAR0 %08x\n", bar));

        /* Cache line size, as every reference stack programs */
        tmp = rd32(regs, PCIE_EXT_CFG_DATA + 0x0c);
        wr32(regs, PCIE_EXT_CFG_DATA + 0x0c, (tmp & ~0xffUL) | 16);

        /*
         * The command register is left alone: the endpoint stays
         * addressed but quiescent until its own driver enables it,
         * which is the moment the firmware load happens.
         */
    }
    Enable();

    D(bug("[PCIBcm2711] endpoint 1:0.0 id %08x\n", id));
}

static uint32_t EndpointCfgRead(struct pci_staticdata *psd, uint32_t reg)
{
    uint32_t val;

    Disable();
    wr32(psd->regs, PCIE_EXT_CFG_INDEX, EXT_CFG_ADDR(1, 0, 0));
    val = rd32(psd->regs, PCIE_EXT_CFG_DATA + reg);
    Enable();

    return val;
}

static void EndpointCfgWrite(struct pci_staticdata *psd, uint32_t reg, uint32_t val)
{
    Disable();
    wr32(psd->regs, PCIE_EXT_CFG_INDEX, EXT_CFG_ADDR(1, 0, 0));
    wr32(psd->regs, PCIE_EXT_CFG_DATA + reg, val);
    Enable();
}

/*
 * The loader leaves the controller advertising larger payloads than the
 * root complex is set up for; bring it down to match. Error status is
 * sticky across everything but a reset, so clear it too: anything seen
 * later was earned later.
 */
static void EndpointPostLoad(struct pci_staticdata *psd)
{
    uint32_t cap, guard;

    cap = EndpointCfgRead(psd, 0x34) & 0xff;
    for (guard = 12; cap && guard; guard--)
    {
        uint32_t hdr = EndpointCfgRead(psd, cap);

        if ((hdr & 0xff) == 0x10)
        {
            uint32_t devctl = EndpointCfgRead(psd, cap + 0x08);

            EndpointCfgWrite(psd, cap + 0x08, devctl & ~(7 << 5));
            /* Device status is W1C in the same dword */
            EndpointCfgWrite(psd, cap + 0x08,
                             EndpointCfgRead(psd, cap + 0x08));
            break;
        }
        cap = (hdr >> 8) & 0xff;
    }

    EndpointCfgWrite(psd, 0x104, EndpointCfgRead(psd, 0x104));
    EndpointCfgWrite(psd, 0x110, EndpointCfgRead(psd, 0x110));
}

/* Firmware revision of the attached controller, 0 if it is running none. */
static uint32_t EndpointFWVersion(struct pci_staticdata *psd)
{
    uint32_t ver;

    Disable();
    wr32(psd->regs, PCIE_EXT_CFG_INDEX, EXT_CFG_ADDR(1, 0, 0));
    ver = rd32(psd->regs, PCIE_EXT_CFG_DATA + VL805_CFG_FWVERSION);
    Enable();

    return (ver == 0xffffffff) ? 0 : ver;
}

/*
 * Runs when the controller's driver first enables it, immediately
 * before the driver initialises it. A controller that carries its own
 * firmware store, or that survived a warm restart, is already running
 * and is left alone: reloading a live controller leaves it inert.
 */
void EnsureEndpointFirmware(struct pci_staticdata *psd)
{
    int ms;

    if (psd->fw_loaded)
        return;

    if (EndpointFWVersion(psd))
    {
        psd->fw_loaded = TRUE;
        BRINGUP(bug("[PCIBcm2711] controller firmware %08x already running, no load needed\n",
                    EndpointFWVersion(psd)));
        EndpointPostLoad(psd);
        return;
    }

    NotifyXHCIReset();

    for (ms = 0; ms < 1000; ms += 10)
    {
        if (EndpointFWVersion(psd))
            break;
        delay_us(10000);
    }

    /* The version reports in a little before the controller is ready;
       a failed load stays unmarked so the next enable tries again. */
    if (EndpointFWVersion(psd))
    {
        psd->fw_loaded = TRUE;
        D(bug("[PCIBcm2711] controller firmware %08x, %dms after load request\n",
              EndpointFWVersion(psd), ms));
        delay_us(1000);
        EndpointPostLoad(psd);
    }
    else
        bug("[PCIBcm2711] controller firmware did not load\n");
}

static int PCIBcm2711_InitClass(LIBBASETYPEPTR LIBBASE)
{
    struct pci_staticdata *psd = &LIBBASE->psd;
    APTR KernelBase;
    OOP_Object *pci;

    KernelBase = OpenResource("kernel.resource");
    psd->kernelBase = KernelBase;
    if (!KernelBase)
        return TRUE;

    __arm_periiobase = (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase);

    /* BCM2711 only */
    if (__arm_periiobase != BCM2711_PERIIOBASE)
        return TRUE;

    if (!PCIeEnabled())
    {
        BRINGUP(bug("[PCIBcm2711] switched off on the command line\n"));
        return TRUE;
    }

    if (!PCIeNodeUsable())
    {
        BRINGUP(bug("[PCIBcm2711] no usable brcm,bcm2711-pcie node in the device tree\n"));
        return TRUE;
    }

    BRINGUP(bug("[PCIBcm2711] starting bring-up (peripheral base %p)\n",
                (APTR)__arm_periiobase));

    psd->regs = (volatile uint8_t *)BCM2711_PCIE_REG_BASE;
    DMARangesFromDT(&psd->dma_offset, &psd->dma_exp);
    MSIIrqFromDT(psd);
    D(bug("[PCIBcm2711] bus master address offset %08x%08x\n",
          (unsigned)(psd->dma_offset >> 32), (unsigned)psd->dma_offset));


    if (!BridgeInit(psd))
        return TRUE;    /* no link; nothing to drive, but do not stop boot */

    /*
     * Own the bridge's MSI output. This handler only acknowledges; the
     * device handlers that care about the event join the same interrupt
     * and run after it.
     */
    psd->msi_handle = KrnAddIRQHandler(psd->msi_irq, PCIeMSIHandler, psd, NULL);
    BRINGUP(bug("[PCIBcm2711] MSI target %08x%08x, INTID %u, handler @ 0x%p\n",
                (unsigned)(BCM2711_MSI_TARGET >> 32), (unsigned)BCM2711_MSI_TARGET,
                (unsigned)psd->msi_irq, psd->msi_handle));

    /* Address the endpoint; the firmware load waits for its driver. */
    SetupEndpoint(psd);
    D(bug("[PCIBcm2711] controller firmware %08x at init\n",
          EndpointFWVersion(psd)));

    psd->hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    psd->hiddPCIDeviceAB = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    psd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    if (psd->hiddPCIDriverAB == 0 || psd->hiddPCIDeviceAB == 0 || psd->hiddAB == 0)
    {
        D(bug("[PCIBcm2711] ObtainAttrBases failed\n"));
        return TRUE;
    }

    struct pHidd_PCI_AddHardwareDriver msg;

    msg.driverClass = psd->driverClass;
    msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (pci)
    {
        OOP_DoMethod(pci, (OOP_Msg)&msg);
        OOP_DisposeObject(pci);
        D(bug("[PCIBcm2711] driver registered\n"));
    }

    return TRUE;
}

static int PCIBcm2711_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    OOP_ReleaseAttrBase(IID_Hidd);

    return TRUE;
}

ADD2INITLIB(PCIBcm2711_InitClass, 0)
ADD2EXPUNGELIB(PCIBcm2711_ExpungeClass, 0)
