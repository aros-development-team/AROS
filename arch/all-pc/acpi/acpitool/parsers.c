#include <resources/acpi.h>
#include <proto/acpi.h>

#include <stdio.h>

#include "locale.h"
#include "parsers.h"

#define D(x)

/* Buffer for text formatting */
char buf[BUFFER_SIZE];

static inline void MakeString(void (*callback)(const char *), const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    D(printf("%s\n", buf));
    
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

#define FLAG_VAL(v) ((v) ? _(MSG_YES) : _(MSG_NO))

static void parse_flags(ULONG flags, const char **table, void (*cb)(const char *))
{
    unsigned int i;

    for (i = 0; table[i]; i++)
    {
	MakeString(cb, "  %s: %s", table[i], FLAG_VAL(flags & (1 << i)));
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

static void parse_addr(const char *desc, struct GENERIC_ACPI_ADDR *addr, void (*cb)(const char *))
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
		addr->register_bit_width, addr->register_bit_offset,
		space, addr->address_space_id);
}

void header_parser(struct ACPI_TABLE_DEF_HEADER *table, void (*cb)(const char *))
{
    MakeString(cb, "%s: %.4s %s %u, %s 0x%p",
	       _(MSG_TABLE_SIGNATURE), &table->signature,
	       _(MSG_REVISION), table->revision, _(MSG_ADDRESS), table);
    MakeString(cb, "%s: %.6s", _(MSG_OEM_ID), &table->oem_id);
    MakeString(cb, "%s: %.8s %s %u",
	       _(MSG_OEM_TABLE_ID), &table->oem_table_id,
	       _(MSG_REVISION), table->oem_revision);
    MakeString(cb, "%s: %.4s %s %u",
	       _(MSG_CREATOR_ID), &table->asl_compiler_id,
	       _(MSG_REVISION), table->asl_compiler_revision);
}

static void rsdt_parser(struct ACPI_TABLE_DEF_HEADER *rsdt, void (*cb)(const char *))
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

static void fadt_parser(struct ACPI_TABLE_TYPE_FADT *fadt, void (*cb)(const char *))
{
    header_parser(&fadt->header, cb);
    
    parse_enum(_(MSG_PM_PROFILE), fadt->pm_profile, Profiles, cb);

    MakeString(cb, "%s: %d", _(MSG_SCI_INT), fadt->sci_int);
    MakeString(cb, "%s: 0x%08X", _(MSG_SMI_CMD), fadt->smi_cmd);
    MakeString(cb, "%s: 0x%02X", _(MSG_ACPI_ENABLE), fadt->acpi_enable);
    MakeString(cb, "%s: 0x%02X", _(MSG_ACPI_DISABLE), fadt->acpi_disable);
    MakeString(cb, "%s: 0x%02X", _(MSG_S4BIOS), fadt->s4bios_req);
    MakeString(cb, "%s: 0x%02X", _(MSG_PSTATE), fadt->pstate_cnt);
    
    if (!(fadt->flags & FACP_FF_WBINVD))
    {
	MakeString(cb, "%s: %u", _(MSG_FLUSH_SIZE), fadt->flush_size);
	MakeString(cb, "%s: %u", _(MSG_FLUSH_STRIDE), fadt->flush_stride);
    }

    if (fadt->day_alarm)
	MakeString(cb, "%s: 0x%02X", _(MSG_RTC_DAY_ALARM), fadt->day_alarm);
    if (fadt->mon_alarm)
	MakeString(cb, "%s: 0x%02X", _(MSG_RTC_MON_ALARM), fadt->mon_alarm);
    if (fadt->century)
	MakeString(cb, "%s: 0x%02X", _(MSG_RTC_CENTURY), fadt->century);

    MakeString(cb, "%s:", _(MSG_PC_FLAGS));
    parse_flags(fadt->pc_arch, pc_flags, cb);
    
    MakeString(cb, "%s:", _(MSG_FF_FLAGS));
    parse_flags(fadt->flags, facp_flags, cb);

    parse_addr(_(MSG_RESET_REG), &fadt->reset_reg, cb);
    MakeString(cb, "%s: 0x%02X", _(MSG_RESET_VAL), fadt->reset_value);
}

static const char *irq_polarity[] =
{
    "Bus default",
    "Active high",
    "Invalid (2)",
    "Active low",
    NULL
};

static const char *irq_trigger[] =
{
    "Bus default",
    "Edge",
    "Invalid (2)",
    "Level",
    NULL
};

static inline void parse_int_flags(ACPI_INT_FLAGS flags, void (*cb)(const char *))
{
    parse_enum(_(MSG_POLARITY), flags.polarity, irq_polarity, cb);
    parse_enum(_(MSG_TRIGGER), flags.trigger, irq_trigger, cb);
}

static const char *srcovr_buses[] =
{
    "ISA",
    NULL
};

static const char *int_types[] =
{
    "Unknown (0)",
    "PMI",
    "INIT",
    "Corrected platform error",
    NULL
};

AROS_UFH3(static void, madt_entry_parser,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_DEF_ENTRY_HEADER *, entry, A2),
	  AROS_UFHA(out_func, cb, A1))
{
    AROS_USERFUNC_INIT

    struct ACPI_TABLE_TYPE_LAPIC *lapic;
    struct ACPI_TABLE_TYPE_IOAPIC *ioapic;
    struct ACPI_TABLE_TYPE_INT_SRCOVR *srcovr;
    struct ACPI_TABLE_TYPE_LAPIC_NMI_SRC *nmisrc;
    struct ACPI_TABLE_TYPE_LAPIC_NMI *nmi;
    struct ACPI_TABLE_TYPE_IOSAPIC *iosapic;
    struct ACPI_TABLE_TYPE_LSAPIC *lsapic;
    struct ACPI_TABLE_TYPE_PLAT_INTSRC *intsrc;
    struct ACPI_TABLE_TYPE_X2APIC *x2apic;
    struct ACPI_TABLE_TYPE_X2APIC_NMI *x2nmi;

    cb("");	/* Insert empty line */

    switch (entry->type)
    {
    case ACPI_MADT_LAPIC:
	lapic = (struct ACPI_TABLE_TYPE_LAPIC *)entry;

	MakeString(cb, "%s:", _(MSG_LOCAL_APIC));
	MakeString(cb, "  %s: 0x%02X", _(MSG_CPU_ID), lapic->acpi_id);
	MakeString(cb, "  %s: 0x%02X", _(MSG_LAPIC_ID), lapic->id);
	MakeString(cb, "  %s: %s", _(MSG_ENABLED), FLAG_VAL(lapic->flags.enabled));
	break;

    case ACPI_MADT_IOAPIC:
	ioapic = (struct ACPI_TABLE_TYPE_IOAPIC *)entry;

	MakeString(cb, "%s:", _(MSG_IOAPIC));
	MakeString(cb, "  %s: 0x%02X", _(MSG_ID), ioapic->id);
	MakeString(cb, "  %s: 0x%04X", _(MSG_ADDRESS), ioapic->address);
	MakeString(cb, "  %s: %d", _(MSG_IRQ_BASE), ioapic->global_irq_base);
	break;

    case ACPI_MADT_INT_SRC_OVR:
	srcovr = (struct ACPI_TABLE_TYPE_INT_SRCOVR *)entry;

	MakeString(cb, "%s:", _(MSG_INT_SRC_OVR));
	parse_enum(_(MSG_BUS), srcovr->bus, srcovr_buses, cb);
	MakeString(cb, "  %s: %u", _(MSG_BUS_IRQ), srcovr->bus_irq);
	MakeString(cb, "  %s: %u", _(MSG_GLOBAL_IRQ), srcovr->global_irq);
	parse_int_flags(srcovr->flags, cb);
	break;

    case ACPI_MADT_NMI_SRC:
    	nmisrc = (struct ACPI_TABLE_TYPE_LAPIC_NMI_SRC *)entry;

	MakeString(cb, "%s:", _(MSG_NMI_SRC));
	parse_int_flags(nmisrc->flags, cb);
	MakeString(cb, "  %s: %u", _(MSG_GLOBAL_IRQ), nmisrc->global_irq);
	break;

    case ACPI_MADT_LAPIC_NMI:
	nmi = (struct ACPI_TABLE_TYPE_LAPIC_NMI *)entry;

	MakeString(cb, "%s:", _(MSG_LAPIC_NMI));
	if (nmi->acpi_id == ACPI_ID_BROADCAST)
	    MakeString(cb, "  %s: %s", _(MSG_CPU_ID), _(MSG_ALL));
	else
	    MakeString(cb, "  %s: 0x%02X", _(MSG_CPU_ID), nmi->acpi_id);
	parse_int_flags(nmi->flags, cb);
	MakeString(cb, "  %s: %u", _(MSG_LINT), nmi->lint);
	break;

    case ACPI_MADT_LAPIC_ADDR_OVR:
	MakeString(cb, "%s: 0x%016llX", _(MSG_LAPIC_ADDR_OVR),
		   ((struct ACPI_TABLE_TYPE_LAPIC_ADDROVR *)entry)->address);
	break;

    case ACPI_MADT_IOSAPIC:
	iosapic = (struct ACPI_TABLE_TYPE_IOSAPIC *)entry;

	MakeString(cb, "%s:", _(MSG_IOSAPIC));
	MakeString(cb, "  %s: 0x%02X", _(MSG_ID), iosapic->id);
	MakeString(cb, "  %s: %u", _(MSG_IRQ_BASE), iosapic->global_irq_base);
	MakeString(cb, "  %s: 0x%016llX", _(MSG_ADDRESS), iosapic->address);
	break;

    case ACPI_MADT_LSAPIC:
	lsapic = (struct ACPI_TABLE_TYPE_LSAPIC *)entry;

	MakeString(cb, "%s:", _(MSG_LSAPIC));
	MakeString(cb, "  %s: 0x%02X", _(MSG_CPU_ID), lsapic->acpi_id);
	MakeString(cb, "  %s: 0x%02X", _(MSG_LSAPIC_ID), lsapic->id);
	MakeString(cb, "  %s: 0x%02X", _(MSG_LSAPIC_EID), lsapic->eid);
	MakeString(cb, "  %s: %s", _(MSG_ENABLED), FLAG_VAL(lsapic->flags.enabled));
	break;

    case ACPI_MADT_PLAT_INT_SRC:
	intsrc = (struct ACPI_TABLE_TYPE_PLAT_INTSRC *)entry;

	MakeString(cb, "%s:", _(MSG_PLAT_INT_SRC));
	parse_int_flags(intsrc->flags, cb);
	parse_enum(_(MSG_PLAT_INT_TYPE), intsrc->type, int_types, cb);
	MakeString(cb, "  %s: 0x%02X", _(MSG_DEST_LSAPIC_ID), intsrc->id);
	MakeString(cb, "  %s: 0x%02X", _(MSG_DEST_EID), intsrc->eid);
	MakeString(cb, "  %s: 0x%02X", _(MSG_IOSAPIC_VECTOR), intsrc->iosapic_vector);
	MakeString(cb, "  %s: %u", _(MSG_GLOBAL_IRQ), intsrc->global_irq);
	MakeString(cb, "  %s: %s", _(MSG_CPEI_PROC_OVR),
		   FLAG_VAL(intsrc->srcflags & ACPI_CPEI_PROC_OVERRIDE));
	break;

    case ACPI_MADT_X2APIC:
	x2apic = (struct ACPI_TABLE_TYPE_X2APIC *)entry;

	MakeString(cb, "%s:", _(MSG_LOCAL_X2APIC));
	MakeString(cb, "  %s: 0x%08X", _(MSG_X2APIC_ID), x2apic->id);
	MakeString(cb, "  %s: %s", _(MSG_ENABLED),
		   FLAG_VAL(x2apic->flags & ACPI_X2APIC_ENABLED));
	MakeString(cb, "  %s: 0x%08X", _(MSG_CPU_UID), x2apic->acpi_uid);
	break;

    case ACPI_MADT_X2APIC_NMI:
	x2nmi = (struct ACPI_TABLE_TYPE_X2APIC_NMI *)entry;

	MakeString(cb, "%s:",_(MSG_X2APIC_NMI));
	parse_int_flags(x2nmi->flags, cb);
	if (x2nmi->acpi_uid == ACPI_UID_BROADCAST)
	    MakeString(cb, "  %s: %s", _(MSG_CPU_UID), _(MSG_ALL));
	else
	    MakeString(cb, "  %s: 0x%08X", _(MSG_CPU_UID), x2nmi->acpi_uid);
	MakeString(cb, "  %s: %u", _(MSG_LINT), x2nmi->lint);
	break;

    default:
	MakeString(cb, _(MSG_UNKNOWN_ENTRY), entry->type, entry->length);
	break;
    }

    AROS_USERFUNC_EXIT
}

static const struct Hook madtHook =
{
    .h_Entry = (APTR)madt_entry_parser
};

static void madt_parser(struct ACPI_TABLE_TYPE_MADT *madt, void (*cb)(const char *))
{
    D(printf("MADT at 0x%p, length %u\n", madt, len));

    header_parser(&madt->header, cb);

    MakeString(cb, "%s: 0x%08X", _(MSG_LAPIC_ADDR), madt->lapic_address);
    MakeString(cb, "%s: %s", _(MSG_PCAT_COMPAT), FLAG_VAL(madt->flags.pcat_compat));

    ACPI_ScanEntries(&madt->header, ACPI_ENTRY_TYPE_ALL, &madtHook, cb);
}

static const struct Parser Tables[] =
{
    {ACPI_MAKE_ID('R','S','D','T'), "System"    , rsdt_parser},
    {ACPI_MAKE_ID('X','S','D','T'), "System"    , rsdt_parser},
    {ACPI_MAKE_ID('F','A','C','P'), "Hardware"  , fadt_parser},
    {ACPI_MAKE_ID('A','P','I','C'), "Interrupts", madt_parser},
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
