/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS Generic ACPI Definitions.
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

#define     MAX_ACPI_TABLES                 32

/********** ACPI DEFINITIONS ****************/
 
struct acpi_table_hook
{
    struct MinNode                          h_MinNode;
    IPTR	                            (*h_Entry)();                   /* Main entry point */
    IPTR	                            (*h_SubEntry)();                /* Secondary entry point */
    unsigned long                           phys_addr;                      /* points to "tbl_header" for madt entry hooks */
    unsigned long                           size;
};

/********** ACPI DEFINITIONS ****************/

/* Table Type Definitions */

enum ACPI_TABLE_IDS {
    ACPI_ID_UNKNOWN                         = 0,
    ACPI_ID_APIC,
    ACPI_ID_BOOT,
    ACPI_ID_DBGP,
    ACPI_ID_DSDT,
    ACPI_ID_ECDT,
    ACPI_ID_ETDT,
    ACPI_ID_FADT,
    ACPI_ID_FACS,
    ACPI_ID_OEMX,
    ACPI_ID_PSDT,
    ACPI_ID_SBST,
    ACPI_ID_SLIT,
    ACPI_ID_SPCR,
    ACPI_ID_SRAT,
    ACPI_ID_SSDT,
    ACPI_ID_SPMI,
    ACPI_ID_HPET,
    ACPI_ID_COUNT
};

#ifdef __ACPI_C__

char *ACPI_ID_STRINGS[ACPI_ID_COUNT] = {
    [ACPI_ID_UNKNOWN]	                    = "????",
    [ACPI_ID_APIC]		            = "APIC",
    [ACPI_ID_BOOT]		            = "BOOT",
    [ACPI_ID_DBGP]		            = "DBGP",
    [ACPI_ID_DSDT]		            = "DSDT",
    [ACPI_ID_ECDT]		            = "ECDT",
    [ACPI_ID_ETDT]		            = "ETDT",
    [ACPI_ID_FADT]		            = "FACP",
    [ACPI_ID_FACS]		            = "FACS",
    [ACPI_ID_OEMX]		            = "OEM",
    [ACPI_ID_PSDT]		            = "PSDT",
    [ACPI_ID_SBST]		            = "SBST",
    [ACPI_ID_SLIT]		            = "SLIT",
    [ACPI_ID_SPCR]		            = "SPCR",
    [ACPI_ID_SRAT]		            = "SRAT",
    [ACPI_ID_SSDT]		            = "SSDT",
    [ACPI_ID_SPMI]		            = "SPMI",
    [ACPI_ID_HPET]		            = "HPET",
};

#else
    char *ACPI_ID_STRINGS;
#endif /* __ACPI_C__ */

/* ACPI 2.0 Generic Address Structure (GAS) */

struct GENERIC_ACPI_ADDR
{
    unsigned char                           address_space_id;               /* Address space where struct or register exists. */
    unsigned char                           register_bit_width;             /* Size in bits of given register */
    unsigned char                           register_bit_offset;            /* Bit offset within the register */
    unsigned char                           reserved;                       /* Must be 0 */
    UQUAD                                   address;                        /* 64-bit address of struct or register */
};

#define ACPI_TABLE_HEADER                                               /* ACPI common table tbl_header */ \
    char                                    signature [4];                  /* ACPI signature (4 ASCII characters) */\
    unsigned int                            length;                         /* Length of table, in bytes, including tbl_header */\
    unsigned char                           revision;                       /* ACPI Specification minor version # */\
    unsigned char                           checksum;                       /* To make sum of entire table == 0 */\
    char                                    oem_id [6];                     /* OEM identification */\
    char                                    oem_table_id [8];               /* OEM table identification */\
    unsigned int                            oem_revision;                   /* OEM revision number */\
    char                                    asl_compiler_id [4];            /* ASL compiler vendor ID */\
    unsigned int                            asl_compiler_revision;          /* ASL compiler revision number */


struct ACPI_TABLE_DEF_HEADER                                                /* ACPI common table tbl_header */
{
    ACPI_TABLE_HEADER
};

/* Table Handlers */

enum ACPI_IRQ_PICS 
{
    ACPI_IRQ_PIC_8259                       = 0,
    ACPI_IRQ_PIC_IOAPIC,
    ACPI_IRQ_PIC_IOSAPIC,
    ACPI_IRQ_PIC_COUNT
};

/* Root System Description Pointer "RSDP" structures */

struct ACPI_TABLE_TYPE_RSDP 
{
    char			            signature[8];
    unsigned char			    checksum;
    char			            oem_id[6];
    unsigned char			    revision;
    unsigned int			    rsdt_address;
};

struct ACPI2_TABLE_TYPE_RSDP 
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

