#/*
#    (C) 2000 AROS - The Amiga Research OS
#    $Id$
#
#    Desc: i386 compatable CPU detection routine
#    Lang: english
#*/

#include <aros/asmcall.h>
#include "cpu_intern.h"

unsigned long       unused;
UBYTE               cpu_ready=0;

#define i386_getcr0(currcr0)      asm volatile ( "movl %%cr0,%0" : "=r" (currcr0) :);
#define i386_setcr0(newcr0)       asm volatile ( "movl %0,%%cr0" :: "r" (newcr0) );

UWORD i386_CheckCPU_FPU( void );

/** 

    THIS FUNCTION IS CALLED A> BY THE BOOT CPU DURING EXEC INIT, AND B> BY SECONDARY PROCESSORS IN THEIR INIT PHASE

    N.B

    This code is run in a supervisor state - and as such cant contain kprinf output.
    also - 

**/

void i386_CheckCPU_Type( struct i386_compat_intern *CPU_intern )
{
    unsigned long           li,maxi,maxei,eax,ebx,ecx,edx,newcr0;
    BOOL                    CRxSet = FALSE;
    int                     i,loop;
    char                    *strVendor;
    
    CPU_intern->x86_cpuid = -1;
    CPU_intern->x86 = 3;

#warning TODO: code the EFLAGS-> AC Bit check
    if ( TRUE ) /* Can we set EFLAGS-> AC Bit? */
    {
        CPU_intern->x86 = 4;                                                /* Ok we have something better than a 386.. */

#warning TODO: code the ID flag check
        if ( TRUE ) /* Try to change ID flag -if we can CPUID is implemented */
        {
            /* Ok we CPUID!! Hurray!.. */
            i386_cpuid( 0,maxi,unused,unused,unused );
            maxi &= 0xffff;                                                 /* The high-order word is non-zero on some Cyrix CPUs */

            i386_cpuid( 0,unused,ebx,ecx,edx );                             /* Max CPUID level supported &  Vendor Name */

            *strVendor = &CPU_intern->x86_vendor_id;

            for(i=0;i<4;i++) strVendor[i]   = ( ebx >> (8*i) );
            for(i=0;i<4;i++) strVendor[4+i] = ( edx >> (8*i) );
            for(i=0;i<4;i++) strVendor[8+i] = ( ecx >> (8*i) );

            if ( maxi >= 1 )                                                /* Is more available? Do standard stuff */
            {
                i386_cpuid(1,eax,ebx,unused,edx);

                CPU_intern->x86_mask        = eax & 0xf;                    /* cpu stepping */
                CPU_intern->x86_model       = (eax >> 4) & 0xf;             /* cpu model */
                CPU_intern->x86_vendor      = (eax >> 8) & 0xf;             /* cpu family */
                CPU_intern->x86_reserved    = eax >> 12;

                CPU_intern->x86_capability = edx;
            }
            
        }
        CRxSet = TRUE;
    }
    /* */

    i386_getcr0(newcr0);

    if ( CRxSet == TRUE )
    {
        /* Update CR0 register */
		/* Save PG,PE,ET */
		/* Set AM,WP,NE and MP */
        newcr0 &= 0x80000011;
        newcr0 |= 0x00050022;
    }
    else
    {
		/* Update CR0 reg. i386 version */
		/* Save PG,PE,ET */
		/* Set MP */
        newcr0 &= 0x80000011;
        newcr0 |= 0x2;
    }

#warning TODO: Next line disabled due to problems .. (setcr0)
    i386_setcr0(newcr0);

    CPU_intern->x86_hard_math = i386_CheckCPU_FPU();


    cpu_ready += 1;

#warning TODO: Next line disabled due to problems .. (invalidate LDT etc,)
    /* Invalidate LDT */
	/* Clear D flag as needed by AROS and gcc */

    asm volatile(   " xorl %%eax,%%eax\n\t"
                    " lldt %%ax\n\t"
                    " cld"
                    :: "m" (unused) : "eax","ax","memory" ); 

    if ( cpu_ready > 1 )
    {
        /*  We have been called as the start of the cpu init routine for a secondary processor
            so call the initialise routine now*/
        
        //initialise_secondary();                                           /*call the initialise secondary MP function!*/
        //THIS NEVER RETURNS!
    }
}

UWORD i386_CheckCPU_FPU( void )                                             /* This checks for 287/387. */
{
    UWORD                   RetVAL;
    UBYTE                   fpu;
    int                     loop;

    RetVAL = 0;

    asm volatile(   " clts\n\t"
                    " fninit\n\t"
                    " fstsw %%ax\n\t"
                    " movb %%al,%0"
                    : "=r" (fpu) : "m" (unused) : "%0", "ax", "al", "memory" );

    //kprintf(" tested ..");
    for (loop = 0; loop < 100000000; loop++)
        loop = loop;

    if (fpu!=0)
    {   
    	asm volatile(   " movl %%cr0,%%eax\n\t"		                        /* no coprocessor: have to set bits */
    	                " xorl $4,%%eax\n\t"		                        /* set EM */
	                    " movl %%eax,%%cr0"
                        :: "m" (unused) : "eax","memory" );
    }
    else
    {
        RetVAL = 1;
        asm volatile( ".byte 0xDB,0xE4" );                                  /* fsetpm for 287, ignored by 387 */
    }
    return RetVAL;
}

