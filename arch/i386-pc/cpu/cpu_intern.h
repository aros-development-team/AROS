/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#ifndef _CPU_INTERN_H
#define _CPU_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif
#ifndef HIDD_HIDD_H
#include <hidd/hidd.h>
#endif
#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif

#define DEBUG   1

#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <hardware/intbits.h>
#include <asm/segments.h>
#include <asm/linkage.h>
#include <asm/ptrace.h>
#include <dos/dosextens.h>

#include <aros/arossupportbase.h>
#include <aros/asmcall.h>
#include <aros/config.h>
#include <aros/debug.h>
#define  DEBUG_NAME_STR                     "[" NAME_STRING "]"
#include <aros/multiboot.h>

#include <hardware/custom.h>

//#include <proto/acpi.h>
#include <proto/exec.h>
#include <proto/cpu.h>

#include <aros/debug.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Edit the following files at compiler/include/cpu */

#include <hardware/cpu/cpu.h>                   /* NicJA - Experimental CPU specific Data Structs */
#include <hardware/cpu/cpu_i386.h>              /* NicJA - Experimental CPU specific Data Structs */
#include <hardware/cpu/cpu_mpspec.h>            /* NicJA - Experimental SMP specific Data Structs */

#include <hardware/acpi/acpi.h>                 /* NicJA - Experimental ACPI/APIC specific Data Structs */

#include LC_LIBDEFS_FILE

#undef memcpy
#define memcpy(_d, _s, _len)                     \
{                                                \
    int len = _len;                              \
    while (len)                                  \
    {                                            \
       ((char *)_d)[len-1] = ((char *)_s)[len-1];\
       len--;                                    \
    }                                            \
}


/* Temporary information */
#if 1

void clr();
void scr_RawPutChars(char *, int);

char tab[127];
#ifdef rkprintf
# undef rkprintf
#endif
#define rkprintf(x...)	scr_RawPutChars(tab, snprintf(tab,126, x))

#else

#define clr() /* eps */
#define rkprintf(x...) /* eps */

#endif

#define i386_cpuid(in,a,b,c,d)           asm("cpuid": "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (in));

struct CPUBase *CPUBase;

#define APICBase        (CPUBase->CPUB_APICBase)

#define SysBase		(CPUBase->CPUB_SysBase)
#ifdef UtilityBase
#   undef UtilityBase
#endif
#define UtilityBase     (CPUBase->CPUB_UtilBase)


/**********************************************************************/
/* from smp.c ........... */
void    MP_processor_info ( struct mpc_config_processor *mc, struct SMP_Definition *SMP_Group );
void    MP_bus_info ( struct mpc_config_bus *mc, struct SMP_Definition *SMP_Group );
void    MP_ioapic_info ( struct mpc_config_ioapic *mc, struct SMP_Definition *SMP_Group );
void    MP_intsrc_info ( struct mpc_config_intsrc *mc, struct SMP_Definition *SMP_Group );
void    MP_lintsrc_info ( struct mpc_config_lintsrc *mc, struct SMP_Definition *SMP_Group );
void    MP_translation_info ( struct mpc_config_translation *mc, struct SMP_Definition *SMP_Group );

int     mpfcb_checksum( unsigned char *mpcb, int len );
void    smp_read_mpc_oem( struct mp_config_oemtable *oemtable, unsigned short oemsize, struct SMP_Definition *SMP_Group );
void    mps_oem_check ( struct mp_config_table *mpcf, char *oem, char *productid, struct SMP_Definition *SMP_Group );
int     smp_alloc_memory ( void );
int     scan_for_smpconfig ( unsigned long base, unsigned long length );
int     find_smp_config ( void );
void    get_smp_config ( struct intel_mp_confblock *mpcfb, struct CPUBase *CPUBase  );
int     smp_read_mpcfb ( struct mp_config_table *mpcf, struct CPUBase *CPUBase );

#endif /* _CPU_INTERN_H */
