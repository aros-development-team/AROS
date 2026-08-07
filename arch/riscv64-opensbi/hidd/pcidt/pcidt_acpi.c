/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI host bridge discovery from the ACPI tables.

    A board describes its bridges in the device tree, and that is what
    this driver normally reads. A machine brought up by UEFI firmware
    may have no tree at all - it describes itself with ACPI - so the
    same bridges are found here instead, out of MCFG.

    The tables come from acpica.library, which the kickstart brings up
    long before any bus driver; this is the same way the PC's driver
    finds its ECAM windows. Nothing here runs on a machine described by
    a device tree, and a machine with no ACPI tables finds nothing.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/acpica.h>

#include <libraries/acpica.h>
#include <hidd/pci.h>
#include <hardware/pci.h>

#include "pcidt.h"

#if defined(ACPI_CA_VERSION) && (ACPI_CA_VERSION >= 0x20250404)
#define ACPICAIRQIntr Interrupts
#else
#define ACPICAIRQIntr u.Interrupts
#endif

/*
 * acpica.library is a stack-call library reached through a stub, so its
 * base has to be an explicit global for the linkage to resolve. Opening
 * it by hand (rather than leaving it to autoinit) also keeps this class
 * loadable on a machine that has no acpica.library at all.
 */
struct Library *ACPICABase;

/*
 * Map the regions the firmware assigned.
 *
 * The tree path is handed the bridge's windows and maps them whole.
 * Here there is no such description without interpreting AML, so each
 * base register is read and the region it points at is mapped on its
 * own. That is narrower than the window, which suits the 64-bit one -
 * it is tens of gigabytes wide and could not be mapped whole anyway.
 */
static void pcidt_acpi_mapbars(struct pcidt_staticdata *psd,
                               struct pcidt_bridge *b)
{
    UBYTE bus;

