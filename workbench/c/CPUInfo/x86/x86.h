/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Probe installed CPUs and display relevant information
    Lang: english
*/

/* BIG TO DO - SEPERATE THE INDIVIDUAL PROCESSOR FAMILY "PROBES" INTO RUNTIME SHARED LIBS OR SIMILAR */

/****************************************************************************************************
     Currently Supports:

        i386 compatable families...
            AMD 486/5x86/K5/K6/K6-II/K6-III/Athlon/Duron/Opteron/Athlon64
            Intel P5/P54C/P55C/P24T/P6/P2/P3/PM/Itanium(IA-64) 
            Cyrix  5x86/M1/MediaGX/M2
            UMC  
            NexGen  Nx586
            Centaur  C6/C2/C3
            Rise Technology mP6
            SiS  55x
            Transmeta Crusoe TM3x00 and TM5x00
            National Semiconductor  Geode

*****************************************************************************************************/
#ifndef _CPU_x86INTERN_H
#define _CPU_x86INTERN_H

#include    <proto/exec.h>
#include    <proto/dos.h>
#include    <proto/timer.h>
#include    <proto/cpu.h>

#include    <exec/types.h>
#include    <exec/lists.h>
#include    <exec/io.h>
#include    <exec/memory.h>

#include    <dos/dos.h>

#include    <devices/timer.h>

#include    <sys/time.h>

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <time.h>

#include    <asm/io.h>

#include    <hardware/cpu/cpu_i386.h>

#include "../cpuinfo.h"

/********************************************
		       ASM Functions
 ********************************************/

#define i386_cpuid(in,a,b,c,d)          asm("cpuid": "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (in));
#define i386_rdmsr(msr,val1,val2)       __asm__ __volatile__ ("rdmsr": "=a" (val1), "=d" (val2) : "c" (msr));

/********************************************
		         Stub Calls...
 ********************************************/

ULONG   i386_approx_mhz ( void );
void    i386_getregs ( char *out, int eax,int ebx,int ecx,int edx );
void    i386_printregs ( int eax,int ebx,int ecx,int edx );
int     i386_sprintregs ( int buffpos, char *buffer, int eax,int ebx,int ecx,int edx);

void    i386_Parse_MSR ( unsigned int msr, int size);

void    parse_i386_AMD ( int maxi, struct i386_compat_intern * CPUi386 );
void    parse_i386_Intel ( int maxi, struct i386_compat_intern * CPUi386 );
void    parse_i386_Cyrix ( int maxi, struct i386_compat_intern * CPUi386 );

void    parse_i386_Transmeta ( int maxi, struct i386_compat_intern * CPUi386 );
void    parse_i386_UMC ( int maxi, struct i386_compat_intern * CPUi386 );
void    parse_i386_NexGen ( int maxi, struct i386_compat_intern * CPUi386 );
void    parse_i386_Centaur ( int maxi, struct i386_compat_intern * CPUi386 );
void    parse_i386_Rise ( int maxi, struct i386_compat_intern * CPUi386 );
void    parse_i386_SiS ( int maxi, struct i386_compat_intern * CPUi386 );
void    parse_i386_NSC ( int maxi, struct i386_compat_intern * CPUi386 );

/********************************************
		         Structures
 ********************************************/

struct CPU_INTERN_DATA
{
	UBYTE       CPU_BUFF[4096];     /* TEMPORARY BUFFER */
    UBYTE		CPU_NAME[2048];
	UBYTE		CPU_IDENTITY[4096];
	UBYTE		CPU_FEATURES[4096];
	UBYTE		CPU_CACHE[2048];
	UBYTE		CPU_ADDR[2048];
};

#define CONN_UNKNOWN		    0
#define CONN_SOCKET_3		    1
#define CONN_SOCKET_4		    2
#define CONN_SOCKET_5	    	3
#define CONN_SOCKET_7	    	4
#define CONN_SOCKET_370	    	5
#define CONN_SOCKET_370_FCPGA	6
#define CONN_SOCKET_5_7		    7
#define CONN_SUPER_SOCKET_7	    8
#define CONN_SLOT_A		        9
#define CONN_SOCKET_A		    10
#define CONN_SOCKET_A_SLOT_A	11
#define CONN_SOCKET_A_OR_SLOT_A	12
#define CONN_SOCKET_57B		    13
#define CONN_MOBILE_7		    14
#define CONN_SOCKET_8	    	15
#define CONN_SLOT_1		        16
#define CONN_SLOT_2		        17
#define CONN_SOCKET_423	    	18
#define CONN_MMC		        19
#define CONN_MMC2		        20
#define CONN_BGA474		        21
#define CONN_BGA		        22
#define CONN_SOCKET_754	        23
#define CONN_SOCKET_478         24

#endif /* _CPU_x86INTERN_H */
