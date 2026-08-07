/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ACPI table dump for the opensbi-riscv64 target.

    A UEFI machine describes itself with ACPI, and may hand over a device
    tree that covers only part of what is fitted. This walks the tables
    from the RSDP the EFI stub recorded and reports what is there, with
    the detail spelled out for the tables the port needs next: MADT (the
    harts and the interrupt controllers) and MCFG (the PCIe ECAM
    windows).
*/

#include <inttypes.h>

#include "kernel_intern.h"

/* The RSDP: signature "RSD PTR " */
struct acpi_rsdp
{
    char        signature[8];
    uint8_t     checksum;
    char        oem_id[6];
    uint8_t     revision;
    uint32_t    rsdt_address;
    /* revision >= 2 */
    uint32_t    length;
    uint64_t    xsdt_address;
    uint8_t     ext_checksum;
    uint8_t     reserved[3];
} __attribute__((packed));

/* Common header on every other table */
struct acpi_header
{
    char        signature[4];
    uint32_t    length;
    uint8_t     revision;
    uint8_t     checksum;
    char        oem_id[6];
    char        oem_table_id[8];
    uint32_t    oem_revision;
    char        creator_id[4];
    uint32_t    creator_revision;
} __attribute__((packed));

struct acpi_madt
{
    struct acpi_header  header;
    uint32_t            lapic_address;
    uint32_t            flags;
} __attribute__((packed));

struct acpi_madt_entry
{
    uint8_t     type;
    uint8_t     length;
} __attribute__((packed));

/* MADT entry types this port cares about */
#define ACPI_MADT_TYPE_RISCV_INTC   24
#define ACPI_MADT_TYPE_RISCV_IMSIC  25
#define ACPI_MADT_TYPE_RISCV_APLIC  26
#define ACPI_MADT_TYPE_RISCV_PLIC   27

/* Type 24: RINTC - one per hart on RISC-V */
struct acpi_madt_rintc
{
    struct acpi_madt_entry  entry;
    uint8_t                 version;
    uint8_t                 reserved;
    uint32_t                flags;
    uint64_t                hart_id;
    uint32_t                uid;
    uint32_t                ext_intc_id;
    uint64_t                imsic_addr;
    uint32_t                imsic_size;
} __attribute__((packed));

/* Type 27: PLIC - the platform interrupt controller itself */
struct acpi_madt_plic
{
    struct acpi_madt_entry  entry;
    uint8_t                 version;
    uint8_t                 id;
    uint8_t                 hardware_id[8];
    uint16_t                nsources;
    uint16_t                max_priority;
    uint32_t                flags;
    uint32_t                size;
    uint64_t                base;
    uint32_t                gsi_base;
} __attribute__((packed));

struct acpi_mcfg_alloc
{
    uint64_t    address;
    uint16_t    segment;
    uint8_t     start_bus;
    uint8_t     end_bus;
    uint32_t    reserved;
} __attribute__((packed));

/* Set by the EFI stub (see efi_stub.c); zero when not booted via UEFI */
extern unsigned long __efi_rsdp;
extern unsigned long __efi_systab;
extern unsigned long __efi_ntables;
extern unsigned char __efi_tableguids[][16];

static void put_sig(const char *s, int n)
{
    char buf[16];
    int i;

    for (i = 0; i < n; i++)
        buf[i] = (s[i] >= ' ' && s[i] < 0x7f) ? s[i] : '.';
    buf[n] = 0;
    krnSBIPutStr(buf);
}

static void put_dec(uint64_t v)
{
    char buf[24];
    int i = sizeof(buf) - 1;

    buf[i] = 0;
    do {
        buf[--i] = '0' + (v % 10);
        v /= 10;
    } while (v && i);
    krnSBIPutStr(&buf[i]);
}

