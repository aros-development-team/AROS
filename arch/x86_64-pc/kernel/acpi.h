/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS ACPI Definitions.
    Lang: english
*/

#include <resources/acpi.h>
#include <utility/hooks.h>

/* ACPI Table Parser func protos */
void ACPI_Table_MADT_Parse(struct ACPI_TABLE_TYPE_MADT *madt);
int ACPI_Table_HPET_Parse(struct ACPI_TABLE_TYPE_HPET *hpet_tbl);

AROS_UFP2(IPTR, ACPI_hook_Table_LAPIC_Count,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC *, processor, A2));
AROS_UFP2(IPTR, ACPI_hook_Table_LAPIC_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC *, processor, A2));
AROS_UFP2(IPTR, ACPI_hook_Table_LAPIC_Addr_Ovr_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC_ADDROVR *, lapic_addr_ovr, A2));
AROS_UFP2(IPTR, ACPI_hook_Table_LAPIC_NMI_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC_NMI *, lapic_nmi, A2));
AROS_UFP2(IPTR, ACPI_hook_Table_IOAPIC_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_IOAPIC *, ioapic, A2));
AROS_UFP2(IPTR, ACPI_hook_Table_Int_Src_Ovr_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_INT_SRCOVR *, intsrc, A2));
AROS_UFP2(IPTR, ACPI_hook_Table_NMI_Src_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC_NMI_SRC *, nmi_src, A2));

/** ACPI Functions **/
ULONG core_ACPIInitialise(void);
