/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: acpi.h,v 1.1 2004/01/02 22:23:57 nicja Exp $

    Desc: AROS ACPI Definitions.
    Lang: english
*/
#ifndef __AROS_ACPI_H__
#define __AROS_ACPI_H__

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef EXEC_SEMAPHORES
#   include <exec/semaphores.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif

#ifndef ACPI_MAX_TABLES
#define   ACPI_MAX_TABLES           32
#endif

struct KernelACPIData {
    BOOL                kb_APIC_Enabled;
    int                 kb_APIC_IRQ_Model;
    IPTR                kb_APIC_Driver;

    IPTR                kb_ACPI_SDT_Phys;
    int                 kb_ACPI_SDT_Count;
    IPTR                kb_ACPI_SDT_Entry[ACPI_MAX_TABLES];
/*..*/
    int                 kb_ACPI_Disabled;
    int                 kb_ACPI_IRQ;

    int                 kb_ACPI_HT;

    int                 kb_ACPI_LAPIC;
    int                 kb_ACPI_IOAPIC;

    BOOL                kb_SMP_Enabled;
    int                 kb_SMP_Config;
};

/* Table Handlers */
enum acpi_table_id {
	ACPI_TABLE_UNKNOWN          = 0,
	ACPI_APIC,
	ACPI_BOOT,
	ACPI_DBGP,
	ACPI_DSDT,
	ACPI_ECDT,
	ACPI_ETDT,
	ACPI_FADT,
	ACPI_FACS,
	ACPI_OEMX,
	ACPI_PSDT,
	ACPI_SBST,
	ACPI_SLIT,
	ACPI_SPCR,
	ACPI_SRAT,
	ACPI_SSDT,
	ACPI_SPMI,
	ACPI_HPET,
	ACPI_TABLE_COUNT
};

/* System Description Table (RSDT/XSDT) */
struct acpi_table_sdt
{
    unsigned long		        pa;
    enum acpi_table_id	        id;
    unsigned long		        size;
};

struct acpi_table_hook
{
    struct MinNode              h_MinNode;
    IPTR	                    (*h_Entry)();     /* Main entry point */
    IPTR	                    (*h_SubEntry)();  /* Secondary entry point */
    unsigned long               phys_addr;
    unsigned long               size;
};

struct acpi_madt_entry_hook
{
    struct MinNode              h_MinNode;
    IPTR	                    (*h_Entry)();     /* Main entry point */
    IPTR	                    (*h_SubEntry)();  /* Secondary entry point */
    struct acpi_table_entry_header     *header;
};

/********** ACPI structure definitions ****************/

/* ACPI 2.0 Generic Address Structure (GAS) */

struct acpi_generic_address
{
	unsigned char               address_space_id;               /* Address space where struct or register exists. */
	unsigned char               register_bit_width;             /* Size in bits of given register */
	unsigned char               register_bit_offset;            /* Bit offset within the register */
	unsigned char               reserved;                       /* Must be 0 */
	UQUAD                       address;                        /* 64-bit address of struct or register */
};

/********** ACPI DEFINITIONS ****************/

#define ACPI_TABLE_HEADER_DEF   /* ACPI common table header */ \
	char                        signature [4];                  /* ACPI signature (4 ASCII characters) */\
	unsigned int                length;                         /* Length of table, in bytes, including header */\
	unsigned char               revision;                       /* ACPI Specification minor version # */\
	unsigned char               checksum;                       /* To make sum of entire table == 0 */\
	char                        oem_id [6];                     /* OEM identification */\
	char                        oem_table_id [8];               /* OEM table identification */\
	unsigned int                oem_revision;                   /* OEM revision number */\
	char                        asl_compiler_id [4];            /* ASL compiler vendor ID */\
	unsigned int                asl_compiler_revision;          /* ASL compiler revision number */


struct acpi_table_header                                        /* ACPI common table header */
{
	ACPI_TABLE_HEADER_DEF
};

/* Table Handlers */

enum acpi_irq_model_id 
{
	ACPI_IRQ_MODEL_PIC          = 0,
	ACPI_IRQ_MODEL_IOAPIC,
	ACPI_IRQ_MODEL_IOSAPIC,
	ACPI_IRQ_MODEL_COUNT
};

/* Root System Description Pointer "RSDP" structures */

struct acpi_table_rsdp 
{
	char			            signature[8];
	unsigned char			            checksum;
	char			            oem_id[6];
	unsigned char			            revision;
	unsigned int			            rsdt_address;
};

