/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS Generic ACPI Definitions.
    Lang: english
*/
#ifndef __AROS_ACPI_H__
#define __AROS_ACPI_H__

#include <exec/lists.h>
#include <exec/semaphores.h>
#include <utility/hooks.h>

#define     MAX_ACPI_TABLES                 32

/********** ACPI DEFINITIONS ****************/
 
struct acpi_table_hook
{
    struct MinNode                          h_MinNode;
    IPTR	                            (*h_Entry)();                   /* Main entry point */
    IPTR	                            (*h_SubEntry)();                /* Secondary entry point */
    unsigned long                           phys_addr;                      /* points to "header" for madt entry hooks */
    unsigned long                           size;
};

/********** ACPI DEFINITIONS ****************/

/*
 * Actually a reverse of AROS_MAKE_ID().
 * Reverse because ACPI signatures appear to be stored in bigendian format.
 */
#define ACPI_MAKE_ID(a, b, c, d) (((ULONG) (d)<<24) | ((ULONG) (c)<<16) | \
                                  ((ULONG) (b)<<8)  | ((ULONG) (a)))

/* ACPI 2.0 Generic Address Structure (GAS) */

struct GENERIC_ACPI_ADDR
{
    unsigned char                           address_space_id;               /* Address space where struct or register exists. */
    unsigned char                           register_bit_width;             /* Size in bits of given register */
    unsigned char                           register_bit_offset;            /* Bit offset within the register */
    unsigned char                           reserved;                       /* Must be 0 */
    UQUAD                                   address;                        /* 64-bit address of struct or register */
};

struct ACPI_TABLE_DEF_HEADER                                                /* ACPI common table header */
{
    unsigned int                            signature;                      /* ACPI signature (4 ASCII characters) */
    unsigned int                            length;                         /* Length of table, in bytes, including header */
    unsigned char                           revision;                       /* ACPI Specification minor version # */
    unsigned char                           checksum;                       /* To make sum of entire table == 0 */
    char                                    oem_id [6];                     /* OEM identification */
    char                                    oem_table_id [8];               /* OEM table identification */
    unsigned int                            oem_revision;                   /* OEM revision number */
    char                                    asl_compiler_id [4];            /* ASL compiler vendor ID */
    unsigned int                            asl_compiler_revision;          /* ASL compiler revision number */
};

/* Table Handlers */

enum ACPI_IRQ_PICS 
{
    ACPI_IRQ_PIC_8259                       = 0,
    ACPI_IRQ_PIC_IOAPIC,
    ACPI_IRQ_PIC_IOSAPIC,
    ACPI_IRQ_PIC_COUNT
};

/* Root System Description Pointer "RSDP" structure */
struct ACPI_TABLE_TYPE_RSDP 
{
    char			            signature[8];
    unsigned char			    checksum;
    char			            oem_id[6];
    unsigned char			    revision;
    unsigned int			    rsdt_address;
    /* The following fields are present only if revision >= 2 */
    unsigned int			    length;
    UQUAD			            xsdt_address;
    unsigned char			    ext_checksum;
    unsigned char			    reserved[3];
};

struct ACPI_TABLE_DEF_ENTRY_HEADER
{
    unsigned char			    type;
    unsigned char			    length;
};

struct ACPI_TABLE_TYPE_RSDT                                                 /* Root System Description Table "RSDT" structures */
{
    struct ACPI_TABLE_DEF_HEADER            header;
    unsigned int		            entry[8];
};

struct ACPI_TABLE_TYPE_XSDT                                                 /* Extended System Description Table "XSDT" structures */
{
    struct ACPI_TABLE_DEF_HEADER            header;
    UQUAD			            entry[1];
};

struct ACPI_TABLE_TYPE_FADT                                                 /* Fixed ACPI Description Table "FADT" structures  */
{
    struct ACPI_TABLE_DEF_HEADER            header;
    unsigned int                            facs_addr;
    unsigned int                            dsdt_addr;
    /* ... */
};

