/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Probe installed x86 compatable CPUs and display relevant information
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

     Soon....

        PPC?

*****************************************************************************************************/

#include "x86.h"

/********************************************
		    Variables
 ********************************************/

ULONG   st_low, st_high, end_low, end_high;

/********************************************/

void    i386_getregs ( char *out, int eax,int ebx,int ecx,int edx )
{
    int                     loop;

    out[16] = '\0';
    for( loop = 0 ; loop < 4 ; loop++ )
    {
        out[loop] = eax >> (8*loop);
        out[loop+4] = ebx >> (8*loop);
        out[loop+8] = ecx >> (8*loop);
        out[loop+12] = edx >> (8*loop);
    }
}

/********************************************/

void    i386_printregs ( int eax,int ebx,int ecx,int edx )
{
    char                    *out[17];
    i386_getregs( out, eax, ebx, ecx, edx );
    printf( out );
}

/********************************************/

int     i386_sprintregs ( int buffpos, char *buffer, int eax,int ebx,int ecx,int edx) /* returns buffer position */
{
    char                    *out[17];
	ULONG                   size;

    i386_getregs( out, eax, ebx, ecx, edx );
    size = strlen( out );
    AddBufferLine( buffpos, buffer, out );

    return  ( buffpos += size );
}

/********************************************/

void    Convert32 (unsigned long value)
{
	int                         loop;

	for( loop=0 ; loop<32 ; loop++, value <<= 1 )
    {
		putchar( ( 1 << 31 & value ) ? '1' : '0' );

		if ( loop == 23 || loop == 15 || loop == 7 )	putchar(' ');
	}
	putchar('\n');
}

/********************************************/

void    Convert64(unsigned long long value)
{
	Convert32 ( value >> 32 );
	printf ("           ");
	Convert32 ( value );
}

/********************************************/

void    i386_Parse_MSR ( unsigned int msr, int size)
{
	unsigned long               msrvala=0,msrvalb=0;
    unsigned long long          msrres;

    i386_rdmsr( msr, msrvala, msrvalb );
    msrres = ( ( msrvalb << 32 ) | msrvala );

	if ( msrres  == 1 )
    {
		printf ( "MSR: 0x%08x=0x%08llx : ", msr, msrres );
		if ( size == 32 ) Convert32( msrres );
		if ( size == 64 ) Convert64( msrres );
		return;
	}
	printf ("  Couldn't read MSR 0x%x\n", msr );
}

/********************************************/

#define TICKS (65536 - 8271)

/* Returns CPU clock in khz */

int     i386_cpuspeed ( void )
{
	int                         loops;

    Forbid();

	/* Setup timer */
	outb((inb(0x61) & ~0x02) | 0x01, 0x61);
	outb(0xb0, 0x43); 
	outb(TICKS & 0xff, 0x42);
	outb(TICKS >> 8, 0x42);

	asm("rdtsc":"=a" (st_low),"=d" (st_high));

	loops = 0;

	do loops++;
	while ((inb(0x61) & 0x20) == 0);

	asm( "rdtsc\n\t" "subl st_low,%%eax\n\t" "subl st_high,%%edx\n\t" :"=a" (end_low), "=d" (end_high)	);

    Permit();

	if (loops < 4 || end_low < 50000) return(-1);                      /* Make sure we have a credible result */

	return( end_low/48 );
}

/********************************************/

ULONG    i386_approx_mhz ( void )
{
    ULONG                   speed;

    printf("    Approximate Clock Frequency = ");

    if ((speed = i386_cpuspeed()) > 0)
    {
        speed += i386_cpuspeed();
        speed += i386_cpuspeed();

        speed /= 3;                                                 /* get an average for better results */

        if (speed < 1000000)
        {
			speed += 50;                                            /* for rounding         */

            printf("%d.%02d Mhz\n",speed/1000,(speed/100)%10);
		}
        else 
        {
			speed += 500;                                           /* for rounding         */
            printf("%d Mhz\n",speed/1000);
		}
	}
    return speed;
}

/********************************************
        PARSE INTEL 386 COMPATABLEs
 ********************************************/

