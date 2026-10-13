/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Intel VMD domain access, enumeration and resource assignment.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hardware/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "pcivmd.h"

#define VMD_PASS_LO             0
#define VMD_PASS_HI             1

#define VMD_BRIDGE_ALIGN        0x100000ULL
#define VMD_BAR_MINALIGN        0x1000ULL
#define VMD_4GB                 0x100000000ULL

#define ALIGN_UP(x, a)          (((x) + ((a) - 1)) & ~((UQUAD)(a) - 1))

/**************************************************************************
 * Configuration space access
 **************************************************************************/

static inline volatile UBYTE *vmd_cfgptr(struct VMDDomain *dom, UBYTE bus,
    UBYTE dev, UBYTE sub, UWORD reg, UBYTE len)
{
    UQUAD off;

    if (bus >= dom->vd_BusCount)
        return NULL;

    off = ((UQUAD)bus << 20) | ((UQUAD)(dev & 31) << 15) |
          ((UQUAD)(sub & 7) << 12) | (reg & 0xfff);
    if (off + len > dom->vd_CfgSize)
        return NULL;

    return dom->vd_CfgBar + off;
}

/*
 * Some versions of the hardware can lock the CPU up if two accesses to
 * the configuration window overlap, so every one of them is serialised.
 */
static inline void vmd_cfglock(struct VMDDomain *dom)
{
    Disable();
#if defined(__AROSEXEC_SMP__)
    {
        struct pcivmd_staticdata *psd = dom->vd_psd;
        KrnSpinLock(&dom->vd_CfgLock, NULL, SPINLOCK_MODE_WRITE);
    }
#endif
}

static inline void vmd_cfgunlock(struct VMDDomain *dom)
{
#if defined(__AROSEXEC_SMP__)
    {
        struct pcivmd_staticdata *psd = dom->vd_psd;
        KrnSpinUnLock(&dom->vd_CfgLock);
    }
#endif
    Enable();
}

static inline ULONG vmd_lenmask(UBYTE len)
{
    return (len >= 4) ? 0xffffffff : ((1UL << (len * 8)) - 1);
}

/* Access by position in the configuration window, no translation */
ULONG VMD_RawRead(struct VMDDomain *dom, UBYTE bus, UBYTE dev, UBYTE sub,
    UWORD reg, UBYTE len)
{
    volatile UBYTE *addr = vmd_cfgptr(dom, bus, dev, sub, reg, len);
    ULONG val;

    if (!addr)
        return vmd_lenmask(len);

    vmd_cfglock(dom);
    switch (len)
    {
    case 1:
        val = *addr;
        break;
    case 2:
        val = *(volatile UWORD *)addr;
        break;
    default:
        val = *(volatile ULONG *)addr;
        break;
    }
    vmd_cfgunlock(dom);

    return val;
}

void VMD_RawWrite(struct VMDDomain *dom, UBYTE bus, UBYTE dev, UBYTE sub,
    UWORD reg, UBYTE len, ULONG val)
{
    volatile UBYTE *addr = vmd_cfgptr(dom, bus, dev, sub, reg, len);

    if (!addr)
        return;

    vmd_cfglock(dom);
    switch (len)
    {
    case 1:
        *addr = (UBYTE)val;
        break;
    case 2:
        *(volatile UWORD *)addr = (UWORD)val;
        break;
    default:
        *(volatile ULONG *)addr = val;
        break;
    }
    vmd_cfgunlock(dom);
}

#define rd8(reg)        VMD_RawRead(dom, bus, dev, sub, (reg), 1)
#define rd16(reg)       VMD_RawRead(dom, bus, dev, sub, (reg), 2)
#define rd32(reg)       VMD_RawRead(dom, bus, dev, sub, (reg), 4)
#define wr8(reg, val)   VMD_RawWrite(dom, bus, dev, sub, (reg), 1, (val))
#define wr16(reg, val)  VMD_RawWrite(dom, bus, dev, sub, (reg), 2, (val))
#define wr32(reg, val)  VMD_RawWrite(dom, bus, dev, sub, (reg), 4, (val))

static inline BOOL vmd_onedev(struct VMDDomain *dom, UBYTE bus)
{
    return (dom->vd_OneDev[bus >> 3] & (1 << (bus & 7))) != 0;
}

/*
 * The buses behind a VMD may be numbered from 128 or 224 rather than 0,
 * and its bridges have to carry those real numbers. The rest of the
 * system is shown a domain that starts at bus 0 instead - that is where
 * the PCI subsystem begins looking - so the bus number registers of the
 * bridges are translated on their way through.
 */