struct ACPI_TABLE_TYPE_MADT                                                 /* Multiple APIC Description Table "MADT" structures */
{
    struct ACPI_TABLE_DEF_HEADER            header;
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
    struct ACPI_TABLE_DEF_ENTRY_HEADER      header;
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
    struct ACPI_TABLE_DEF_ENTRY_HEADER      header;
    unsigned char			    id;
    unsigned char			    reserved;
    unsigned int			    address;
    unsigned int			    global_irq_base;
};

struct ACPI_TABLE_TYPE_INT_SRCOVR 
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER      header;
    unsigned char			    bus;
    unsigned char			    bus_irq;
    unsigned int			    global_irq;
    ACPI_INT_FLAGS                          flags;
};

struct ACPI_TABLE_TYPE_LAPIC_NMI_SRC 
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER      header;
    ACPI_INT_FLAGS                          flags;
    unsigned int			    global_irq;
};

struct ACPI_TABLE_TYPE_LAPIC_NMI 
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER      header;
    unsigned char			    acpi_id;
    ACPI_INT_FLAGS	                    flags;
    unsigned char			    lint;
};

struct ACPI_TABLE_TYPE_LAPIC_ADDROVR 
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER      header;
    unsigned char			    reserved[2];
    UQUAD			            address;
};

struct ACPI_TABLE_TYPE_IOSAPIC 
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER      header;
    unsigned char			    id;
    unsigned char			    reserved;
    unsigned int			    global_irq_base;
    UQUAD			            address;
};

struct ACPI_TABLE_TYPE_LSAPIC
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER	    header;
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
    struct ACPI_TABLE_DEF_ENTRY_HEADER	    header;
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
    struct ACPI_TABLE_DEF_HEADER            header;
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
    struct ACPI_TABLE_DEF_HEADER            header;
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
    struct ACPI_TABLE_DEF_ENTRY_HEADER	    header;
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
    struct ACPI_TABLE_DEF_ENTRY_HEADER	    header;
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
    struct ACPI_TABLE_DEF_HEADER            header;
    UQUAD			            localities;
    unsigned char			    entry[1];	                    /* real size = localities^2 */
};

struct ACPI_TABLE_TYPE_SBST                                                 /* Smart Battery Description Table (SBST) */
{
    struct ACPI_TABLE_DEF_HEADER            header;
    unsigned int			    warning;	                    /* Warn user */
    unsigned int			    low;		            /* Critical sleep */
    unsigned int			    critical;	                    /* Critical shutdown */
};

struct ACPI_TABLE_TYPE_ECDT                                                 /* Embedded Controller Boot Resources Table (ECDT) */
{
    struct ACPI_TABLE_DEF_HEADER 	    header;
    struct GENERIC_ACPI_ADDR	            ec_control;
    struct GENERIC_ACPI_ADDR	            ec_data;
    unsigned int			    uid;
    unsigned char			    gpe_bit;
    char				    ec_id[0];
};


struct ACPIBase
{
    struct  Node                            ACPIB_Node;

    struct ACPI_TABLE_TYPE_RSDP            *ACPIB_RSDP_Addr;
    struct ACPI_TABLE_DEF_HEADER     	   *ACPIB_SDT_Addr;			/* Raw XSDT or RSDT pointer	   	*/
    int                                     ACPIB_SDT_Count;			/* Number of entries in the array below */
    struct ACPI_TABLE_DEF_HEADER    	  **ACPIB_SDT_Entry;			/* Array of pointers to SDT tables 	*/
/*..*/
    APTR                                    ACPIB_ACPI_Data;                    /* Base address of acpi data block 	*/
    APTR                                    ACPIB_ACPI_Data_End;
    APTR                                    ACPIB_ACPI_NVM;                     /* Base address of acpi data block */

    int                                     ACPIB_ACPI_IRQ;

    int                                     ACPIB_ACPI_HT;

    BOOL                                    ACPIB_SMP_Enabled;
    int                                     ACPIB_SMP_Config;
/** OBSOLETE _ DO NOT USE (only here to fix build probs till acpi,resource is up ;) */
    int                                     ACPIB_ACPI_LAPIC;
    int                                     ACPIB_ACPI_IOAPIC;
};

#endif /* __AROS_ACPI_H__ */
