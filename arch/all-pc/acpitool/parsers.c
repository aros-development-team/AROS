#include <proto/acpi.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

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

const char *sizes[] =
{
    "byte",
    "word",
    "double word",
    "quad word"
};

#define ACPI_PCI_OFFSET(addr) (unsigned short)((addr) & 0x0FFFF)
#define ACPI_PCI_FUNC(addr)   (unsigned short)(((addr) >> 16) & 0x0FFFF)
#define ACPI_PCI_DEV(addr)    (unsigned short)(((addr) >> 32) & 0x0FFFF)

static void parse_addr(const char *desc, const ACPI_GENERIC_ADDRESS *addr, void (*cb)(const char *))
{
    char *p = buf;
    int len = sizeof(buf);
    const char *space;
    int n, i;

    n = snprintf(p, len, "%s: ", desc);    
    p += n;
    len -= n;

    for (i = 0; i < 4; i++) {
        if (addr->BitWidth == 8*(1 << i)) {
            n = snprintf(p, len, "%s %s ", sizes[i], _(MSG_AT));
            break;
        }
    }
    if (i == 4)
    	n = snprintf(p, len, "%s (%u) %s ", _(MSG_UNKNOWN_SIZE), addr->BitWidth, _(MSG_AT));

    p += n;
    len -= n;

    if (addr->SpaceId == ACPI_ADR_SPACE_PCI_CONFIG)
    	n = snprintf(p, len, "0:%u:%u:0x%04X", ACPI_PCI_DEV(addr->Address), ACPI_PCI_FUNC(addr->Address),
    		     ACPI_PCI_OFFSET(addr->Address));
    else
    	n = snprintf(p, len, "0x%llX", (long long)addr->Address);

    p += n;
    len -= n;

    if (addr->BitWidth)
    {
    	n = snprintf(p, len, ", %s %u - %u", _(MSG_BITS), addr->BitOffset,
    		     addr->BitOffset + addr->BitWidth - 1);
    	p += n;
    	len -= n;
    }

    if (addr->SpaceId == ACPI_ADR_SPACE_FIXED_HARDWARE)
	space = _(MSG_SPACE_FIXED);
    else if (addr->SpaceId >= 0x80)
	space = _(MSG_SPACE_OEM);
    else
    {
	space = decode_enum(addr->SpaceId, spaces);
    }

    if (space)
        snprintf(p, len, _(MSG_FMT_KNOWN_SPACE), space);
    else
        snprintf(p, len, _(MSG_FMT_UNKNOWN_SPACE), addr->SpaceId);

    cb(buf);
}

static void header_parser(const ACPI_TABLE_HEADER *table, void (*cb)(const char *))
{
    MakeString(cb, "%s: %.4s, %s %u, %s 0x%p",
	       _(MSG_TABLE_SIGNATURE), table->Signature,
	       _(MSG_REVISION), table->Revision, _(MSG_ADDRESS), table);
    MakeString(cb, "%s: %.6s", _(MSG_OEM_ID), &table->OemId);
    MakeString(cb, "%s: %.8s %s 0x%08X",
	       _(MSG_OEM_TABLE_ID), table->OemTableId,
	       _(MSG_REVISION), table->OemRevision);
    MakeString(cb, "%s: %.4s %s 0x%08X",
	       _(MSG_CREATOR_ID), table->AslCompilerId,
	       _(MSG_REVISION), table->AslCompilerRevision);
}

static void dumpData(const unsigned char *data, int length, void (*cb)(const char *))
{
    int len;
    int i;
    int left = length;

    while (left > 0)
    {
	char *p = buf;
	int buflen = sizeof(buf);

        len = snprintf(p, buflen, "%p:", data + length - left);
        p += len;
        buflen -= len;

        for (i = 0; i < 16 && i < left; i++)
        {
            len = snprintf(p, buflen, " %02x", data[i]);
            p += len;
            buflen -= len;
        }

        if (i != 15)
        {
            for (; i < 16; i++)
            {
            	*p++ = ' ';
            	*p++ = ' ';
            	*p++ = ' ';
            }
        }

        *p++ = ' ';
        *p++ = ' ';

        for (i = 0; i < 16 && i < left; i++)
        {
            *p++ = isprint(data[i]) ? data[i] : '.';
        }
        
        cb(buf);

        data += 16;
        left -= 16;
    }
}