static BOOL vmd_busregs(struct VMDDomain *dom, UBYTE bus, UBYTE dev,
    UBYTE sub, UWORD reg, UBYTE len)
{
    if (!dom->vd_BusStart)
        return FALSE;
    if ((reg > PCIBR_SUBBUS) || (reg + len <= PCIBR_PRIBUS))
        return FALSE;

    return (rd8(PCICS_HEADERTYPE) & PCIHT_MASK) == PCIHT_BRIDGE;
}

ULONG VMD_ReadConfig(struct VMDDomain *dom, UBYTE bus, UBYTE dev, UBYTE sub,
    UWORD reg, UBYTE len)
{
    ULONG val;

    if (dev && (bus < dom->vd_BusCount) && vmd_onedev(dom, bus))
        return vmd_lenmask(len);

    val = VMD_RawRead(dom, bus, dev, sub, reg, len);

    if (vmd_busregs(dom, bus, dev, sub, reg, len))
    {
        UBYTE i;

        for (i = 0; i < len; i++)
        {
            UWORD r = reg + i;

            if ((r >= PCIBR_PRIBUS) && (r <= PCIBR_SUBBUS))
            {
                UBYTE b = (val >> (i * 8)) & 0xff;

                b = (b >= dom->vd_BusStart) ? (b - dom->vd_BusStart) : 0;
                val = (val & ~(0xffUL << (i * 8))) | ((ULONG)b << (i * 8));
            }
        }
    }

    return val;
}

void VMD_WriteConfig(struct VMDDomain *dom, UBYTE bus, UBYTE dev, UBYTE sub,
    UWORD reg, UBYTE len, ULONG val)
{
    if (dev && (bus < dom->vd_BusCount) && vmd_onedev(dom, bus))
        return;

    if (vmd_busregs(dom, bus, dev, sub, reg, len))
    {
        UBYTE i;

        for (i = 0; i < len; i++)
        {
            UWORD r = reg + i;

            if ((r >= PCIBR_PRIBUS) && (r <= PCIBR_SUBBUS))
            {
                UBYTE b = ((val >> (i * 8)) & 0xff) + dom->vd_BusStart;

                val = (val & ~(0xffUL << (i * 8))) | ((ULONG)b << (i * 8));
            }
        }
    }

    VMD_RawWrite(dom, bus, dev, sub, reg, len, val);
}

/* Address of a function's configuration space, for extended access */
APTR VMD_ConfigAddr(struct VMDDomain *dom, UBYTE bus, UBYTE dev, UBYTE sub)
{
    return (APTR)vmd_cfgptr(dom, bus, dev, sub, 0, 4);
}

UBYTE VMD_FindCapability(struct VMDDomain *dom, UBYTE bus, UBYTE dev,
    UBYTE sub, UBYTE capid)
{
    UBYTE where, guard = 48;

    if (!(rd16(PCICS_STATUS) & PCISTF_CAPABILITIES))
        return 0;

    where = rd8(PCICS_CAP_PTR) & ~3;
    while ((where >= 0x40) && guard--)
    {
        UBYTE id = rd8(where);

        if (id == 0xff)
            break;
        if (id == capid)
            return where;

        where = rd8(where + 1) & ~3;
    }

    return 0;
}

/*
 * Turn an address a hidden device decodes into one the CPU can use.
 * The two only differ when the VMD has been handed to a guest, in which
 * case the devices still decode the host's addresses.
 */
APTR VMD_MapBus(struct VMDDomain *dom, UQUAD busaddr, ULONG length)
{
    struct pcivmd_staticdata *psd = dom->vd_psd;
    UQUAD cpuaddr = busaddr;
    APTR va;
    int w;

    for (w = 0; w < 2; w++)
    {
        struct VMDWindow *win = &dom->vd_Win[w];

        if (win->size && (busaddr >= win->busbase) &&
            (busaddr < win->busbase + win->size))
        {
            cpuaddr = busaddr - win->busbase + win->cpubase;
            break;
        }
    }

    va = HIDD_PCIDriver_MapPCI(dom->vd_ParentDrv, (APTR)(IPTR)cpuaddr, length);
    if (va == (APTR)-1)
        va = NULL;

    return va;
}

/**************************************************************************
 * Bus enumeration
 **************************************************************************/