struct ACPI_TABLE_TYPE_SDT                                                      /* System Description Table (RSDT/XSDT) */
{
    unsigned long		            phys_addr;
    enum ACPI_TABLE_IDS	                    id;
    unsigned long		            size;
};

struct ACPI_TABLE_DEF_ENTRY_HEADER
{
    unsigned char			    type;
    unsigned char			    length;
};



struct ACPI_TABLE_TYPE_RSDT                                                 /* Root System Description Table "RSDT" structures */
{
    struct ACPI_TABLE_DEF_HEADER            tbl_header;
    unsigned int		            entry[8];
};



struct ACPI_TABLE_TYPE_XSDT                                                 /* Extended System Description Table "XSDT" structures */
{
    struct ACPI_TABLE_DEF_HEADER            tbl_header;
    UQUAD			            entry[1];
};



struct ACPI_TABLE_TYPE_FADT                                                 /* Fixed ACPI Description Table "FADT" structures  */
{
    struct ACPI_TABLE_DEF_HEADER            tbl_header;
    unsigned int                            facs_addr;
    unsigned int                            dsdt_addr;
    /* ... */
};



struct ACPI_TABLE_TYPE_MADT                                                 /* Multiple APIC Description Table "MADT" structures */
{
    struct ACPI_TABLE_DEF_HEADER            tbl_header;
    unsigned int			    lapic_address;
    struct
    {
	unsigned int		            pcat_compat:1;
	unsigned int		            reserved:31;
    }	flags;
};

enum ACPI_MADT_TYPES 
{
    ACPI_MADT_LAPIC                         = 0,
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
}  ACPI_INT_FLAGS;

struct ACPI_TABLE_TYPE_LAPIC 
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER      tbl_header;
    unsigned char			    acpi_id;
    unsigned char			    id;
    struct 
    {
	unsigned int			        enabled:1;
	unsigned int			        reserved:31;
    }	flags;
};

struct ACPI_TABLE_TYPE_IOAPIC 
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER      tbl_header;
    unsigned char			    id;
    unsigned char			    reserved;
    unsigned int			    address;
    unsigned int			    global_irq_base;
};

struct ACPI_TABLE_TYPE_INT_SRCOVR 
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER      tbl_header;
    unsigned char			    bus;
    unsigned char			    bus_irq;
    unsigned int			    global_irq;
    ACPI_INT_FLAGS                          flags;
};

struct ACPI_TABLE_TYPE_LAPIC_NMI_SRC 
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER      tbl_header;
    ACPI_INT_FLAGS                          flags;
    unsigned int			    global_irq;
};

struct ACPI_TABLE_TYPE_LAPIC_NMI 
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER      tbl_header;
    unsigned char			    acpi_id;
    ACPI_INT_FLAGS	                    flags;
    unsigned char			    lint;
};

struct ACPI_TABLE_TYPE_LAPIC_ADDROVR 
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER      tbl_header;
    unsigned char			    reserved[2];
    UQUAD			            address;
};

struct ACPI_TABLE_TYPE_IOSAPIC 
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER      tbl_header;
    unsigned char			    id;
    unsigned char			    reserved;
    unsigned int			    global_irq_base;
    UQUAD			            address;
};

struct ACPI_TABLE_TYPE_LSAPIC
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER	    tbl_header;
    unsigned char			    acpi_id;
    unsigned char			    id;
    unsigned char			    eid;
    unsigned char			    reserved[3];
    struct {
	unsigned int			        enabled:1;
	unsigned int			        reserved:31;
    }	flags;
};

struct ACPI_TABLE_TYPE_PLAT_INTSRC
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER	    tbl_header;
    ACPI_INT_FLAGS	                    flags;
    unsigned char			    type;	                        /* See acpi_interrupt_type */
    unsigned char			    id;
    unsigned char			    eid;
    unsigned char			    iosapic_vector;
    unsigned int			    global_irq;
    unsigned int			    reserved;
};

enum ACPI_INT_IDS 
{
    ACPI_INTERRUPT_PMI                      = 1,
    ACPI_INTERRUPT_INIT,
    ACPI_INTERRUPT_CPEI,
    ACPI_INTERRUPT_COUNT
};

#define	ACPI_SPACE_MEM		            0

struct ACPI_GEN_REGADDR
{
    unsigned char                           space_id;
    unsigned char                           bit_width;
    unsigned char                           bit_offset;
    unsigned char                           resv;
    unsigned int                            addrl;
    unsigned int                            addrh;
};

struct ACPI_TABLE_TYPE_HPET
{
    struct ACPI_TABLE_DEF_HEADER            tbl_header;
    unsigned int                            id;
    struct ACPI_GEN_REGADDR                 addr;
    unsigned char                           number;
    UWORD                                   min_tick;
    unsigned char                           page_protect;
};