/* Print a GUID in the usual form, and name it when we know it */
static void dump_guid(const unsigned char *g)
{
    static const struct
    {
        unsigned char guid[16];
        const char   *name;
    } known[] = {
        { { 0x71,0xe8,0x68,0x88, 0xf1,0xe4, 0xd3,0x11,
            0xbc,0x22, 0x00,0x80,0xc7,0x3c,0x88,0x81 }, "ACPI 2.0+" },
        { { 0x30,0x2d,0x9d,0xeb, 0x88,0x2d, 0xd3,0x11,
            0x9a,0x16, 0x00,0x90,0x27,0x3f,0xc1,0x4d }, "ACPI 1.0" },
        { { 0xd5,0x21,0xb6,0xb1, 0x9c,0xf1, 0xa5,0x41,
            0x83,0x0b, 0xd9,0x15,0x2c,0x69,0xaa,0xe0 }, "device tree" },
        { { 0x31,0x2d,0x9d,0xeb, 0x88,0x2d, 0xd3,0x11,
            0x9a,0x16, 0x00,0x90,0x27,0x3f,0xc1,0x4d }, "SMBIOS" },
        { { 0x44,0x15,0xfd,0xf2, 0x94,0x97, 0x2c,0x4a,
            0x99,0x2e, 0xe5,0xbb,0xcf,0x20,0xe3,0x94 }, "SMBIOS 3" },
    };
    static const char hex[] = "0123456789abcdef";
    /* 8-4-4-4-12: the first three fields are little endian */
    static const signed char seq[] =
        { 3,2,1,0, -1, 5,4, -1, 7,6, -1, 8,9, -1, 10,11,12,13,14,15 };
    char buf[40];
    unsigned int i, n = 0, k;

    for (i = 0; i < sizeof(seq) / sizeof(seq[0]); i++)
    {
        if (seq[i] < 0)
            buf[n++] = '-';
        else
        {
            unsigned char v = g[(unsigned int)seq[i]];

            buf[n++] = hex[(v >> 4) & 0xF];
            buf[n++] = hex[v & 0xF];
        }
    }
    buf[n] = 0;

    krnSBIPutStr("    ");
    krnSBIPutStr(buf);

    for (k = 0; k < sizeof(known) / sizeof(known[0]); k++)
    {
        unsigned int b;

        for (b = 0; b < 16; b++)
            if (known[k].guid[b] != g[b])
                break;
        if (b == 16)
        {
            krnSBIPutStr("  ");
            krnSBIPutStr(known[k].name);
            break;
        }
    }
    krnSBIPutStr("\n");
}

static void dump_madt(const struct acpi_madt *madt)
{
    const uint8_t *p = (const uint8_t *)madt + sizeof(*madt);
    const uint8_t *end = (const uint8_t *)madt + madt->header.length;

    while (p + sizeof(struct acpi_madt_entry) <= end)
    {
        const struct acpi_madt_entry *e = (const struct acpi_madt_entry *)p;

        if (e->length < sizeof(*e))
            break;

        if (e->type == ACPI_MADT_TYPE_RISCV_INTC &&
            e->length >= sizeof(struct acpi_madt_rintc))
        {
            const struct acpi_madt_rintc *r =
                (const struct acpi_madt_rintc *)p;

            krnSBIPutStr("      RINTC hart ");
            put_dec(r->hart_id);
            krnSBIPutStr(r->flags & 1 ? " (enabled)" : " (disabled)");
            krnSBIPutStr(" uid ");
            put_dec(r->uid);
            krnSBIPutStr(" extintc ");
            krnSBIPutHex32(r->ext_intc_id);
            if (r->imsic_addr)
            {
                krnSBIPutStr(" imsic ");
                krnSBIPutHex(r->imsic_addr);
            }
            krnSBIPutStr("\n");
        }
        else if (e->type == ACPI_MADT_TYPE_RISCV_PLIC &&
                 e->length >= sizeof(struct acpi_madt_plic))
        {
            const struct acpi_madt_plic *pl =
                (const struct acpi_madt_plic *)p;

            krnSBIPutStr("      PLIC id ");
            put_dec(pl->id);
            krnSBIPutStr(" base ");
            krnSBIPutHex(pl->base);
            krnSBIPutStr(" size ");
            krnSBIPutHex32(pl->size);
            krnSBIPutStr(" sources ");
            put_dec(pl->nsources);
            krnSBIPutStr(" gsibase ");
            put_dec(pl->gsi_base);
            krnSBIPutStr("\n");
        }
        else
        {
            /* 24 RINTC, 25 IMSIC, 26 APLIC, 27 PLIC on RISC-V */
            krnSBIPutStr("      entry type ");
            put_dec(e->type);
            krnSBIPutStr(" length ");
            put_dec(e->length);
            krnSBIPutStr("\n");
        }

        p += e->length;
    }
}

/*
 * Find a table by signature, walking the XSDT (or the RSDT on a
 * revision 1 pointer). Returns NULL when the firmware left no tables.
 */