/* Is there a function here? Also reports its header type */
static BOOL vmd_present(struct VMDDomain *dom, UBYTE bus, UBYTE dev,
    UBYTE sub, UBYTE *hdr)
{
    UWORD vendor = rd16(PCICS_VENDOR);

    if ((vendor == 0xffff) || (vendor == 0x0000))
        return FALSE;

    *hdr = rd8(PCICS_HEADERTYPE);
    return TRUE;
}

/* Take the bridge's forwarding windows away, the way they are when unused */
static void vmd_closebridge(struct VMDDomain *dom, UBYTE bus, UBYTE dev,
    UBYTE sub)
{
    /* I/O: nothing behind a VMD can have any */
    wr32(PCIBR_IOBASEUPPER, 0x0000ffff);
    wr16(PCIBR_IOBASE, 0x00f0);
    wr32(PCIBR_IOBASEUPPER, 0);

    wr32(PCIBR_MEMBASE, 0x0000fff0);

    wr32(PCIBR_PRELIMITUPPER, 0);
    wr32(PCIBR_PREFETCHBASE, 0x0000fff0);
    wr32(PCIBR_PREBASEUPPER, 0xffffffff);
}

/*
 * Number the buses, depth first. Whatever the firmware left behind is
 * not relied upon - it does not have to configure the domain at all -
 * so every bridge is renumbered and every function is quietened, ready
 * to be given its address space afresh.
 */
static void vmd_scanbus(struct VMDDomain *dom, UBYTE bus)
{
    UBYTE dev;

    for (dev = 0; dev < 32; dev++)
    {
        UBYTE sub, nfunc = 1;

        if (dev && vmd_onedev(dom, bus))
            break;

        for (sub = 0; sub < nfunc; sub++)
        {
            UBYTE hdr;
            UWORD cmd;

            if (!vmd_present(dom, bus, dev, sub, &hdr))
            {
                if (!sub)
                    break;
                continue;
            }
            if (!sub && (hdr & PCIHT_MULTIFUNC))
                nfunc = 8;

            cmd = rd16(PCICS_COMMAND);
            wr16(PCICS_COMMAND, cmd & ~(PCICMF_IODECODE | PCICMF_MEMDECODE | PCICMF_BUSMASTER));

            D(bug("[PCIVMD] %s: %02x:%02x.%x %04x:%04x header %02x\n", __func__,
                bus, dev, sub, rd16(PCICS_VENDOR), rd16(PCICS_PRODUCT), hdr);)

            if ((hdr & PCIHT_MASK) == PCIHT_BRIDGE)
            {
                UBYTE child, pcie;

                vmd_closebridge(dom, bus, dev, sub);

                if (dom->vd_LastBus + 1 >= dom->vd_BusCount)
                {
                    bug("[PCIVMD] %02x:%02x.%x: out of bus numbers, bridge ignored\n",
                        bus, dev, sub);
                    wr8(PCIBR_PRIBUS, dom->vd_BusStart + bus);
                    wr8(PCIBR_SECBUS, 0);
                    wr8(PCIBR_SUBBUS, 0);
                    continue;
                }
                child = ++dom->vd_LastBus;

                wr8(PCIBR_PRIBUS, dom->vd_BusStart + bus);
                wr8(PCIBR_SECBUS, dom->vd_BusStart + child);
                wr8(PCIBR_SUBBUS, dom->vd_BusStart + dom->vd_BusCount - 1);

                /* A PCI Express link has a single device at its far end */
                pcie = VMD_FindCapability(dom, bus, dev, sub, PCICAP_PCIE);
                if (pcie)
                {
                    UBYTE type = (rd16(pcie + 2) >> 4) & 0xf;

                    if ((type == PCIE_TYPE_ROOT_PORT) ||
                        (type == PCIE_TYPE_DOWNSTREAM) ||
                        (type == PCIE_TYPE_PCIE_BRIDGE))
                        dom->vd_OneDev[child >> 3] |= (1 << (child & 7));
                }

                vmd_scanbus(dom, child);

                wr8(PCIBR_SUBBUS, dom->vd_BusStart + dom->vd_LastBus);
            }
        }
    }
}

/**************************************************************************
 * Address space assignment
 **************************************************************************/

