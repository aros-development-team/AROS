/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Structure definitions for SMP machines following the Intel Multiprocessing Specification 1.1 and 1.4.
    Lang: english
*/

#ifndef __AROS_CPU_MPSPEC_H__
#define __AROS_CPU_MPSPEC_H__

#ifndef __AROS_CPU_H__
#   include <cpu/cpu.h>
#endif

/* ADJUSTABLE SETTINGS --------------- */

#define MAX_MPC_ENTRY       1024

/*  a maximum of 16 APICs with the current APIC ID architecture. */

#define MAX_APICS           16

/* ----------------------------------- */

/*
    This tag identifies where the SMP configuration
    information is. 
*/
 
#define SMP_MAGIC_IDENT     (('_'<<24)|('P'<<16)|('M'<<8)|'_')

struct intel_mp_confblock
{
	char                    mpcf_signature[4];                       /* "_MP_"                       */
	unsigned int            mpcf_physptr;                            /* Configuration table address  */
	unsigned char           mpcf_length;                             /* Our length (paragraphs)      */
	unsigned char           mpcf_specification;                      /* Specification version        */
	unsigned char           mpcf_checksum;                           /* Checksum (makes sum 0)       */
	unsigned char           mpcf_feature1;                           /* Standard or configuration ?  */
	unsigned char           mpcf_feature2;                           /* Bit7 set for IMCR|PIC        */
	unsigned char           mpcf_feature3;                           /* Unused (0)                   */
	unsigned char           mpcf_feature4;                           /* Unused (0)                   */
	unsigned char           mpcf_feature5;                           /* Unused (0)                   */
};

struct mp_config_table
{
	char                    mpc_signature[4];
	unsigned short          mpc_length;                             /* Size of table                */
	char                    mpc_spec;                               /* 0x01                         */
	char                    mpc_checksum;
	char                    mpc_oem[8];
	char                    mpc_productid[12];
	unsigned int            mpc_oemptr;                             /* 0 if not present             */
	unsigned short          mpc_oemsize;                            /* 0 if not present             */
	unsigned short          mpc_oemcount;
	unsigned int            mpc_lapic;                              /* APIC address                 */
	unsigned int            reserved;
};

#define MPC_SIGNATURE       "PCMP"

/* Followed by entries */

#define	MP_PROCESSOR        0
#define	MP_BUS              1
#define	MP_IOAPIC           2
#define	MP_INTSRC           3
#define	MP_LINTSRC          4
#define	MP_TRANSLATION      192                                     /* Used by IBM NUMA-Q to describe node locality */

struct mpc_config_processor
{
	unsigned char           mpc_type;
	unsigned char           mpc_apicid;                             /* Local APIC number */
	unsigned char           mpc_apicver;                            /* Its versions */
	unsigned char           mpc_cpuflag;
	unsigned int            mpc_cpufeature;		
	unsigned int            mpc_featureflag;                        /* CPUID feature value */
	unsigned int            mpc_reserved[2];
};

#define CPU_ENABLED         1                                       /* Processor is available */
#define CPU_BOOTPROCESSOR	2                                       /* Processor is the BP */

#define CPU_STEPPING_MASK   0x0F
#define CPU_MODEL_MASK      0xF0
#define CPU_FAMILY_MASK     0xF00

struct mpc_config_bus
{
	unsigned char           mpc_type;
	unsigned char           mpc_busid;
	unsigned char           mpc_bustype[6] __attribute((packed));
};

/* List of Bus Type string values, Intel MP Spec. */

#define BUSTYPE_EISA        "EISA"
#define BUSTYPE_ISA         "ISA"
#define BUSTYPE_INTERN      "INTERN"                                /* Internal BUS */
#define BUSTYPE_MCA         "MCA"
#define BUSTYPE_VL          "VL"                                    /* Local bus */
#define BUSTYPE_PCI         "PCI"
#define BUSTYPE_PCMCIA      "PCMCIA"
#define BUSTYPE_CBUS        "CBUS"
#define BUSTYPE_CBUSII      "CBUSII"
#define BUSTYPE_FUTURE      "FUTURE"
#define BUSTYPE_MBI         "MBI"
#define BUSTYPE_MBII        "MBII"
#define BUSTYPE_MPI         "MPI"
#define BUSTYPE_MPSA        "MPSA"
#define BUSTYPE_NUBUS       "NUBUS"
#define BUSTYPE_TC          "TC"
#define BUSTYPE_VME         "VME"
#define BUSTYPE_XPRESS      "XPRESS"
#define BUSTYPE_NEC98       "NEC98"

struct mpc_config_ioapic
{
	unsigned char           mpc_type;
	unsigned char           mpc_apicid;
	unsigned char           mpc_apicver;
	unsigned char           mpc_flags;
	unsigned int            mpc_apicaddr;
};

#define MPC_APIC_USABLE		0x01

struct mpc_config_intsrc
{
	unsigned char           mpc_type;
	unsigned char           mpc_irqtype;
	unsigned short          mpc_irqflag;
	unsigned char           mpc_srcbus;
	unsigned char           mpc_srcbusirq;
	unsigned char           mpc_dstapic;
	unsigned char           mpc_dstirq;
};

enum mp_irq_source_types {
	mp_INT      = 0,
	mp_NMI      = 1,
	mp_SMI      = 2,
	mp_ExtINT   = 3
};

#define MP_IRQDIR_DEFAULT	0
#define MP_IRQDIR_HIGH		1
#define MP_IRQDIR_LOW		3


struct mpc_config_lintsrc
{
	unsigned char           mpc_type;
	unsigned char           mpc_irqtype;
	unsigned short          mpc_irqflag;
	unsigned char           mpc_srcbusid;
	unsigned char           mpc_srcbusirq;
	unsigned char           mpc_destapic;	
	unsigned char           mpc_destapiclint;
};

#define MP_APIC_ALL	0xFF

struct mp_config_oemtable
{
	char                    oem_signature[4];
	unsigned short          oem_length;                                 /* Size of table */
	char                    oem_rev;                                    /* 0x01 */
	char                    oem_checksum;
	char                    mpc_oem[8];
};

#define MPC_OEM_SIGNATURE "_OEM"

struct mpc_config_translation
{
    unsigned char           mpc_type;
    unsigned char           trans_len;
    unsigned char           trans_type;
    unsigned char           trans_quad;
    unsigned char           trans_global;
    unsigned char           trans_local;
    unsigned short          trans_reserved;
};

#define MAX_IRQ_SOURCES 256
#define MAX_MP_BUSSES 32

/*
    Default configurations

    1	2 CPU ISA 82489DX
    2	2 CPU EISA 82489DX neither IRQ 0 timer nor IRQ 13 DMA chaining
    3	2 CPU EISA 82489DX
    4	2 CPU MCA 82489DX
    5	2 CPU ISA+PCI
    6	2 CPU EISA+PCI
    7	2 CPU MCA+PCI
*/

enum mp_bustype {
	MP_BUS_ISA = 1,
	MP_BUS_EISA,
	MP_BUS_PCI,
	MP_BUS_MCA,
	MP_BUS_NEC98
};

#endif  /* __AROS_CPU_MPSPEC_H__ */
