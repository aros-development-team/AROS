/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Probe installed IDT CPUs and display relevant information
    Lang: english
*/

/* BIG TO DO - SEPERATE THE INDIVIDUAL PROCESSOR FAMILY "PROBES" INTO RUNTIME SHARED LIBS OR SIMILAR */

/****************************************************************************************************
     Currently Supports:

        i386 compatable families...


*****************************************************************************************************/

#include "../x86.h"

/********************************************
		 IDT Specific information..
 ********************************************/

void    parse_i386_IDT( int maxi, struct i386_compat_intern * CPUi386 )
{

    struct  CPU_INTERN_DATA *global;
    ULONG                   speed, maxei,unused;
    int                     family = 0;
    char                    *BUFF_STR;

    if ((global = AllocMem(sizeof(struct CPU_INTERN_DATA),MEMF_PUBLIC|MEMF_CLEAR)))
    {
            speed = i386_approx_mhz();
    }
    else
    {
        printf( "ERROR: Couldn't allocate memory to parse CPU information .." );
        return;
    }
    FreeMem(global,sizeof(struct CPU_INTERN_DATA));

}