struct acpi20_table_rsdp 
{
	char			            signature[8];
	unsigned char			            checksum;
	char			            oem_id[6];
	unsigned char			            revision;
	unsigned int			            rsdt_address;
	unsigned int			            length;
	UQUAD			            xsdt_address;
	unsigned char			            ext_checksum;
	unsigned char			            reserved[3];
};

struct acpi_table_entry_header
{
	unsigned char			            type;
	unsigned char			            length;
};

/* Root System Description Table "RSDT" structures */

struct acpi_table_rsdt 
{
	struct acpi_table_header    header;
	unsigned int			            entry[8];
};

/* Extended System Description Table "XSDT" structures */

struct acpi_table_xsdt 
{
	struct acpi_table_header    header;
	UQUAD			            entry[1];
};

/* Fixed ACPI Description Table "FADT" structures  */

struct acpi_table_fadt 
{
	struct acpi_table_header    header;
	unsigned int                       facs_addr;
	unsigned int                       dsdt_addr;
	/* ... */
};

/* Multiple APIC Description Table "MADT" structures */

struct acpi_table_madt 
{
	struct acpi_table_header    header;
	unsigned int			            lapic_address;
	struct
    {
		unsigned int			            pcat_compat:1;
		unsigned int			            reserved:31;
	}	flags;
};

enum acpi_madt_entry_id 
{
	ACPI_MADT_LAPIC             = 0,
	ACPI_MADT_IOAPIC,
	ACPI_MADT_INT_SRC_OVR,
	ACPI_MADT_NMI_SRC,
	ACPI_MADT_LAPIC_NMI,
	ACPI_MADT_LAPIC_ADDR_OVR,
	ACPI_MADT_IOSAPIC,
	ACPI_MADT_LSAPIC,
	ACPI_MADT_PLAT_INT_SRC,
	ACPI_MADT_ENTRY_COUNT
};

typedef struct 
{
	UWORD			            polarity:2;
	UWORD			            trigger:2;
	UWORD			            reserved:12;
}  acpi_interrupt_flags;

struct acpi_table_lapic 
{
	struct acpi_table_entry_header     header;
	unsigned char			            acpi_id;
	unsigned char			            id;
	struct 
    {
		unsigned int			            enabled:1;
		unsigned int			            reserved:31;
	}	flags;
};

struct acpi_table_ioapic 
{
	struct acpi_table_entry_header     header;
	unsigned char			            id;
	unsigned char			            reserved;
	unsigned int			            address;
	unsigned int			            global_irq_base;
};

struct acpi_table_int_src_ovr 
{
	struct acpi_table_entry_header     header;
	unsigned char			            bus;
	unsigned char			            bus_irq;
	unsigned int			            global_irq;
	acpi_interrupt_flags        flags;
};

struct acpi_table_nmi_src 
{
	struct acpi_table_entry_header     header;
	acpi_interrupt_flags        flags;
	unsigned int			            global_irq;
};

struct acpi_table_lapic_nmi 
{
	struct acpi_table_entry_header     header;
	unsigned char			            acpi_id;
	acpi_interrupt_flags	    flags;
	unsigned char			            lint;
};

struct acpi_table_lapic_addr_ovr 
{
	struct acpi_table_entry_header     header;
	unsigned char			            reserved[2];
	UQUAD			            address;
};

struct acpi_table_iosapic 
{
	struct acpi_table_entry_header     header;
	unsigned char			            id;
	unsigned char			            reserved;
	unsigned int			            global_irq_base;
	UQUAD			            address;
};

struct acpi_table_lsapic
{
	struct acpi_table_entry_header	    header;
	unsigned char			            acpi_id;
	unsigned char			            id;
	unsigned char			            eid;
	unsigned char			            reserved[3];
	struct {
		unsigned int			            enabled:1;
		unsigned int			            reserved:31;
	}	flags;
};

struct acpi_table_plat_int_src
{
	struct acpi_table_entry_header	    header;
	acpi_interrupt_flags	    flags;
	unsigned char			            type;	                        /* See acpi_interrupt_type */
	unsigned char			            id;
	unsigned char			            eid;
	unsigned char			            iosapic_vector;
	unsigned int			            global_irq;
	unsigned int			            reserved;
};

enum acpi_interrupt_id 
{
	ACPI_INTERRUPT_PMI          = 1,
	ACPI_INTERRUPT_INIT,
	ACPI_INTERRUPT_CPEI,
	ACPI_INTERRUPT_COUNT
};

#define	ACPI_SPACE_MEM		    0

struct acpi_gen_regaddr
{
	unsigned char                       space_id;
	unsigned char                       bit_width;
	unsigned char                       bit_offset;
	unsigned char                       resv;
	unsigned int                       addrl;
	unsigned int                       addrh;
};

