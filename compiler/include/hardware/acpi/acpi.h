/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS Generic ACPI & APIC Definitions.
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

#define     ACPI_MAX_TABLES         32
#define     MAX_IO_APICS            32


/********** APIC DEFINITIONS ****************/

struct GenericAPIC
{ 
	char                        *name; 
	IPTR                        (*probe)(); 
	IPTR                        (*apic_id_registered)();

};

/*
	cpumask_t                 (*target_cpus)(void);
    int                         int_delivery_mode;
	int                         int_dest_mode; 
	int                         apic_broadcast_id; 
	int                         esr_disable;
	unsigned long               (*check_apicid_used)(physid_mask_t bitmap, int apicid);
	unsigned long               (*check_apicid_present)(int apicid); 
	int                         no_balance_irq;
	int                         no_ioapic_check;
	void                        (*init_apic_ldr)(void);
	physid_mask_t               (*ioapic_phys_id_map)(physid_mask_t map);

	void                        (*clustered_apic_check)(void);
	int                         (*multi_timer_check)(int apic, int irq);
	int                         (*apicid_to_node)(int logical_apicid); 
	int                         (*cpu_to_logical_apicid)(int cpu);
	int                         (*cpu_present_to_apicid)(int mps_cpu);
	physid_mask_t               (*apicid_to_cpu_present)(int phys_apicid);
	int                         (*mpc_apic_id)(struct mpc_config_processor *m, struct mpc_config_translation *t); 
	void                        (*setup_portio_remap)(void); 
	int                         (*check_phys_apicid_present)(int boot_cpu_physical_apicid);
	void                        (*enable_apic_mode)(void);

	// mpparse
	void                        (*mpc_oem_bus_info)(struct mpc_config_bus *, char *, struct mpc_config_translation *);
	void                        (*mpc_oem_pci_bus)(struct mpc_config_bus *, struct mpc_config_translation *); 

	// When one of the next two hooks returns 1 the GenericAPIC
	//  is switched to this. Essentially they are additional probe 
	//   functions. 
	int                         (*mps_oem_check)(struct mp_config_table *mpc, char *oem, char *productid);
	int                         (*acpi_madt_oem_check)(char *oem_id, char *oem_table_id);

	unsigned                    (*get_apic_id)(unsigned long x);
	unsigned long               apic_id_mask;
	unsigned int                (*cpu_mask_to_apicid)(cpumask_const_t cpumask);
	
	// ipi 
	void                        (*send_IPI_mask)(cpumask_t mask, int vector);
	void                        (*send_IPI_allbutself)(int vector);
	void                        (*send_IPI_all)(int vector);
};
*/

/********** ACPI DEFINITIONS ****************/
 
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

/********** ACPI DEFINITIONS ****************/

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

/********** ACPI DEFINITIONS ****************/

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
	unsigned char			    checksum;
	char			            oem_id[6];
	unsigned char			    revision;
	unsigned int			    rsdt_address;
};

struct acpi20_table_rsdp 
{
	char			            signature[8];
	unsigned char			    checksum;
	char			            oem_id[6];
	unsigned char			    revision;
	unsigned int			    rsdt_address;
	unsigned int			    length;
	UQUAD			            xsdt_address;
	unsigned char			    ext_checksum;
	unsigned char			    reserved[3];
};

struct acpi_table_entry_header
{
	unsigned char			    type;
	unsigned char			    length;
};

/* Root System Description Table "RSDT" structures */

struct acpi_table_rsdt 
{
	struct acpi_table_header            header;
	unsigned int		            entry[8];
};

/* Extended System Description Table "XSDT" structures */

