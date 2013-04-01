/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS Generic ACPI Definitions.
    Lang: english
*/
#ifndef __AROS_ACPI_H__
#define __AROS_ACPI_H__

#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <utility/hooks.h>

/********** ACPI DEFINITIONS ****************/

/*
 * Actually a reverse of AROS_MAKE_ID().
 * Reverse because ACPI signatures appear to be stored in bigendian format.
 */
#define ACPI_MAKE_ID(a, b, c, d) (((ULONG) (d)<<24) | ((ULONG) (c)<<16) | \
                                  ((ULONG) (b)<<8)  | ((ULONG) (a)))

#define ACPI_ID_ALL 0xFFFFFFFF

/* ACPI 2.0 Generic Address Structure (GAS) */

struct GENERIC_ACPI_ADDR
{
    unsigned char                           address_space_id;     /* Address space where struct or register exists. */
    unsigned char                           register_bit_width;   /* Size in bits of given register */
    unsigned char                           register_bit_offset;  /* Bit offset within the register */
    unsigned char                           size;                 /* Acces size (see below) */
    UQUAD                                   address;              /* 64-bit address of struct or register */
} __attribute__ ((packed));

/* Address spaces */
#define	ACPI_SPACE_MEM		0
#define ACPI_SPACE_IO		1
#define ACPI_SPACE_PCI		2
#define ACPI_SPACE_EMBEDDED	3
#define ACPI_SPACE_SMBUS	4
#define ACPI_SPACE_FIXED	0x7F
#define ACPI_SPACE_OEM		0xC0

/* Access sizes */
#define ACPI_SIZE_UNDEFINED 0	/* Specified by bit offset and width */
#define ACPI_SIZE_BYTE	    1
#define ACPI_SIZE_WORD	    2
#define ACPI_SIZE_DWORD	    3
#define ACPI_SIZE_QUAD	    4

#define ACPI_PCI_OFFSET(addr) (unsigned short)((addr) & 0x0FFFF)
#define ACPI_PCI_FUNC(addr)   (unsigned short)(((addr) >> 16) & 0x0FFFF)
#define ACPI_PCI_DEV(addr)    (unsigned short)(((addr) >> 32) & 0x0FFFF)

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

#define ACPI_ENTRY_TYPE_ALL -1

struct ACPI_TABLE_TYPE_RSDT                                                 /* Root System Description Table "RSDT" structures */
{
    struct ACPI_TABLE_DEF_HEADER            header;
    unsigned int		            entry[8];
};

struct ACPI_TABLE_TYPE_XSDT                                                 /* Extended System Description Table "XSDT" structures */
{
    struct ACPI_TABLE_DEF_HEADER            header;
    UQUAD			            entry[1];
} __attribute__ ((packed));