struct acpi_table_hpet
{
	struct acpi_table_header    header;
	unsigned int                       id;
	struct acpi_gen_regaddr     addr;
	unsigned char                       number;
	UWORD                       min_tick;
	unsigned char                       page_protect;
};

/*
 * System Resource Affinity Table (SRAT)
 *   see http://www.microsoft.com/hwdev/design/srat.htm
 */
struct acpi_table_srat 
{
	struct acpi_table_header    header;
	unsigned int			            table_revision;
	UQUAD			            reserved;
};

enum acpi_srat_entry_id 
{
	ACPI_SRAT_PROCESSOR_AFFINITY = 0,
	ACPI_SRAT_MEMORY_AFFINITY,
	ACPI_SRAT_ENTRY_COUNT
};

struct acpi_table_processor_affinity {
	struct acpi_table_entry_header	    header;
	unsigned char			            proximity_domain;
	unsigned char			            apic_id;
	struct 
    {
		unsigned int			                enabled:1;
		unsigned int			                reserved:31;
	}	flags;
	unsigned char			            lsapic_eid;
	unsigned char			            reserved[7];
};

struct acpi_table_memory_affinity {
	struct acpi_table_entry_header	    header;
	unsigned char			            proximity_domain;
	unsigned char			            reserved1[5];
	unsigned int			            base_addr_lo;
	unsigned int			            base_addr_hi;
	unsigned int			            length_lo;
	unsigned int			            length_hi;
	unsigned int			            memory_type;	                /* See acpi_address_range_id */
	struct
    {
		unsigned int			            enabled:1;
		unsigned int			            hot_pluggable:1;
		unsigned int			            reserved:30;
	}	flags;
	UQUAD			            reserved2;
};

enum acpi_address_range_id 
{
	ACPI_ADDRESS_RANGE_MEMORY   = 1,
	ACPI_ADDRESS_RANGE_RESERVED = 2,
	ACPI_ADDRESS_RANGE_ACPI     = 3,
	ACPI_ADDRESS_RANGE_NVS      = 4,
	ACPI_ADDRESS_RANGE_COUNT
};

/*
 * System Locality Information Table (SLIT)
 *   see http://devresource.hp.com/devresource/docs/techpapers/ia64/slit.pdf
 */
struct acpi_table_slit 
{
	struct acpi_table_header    header;
	UQUAD			            localities;
	unsigned char			            entry[1];	                    /* real size = localities^2 */
};

/* Smart Battery Description Table (SBST) */
struct acpi_table_sbst
{
	struct acpi_table_header    header;
	unsigned int			            warning;	                    /* Warn user */
	unsigned int			            low;		                    /* Critical sleep */
	unsigned int			            critical;	                    /* Critical shutdown */
};

/* Embedded Controller Boot Resources Table (ECDT) */
struct acpi_table_ecdt
{
	struct acpi_table_header 	header;
	struct acpi_generic_address	ec_control;
	struct acpi_generic_address	ec_data;
	unsigned int				        uid;
	unsigned char				        gpe_bit;
	char				        ec_id[0];
};

/* ACPI Table Parser hook func protos */

AROS_UFP1(int, ACPI_hook_Table_MADT_Parse,
    AROS_UFPA(struct acpi_table_hook *,	table_hook,	A0));
AROS_UFP1(int, ACPI_hook_Table_LAPIC_Parse,
    AROS_UFPA(struct acpi_madt_entry_hook *,	table_hook,	A0));
AROS_UFP1(int, ACPI_hook_Table_LAPIC_Addr_Ovr_Parse,
    AROS_UFPA(struct acpi_madt_entry_hook *,	table_hook,	A0));
AROS_UFP1(int, ACPI_hook_Table_LAPIC_NMI_Parse,
    AROS_UFPA(struct acpi_madt_entry_hook *,	table_hook,	A0));
AROS_UFP1(int, ACPI_hook_Table_IOAPIC_Parse,
    AROS_UFPA(struct acpi_madt_entry_hook *,	table_hook,	A0));
AROS_UFP1(int, ACPI_hook_Table_Int_Src_Ovr_Parse,
    AROS_UFPA(struct acpi_madt_entry_hook *,	table_hook,	A0));
AROS_UFP1(int, ACPI_hook_Table_NMI_Src_Parse,
    AROS_UFPA(struct acpi_madt_entry_hook *,	table_hook,	A0));
AROS_UFP1(int, ACPI_hook_Table_HPET_Parse,
    AROS_UFPA(struct acpi_table_hook *,	table_hook,	A0));

#endif /* __AROS_ACPI_H__ */