    for (bus = b->busStart; ; bus++)
    {
        UBYTE dev;

        for (dev = 0; dev < 32; dev++)
        {
            UBYTE sub, nfunc = 1;

            for (sub = 0; sub < nfunc; sub++)
            {
                ULONG hdr, reg, lastreg;

                if (PCIDT_ReadConfig(b, bus, dev, sub, 0x00) == 0xffffffff)
                    continue;

                hdr = (PCIDT_ReadConfig(b, bus, dev, sub, 0x0c) >> 16) & 0xff;
                if (sub == 0 && (hdr & 0x80))
                    nfunc = 8;

                /* Type 0 headers have six base registers, bridges two */
                lastreg = ((hdr & 0x7f) == 0) ? 0x24 : 0x14;

                for (reg = 0x10; reg <= lastreg; reg += 4)
                {
                    ULONG lo, hi = 0, cmd, szlo, szhi = 0xffffffff;
                    UQUAD addr, size;
                    BOOL is64;

                    lo = PCIDT_ReadConfig(b, bus, dev, sub, reg);

                    /* I/O space is not reached through a mapping */
                    if ((lo & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_IO)
                        continue;

                    is64 = ((lo & PCIBAR_MEMTYPE_MASK) == PCIBAR_MEMTYPE_64BIT);
                    if (is64)
                    {
                        if (reg == lastreg)
                            break;
                        hi = PCIDT_ReadConfig(b, bus, dev, sub, reg + 4);
                    }

                    addr = ((UQUAD)hi << 32) | (lo & 0xfffffff0);

                    /*
                     * Size it the usual way, restoring the register
                     * afterwards. Decode is turned off around this:
                     * the all-ones value written while sizing would
                     * otherwise be live on the bus.
                     */
                    cmd = PCIDT_ReadConfig(b, bus, dev, sub, 0x04);
                    PCIDT_WriteConfig(b, bus, dev, sub, 0x04,
                                      cmd & ~(ULONG)PCICMF_MEMDECODE);

                    PCIDT_WriteConfig(b, bus, dev, sub, reg, 0xffffffff);
                    szlo = PCIDT_ReadConfig(b, bus, dev, sub, reg);
                    PCIDT_WriteConfig(b, bus, dev, sub, reg, lo);

                    if (is64)
                    {
                        PCIDT_WriteConfig(b, bus, dev, sub, reg + 4, 0xffffffff);
                        szhi = PCIDT_ReadConfig(b, bus, dev, sub, reg + 4);
                        PCIDT_WriteConfig(b, bus, dev, sub, reg + 4, hi);
                        reg += 4;
                    }

                    PCIDT_WriteConfig(b, bus, dev, sub, 0x04, cmd);

                    size = ~(((UQUAD)szhi << 32) | (szlo & ~(UQUAD)0xf)) + 1;
                    if (!szlo || szlo == 0xffffffff)
                        size = 0;

                    if (!addr || !size)
                        continue;

                    D(bug("[PCIDT:ACPI] %02x:%02x.%x region %p (%p)\n",
                          bus, dev, sub, (APTR)(IPTR)addr, (APTR)(IPTR)size);)

                    PCIDT_Map(psd, (IPTR)(addr & ~(UQUAD)0xfff),
                              (IPTR)((size + 0xfff) & ~(UQUAD)0xfff));
                }
            }
        }

        if (bus == b->busEnd)
            break;
    }
}

/*
 * The source a link device is currently routed to.
 *
 * Called for each resource in the link's _CRS; the first interrupt it
 * describes is the answer, so later ones are ignored.
 */
static ACPI_STATUS pcidt_acpi_linkres(ACPI_RESOURCE *res, void *context)
{
    ULONG *irq = (ULONG *)context;

    if (*irq != (ULONG)-1)
        return AE_OK;

    if (res->Type == ACPI_RESOURCE_TYPE_IRQ &&
        res->Data.Irq.InterruptCount > 0)
        *irq = res->Data.Irq.ACPICAIRQIntr[0];
    else if (res->Type == ACPI_RESOURCE_TYPE_EXTENDED_IRQ &&
             res->Data.ExtendedIrq.InterruptCount > 0)
        *irq = res->Data.ExtendedIrq.ACPICAIRQIntr[0];

    return AE_OK;
}

/*
 * A _PRT entry may name the source outright, or name a link device
 * that carries it. Firmware that describes a board with a fixed
 * interrupt controller still tends to use links - QEMU routes all four
 * pins through GSI0..GSI3 - so both forms have to be followed.
 *
 * Returns the source, or 0 when it could not be resolved.
 */
static ULONG pcidt_acpi_source(ACPI_HANDLE scope, ACPI_PCI_ROUTING_TABLE *entry)
{
    ACPI_HANDLE link = NULL;
    ULONG irq = (ULONG)-1;

    if (entry->Source[0] == '\0')
        return (ULONG)entry->SourceIndex;

    if (ACPI_FAILURE(AcpiGetHandle(scope, entry->Source, &link)) || !link)
    {
        D(bug("[PCIDT:ACPI] no such link device '%s'\n", entry->Source);)
        return 0;
    }

    /*
     * _CRS is what the link is set to now. A link firmware left
     * unconfigured would need _PRS and _SRS to be programmed, which
     * this port has no reason to do: the controllers it meets are
     * fixed, and the firmware has already routed them.
     */
    AcpiWalkResources(link, "_CRS", pcidt_acpi_linkres, &irq);

    if (irq == (ULONG)-1)
    {
        D(bug("[PCIDT:ACPI] link '%s' describes no interrupt\n", entry->Source);)
        return 0;
    }

    return irq;
}

/*
 * Which sources a bridge's devices raise, out of _PRT.
 *
 * Firmware that publishes ACPI usually programs the interrupt line
 * register too, and the driver class answers from that. QEMU's does
 * not - every device reads back 0xff - so the routing has to come from
 * the same place the tree's interrupt-map would have: one _PRT entry
 * per device and pin, naming the source directly.
 *
 * The entries go into the bridge's own map, so the lookup that already
 * serves the tree path serves this one unchanged.
 */
static void pcidt_acpi_prt(struct pcidt_bridge *b, ACPI_HANDLE handle,
                           UBYTE bus)
{
    ACPI_PCI_ROUTING_TABLE *entry;
    ACPI_BUFFER buffer;
    ACPI_STATUS status;

    /*
     * The same bridge answers to both host-bridge names - QEMU's is a
     * PNP0A08 with PNP0A03 as its compatible id - so it is walked
     * twice. One description of the routing is enough.
     */
    if (b->intMapCount)
        return;

    buffer.Length = ACPI_ALLOCATE_BUFFER;
    buffer.Pointer = NULL;

    status = AcpiGetIrqRoutingTable(handle, &buffer);
    if (ACPI_FAILURE(status) || !buffer.Pointer)
        return;

    /*
     * _PRT names the device and the pin, so both take part in the
     * comparison - unlike a tree, which masks the device away.
     */
    b->intMask[0] = 0x00fff800;         /* bus and device */
    b->intMask[1] = 0;
    b->intMask[2] = 0;
    b->intMask[3] = 0x7;                /* pin */

    for (entry = buffer.Pointer; entry->Length != 0;
         entry = (APTR)((IPTR)entry + entry->Length))
    {
        ULONG dev = (ULONG)((entry->Address >> 16) & 0xffff);

        ULONG irq = pcidt_acpi_source(handle, entry);

        if (!irq)
            continue;

        if (b->intMapCount >= PCIDT_MAX_INTMAP)
        {
            D(bug("[PCIDT:ACPI] _PRT has more entries than we can hold\n");)
            break;
        }

        b->intMap[b->intMapCount].key[0] =
            (((ULONG)bus << 16) | ((dev & 31) << 11)) & b->intMask[0];
        b->intMap[b->intMapCount].key[1] = 0;
        b->intMap[b->intMapCount].key[2] = 0;
        /* _PRT counts pins from zero, the config register from one */
        b->intMap[b->intMapCount].key[3] = (entry->Pin + 1) & b->intMask[3];
        b->intMap[b->intMapCount].irq    = irq;
        b->intMapCount++;
    }

    D(bug("[PCIDT:ACPI] _PRT gave %u routing entries\n", b->intMapCount);)

    FreeVec(buffer.Pointer);
}

struct pcidt_prtargs
{
    struct pcidt_bridge *bridge;
};

static ACPI_STATUS pcidt_acpi_hostbridge(ACPI_HANDLE handle, ULONG nesting,
                                         void *context, void **retval)
{
    struct pcidt_prtargs *a = (struct pcidt_prtargs *)context;
    ACPI_BUFFER buffer;
    ACPI_STATUS status;
    UBYTE bus = 0;

    /* _BBN names the bridge's first bus; absent means bus zero */
    buffer.Length = ACPI_ALLOCATE_BUFFER;
    buffer.Pointer = NULL;
    status = AcpiEvaluateObject(handle, "_BBN", NULL, &buffer);
    if (ACPI_SUCCESS(status) && buffer.Pointer)
    {
        ACPI_OBJECT *obj = (ACPI_OBJECT *)buffer.Pointer;

        if (obj->Type == ACPI_TYPE_INTEGER)
            bus = (UBYTE)obj->Integer.Value;
        FreeVec(buffer.Pointer);
    }

    if (bus >= a->bridge->busStart && bus <= a->bridge->busEnd)
        pcidt_acpi_prt(a->bridge, handle, bus);

    return AE_OK;
}

ULONG PCIDT_DiscoverACPI(struct pcidt_staticdata *psd)
{
    ACPI_TABLE_MCFG *mcfg = NULL;
    ACPI_MCFG_ALLOCATION *alloc;
    ACPI_STATUS status;
    IPTR offset;

    ACPICABase = OpenLibrary("acpica.library", 0);
    if (!ACPICABase)
    {
        D(bug("[PCIDT:ACPI] no acpica.library\n");)
        return psd->bridgeCount;
    }

    status = AcpiGetTable(ACPI_SIG_MCFG, 1, (ACPI_TABLE_HEADER **)&mcfg);
    if (ACPI_FAILURE(status) || !mcfg)
    {
        D(bug("[PCIDT:ACPI] no MCFG table\n");)
        CloseLibrary(ACPICABase);
        return psd->bridgeCount;
    }

    offset = sizeof(ACPI_TABLE_MCFG);
    alloc = ACPI_ADD_PTR(ACPI_MCFG_ALLOCATION, mcfg, offset);

    while ((offset + sizeof(ACPI_MCFG_ALLOCATION)) <= mcfg->Header.Length)
    {
        struct pcidt_bridge *b;

        if (psd->bridgeCount >= PCIDT_MAX_BRIDGES)
        {
            D(bug("[PCIDT:ACPI] more bridges than we can hold\n");)
            break;
        }

        b = &psd->bridges[psd->bridgeCount];

        /* MCFG describes flat ECAM windows and nothing else */
        b->access   = PCIDT_ACCESS_ECAM;
        b->index    = psd->bridgeCount;
        b->cfgBase  = (IPTR)alloc->Address;
        b->busStart = alloc->StartBusNumber;
        b->busEnd   = alloc->EndBusNumber;
        b->cfgSize  = ((IPTR)(b->busEnd - b->busStart) + 1) << 20;

        /*
         * No routing map and no message interrupt. Both are deliberate:
         * without _PRT a device's interrupt is whatever the firmware
         * left in its interrupt line register, and with no description
         * of a message controller a driver asking for a vector is
         * refused and falls back to its pin.
         */
        b->intMapCount = 0;
        b->msiIrq      = 0;
        b->msiReady    = 0;

        D(bug("[PCIDT:ACPI] segment %u: ECAM %p+%p buses %d-%d\n",
              alloc->PciSegment, b->cfgBase, b->cfgSize,
              b->busStart, b->busEnd);)

        if (PCIDT_Map(psd, b->cfgBase, b->cfgSize))
        {
            struct pcidt_prtargs args = { b };

            pcidt_acpi_mapbars(psd, b);

            /* Both spellings of a PCI host bridge carry a _PRT */
            AcpiGetDevices("PNP0A03", pcidt_acpi_hostbridge, &args, NULL);
            AcpiGetDevices("PNP0A08", pcidt_acpi_hostbridge, &args, NULL);

            psd->bridgeCount++;
        }

        offset += sizeof(ACPI_MCFG_ALLOCATION);
        alloc = ACPI_ADD_PTR(ACPI_MCFG_ALLOCATION, mcfg, offset);
    }

    CloseLibrary(ACPICABase);

    return psd->bridgeCount;
}
