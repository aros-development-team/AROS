#include <resources/acpi.h>

#include <stdio.h>

#include "locale.h"
#include "parsers.h"

/* Buffer for text formatting */
char buf[BUFFER_SIZE];

static inline void MakeString(void (*callback)(const char *), const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    callback(buf);
}    

static const char *decode_enum(ULONG val, const char **table)
{
    unsigned int i;

    for (i = 0; table[i]; i++)
    {
	if (val == i)
	    return table[i];
    }
    return NULL;
}

static void parse_enum(const char *desc, ULONG val, const char **table, void (*cb)(const char *))
{
    const char *valname = decode_enum(val, table);

    if (valname)
	MakeString(cb, "%s: %s", desc, valname);
    else
	MakeString(cb, "%s: %s (%u)", desc, _(MSG_UNKNOWN), val);
}    

static void parse_flags(ULONG flags, const char **table, void (*cb)(const char *))
{
    unsigned int i;

    for (i = 0; table[i]; i++)
    {
        const char *val = (flags & (1 << i)) ? _(MSG_YES) : _(MSG_NO);
	
	MakeString(cb, "  %s: %s", table[i], val);
    }
}    

const char *spaces[] =
{
    "memory",
    "I/O ports",
    "PCI configuration space",
    "Embedded controller",
    "SMBus",
    NULL
};

void parse_addr(const char *desc, struct GENERIC_ACPI_ADDR *addr, void (*cb)(const char *))
{
    BOOL print_id = FALSE;
    const char *space;
    const char *fmt;

    if (addr->address_space_id == ACPI_SPACE_FIXED)
	space = _(MSG_SPACE_FIXED);
    else if (addr->address_space_id >= ACPI_SPACE_OEM)
    {
	space = _(MSG_SPACE_OEM);
	print_id = TRUE;
    }
    else
    {
	space = decode_enum(addr->address_space_id, spaces);
	if (!space)
	{
	    space = _(MSG_SPACE_UNKNOWN);
	    print_id = TRUE;
	}
    }

    if (print_id)
	fmt = _(MSG_FMT_UNKNOWN_SPACE);
    else
	fmt = _(MSG_FMT_KNOWN_SPACE);

    MakeString(cb, fmt, desc, addr->address,
		addr->register_bit_offset, addr->register_bit_width,
		space, addr->address_space_id);
}

void header_parser(struct ACPI_TABLE_DEF_HEADER *table, void (*cb)(const char *))
{
    MakeString(cb, "%s: %.4s %s %u",
	       _(MSG_TABLE_SIGNATURE), &table->signature,
	       _(MSG_REVISION), table->revision);    
    MakeString(cb, "%s: %.6s", _(MSG_OEM_ID), &table->oem_id);
    MakeString(cb, "%s: %.8s %s %u",
	       _(MSG_OEM_TABLE_ID), &table->oem_table_id,
	       _(MSG_REVISION), table->oem_revision);
    MakeString(cb, "%s: %.4s %s %u",
	       _(MSG_CREATOR_ID), &table->asl_compiler_id,
	       _(MSG_REVISION), table->asl_compiler_revision);
}

void rsdt_parser(struct ACPI_TABLE_DEF_HEADER *rsdt, void (*cb)(const char *))
{
    header_parser(rsdt, cb);
}

static const char *Profiles[] =
{
    "Unspecified",
    "Desktop",
    "Mobile",
    "Workstation",
    "Enterprize server",
    "SOHO server",
    "Appliance",
    "Performance server"
};

static const char *pc_flags[] =
{
    "Have legacy devices",
    "Have 8042 keyboard controller",
    "VGA not present",
    "MSI not supported",
    "PCIe ASPM controls",
    NULL
};

static const char *facp_flags[] =
{
    "CPU has WBINVD instruction",
    "WBINVD flushes all caches",
    "C1 power state supported",
    "C2 power state for multiple CPUs",
    "No fixed power button",
    "No fixed sleep button",
    "RTC wake status in fixed registers",
    "RTC wakeup from S4 supported",
    "TMR_VAL is 32-bit",
    "Docking supported",
    "Reset register supported",
    "Sealed case",
    "CPU instruction execution needed for sleep",
    "PCI Express wakeup supported",
    "Use platform clock",
    "RTC_STS valid after S4",
    "Remote power on supported",
    "Force APIC cluster model",
    "Force APIC physical destination",
    NULL
};

void facs_parser(struct ACPI_TABLE_TYPE_FADT *facs, void (*cb)(const char *))
{
    header_parser(&facs->header, cb);
    
    parse_enum(_(MSG_PM_PROFILE), facs->pm_profile, Profiles, cb);

    MakeString(cb, "%s: %d", _(MSG_SCI_INT), facs->sci_int);
    MakeString(cb, "%s: 0x%08X", _(MSG_SMI_CMD), facs->smi_cmd);
    MakeString(cb, "%s: 0x%02X", _(MSG_ACPI_ENABLE), facs->acpi_enable);
    MakeString(cb, "%s: 0x%02X", _(MSG_ACPI_DISABLE), facs->acpi_disable);
    MakeString(cb, "%s: 0x%02X", _(MSG_S4BIOS), facs->s4bios_req);
    MakeString(cb, "%s: 0x%02X", _(MSG_PSTATE), facs->pstate_cnt);
    
    if (!(facs->flags & FACP_FF_WBINVD))
    {
	MakeString(cb, "%s: %u", _(MSG_FLUSH_SIZE), facs->flush_size);
	MakeString(cb, "%s: %u", _(MSG_FLUSH_STRIDE), facs->flush_stride);
    }

    if (facs->day_alarm)
	MakeString(cb, "%s: 0x%02X", _(MSG_RTC_DAY_ALARM), facs->day_alarm);
    if (facs->mon_alarm)
	MakeString(cb, "%s: 0x%02X", _(MSG_RTC_MON_ALARM), facs->mon_alarm);
    if (facs->century)
	MakeString(cb, "%s: 0x%02X", _(MSG_RTC_CENTURY), facs->century);

    MakeString(cb, "%s:", _(MSG_PC_FLAGS));
    parse_flags(facs->pc_arch, pc_flags, cb);
    
    MakeString(cb, "%s:", _(MSG_FF_FLAGS));
    parse_flags(facs->flags, facp_flags, cb);

    parse_addr(_(MSG_RESET_REG), &facs->reset_reg, cb);
    MakeString(cb, "%s: 0x%02X", _(MSG_RESET_VAL), facs->reset_value);
}

static const struct Parser Tables[] =
{
    {ACPI_MAKE_ID('R','S','D','T'), "System"  , rsdt_parser},
    {ACPI_MAKE_ID('X','S','D','T'), "System"  , rsdt_parser},
    {ACPI_MAKE_ID('F','A','C','P'), "Hardware", facs_parser},
    {0, NULL, NULL}
};

const struct Parser *FindParser(unsigned int signature)
{
    const struct Parser *t;

    for (t = Tables; t->name; t++)
    {
	if (signature == t->signature)
	    return t;
    }
    return NULL;
}
