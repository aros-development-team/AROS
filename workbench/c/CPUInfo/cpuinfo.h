/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Probe installed CPUs and display relevant information
    Lang: english
*/

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

     Soon....

        PPC?

*****************************************************************************************************/
#ifndef _CPUINFO_INTERN_H
#define _CPUINFO_INTERN_H

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

#include    <hardware/cpu/cpu.h>
#include    <hardware/cpu/cpu_i386.h>

/********************************************
		    Version Information
 ********************************************/

#define	APPNAME                     "CPUInfo"
#define	VERSION                     45
#define	REVISION                    30
#define VERSSTRING                  "45.30"
#define	DATE                        "01.02.2004"
#define	VERS                        APPNAME " " VERSSTRING
#define	VSTRING                     APPNAME " "VERSSTRING" (" DATE ")\n\r"
#define	VERSTAG                     "\0$VER: " APPNAME " "VERSSTRING" (" DATE ")\n"

/********************************************
		   Command Line Arguments
 ********************************************/

#define ARG_TEMPLATE "V=VERBOSE/S"

enum
{
    ARG_VERBOSE,
    NOOFARGS
};

/********************************************
		         Stub Calls...
 ********************************************/

BOOL    isLastNode ( struct MinNode *CurrNode );
int     AddBufferLine ( int buffpos, char *buffer, char *line );
void    parse_i386 ( struct i386_compat_intern * CPUi386, ULONG CPU_ID  );

#endif /* _CPUINFO_INTERN_H */