void    parse_i386 ( struct i386_compat_intern * CPUi386, ULONG CPU_ID )
{
    unsigned long           li,maxi,maxei,ebx,ecx,edx,unused;
    char                    *strVendor;
    int                     i;

    /* . Display what we know from the CPUs i386 private structure .. */
    printf("\nProcessor internal (private) for i386 compatable\n");
    printf("(stored @  %p)\n\n",CPUi386);

/*    printf("    intern.x86             : %p\n",CPUi386->x86);
    printf("    intern.x86_vendor      : %p\n",CPUi386->x86_vendor);
    printf("    intern.x86_model       : %p\n",CPUi386->x86_model);
    printf("    intern.x86_mask        : %08x\n",CPUi386->x86_mask);
    printf("    intern.x86_hard_math   : %p\n",CPUi386->x86_hard_math);
    printf("    intern.x86_cpuid       : %08x\n",CPUi386->x86_cpuid);
    printf("    intern.x86_capability  : %08x\n",CPUi386->x86_capability); */

    strVendor = &CPUi386->x86_vendor_id;

#warning TODO: Insert code to verify CPUID instruction availability

    i386_cpuid(0,maxi,unused,unused,unused);
    maxi &= 0xffff;                                                     /* The high-order word is non-zero on some Cyrix CPUs */

    /* Max CPUID level supported &  Vendor Name */
    i386_cpuid(0,unused,ebx,ecx,edx);

    printf("    Vendor ID: \"");
    for(i=0;i<4;i++) putchar(ebx >> (8*i));
    for(i=0;i<4;i++) putchar(edx >> (8*i));
    for(i=0;i<4;i++) putchar(ecx >> (8*i));
    printf("\"; CPUID level %ld\n\n",maxi);

/* Use the first 4 letters of the vender string to ID our processor.. */

/*      

        EBX-EDX-ECX             Vendor

        "AuthenticAMD"        AMD processor 
        "GenuineIntel"        Intel processor 
        "CyrixInstead"        Cyrix processor 
        "UMC UMC UMC "        UMC processor 
        "NexGenDriven"        NexGen processor 
        "CentaurHauls"        Centaur processor 
        "RiseRiseRise"        Rise Technology processor 
        "SiS SiS SiS "        SiS processor 
        "GenuineTMx86"        Transmeta processor 
        "Geode by NSC"        National Semiconductor processor 
*/

    switch(ebx)
    {
    case 0x68747541:                                                    /* AMD processor */
        parse_i386_AMD(maxi, CPUi386);
        break;

    case 0x756e6547:                          
        switch(ecx)
        {
        case 0x6c65746e:
            parse_i386_Intel(maxi, CPUi386);                                /* Intel processor */
            break;
        case 0x756e6547:
            parse_i386_Transmeta(maxi, CPUi386);                        /* Transmeta processor */
            break;
        }

    case 0x69727943:                                                    /* Cyrix processor */
        parse_i386_Cyrix(maxi, CPUi386);
        break;

    //case 0x68747541:                                                    /* UMC processor */
    //    parse_i386_UMC(maxi, CPUi386);
    //    break;

    //case 0x756e6547:                                                    /* NexGen processor */
    //    parse_i386_NexGen(maxi, CPUi386);
    //    break;

    //case 0x69727943:                                                    /* Centaur processor */
    //    parse_i386_Centaur(maxi, CPUi386);
    //    break;

    //case 0x68747541:                                                    /* Rise Technology processor */
    //    parse_i386_Rise(maxi, CPUi386);
    //    break;

    //case 0x756e6547:                                                    /* SiS processor  */
    //    parse_i386_SiS(maxi, CPUi386);
    //    break;

    //case 0x69727943:                                                    /* National Semiconductor processor */
    //    parse_i386_NSC(maxi, CPUi386);
    //    break;

    default:
        printf("Unknown vendor\n");
        break;
    }

    /* Dump all the CPUID results in raw hex */
    printf("\n    Raw CPUID Dump:\n    ---------------\n\n");
    printf("         eax in    eax      ebx      ecx      edx\n");

    for(i=0;i<=maxi;i++)
    {
        unsigned long eax,ebx,ecx,edx;

        i386_cpuid(i,eax,ebx,ecx,edx);
        printf("        %08x %08lx %08lx %08lx %08lx\n",i,eax,ebx,ecx,edx);
    }

    i386_cpuid(0x80000000,maxei,unused,unused,unused);

    for(li=0x80000000;li<=maxei;li++)
    {
        unsigned long eax,ebx,ecx,edx;

        i386_cpuid(li,eax,ebx,ecx,edx);
        printf("        %08lx %08lx %08lx %08lx %08lx\n",li,eax,ebx,ecx,edx);
    }
    printf("\n");
}

/********************************************
    Transmeta  specific information
 ********************************************/

void    parse_i386_Transmeta( int maxi, struct i386_compat_intern * CPUi386 )
{

    struct  CPU_INTERN_DATA *global;
    ULONG                   speed, maxei,unused;
    int                     family = 0;
    char                    *BUFF_STR;

    if ((global = AllocMem(sizeof(struct CPU_INTERN_DATA),MEMF_PUBLIC|MEMF_CLEAR)))
    {
        //if ( maxi >= 3 )                                                                   /* Crusoe CPU serial number */
        //{
        //  unsigned long signature,unused,ebx;

        //  i386_cpuid(3,unused,ebx,unused,unused);
            
        //  Intel_CPU_NAME_cnt = AddBufferLine(Intel_CPU_NAME_cnt, Intel_CPU_NAME, "    Processor serial: ");

        //  sprintf( BUFF_STR,"-%04lX",ebx >> 16);
        //  Intel_CPU_NAME_cnt = AddBufferLine(Intel_CPU_NAME_cnt, Intel_CPU_NAME, BUFF_STR);
        //  sprintf( BUFF_STR,"-%04lX",ebx & 0xffff);
        //  Intel_CPU_NAME_cnt = AddBufferLine(Intel_CPU_NAME_cnt, Intel_CPU_NAME, BUFF_STR);

        //}
            speed = i386_approx_mhz();
        }
    else
    {
        printf( "ERROR: Couldn't allocate memory to parse CPU information .." );
        return;
    }
    FreeMem(global,sizeof(struct CPU_INTERN_DATA));

}

/********************************************
       UMC  specific information
 ********************************************/

void    parse_i386_UMC( int maxi, struct i386_compat_intern * CPUi386 )
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

/********************************************
     NexGen  specific information
 ********************************************/

void    parse_i386_NexGen( int maxi, struct i386_compat_intern * CPUi386 )
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

/********************************************
     Centaur  specific information
 ********************************************/

void    parse_i386_Centaur( int maxi, struct i386_compat_intern * CPUi386 )
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

/********************************************
           SiS  specific information
 ********************************************/

void    parse_i386_SiS( int maxi, struct i386_compat_intern * CPUi386 )
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