/*
    System Resource Affinity Table (SRAT) [ see http://www.microsoft.com/hwdev/design/srat.htm ]
*/

struct ACPI_TABLE_TYPE_SRAT 
{
    struct ACPI_TABLE_DEF_HEADER            tbl_header;
    unsigned int			    table_revision;
    UQUAD			            reserved;
};

enum ACPI_SRAT_ENTRY_IDS 
{
    ACPI_SRAT_PROCESSOR_AFFINITY = 0,
    ACPI_SRAT_MEMORY_AFFINITY,
    ACPI_SRAT_ENTRY_COUNT
};

struct ACPI_TABLE_AFFIN_PROCESSOR
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER	    tbl_header;
    unsigned char			    proximity_domain;
    unsigned char			    apic_id;
    struct 
    {
	unsigned int			        enabled:1;
	unsigned int			        reserved:31;
    }	flags;
    unsigned char			    lsapic_eid;
    unsigned char			    reserved[7];
};

struct ACPI_TABLE_AFFIN_MEMORY
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER	    tbl_header;
    unsigned char			    proximity_domain;
    unsigned char			    reserved1[5];
    unsigned int			    base_addr_lo;
    unsigned int			    base_addr_hi;
    unsigned int			    length_lo;
    unsigned int			    length_hi;
    unsigned int			    memory_type;	                /* See acpi_address_range_id */
    struct
    {
	unsigned int			        enabled:1;
	unsigned int			        hot_pluggable:1;
	unsigned int			        reserved:30;
    }	flags;
    UQUAD			            reserved2;
};

enum acpi_address_range_id 
{
    ACPI_ADDRESS_RANGE_MEMORY               = 1,
    ACPI_ADDRESS_RANGE_RESERVED             = 2,
    ACPI_ADDRESS_RANGE_ACPI                 = 3,
    ACPI_ADDRESS_RANGE_NVS                  = 4,
    ACPI_ADDRESS_RANGE_COUNT
};

/*
    System Locality Information Table (SLIT) [ see http://devresource.hp.com/devresource/docs/techpapers/ia64/slit.pdf ]
*/

struct ACPI_TABLE_TYPE_SLIT 
{
    struct ACPI_TABLE_DEF_HEADER            tbl_header;
    UQUAD			            localities;
    unsigned char			    entry[1];	                    /* real size = localities^2 */
};

struct ACPI_TABLE_TYPE_SBST                                                 /* Smart Battery Description Table (SBST) */
{
    struct ACPI_TABLE_DEF_HEADER            tbl_header;
    unsigned int			    warning;	                    /* Warn user */
    unsigned int			    low;		            /* Critical sleep */
    unsigned int			    critical;	                    /* Critical shutdown */
};

struct ACPI_TABLE_TYPE_ECDT                                                 /* Embedded Controller Boot Resources Table (ECDT) */
{
    struct ACPI_TABLE_DEF_HEADER 	    tbl_header;
    struct GENERIC_ACPI_ADDR	            ec_control;
    struct GENERIC_ACPI_ADDR	            ec_data;
    unsigned int			    uid;
    unsigned char			    gpe_bit;
    char				    ec_id[0];
};


struct ACPIBase
{
    struct  Node                            ACPIB_Node;
    struct  ExecBase                        *ACPIB_SysBase;
    struct  UtilityBase                     *ACPIB_UtilBase;
    struct  CPUBase                         *ACPIB_CPUBase;

    struct GenericAPIC                      *ACPIB_GenericAPIC;         /* !! DO NOT USE!! THIS WILL BE REMOVED SOON!! */

    APTR                                    ACPIB_RSDP_Addr;
    APTR                                    ACPIB_SDT_Addr;
    int                                     ACPIB_SDT_Count;
    APTR                                    *ACPIB_SDT_Entry[MAX_ACPI_TABLES];
/*..*/
    APTR                                    ACPIB_ACPI_Data;                    /* Base address of acpi data block */
    APTR                                    ACPIB_ACPI_Data_End;
    APTR                                    ACPIB_ACPI_NVM;                     /* Base address of acpi data block */

    int                                     ACPIB_ACPI_Disabled;
    int                                     ACPIB_ACPI_IRQ;

    int                                     ACPIB_ACPI_IOAPIC;

    int                                     ACPIB_ACPI_LAPIC;
    APTR                                    ACPIB_ACPI_LAPIC_addr;              /* Local APIC address */
/*..*/

    int                                     ACPIB_ACPI_HT;

    BOOL                                    ACPIB_SMP_Enabled;
    int                                     ACPIB_SMP_Config;
};

#endif /* __AROS_ACPI_H__ */