static const struct acpi_header *acpi_find(const char *sig)
{
    const struct acpi_rsdp *rsdp = (const struct acpi_rsdp *)__efi_rsdp;
    const struct acpi_header *root;
    uint32_t entries, i;
    int wide;

    if (!__efi_rsdp)
        return NULL;

    wide = (rsdp->revision >= 2) && rsdp->xsdt_address;
    root = (const struct acpi_header *)(unsigned long)
           (wide ? rsdp->xsdt_address : (uint64_t)rsdp->rsdt_address);
    if (!root || root->length <= sizeof(*root))
        return NULL;

    entries = (root->length - sizeof(*root)) / (wide ? 8 : 4);

    for (i = 0; i < entries; i++)
    {
        const struct acpi_header *h;
        unsigned long addr;

        if (wide)
        {
            /* The array is only 4-byte aligned, so read it in halves */
            const uint32_t *e = (const uint32_t *)((const uint8_t *)(root + 1) + i * 8);

            addr = (unsigned long)e[0] | ((unsigned long)e[1] << 32);
        }
        else
            addr = (unsigned long)((const uint32_t *)(root + 1))[i];

        h = (const struct acpi_header *)addr;
        if (h && h->signature[0] == sig[0] && h->signature[1] == sig[1] &&
            h->signature[2] == sig[2] && h->signature[3] == sig[3])
            return h;
    }

    return NULL;
}

/*
 * Describe the interrupt controller from the MADT.
 *
 * A UEFI machine may hand over no device tree at all, and the tree is
 * the only thing krnPLICInit() knows how to read. The same facts are
 * in the MADT: one PLIC structure says where the controller is and how
 * many sources it has, and one RINTC per hart carries the PLIC context
 * that hart's supervisor mode claims from.
 *
 * Filling the same krnFDTInfo fields keeps this to a source of facts -
 * the controller is still brought up by the one function, the same way,
 * whichever description it came from.
 *
 * Returns 0 when there is nothing to find, leaving info untouched.
 */
int krnACPIPLICInfo(struct krnFDTInfo *info)
{
    const struct acpi_header *madt = acpi_find("APIC");
    const uint8_t *p, *end;
    int found = 0;

    if (!madt)
        return 0;

    p = (const uint8_t *)madt + sizeof(struct acpi_madt);
    end = (const uint8_t *)madt + madt->length;

    info->plic_ncontexts = 0;

    while (p + sizeof(struct acpi_madt_entry) <= end)
    {
        const struct acpi_madt_entry *e = (const struct acpi_madt_entry *)p;

        if (e->length < sizeof(*e))
            break;

        if (e->type == ACPI_MADT_TYPE_RISCV_PLIC &&
            e->length >= sizeof(struct acpi_madt_plic))
        {
            const struct acpi_madt_plic *pl = (const struct acpi_madt_plic *)p;

            /*
             * Only the first controller is taken. Everything this port
             * drives hangs off one PLIC, and a second would need the
             * sources renumbered by its GSI base to tell them apart.
             */
            if (!info->plic_base)
            {
                info->plic_base = pl->base;
                info->plic_size = pl->size;
                info->plic_ndev = pl->nsources;
                found = 1;
            }
        }
        else if (e->type == ACPI_MADT_TYPE_RISCV_INTC &&
                 e->length >= sizeof(struct acpi_madt_rintc))
        {
            const struct acpi_madt_rintc *r = (const struct acpi_madt_rintc *)p;

            /*
             * The low half of the external interrupt controller id is
             * the PLIC context this hart's supervisor mode uses; the
             * top byte names the controller, and only the first one is
             * described here. A hart the firmware marked disabled has
             * no usable context.
             */
            if ((r->flags & 1) && ((r->ext_intc_id >> 24) == 0) &&
                info->plic_ncontexts < KRN_MAX_PLIC_CONTEXTS)
            {
                info->plic_contexts[info->plic_ncontexts++] =
                    (int)(r->ext_intc_id & 0xffff);
            }
        }

        p += e->length;
    }

    if (!found)
        return 0;

    /*
     * krnPLICInit() enables on every context it is given and claims
     * from whichever answers, so the one named here only has to be a
     * real context - the boot hart's is the obvious choice.
     */
    if (info->plic_ncontexts)
        info->plic_context = info->plic_contexts[0];

    krnSBIPutStr("plic:      described by ACPI, ");
    put_dec(info->plic_ncontexts);
    krnSBIPutStr(" supervisor context(s)\n");

    return 1;
}