void unknown_parser(const ACPI_TABLE_HEADER *table, void (*cb)(const char *))
{
    header_parser(table, cb);
    cb("");

    dumpData((const unsigned char *)table, table->Length, cb);
}

static void rsdt_parser(const ACPI_TABLE_HEADER *rsdt, void (*cb)(const char *))
{
    MakeString(cb, "%s: 0x%p", _(MSG_RSDP_ADDR), rsdt);

    cb("");

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

static void fadt_parser(const ACPI_TABLE_HEADER *header, void (*cb)(const char *))
{
    const ACPI_TABLE_FADT *fadt = (const ACPI_TABLE_FADT *)header;
    header_parser(&fadt->Header, cb);
    cb("");
    
    parse_enum(_(MSG_PM_PROFILE), fadt->PreferredProfile, Profiles, cb);

    MakeString(cb, "%s: %d", _(MSG_SCI_INT), fadt->SciInterrupt);
    MakeString(cb, "%s: 0x%08X", _(MSG_SMI_CMD), fadt->SmiCommand);
    MakeString(cb, "%s: 0x%02X", _(MSG_ACPI_ENABLE), fadt->AcpiEnable);
    MakeString(cb, "%s: 0x%02X", _(MSG_ACPI_DISABLE), fadt->AcpiDisable);
    MakeString(cb, "%s: 0x%02X", _(MSG_S4BIOS), fadt->S4BiosRequest);
    MakeString(cb, "%s: 0x%02X", _(MSG_PSTATE), fadt->PstateControl);
    
    if (!(fadt->Flags & ACPI_FADT_WBINVD))
    {
	MakeString(cb, "%s: %u", _(MSG_FLUSH_SIZE), fadt->FlushSize);
	MakeString(cb, "%s: %u", _(MSG_FLUSH_STRIDE), fadt->FlushStride);
    }

    if (fadt->DayAlarm)
	MakeString(cb, "%s: 0x%02X", _(MSG_RTC_DAY_ALARM), fadt->DayAlarm);
    if (fadt->MonthAlarm)
	MakeString(cb, "%s: 0x%02X", _(MSG_RTC_MON_ALARM), fadt->MonthAlarm);
    if (fadt->Century)
	MakeString(cb, "%s: 0x%02X", _(MSG_RTC_CENTURY), fadt->Century);

    MakeString(cb, "%s:", _(MSG_PC_FLAGS));
    parse_flags(fadt->BootFlags, pc_flags, cb);
    
    MakeString(cb, "%s:", _(MSG_FF_FLAGS));
    parse_flags(fadt->Flags, facp_flags, cb);

    /* ACPIBase-> 1.0 FADT ends here */
    if (fadt->Header.Length < offsetof(ACPI_TABLE_FADT, ResetRegister))
    	return;

    parse_addr(_(MSG_RESET_REG), &fadt->ResetRegister, cb);
    MakeString(cb, "%s: 0x%02X", _(MSG_RESET_VAL), fadt->ResetValue);
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

static inline void parse_int_flags(UINT16 flags, void (*cb)(const char *))
{
    parse_enum(_(MSG_POLARITY), flags & 3, irq_polarity, cb);
    parse_enum(_(MSG_TRIGGER), (flags >> 2) & 3, irq_trigger, cb);
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
	  AROS_UFHA(ACPI_SUBTABLE_HEADER *, entry, A2),
	  AROS_UFHA(out_func, cb, A1))
{
    AROS_USERFUNC_INIT

    ACPI_MADT_LOCAL_APIC *lapic;
    ACPI_MADT_IO_APIC *ioapic;
    ACPI_MADT_INTERRUPT_OVERRIDE *srcovr;
    ACPI_MADT_NMI_SOURCE *nmisrc;
    ACPI_MADT_LOCAL_APIC_NMI *nmi;
    ACPI_MADT_IO_SAPIC *iosapic;
    ACPI_MADT_LOCAL_SAPIC *lsapic;
    ACPI_MADT_INTERRUPT_SOURCE *intsrc;
    ACPI_MADT_LOCAL_X2APIC *x2apic;
    ACPI_MADT_LOCAL_X2APIC_NMI *x2nmi;

    cb("");	/* Insert empty line */

    switch (entry->Type)
    {
    case ACPI_MADT_TYPE_LOCAL_APIC:
	lapic = (ACPI_MADT_LOCAL_APIC *)entry;

	MakeString(cb, "%s:", _(MSG_LOCAL_APIC));
	MakeString(cb, "  %s: 0x%02X", _(MSG_CPU_ID), lapic->ProcessorId);
	MakeString(cb, "  %s: 0x%02X", _(MSG_LAPIC_ID), lapic->Id);
	MakeString(cb, "  %s: %s", _(MSG_ENABLED), FLAG_VAL(lapic->LapicFlags & ACPI_MADT_ENABLED));
	break;

    case ACPI_MADT_TYPE_IO_APIC:
	ioapic = (ACPI_MADT_IO_APIC *)entry;

	MakeString(cb, "%s:", _(MSG_IOAPIC));
	MakeString(cb, "  %s: 0x%02X", _(MSG_ID), ioapic->Id);
	MakeString(cb, "  %s: 0x%04X", _(MSG_ADDRESS), ioapic->Address);
	MakeString(cb, "  %s: %d", _(MSG_IRQ_BASE), ioapic->GlobalIrqBase);
	break;

    case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
	srcovr = (ACPI_MADT_INTERRUPT_OVERRIDE *)entry;

	MakeString(cb, "%s:", _(MSG_INT_SRC_OVR));
	parse_enum(_(MSG_BUS), srcovr->Bus, srcovr_buses, cb);
	MakeString(cb, "  %s: %u", _(MSG_BUS_IRQ), srcovr->SourceIrq);
	MakeString(cb, "  %s: %u", _(MSG_GLOBAL_IRQ), srcovr->GlobalIrq);
	parse_int_flags(srcovr->IntiFlags, cb);
	break;

    case ACPI_MADT_TYPE_NMI_SOURCE:
    	nmisrc = (ACPI_MADT_NMI_SOURCE *)entry;

	MakeString(cb, "%s:", _(MSG_NMI_SRC));
	parse_int_flags(nmisrc->IntiFlags, cb);
	MakeString(cb, "  %s: %u", _(MSG_GLOBAL_IRQ), nmisrc->GlobalIrq);
	break;

    case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
	nmi = (ACPI_MADT_LOCAL_APIC_NMI *)entry;

	MakeString(cb, "%s:", _(MSG_LAPIC_NMI));
	if (nmi->ProcessorId == (UINT8)~0)
	    MakeString(cb, "  %s: %s", _(MSG_CPU_ID), _(MSG_ALL));
	else
	    MakeString(cb, "  %s: 0x%02X", _(MSG_CPU_ID), nmi->ProcessorId);
	parse_int_flags(nmi->IntiFlags, cb);
	MakeString(cb, "  %s: %u", _(MSG_LINT), nmi->Lint);
	break;

    case ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE:
	MakeString(cb, "%s: 0x%016llX", _(MSG_LAPIC_ADDR_OVR),
		   ((ACPI_MADT_LOCAL_APIC_OVERRIDE *)entry)->Address);
	break;

    case ACPI_MADT_TYPE_IO_SAPIC:
	iosapic = (ACPI_MADT_IO_SAPIC *)entry;

	MakeString(cb, "%s:", _(MSG_IOSAPIC));
	MakeString(cb, "  %s: 0x%02X", _(MSG_ID), iosapic->Id);
	MakeString(cb, "  %s: %u", _(MSG_IRQ_BASE), iosapic->GlobalIrqBase);
	MakeString(cb, "  %s: 0x%016llX", _(MSG_ADDRESS), iosapic->Address);
	break;

    case ACPI_MADT_TYPE_LOCAL_SAPIC:
	lsapic = (ACPI_MADT_LOCAL_SAPIC *)entry;

	MakeString(cb, "%s:", _(MSG_LSAPIC));
	MakeString(cb, "  %s: 0x%02X", _(MSG_CPU_ID), lsapic->ProcessorId);
	MakeString(cb, "  %s: 0x%02X", _(MSG_LSAPIC_ID), lsapic->Id);
	MakeString(cb, "  %s: 0x%02X", _(MSG_LSAPIC_EID), lsapic->Eid);
	MakeString(cb, "  %s: %s", _(MSG_ENABLED), FLAG_VAL(lsapic->LapicFlags & 1));
	break;

    case ACPI_MADT_TYPE_INTERRUPT_SOURCE:
	intsrc = (ACPI_MADT_INTERRUPT_SOURCE *)entry;

	MakeString(cb, "%s:", _(MSG_PLAT_INT_SRC));
	parse_int_flags(intsrc->IntiFlags, cb);
	parse_enum(_(MSG_PLAT_INT_TYPE), intsrc->Type, int_types, cb);
	MakeString(cb, "  %s: 0x%02X", _(MSG_DEST_LSAPIC_ID), intsrc->Id);
	MakeString(cb, "  %s: 0x%02X", _(MSG_DEST_EID), intsrc->Eid);
	MakeString(cb, "  %s: 0x%02X", _(MSG_IOSAPIC_VECTOR), intsrc->IoSapicVector);
	MakeString(cb, "  %s: %u", _(MSG_GLOBAL_IRQ), intsrc->GlobalIrq);
	MakeString(cb, "  %s: %s", _(MSG_CPEI_PROC_OVR),
		   FLAG_VAL(intsrc->Flags & ACPI_MADT_CPEI_OVERRIDE));
	break;

    case ACPI_MADT_TYPE_LOCAL_X2APIC:
	x2apic = (ACPI_MADT_LOCAL_X2APIC *)entry;

	MakeString(cb, "%s:", _(MSG_LOCAL_X2APIC));
	MakeString(cb, "  %s: 0x%08X", _(MSG_X2APIC_ID), x2apic->LocalApicId);
	MakeString(cb, "  %s: %s", _(MSG_ENABLED),
		   FLAG_VAL(x2apic->LapicFlags & 1));
	MakeString(cb, "  %s: 0x%08X", _(MSG_CPU_UID), x2apic->Uid);
	break;

    case ACPI_MADT_TYPE_LOCAL_X2APIC_NMI:
	x2nmi = (ACPI_MADT_LOCAL_X2APIC_NMI *)entry;

	MakeString(cb, "%s:",_(MSG_X2APIC_NMI));
	parse_int_flags(x2nmi->IntiFlags, cb);
	if (x2nmi->Uid == (UINT32)~0)
	    MakeString(cb, "  %s: %s", _(MSG_CPU_UID), _(MSG_ALL));
	else
	    MakeString(cb, "  %s: 0x%08X", _(MSG_CPU_UID), x2nmi->Uid);
	MakeString(cb, "  %s: %u", _(MSG_LINT), x2nmi->Lint);
	break;

    default:
	MakeString(cb, _(MSG_UNKNOWN_ENTRY), entry->Type, entry->Length);
	break;
    }

    AROS_USERFUNC_EXIT
}

static const struct Hook madtHook =
{
    .h_Entry = (APTR)madt_entry_parser
};

static int MADT_ScanEntries(const ACPI_TABLE_MADT *madt, const struct Hook *hook, APTR userdata)
{
    const UINT8 *madt_entry = (const UINT8 *)&madt[1];
    const UINT8 *madt_end  = (const UINT8 *)madt + madt->Header.Length;
    int count;

    for (count = 0; madt_entry < madt_end; madt_entry += ((const ACPI_SUBTABLE_HEADER *)madt_entry)->Length) {
        const ACPI_SUBTABLE_HEADER *sh = (const ACPI_SUBTABLE_HEADER *)madt_entry;
        BOOL res;
        if (hook == NULL)
            res = TRUE;
        else
            res = CALLHOOKPKT((struct Hook *)hook, (APTR)sh, userdata);
        if (res)
            count++;
    }

    return count;
}

static void madt_parser(const ACPI_TABLE_HEADER *header, void (*cb)(const char *))
{
    const ACPI_TABLE_MADT *madt = (const ACPI_TABLE_MADT *)header;
    D(printf("MADT at 0x%p, length %u\n", madt, len));

    header_parser(&madt->Header, cb);
    cb("");

    MakeString(cb, "%s: 0x%08X", _(MSG_LAPIC_ADDR), madt->Address);
    MakeString(cb, "%s: %s", _(MSG_PCAT_COMPAT), FLAG_VAL(madt->Flags & ACPI_MADT_PCAT_COMPAT));

    MADT_ScanEntries(madt, &madtHook, cb);
}

static const char *hpet_protect[] =
{
    "None",
    "4KB",
    "64KB",
    NULL
};

/* ID components */
#define HPET_HW_REV_MASK		0x000000FF
#define HPET_NUM_COMPARATORS_MASK	0x00001F00
#define HPET_NUM_COMPARATORS_SHIFT	8
#define HPET_COUNTER_SIZE		0x00002000
#define HPET_LEGACY_REPLACEMENT		0x00008000
#define HPET_PCI_VENDOR_MASK		0xFFFF0000
#define HPET_PCI_VENDOR_SHIFT		16

/* page_protect components */
#define HPET_PAGE_PROTECT_MASK	0x0F
#define HPET_OEM_ATTR_MASK	0xF0
#define HPET_OEM_ATTR_SHIFT	4

#define HPET_PAGE_NONE	0
#define HPET_PAGE_4K	1
#define HPET_PAGE_64K	2



static void hpet_parser(const ACPI_TABLE_HEADER *header, void (*cb)(const char *))
{
    const ACPI_TABLE_HPET *hpet = (const ACPI_TABLE_HPET *)header;
    header_parser(&hpet->Header, cb);
    cb("");

    MakeString(cb, "%s: %u", _(MSG_HW_REVISION), hpet->Id & HPET_HW_REV_MASK);
    MakeString(cb, "%s: %u", _(MSG_NUM_COMPARATORS),
	       (hpet->Id & HPET_NUM_COMPARATORS_MASK) >> HPET_NUM_COMPARATORS_SHIFT);
    MakeString(cb, "%s: %s", _(MSG_64BIT_COUNTER), FLAG_VAL(hpet->Id & HPET_COUNTER_SIZE));
    MakeString(cb, "%s: %s", _(MSG_LEGACY_REPLACEMENT), FLAG_VAL(hpet->Id & HPET_LEGACY_REPLACEMENT));
    MakeString(cb, "%s: 0x%04X", _(MSG_PCI_VENDOR),
	       (hpet->Id & HPET_PCI_VENDOR_MASK) >> HPET_PCI_VENDOR_SHIFT);
    parse_addr(_(MSG_BASE_ADDRESS), &hpet->Address, cb);
    MakeString(cb, "%s: %u", _(MSG_NUMBER), hpet->Sequence);
    MakeString(cb, "%s: %u", _(MSG_MIN_TICK), hpet->MinimumTick);
    parse_enum(_(MSG_PAGE_PROTECT), hpet->Flags & HPET_PAGE_PROTECT_MASK,
	       hpet_protect, cb);
    MakeString(cb, "%s: 0x%02X", _(MSG_OEM_ATTRS), hpet->Flags >> HPET_OEM_ATTR_SHIFT);
}

static void sbst_parser(const ACPI_TABLE_HEADER *header, void (*cb)(const char *))
{
    const ACPI_TABLE_SBST *sbst = (const ACPI_TABLE_SBST *)header;
    header_parser(&sbst->Header, cb);
    cb("");

    MakeString(cb, "%s: %u %s", _(MSG_BATT_WARN), sbst->WarningLevel, _(MSG_MWH));
    MakeString(cb, "%s: %u %s", _(MSG_BATT_LOW), sbst->LowLevel, _(MSG_MWH));
    MakeString(cb, "%s: %u %s", _(MSG_BATT_CRITICAL), sbst->CriticalLevel, _(MSG_MWH));
}

static void ecdt_parser(const ACPI_TABLE_HEADER *header, void (*cb)(const char *))
{
    const ACPI_TABLE_ECDT *ecdt = (const ACPI_TABLE_ECDT *)header;
    header_parser(&ecdt->Header, cb);
    cb("");

    parse_addr(_(MSG_CMD_REG), &ecdt->Control, cb);
    parse_addr(_(MSG_DATA_REG), &ecdt->Data, cb);
    MakeString(cb, "%s: 0x%08X", _(MSG_UID), ecdt->Uid);
    MakeString(cb, "%s: %u", _(MSG_SCI_GPE_BIT), ecdt->Gpe);
    MakeString(cb, "%s: %s", _(MSG_NAMESPACE_ID), ecdt->Id);
}

const struct Parser ParserTable[] =
{
    {"RSDT", "System"    , rsdt_parser},
    {"XSDT", "System"    , rsdt_parser},
    {"FACP", "Hardware"  , fadt_parser},
    {"APIC", "Interrupts", madt_parser},
    {"HPET", "Timer"	, hpet_parser},
    {"SBST", "Battery"   , sbst_parser},
    {"ECDT", "Controller", ecdt_parser},
    {NULL, NULL, NULL}
};

const struct Parser *FindParser(const char *signature)
{
    const struct Parser *t;

    for (t = ParserTable; t->name; t++)
    {
	if (memcmp(signature,t->signature,4) == 0)
	    return t;
    }
    return NULL;
}