struct acpi_table_xsdt 
{
	struct acpi_table_header            header;
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

enum acpi_blacklist_predicates
{
        all_versions,
        less_than_or_equal,
        equal,
        greater_than_or_equal,
};

struct acpi_blacklist_item
{
        char                    oem_id[7];
        char                    oem_table_id[9];
        unsigned int                   oem_revision;
        unsigned int                   table;
        enum                    acpi_blacklist_predicates oem_revision_predicate;
        char                    *reason;
        unsigned int                   is_critical_error;
};

/*
    POLICY: If _ANYTHING!_ doesn't work, put it on the blacklist.
            If they are critical errors, mark it critical
*/

#ifdef __ACPI_C__
struct acpi_blacklist_item      acpi_blacklist[] =
{
/* ASUS K7M                                             */
	{"ASUS  ",      "K7M     ", 0x00001000, ACPI_DSDT, less_than_or_equal, "Field beyond end of region", 0},

/* ASUS P2B-S                                           */
	{"ASUS\0\0",    "P2B-S   ", 0, ACPI_DSDT, all_versions, "Bogus PCI routing", 1},

/* Seattle 2 - old BIOS rev.                            */
	{"INTEL ",      "440BX   ", 0x00001000, ACPI_DSDT, less_than_or_equal, "Field beyond end of region", 0},

/* Intel 810 Motherboard                                */
	{"MNTRAL",      "MO81010A", 0x00000012, ACPI_DSDT, less_than_or_equal, "Field beyond end of region", 0},

/* Compaq Presario 711FR                                */
	{"COMAPQ",      "EAGLES",   0x06040000, ACPI_DSDT, less_than_or_equal, "SCI issues (C2 disabled)", 0},

/* Compaq Presario 1700                                 */
	{"PTLTD ",      "  DSDT  ", 0x06040000, ACPI_DSDT, less_than_or_equal, "Multiple problems", 1},

/* Sony FX120, FX140, FX150?                            */
	{"SONY  ",      "U0      ", 0x20010313, ACPI_DSDT, less_than_or_equal, "ACPI driver problem", 1},

/* Compaq Presario 800, Insyde BIOS                     */
	{"INT440",      "SYSFexxx", 0x00001001, ACPI_DSDT, less_than_or_equal, "Does not use _REG to protect EC OpRegions", 1},

/* IBM 600E - _ADR should return 7, but it returns 1    */
	{"IBM   ",      "TP600E  ", 0x00000105, ACPI_DSDT, less_than_or_equal, "Incorrect _ADR", 1},

/* Portege 7020, BIOS 8.10                              */
	{"TOSHIB",      "7020CT  ", 0x19991112, ACPI_DSDT, all_versions, "Implicit Return", 0},

/* Portege 4030                                         */
	{"TOSHIB",      "4030    ", 0x19991112, ACPI_DSDT, all_versions, "Implicit Return", 0},

/* Portege 310/320, BIOS 7.1                            */
	{"TOSHIB",      "310     ", 0x19990511, ACPI_DSDT, all_versions, "Implicit Return", 0},

	{""}
};
#endif /* __ACPI_C__ */

/********** ACPI RESOURCE BASE DEFINITION ****************/

//static const void * const probe_APIC[] =
//{ 
//	NULL,
//};

/* System Description Table (RSDT/XSDT) */
struct acpi_table_sdt
{
	unsigned long		        pa;
	enum acpi_table_id	        id;
	unsigned long		        size;
};

struct ACPIBase
{
    struct  Node                ACPIB_Node;
    struct  ExecBase            *ACPIB_SysBase;
    struct  UtilityBase         *ACPIB_UtilBase;
    struct  CPUBase             *ACPIB_CPUBase;

    BOOL                        ACPIB_APIC_Enabled;
    int                         ACPIB_APIC_IRQ_Model;
    struct GenericAPIC          *ACPIB_GenericAPIC;

    APTR                        ACPIB_SDT_Phys;
    int                         ACPIB_SDT_Count;
    APTR                        *ACPIB_SDT_Entry[ACPI_MAX_TABLES];
/*..*/
    APTR                        ACPIB_ACPI_Data;        /* Base address of acpi data block */
    APTR                        ACPIB_ACPI_NVM;        /* Base address of acpi data block */

    int                         ACPIB_ACPI_Disabled;
    int                         ACPIB_ACPI_IRQ;

    int                         ACPIB_ACPI_IOAPIC;

    int                         ACPIB_ACPI_LAPIC;
    APTR                        ACPIB_ACPI_LAPIC_addr;

    APTR                        *ACPIB_ACPI_Table_Sigs[ACPI_MAX_TABLES];
/*..*/

    int                         ACPIB_ACPI_HT;

    BOOL                        ACPIB_SMP_Enabled;
    int                         ACPIB_SMP_Config;

};

#endif /* __AROS_ACPI_H__ */