static void dump_mcfg(const struct acpi_header *h)
{
    const uint8_t *p = (const uint8_t *)h + sizeof(*h) + 8;
    const uint8_t *end = (const uint8_t *)h + h->length;

    while (p + sizeof(struct acpi_mcfg_alloc) <= end)
    {
        const struct acpi_mcfg_alloc *a = (const struct acpi_mcfg_alloc *)p;

        krnSBIPutStr("      ECAM ");
        krnSBIPutHex(a->address);
        krnSBIPutStr(" segment ");
        put_dec(a->segment);
        krnSBIPutStr(" buses ");
        put_dec(a->start_bus);
        krnSBIPutStr("-");
        put_dec(a->end_bus);
        krnSBIPutStr("\n");

        p += sizeof(*a);
    }
}

static void dump_table(const struct acpi_header *h)
{
    krnSBIPutStr("  ");
    put_sig(h->signature, 4);
    krnSBIPutStr(" @ ");
    krnSBIPutHex((unsigned long)h);
    krnSBIPutStr(" length ");
    put_dec(h->length);
    krnSBIPutStr(" rev ");
    put_dec(h->revision);
    krnSBIPutStr(" oem ");
    put_sig(h->oem_id, 6);
    krnSBIPutStr("\n");

    if (h->signature[0] == 'A' && h->signature[1] == 'P' &&
        h->signature[2] == 'I' && h->signature[3] == 'C')
        dump_madt((const struct acpi_madt *)h);
    else if (h->signature[0] == 'M' && h->signature[1] == 'C' &&
             h->signature[2] == 'F' && h->signature[3] == 'G')
        dump_mcfg(h);
}

void krnDumpACPI(void)
{
    const struct acpi_rsdp *rsdp = (const struct acpi_rsdp *)__efi_rsdp;
    const struct acpi_header *sdt;
    unsigned int entries, i;
    int wide;

    if (!rsdp)
    {
        if (!__efi_systab)
        {
            krnSBIPutStr("--- no ACPI: not booted through UEFI ---\n");
            return;
        }

        /*
         * Booted through UEFI, but the firmware published no ACPI root
         * pointer. List what it did publish - a machine that can do
         * either ACPI or device tree may simply be set to hand over a
         * device tree, and the GUIDs say which.
         */
        krnSBIPutStr("--- no ACPI root pointer in the EFI configuration"
                     " table ---\n");
        krnSBIPutStr("  the firmware published ");
        put_dec(__efi_ntables);
        krnSBIPutStr(" table(s):\n");

        for (i = 0; i < __efi_ntables; i++)
            dump_guid(__efi_tableguids[i]);

        krnSBIPutStr("--- end ACPI ---\n");
        return;
    }

    krnSBIPutStr("--- ACPI ---\n");
    krnSBIPutStr("  RSDP @ ");
    krnSBIPutHex((unsigned long)rsdp);
    krnSBIPutStr(" rev ");
    put_dec(rsdp->revision);
    krnSBIPutStr(" oem ");
    put_sig(rsdp->oem_id, 6);
    krnSBIPutStr("\n");

    /* Revision 2 and later carry an XSDT, with 64bit table pointers */
    wide = (rsdp->revision >= 2 && rsdp->xsdt_address);
    sdt = wide ? (const struct acpi_header *)(unsigned long)rsdp->xsdt_address
               : (const struct acpi_header *)(unsigned long)rsdp->rsdt_address;

    if (!sdt)
    {
        krnSBIPutStr("  no root table!\n");
        return;
    }

    entries = (sdt->length - sizeof(*sdt)) / (wide ? 8 : 4);

    for (i = 0; i < entries; i++)
    {
        unsigned long addr;

        if (wide)
        {
            /* The XSDT's array is not 8-aligned - read it a byte at a time */
            const uint8_t *e = (const uint8_t *)sdt + sizeof(*sdt) + i * 8;
            unsigned int b;

            addr = 0;
            for (b = 0; b < 8; b++)
                addr |= (unsigned long)e[b] << (b * 8);
        }
        else
            addr = ((const uint32_t *)((const uint8_t *)sdt +
                                       sizeof(*sdt)))[i];

        if (addr)
            dump_table((const struct acpi_header *)addr);
    }

    krnSBIPutStr("--- end ACPI ---\n");
}