struct ACPI_TABLE_TYPE_FADT                                                 /* Fixed ACPI Description Table "FADT" structures  */
{
    struct ACPI_TABLE_DEF_HEADER header;
    unsigned int                 facs_addr;
    unsigned int                 dsdt_addr;
    unsigned char		 reserved0;		/* ACPI 1.0: interrupt model. Actually never used. */
    unsigned char		 pm_profile;		/* ACPI >= 2.0: preferred system profile, see below */
    unsigned short		 sci_int;
    unsigned int		 smi_cmd;
    unsigned char		 acpi_enable;
    unsigned char		 acpi_disable;
    unsigned char		 s4bios_req;
    unsigned char		 pstate_cnt;
    unsigned int		 pm1a_evt_blk;
    unsigned int		 pm1b_evt_blk;
    unsigned int		 pm1a_cnt_blk;
    unsigned int		 pm1b_cnt_blk;
    unsigned int		 pm2_cnt_blk;
    unsigned int		 pm_tmr_blk;
    unsigned int		 gpe0_blk;
    unsigned int		 gpe1_blk;
    unsigned char		 pm1_evt_len;
    unsigned char		 pm1_cnt_len;
    unsigned char		 pm2_cnt_len;
    unsigned char		 pm_tmr_len;
    unsigned char		 gpe0_blk_len;
    unsigned char		 gpe1_blk_len;
    unsigned char		 gpe1_base;
    unsigned char		 cst_cnt;
    unsigned short		 p_lvl2_lat;
    unsigned short		 p_lvl3_lat;
    unsigned short		 flush_size;		/* Obsolete leftover from ACPI v1.0 */
    unsigned short		 flush_stride;		/* Obsolete leftover from ACPI v1.0 */
    unsigned char		 duty_offset;
    unsigned char		 duty_width;
    unsigned char		 day_alarm;
    unsigned char		 mon_alarm;
    unsigned char		 century;
    unsigned short		 pc_arch;		/* ACPI >= 2.0: IA-PC architecture flags, see below */
    unsigned char		 reserved1;
    unsigned int		 flags;			/* Fixed feature flags, see below */
    /* The following is present in ACPI >= 2.0 */
    struct GENERIC_ACPI_ADDR	 reset_reg;
    unsigned char		 reset_value;
    unsigned char		 reserved2[3];
    unsigned long long		 x_firmware_ctrl;
    unsigned long long		 x_dsdt;
    struct GENERIC_ACPI_ADDR	 x_pm1a_evt_blk;
    struct GENERIC_ACPI_ADDR	 x_pm1b_evt_blk;
    struct GENERIC_ACPI_ADDR	 x_pm1a_cnt_blk;
    struct GENERIC_ACPI_ADDR	 x_pm1b_cnt_blk;
    struct GENERIC_ACPI_ADDR	 x_pm2_cnt_blk;
    struct GENERIC_ACPI_ADDR	 x_pm_tmr_blk;
    struct GENERIC_ACPI_ADDR	 x_gpe0_blk;
    struct GENERIC_ACPI_ADDR	 x_gpe1_blk;
} __attribute__ ((packed));

/* Preferred system profiles */
#define FACP_PROFILE_UNSPECIFIED	0
#define FACP_PROFILE_DESKTOP		1
#define FACP_PROFILE_MOBILE		2
#define FACP_PROFILE_WORKSTATION	3
#define FACP_PROFILE_ENTERPRIZE		4
#define FACP_PROFILE_SOHO		5
#define FACP_PROFILE_APPLIANCE		6
#define FACP_PROFILE_PERFORMANCE	7

/* IA-PC architecture flags */
#define FACP_PC_LEGACY	(1 << 0)
#define FACP_PC_8042	(1 << 1)
#define FACP_PC_NO_VGA	(1 << 2)
#define FACP_PC_NO_MSI	(1 << 3)
#define FACP_PCIE_ASPM	(1 << 4)

/* Fixed feature flags */
#define FACP_FF_WBINVD		(1 << 0)
#define FACP_FF_WBINVD_FLUSH	(1 << 1)
#define FACP_FF_PROC_C1		(1 << 2)
#define FACP_FF_P_LVL2_UP	(1 << 3)
#define	FACP_FF_PWR_BUTTON	(1 << 4)
#define FACP_FF_SLP_BUTTON	(1 << 5)
#define FACP_FF_FIX_RTC		(1 << 6)
#define FACP_FF_RTC_S4		(1 << 7)
#define FACP_FF_TMR_VAL_EXT	(1 << 8)
#define FACP_FF_DCK_CAP		(1 << 9)
#define FACP_FF_RESET_REG_SUP	(1 << 10)
#define FACP_FF_SEALED_CASE	(1 << 11)
#define FACP_FF_HEADLESS	(1 << 12)
#define FACP_FF_CPU_SW_SLP	(1 << 13)
#define FACP_FF_PCI_EXP_WAK	(1 << 14)
#define FACP_FF_PLATFORM_CLOCK	(1 << 15)
#define FACP_FF_S4_RTC_STS_VALID (1 << 16)
#define FACP_FF_REMOTE_PWRON	(1 << 17)
#define FACP_FF_APIC_CLUSTER	(1 << 18)
#define FACP_FF_APIC_PHYS_DEST	(1 << 19)