/* Size a base register (or pair), leaving it as it was found */
static UQUAD vmd_sizebar(struct VMDDomain *dom, UBYTE bus, UBYTE dev,
    UBYTE sub, UWORD reg, BOOL is64)
{
    ULONG lo, hi, szlo, szhi = 0xffffffff;

    lo = rd32(reg);
    wr32(reg, 0xffffffff);
    szlo = rd32(reg);
    wr32(reg, lo);

    if (is64)
    {
        hi = rd32(reg + 4);
        wr32(reg + 4, 0xffffffff);
        szhi = rd32(reg + 4);
        wr32(reg + 4, hi);
    }

    if ((szlo == 0xffffffff) || (!(szlo & PCIBAR_MASK_MEM) && (!is64 || !szhi)))
        return 0;

    return ~(((UQUAD)szhi << 32) | (szlo & PCIBAR_MASK_MEM)) + 1;
}

/*
 * Place the base registers of one function that belong to this pass.
 *
 * A bridge forwards two ranges: one that can only sit below 4GB, and a
 * prefetchable one that can sit anywhere. So the registers are placed in
 * two passes over the whole tree, one for each range - that keeps
 * everything behind a bridge contiguous in both - and only 64-bit
 * prefetchable registers are left for the second.
 */
static void vmd_assignbars(struct VMDDomain *dom, UBYTE bus, UBYTE dev,
    UBYTE sub, UWORD lastreg, int pass, struct VMDWindow *win)
{
    UWORD reg;

    for (reg = PCICS_BAR0; reg <= lastreg; reg += 4)
    {
        ULONG lo = rd32(reg);
        BOOL is64, pref;
        UQUAD size;

        if ((lo & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_IO)
        {
            /* There is no I/O space to give out */
            if (pass == VMD_PASS_LO)
                wr32(reg, 0);
            continue;
        }

        is64 = ((lo & PCIBAR_MEMTYPE_MASK) == PCIBAR_MEMTYPE_64BIT);
        pref = ((lo & PCIBARF_PREFETCHABLE) != 0);

        /*
         * With nothing to hand out below 4GB the other 64-bit registers
         * go through the prefetchable range too. Nothing behind a PCI
         * Express bridge actually prefetches, so that is harmless.
         */
        if ((is64 && (pref || !dom->vd_Lo)) == (pass == VMD_PASS_HI))
        {
            size = vmd_sizebar(dom, bus, dev, sub, reg, is64);
            if (size)
            {
                UQUAD addr = 0;
                BOOL placed = FALSE;

                if (win)
                {
                    addr = ALIGN_UP(win->next, (size > VMD_BAR_MINALIGN) ? size : VMD_BAR_MINALIGN);
                    if ((addr >= win->next) && (addr + size <= win->limit) &&
                        (is64 || (addr + size <= VMD_4GB)))
                        placed = TRUE;
                }

                if (placed)
                {
                    win->next = addr + size;
                    D(bug("[PCIVMD] %s: %02x:%02x.%x bar %02x -> %p (%p)\n", __func__,
                        bus, dev, sub, reg, (APTR)(IPTR)addr, (APTR)(IPTR)size);)
                }
                else
                {
                    bug("[PCIVMD] %02x:%02x.%x: no room for base register %02x (%p bytes)\n",
                        bus, dev, sub, reg, (APTR)(IPTR)size);
                    addr = 0;
                }

                wr32(reg, (ULONG)addr | (lo & ~PCIBAR_MASK_MEM));
                if (is64)
                    wr32(reg + 4, (ULONG)(addr >> 32));
            }
        }

        if (is64)
            reg += 4;
    }
}

static void vmd_assignbus(struct VMDDomain *dom, UBYTE bus, int pass,
    struct VMDWindow *win)
{
    UBYTE dev;

    for (dev = 0; dev < 32; dev++)
    {
        UBYTE sub, nfunc = 1;

        if (dev && vmd_onedev(dom, bus))
            break;

        for (sub = 0; sub < nfunc; sub++)
        {
            UBYTE hdr;

            if (!vmd_present(dom, bus, dev, sub, &hdr))
            {
                if (!sub)
                    break;
                continue;
            }
            if (!sub && (hdr & PCIHT_MULTIFUNC))
                nfunc = 8;

            if ((hdr & PCIHT_MASK) == PCIHT_BRIDGE)
            {
                struct VMDWindow *bwin = win;
                UQUAD start = 0, saved = 0;
                UBYTE child = rd8(PCIBR_SECBUS);

                if (pass == VMD_PASS_LO)
                    wr32(PCIBR_EXPROMBASE, 0);
                vmd_assignbars(dom, bus, dev, sub, PCIBR_BAR1, pass, win);

                /* Bridges that ran out of bus numbers lead nowhere */
                if (child <= dom->vd_BusStart + bus)
                    continue;
                child -= dom->vd_BusStart;

                if (bwin && (pass == VMD_PASS_HI))
                {
                    /* Is there a prefetchable range, and how far does it reach? */
                    UWORD pbase = rd16(PCIBR_PREFETCHBASE);

                    if (!(pbase & 0xfff0))
                        bwin = NULL;
                    else if (((pbase & 0xf) != 0x1) && (bwin->limit > VMD_4GB))
                        bwin = NULL;
                }

                if (bwin)
                {
                    saved = bwin->next;
                    start = ALIGN_UP(saved, VMD_BRIDGE_ALIGN);
                    if ((start < saved) || (start >= bwin->limit))
                        bwin = NULL;
                    else
                        bwin->next = start;
                }

                vmd_assignbus(dom, child, pass, bwin);

                if (bwin)
                {
                    if (bwin->next > start)
                    {
                        UQUAD end = ALIGN_UP(bwin->next, VMD_BRIDGE_ALIGN);

                        if (end > bwin->limit)
                            end = bwin->limit;
                        bwin->next = end;
                        end--;

                        D(bug("[PCIVMD] %s: %02x:%02x.%x %s range %p - %p\n", __func__,
                            bus, dev, sub, (pass == VMD_PASS_HI) ? "prefetchable" : "memory",
                            (APTR)(IPTR)start, (APTR)(IPTR)end);)

                        if (pass == VMD_PASS_HI)
                        {
                            wr16(PCIBR_PREFETCHBASE, (start >> 16) & 0xfff0);
                            wr16(PCIBR_PREFETCHLIMIT, (end >> 16) & 0xfff0);
                            wr32(PCIBR_PREBASEUPPER, (ULONG)(start >> 32));
                            wr32(PCIBR_PRELIMITUPPER, (ULONG)(end >> 32));
                        }
                        else
                        {
                            wr16(PCIBR_MEMBASE, (start >> 16) & 0xfff0);
                            wr16(PCIBR_MEMLIMIT, (end >> 16) & 0xfff0);
                        }
                    }
                    else
                    {
                        /* Nothing behind it wanted any; the range stays closed */
                        bwin->next = saved;
                    }
                }
            }
            else if ((hdr & PCIHT_MASK) == PCIHT_NORMAL)
            {
                if (pass == VMD_PASS_LO)
                    wr32(PCICS_EXPROM_BASE, 0);
                vmd_assignbars(dom, bus, dev, sub, PCICS_BAR5, pass, win);
            }
        }
    }
}

/* Let everything answer in the space it has been given */
static void vmd_enablebus(struct VMDDomain *dom, UBYTE bus)
{
    UBYTE dev;

    for (dev = 0; dev < 32; dev++)
    {
        UBYTE sub, nfunc = 1;

        if (dev && vmd_onedev(dom, bus))
            break;

        for (sub = 0; sub < nfunc; sub++)
        {
            UBYTE hdr;
            UWORD cmd;

            if (!vmd_present(dom, bus, dev, sub, &hdr))
            {
                if (!sub)
                    break;
                continue;
            }
            if (!sub && (hdr & PCIHT_MULTIFUNC))
                nfunc = 8;

            cmd = rd16(PCICS_COMMAND) | PCICMF_MEMDECODE;

            if ((hdr & PCIHT_MASK) == PCIHT_BRIDGE)
            {
                UBYTE child = rd8(PCIBR_SECBUS);

                /*
                 * A bridge only passes requests from the devices behind
                 * it upstream - their DMA and their interrupt messages -
                 * if it is a bus master itself.
                 */
                cmd |= PCICMF_BUSMASTER;
                wr16(PCICS_COMMAND, cmd);

                if (child > dom->vd_BusStart + bus)
                    vmd_enablebus(dom, child - dom->vd_BusStart);
            }
            else
            {
                wr16(PCICS_COMMAND, cmd);
            }
        }
    }
}

/**************************************************************************
 * The VMD endpoint itself
 **************************************************************************/

/* Read the address and size of one of the VMD's own base registers */
static void vmd_readbar(struct pcivmd_staticdata *psd, OOP_Object *device,
    UBYTE idx, UQUAD *addr, UQUAD *size)
{
    UWORD reg = PCICS_BAR0 + (idx << 2);
    ULONG lo, hi = 0, szlo, szhi = 0xffffffff;
    BOOL is64;

    *addr = 0;
    *size = 0;

    lo = HIDD_PCIDevice_ReadConfigLong(device, reg);
    if ((lo & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_IO)
        return;
    is64 = ((lo & PCIBAR_MEMTYPE_MASK) == PCIBAR_MEMTYPE_64BIT);

    HIDD_PCIDevice_WriteConfigLong(device, reg, 0xffffffff);
    szlo = HIDD_PCIDevice_ReadConfigLong(device, reg);
    HIDD_PCIDevice_WriteConfigLong(device, reg, lo);

    if (is64)
    {
        hi = HIDD_PCIDevice_ReadConfigLong(device, reg + 4);
        HIDD_PCIDevice_WriteConfigLong(device, reg + 4, 0xffffffff);
        szhi = HIDD_PCIDevice_ReadConfigLong(device, reg + 4);
        HIDD_PCIDevice_WriteConfigLong(device, reg + 4, hi);
    }

    if ((szlo == 0xffffffff) || (!(szlo & PCIBAR_MASK_MEM) && (!is64 || !szhi)))
        return;

    *addr = ((UQUAD)hi << 32) | (lo & PCIBAR_MASK_MEM);
    *size = ~(((UQUAD)szhi << 32) | (szlo & PCIBAR_MASK_MEM)) + 1;
}

/*
 * When the VMD has been passed through to a guest its base registers
 * hold guest addresses, but the devices behind it still decode the ones
 * the host assigned. The host's values are left in shadow registers.
 */
static void vmd_readshadow(struct pcivmd_staticdata *psd, struct VMDDomain *dom)
{
    OOP_Object *device = dom->vd_Device;
    UQUAD shadow[2] = { 0, 0 };
    BOOL found = FALSE;
    int w;

    if (dom->vd_Features & VMDF_MEMBAR_SHADOW)
    {
        ULONG vmlock = HIDD_PCIDevice_ReadConfigLong(device, VMD_REG_VMLOCK);

        if ((vmlock & VMLOCK_MEMBAR_SHADOW) &&
            (dom->vd_Win[1].size >= VMD_MB2_SHADOW_OFFSET + VMD_MB2_SHADOW_SIZE))
        {
            volatile UQUAD *regs = HIDD_PCIDriver_MapPCI(dom->vd_ParentDrv,
                (APTR)(IPTR)(dom->vd_Win[1].cpubase + VMD_MB2_SHADOW_OFFSET),
                VMD_MB2_SHADOW_SIZE);

            if (regs && (regs != (APTR)-1))
            {
                shadow[0] = regs[0];
                shadow[1] = regs[1];
                found = TRUE;
            }
        }
    }

    if (dom->vd_Features & VMDF_MEMBAR_SHADOW_VSCAP)
    {
        IPTR pos = 0;

        OOP_GetAttr(device, aHidd_PCIDevice_CapabilityVendorSpecific, &pos);
        if (pos && (HIDD_PCIDevice_ReadConfigLong(device, pos + 4) == VMD_VSCAP_SHADOW_SIG))
        {
            shadow[0] = HIDD_PCIDevice_ReadConfigLong(device, pos + 8) |
                        ((UQUAD)HIDD_PCIDevice_ReadConfigLong(device, pos + 12) << 32);
            shadow[1] = HIDD_PCIDevice_ReadConfigLong(device, pos + 16) |
                        ((UQUAD)HIDD_PCIDevice_ReadConfigLong(device, pos + 20) << 32);
            found = TRUE;
        }
    }

    if (!found)
        return;

    for (w = 0; w < 2; w++)
    {
        shadow[w] &= ~(UQUAD)0xf;
        if (shadow[w] && dom->vd_Win[w].size &&
            (shadow[w] != dom->vd_Win[w].busbase))
        {
            D(bug("[PCIVMD] %s: window %d is at %p on the bus\n", __func__, w,
                (APTR)(IPTR)shadow[w]);)
            dom->vd_Win[w].busbase = shadow[w];
        }
    }
}

/*
 * Get some of the VMD's own interrupt vectors. Every interrupt raised
 * behind it arrives as one of these.
 */
static void vmd_setupirqs(struct pcivmd_staticdata *psd, struct VMDDomain *dom)
{
    OOP_Object *device = dom->vd_Device;
    IPTR capmsix = 0;
    UWORD tsize, want, i;

    OOP_GetAttr(device, aHidd_PCIDevice_CapabilityMSIX, &capmsix);
    if (!capmsix || (dom->vd_Win[1].size < VMD_MB2_MSIX_SIZE))
    {
        bug("[PCIVMD] No MSI-X capability, devices will have no interrupts\n");
        return;
    }

    /* The table is at the start of MEMBAR2; the bus driver reaches it directly */
    HIDD_PCIDriver_MapPCI(dom->vd_ParentDrv, (APTR)(IPTR)dom->vd_Win[1].cpubase,
        VMD_MB2_MSIX_SIZE);

    tsize = (HIDD_PCIDevice_ReadConfigWord(device, capmsix + PCIMSIX_FLAGS) & PCIMSIXF_QSIZE) + 1;

    for (want = (tsize < VMD_MAX_VECTORS) ? tsize : VMD_MAX_VECTORS; want; want >>= 1)
    {
        struct TagItem vectreqs[] =
        {
            { tHidd_PCIVector_Min,  want    },
            { tHidd_PCIVector_Max,  want    },
            { TAG_DONE,             0       }
        };

        if (HIDD_PCIDevice_ObtainVectors(device, vectreqs))
            break;
    }

    for (i = 0; i < want; i++)
    {
        struct TagItem vecattribs[] =
        {
            { tHidd_PCIVector_Int,      (IPTR)-1    },
            { tHidd_PCIVector_Native,   (IPTR)-1    },
            { TAG_DONE,                 0           }
        };

        HIDD_PCIDevice_GetVectorAttribs(device, i, vecattribs);
        if (vecattribs[0].ti_Data == (IPTR)-1)
            break;

        dom->vd_Vec[i].irq = (ULONG)vecattribs[0].ti_Data;
        dom->vd_Vec[i].native = (ULONG)vecattribs[1].ti_Data;
        D(bug("[PCIVMD] %s: vector %u is IRQ %u\n", __func__, i, dom->vd_Vec[i].irq);)
    }
    dom->vd_VecCount = i;

    if (!dom->vd_VecCount)
    {
        bug("[PCIVMD] Failed to obtain MSI-X vectors, devices will have no interrupts\n");
        return;
    }

    /* The first vector also carries the VMD's own events; keep clear if we can */
    if ((dom->vd_Features & VMDF_OFFSET_FIRST_VECTOR) && (dom->vd_VecCount > 1))
        dom->vd_VecFirst = 1;
}

struct VMDDomain *VMD_CreateDomain(struct pcivmd_staticdata *psd,
    OOP_Object *device, ULONG features)
{
    struct TagItem enable[] =
    {
        { aHidd_PCIDevice_isMEM,    TRUE    },
        { aHidd_PCIDevice_isMaster, TRUE    },
        { TAG_DONE,                 0       }
    };
    struct VMDDomain *dom;
    UQUAD addr, size, off;
    IPTR val = 0;
    UWORD cmd, vmconfig;
    int w;

    dom = AllocMem(sizeof(struct VMDDomain), MEMF_PUBLIC | MEMF_CLEAR);
    if (!dom)
        return NULL;

    dom->vd_psd = psd;
    dom->vd_Device = device;
    dom->vd_Features = features;
    OOP_GetAttr(device, aHidd_PCIDevice_Driver, (IPTR *)&dom->vd_ParentDrv);
    OOP_GetAttr(device, aHidd_PCIDevice_ProductID, &val);
    dom->vd_Product = (UWORD)val;
#if defined(__AROSEXEC_SMP__)
    KrnSpinInit(&dom->vd_CfgLock);
#endif

    /* Base registers are sized with the device not decoding them */
    cmd = HIDD_PCIDevice_ReadConfigWord(device, PCICS_COMMAND);
    HIDD_PCIDevice_WriteConfigWord(device, PCICS_COMMAND, cmd & ~PCICMF_MEMDECODE);

    vmd_readbar(psd, device, VMD_CFGBAR, &addr, &size);
    vmd_readbar(psd, device, VMD_MEMBAR1, &dom->vd_Win[0].cpubase, &dom->vd_Win[0].size);
    vmd_readbar(psd, device, VMD_MEMBAR2, &dom->vd_Win[1].cpubase, &dom->vd_Win[1].size);

    HIDD_PCIDevice_WriteConfigWord(device, PCICS_COMMAND, cmd);

    if (!addr || (size < 0x100000) ||
        ((UQUAD)(IPTR)addr != addr) || (size > 0x10000000))
    {
        bug("[PCIVMD] Unusable configuration window (%p, %p bytes)\n",
            (APTR)(IPTR)addr, (APTR)(IPTR)size);
        goto fail;
    }

    OOP_SetAttrs(device, enable);

    dom->vd_CfgSize = size;
    dom->vd_BusCount = (UWORD)(size >> 20);
    dom->vd_CfgBar = HIDD_PCIDriver_MapPCI(dom->vd_ParentDrv, (APTR)(IPTR)addr, (ULONG)size);
    if (!dom->vd_CfgBar || (dom->vd_CfgBar == (APTR)-1))
    {
        bug("[PCIVMD] Failed to map the configuration window\n");
        goto fail;
    }

    /* Where the hidden buses are numbered from */
    if (features & VMDF_BUS_RESTRICTIONS)
    {
        if (HIDD_PCIDevice_ReadConfigWord(device, VMD_REG_VMCAP) & VMCAP_BUS_RESTRICT)
        {
            vmconfig = HIDD_PCIDevice_ReadConfigWord(device, VMD_REG_VMCONFIG);
            switch (VMCONFIG_BUS_RESTRICT(vmconfig))
            {
            case 0:
                dom->vd_BusStart = 0;
                break;
            case 1:
                dom->vd_BusStart = 128;
                break;
            case 2:
                dom->vd_BusStart = 224;
                break;
            default:
                bug("[PCIVMD] Unknown bus restriction %04x\n", vmconfig);
                goto fail;
            }
        }
    }
    if (dom->vd_BusStart + dom->vd_BusCount > 256)
        dom->vd_BusCount = 256 - dom->vd_BusStart;

    /* The memory windows */
    for (w = 0; w < 2; w++)
    {
        struct VMDWindow *win = &dom->vd_Win[w];

        if ((UQUAD)(IPTR)win->cpubase != win->cpubase)
            win->size = 0;
        if (!win->cpubase)
            win->size = 0;
        win->busbase = win->cpubase;
    }
    vmd_readshadow(psd, dom);

    for (w = 0; w < 2; w++)
    {
        struct VMDWindow *win = &dom->vd_Win[w];

        if (!win->size)
            continue;

        /* The start of MEMBAR2 is the VMD's own */
        off = 0;
        if (w == 1)
        {
            off = VMD_MB2_MSIX_SIZE;
            if (features & VMDF_MEMBAR_SHADOW)
                off += VMD_MB2_SHADOW_SIZE;
            off = ALIGN_UP(off, VMD_BAR_MINALIGN);
        }
        win->limit = win->busbase + win->size;
        win->next = win->busbase + off;
        if (win->next >= win->limit)
            continue;

        if (!dom->vd_Lo && (win->limit <= VMD_4GB))
            dom->vd_Lo = win;
        else if (!dom->vd_Hi)
            dom->vd_Hi = win;
    }
    if (!dom->vd_Hi)
        dom->vd_Hi = dom->vd_Lo;

    bug("[PCIVMD] Intel VMD %04x: buses %u-%u, memory %p+%p, %p+%p\n", dom->vd_Product,
        dom->vd_BusStart, dom->vd_BusStart + dom->vd_BusCount - 1,
        (APTR)(IPTR)dom->vd_Win[0].cpubase, (APTR)(IPTR)dom->vd_Win[0].size,
        (APTR)(IPTR)dom->vd_Win[1].cpubase, (APTR)(IPTR)dom->vd_Win[1].size);

    /* Interrupts raised behind the VMD have to come through it */
    vmconfig = HIDD_PCIDevice_ReadConfigWord(device, VMD_REG_VMCONFIG);
    if (vmconfig & VMCONFIG_MSI_REMAP_OFF)
        HIDD_PCIDevice_WriteConfigWord(device, VMD_REG_VMCONFIG, vmconfig & ~VMCONFIG_MSI_REMAP_OFF);
    vmd_setupirqs(psd, dom);

    /* Find what is there and give it somewhere to live */
    vmd_scanbus(dom, 0);
    vmd_assignbus(dom, 0, VMD_PASS_LO, dom->vd_Lo);
    vmd_assignbus(dom, 0, VMD_PASS_HI, dom->vd_Hi);
    vmd_enablebus(dom, 0);

    bug("[PCIVMD] Intel VMD %04x: %u bus(es) in use, %u interrupt vector(s)\n",
        dom->vd_Product, dom->vd_LastBus + 1, dom->vd_VecCount);

    return dom;

fail:
    FreeMem(dom, sizeof(struct VMDDomain));
    return NULL;
}