struct ACPI_TABLE_TYPE_MADT                                                 /* Multiple APIC Description Table "MADT" structures */
{
    struct ACPI_TABLE_DEF_HEADER        header;
    unsigned int			lapic_address;
    struct
    {
	unsigned int		        pcat_compat:1;
	unsigned int		        reserved:31;
    }	flags;
    struct ACPI_TABLE_DEF_ENTRY_HEADER	entries[0];
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
    ACPI_MADT_X2APIC,
    ACPI_MADT_X2APIC_NMI,
    ACPI_MADT_ENTRY_COUNT
};

typedef struct 
{
    UWORD polarity:2;	/* See below */
    UWORD trigger:2;	/* See below */
    UWORD reserved:12;
}  ACPI_INT_FLAGS;

/* Polarity and trigger mode values */
#define INTF_POLARITY_BUS  0	/* Bus default	*/
#define INTF_POLARITY_HIGH 1	/* Active high	*/
#define INTF_POLARITY_LOW  3	/* Active low	*/
#define INTF_TRIGGER_BUS   0	/* Bus default	*/
#define INTF_TRIGGER_EDGE  1
#define INTF_TRIGGER_LEVEL 3

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
} __attribute__ ((packed));

#define ACPI_ID_BROADCAST 0xFF

struct ACPI_TABLE_TYPE_LAPIC_ADDROVR 
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER      header;
    unsigned char			    reserved[2];
    UQUAD			            address;
} __attribute__ ((packed));

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
    unsigned char			    type;	        /* See acpi_interrupt_type */
    unsigned char			    id;
    unsigned char			    eid;
    unsigned char			    iosapic_vector;
    unsigned int			    global_irq;
    unsigned int			    srcflags;		/* See below */
};

/* Interrupt types */
enum ACPI_INT_IDS 
{
    ACPI_INTERRUPT_PMI                      = 1,
    ACPI_INTERRUPT_INIT,
    ACPI_INTERRUPT_CPEI,
    ACPI_INTERRUPT_COUNT
};

/* Interrupt source flags */
#define ACPI_CPEI_PROC_OVERRIDE (1 << 0)

struct ACPI_TABLE_TYPE_X2APIC
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER	header;
    /* 2 bytes of padding here */
    unsigned int			id;
    unsigned int			flags;
    unsigned int			acpi_uid;
};

#define ACPI_X2APIC_ENABLED (1 << 0)
    
struct ACPI_TABLE_TYPE_X2APIC_NMI
{
    struct ACPI_TABLE_DEF_ENTRY_HEADER	header;
    ACPI_INT_FLAGS			flags;
    unsigned int			acpi_uid;
    unsigned char			lint;
};

#define ACPI_UID_BROADCAST 0xFFFFFFFF

struct ACPI_TABLE_TYPE_HPET
{
    struct ACPI_TABLE_DEF_HEADER            header;
    unsigned int                            id;
    struct GENERIC_ACPI_ADDR                addr;
    unsigned char                           number;
    UWORD                                   min_tick;
    unsigned char                           page_protect;
} __attribute__ ((packed));

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

/*
 * acpi.resource base.
 * For a casual user it's actually black box. Use API to access the data.
 * These pointers are provided for diagnostics purposes only.
 */
struct ACPIBase
{
    struct  Library                         ACPIB_LibNode;

    struct Interrupt                        ACPIB_ResetHandler;

    struct ACPI_TABLE_TYPE_RSDP            *ACPIB_RSDP_Addr;		/* Supervisor-only!!!			*/
    struct ACPI_TABLE_DEF_HEADER     	   *ACPIB_SDT_Addr;		/* Raw XSDT or RSDT pointer	   	*/
    int                                     ACPIB_SDT_Count;		/* These two are private. Do not use!   */
    struct ACPI_TABLE_DEF_HEADER    	  **ACPIB_SDT_Entry;
    char			            ACPI_OEM_ID[6];		/* Cached from RSDP			*/
    unsigned char			    ACPI_Revision;
/*..*/
    APTR                                    ACPIB_ACPI_Data;            /* Base address of acpi data block 	*/
    APTR                                    ACPIB_ACPI_Data_End;
    APTR                                    ACPIB_ACPI_NVM;             /* Base address of acpi data block */

    int                                     ACPIB_ACPI_IRQ;

    int                                     ACPIB_ACPI_HT;
};

#endif /* __AROS_ACPI_H__ */
