/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Probe installed Cyrix CPUs and display relevant information
    Lang: english
*/

/* BIG TO DO - SEPERATE THE INDIVIDUAL PROCESSOR FAMILY "PROBES" INTO RUNTIME SHARED LIBS OR SIMILAR */

/****************************************************************************************************
     Currently Supports:

        i386 compatable families...
            Cyrix  5x86/M1/MediaGX/M2

*****************************************************************************************************/

#include "../x86.h"

/********************************************
		    Cyrix CPU Features
 ********************************************/

char *Cyrix_standard_feature_flags_5[] = {
    "FPU    : Floating Point Unit",
    "V86    : Virtual Mode Extensions",
    "       : Debug Extension",
    "       : 4MB Page Size",
    "       : Time Stamp Counter",
    "       : i386_rdmsr/WRMSR (Model Specific Registers)",
    "PAE    : ",
    "       : Machine Check Exception",
    "       : COMPXCHG8B Instruction",
    "APIC   : APIC - On-chip Advanced Programmable Interrupt Controller present and enabled",
    "10     : Reserved",
    "11     : Reserved",
    "MTRR   : Memory Type Range Registers",
    "13     : Reserved",
    "       : RMachine Check",
    "CMOV   : Conditional Move Instruction",
    "16     : Reserved",
    "17     : Reserved",
    "18     : Reserved",
    "19     : Reserved",
    "20     : Reserved",
    "21     : Reserved",
    "22     : Reserved",
    "       : MMX instructions",
    "24     : Reserved",
    "25     : Reserved",
    "26     : Reserved",
    "27     : Reserved",
    "28     : Reserved",
    "29     : Reserved",
    "30     : Reserved",
};

char *Cyrix_standard_feature_flags_not5[] = {
    "FPU    : Floating Point Unit",
    "V86    : Virtual Mode Extensions",
    "       : Debug Extension",
    "       : 4MB Page Size",
    "       : Time Stamp Counter",
    "       : i386_rdmsr/WRMSR (Model Specific Registers)",
    "PAE    : ",
    "       : Machine Check Exception",
    "       : COMPXCHG8B Instruction",
    "       : APIC - On-chip Advanced Programmable Interrupt Controller present and enabled",
    "10     : Reserved",
    "11     : Reserved",
    "MTRR   : Memory Type Range Registers",
    "       : Global Paging Extension",
    "       : Machine Check",
    "CMOV   : Conditional Move Instruction",
    "16     : Reserved",
    "17     : Reserved",
    "18     : Reserved",
    "19     : Reserved",
    "20     : Reserved",
    "21     : Reserved",
    "22     : Reserved",
    "       : MMX instructions",
    "24     : Reserved",
    "25     : Reserved",
    "26     : Reserved",
    "27     : Reserved",
    "28     : Reserved",
    "29     : Reserved",
    "30     : Reserved",
};

char *Cyrix_extended_feature_flags[] = {
    "FPU    : Floating Point Unit",
    "V86    : Virtual Mode Extensions",
    "       : Debug Extension",
    "       : Page Size Extensions",
    "       : Time Stamp Counter",
    "       : Cyrix MSR",
    "PAE    : ",
    "       : MC Exception",
    "       : COMPXCHG8B",
    "       : APIC on chip",
    "       : SYSCALL/SYSRET",
    "11     : Reserved",
    "MTRR   : ",
    "       : Global bit",
    "       : Machine Check",
    "CMOV   : ",
    "       : FPU CMOV",
    "17     : Reserved",
    "18     : Reserved",
    "19     : Reserved",
    "20     : Reserved",
    "21     : Reserved",
    "22     : Reserved",
    "       : MMX",
    "       : Extended MMX",
    "25     : Reserved",
    "26     : Reserved",
    "27     : Reserved",
    "28     : Reserved",
    "29     : Reserved",
    "30     : Reserved",
    "       : 3DNow instructions",
};

/********************************************
        Cyrix specific information
 ********************************************/

char    i386_cyrix_TLB_decode ( int tlb, char *BUFF_STR )
{
    switch(tlb & 0xff)
    {
    case 0:
        break;
    case 0x70:
        sprintf( BUFF_STR, "        TLB: 32 entries 4-way associative 4KB pages\n" );
        break;
    case 0x80:
        sprintf( BUFF_STR, "        L1 Cache: 16KB 4-way associative 16 bytes/line\n" );
        break;
    }
}

/********************************************/

void    parse_i386_Cyrix ( int maxi, struct i386_compat_intern * CPUi386 )
{
    struct  CPU_INTERN_DATA *global;
    ULONG                   speed, maxei,unused;
    int                     family = 0;
    char                    *BUFF_STR, *Cyrix_CPU_NAME, *Cyrix_CPU_IDENTITY, *Cyrix_CPU_FEATURES, *Cyrix_CPU_CACHE, *Cyrix_CPU_ADDR;
    int                     Cyrix_CPU_NAME_cnt, Cyrix_CPU_IDENTITY_cnt, Cyrix_CPU_FEATURES_cnt, Cyrix_CPU_CACHE_cnt, Cyrix_CPU_ADDR_cnt;

    if ((global = AllocMem(sizeof(struct CPU_INTERN_DATA),MEMF_PUBLIC|MEMF_CLEAR)))
    {
        Cyrix_CPU_NAME          = global->CPU_NAME;
        Cyrix_CPU_IDENTITY      = global->CPU_IDENTITY;
        Cyrix_CPU_FEATURES      = global->CPU_FEATURES;
        Cyrix_CPU_CACHE         = global->CPU_CACHE;
        Cyrix_CPU_ADDR          = global->CPU_ADDR;
        BUFF_STR                = global->CPU_BUFF;

        Cyrix_CPU_NAME_cnt = Cyrix_CPU_IDENTITY_cnt = Cyrix_CPU_FEATURES_cnt = Cyrix_CPU_CACHE_cnt = Cyrix_CPU_ADDR_cnt = 0;

        Cyrix_CPU_FEATURES_cnt = AddBufferLine( Cyrix_CPU_FEATURES_cnt, Cyrix_CPU_FEATURES, "    Cyrix-specific functions\n" );

        int i;

        i386_cpuid(0x80000000,maxei,unused,unused,unused);

        /* Do standard stuff */
        if(maxi >= 1)
        {
            unsigned long                   eax,unused,edx;
            int                             stepping,model,family,reserved;

            i386_cpuid(1,eax,unused,unused,edx);

            stepping = eax & 0xf;
            model = (eax >> 4) & 0xf;
            family = (eax >> 8) & 0xf;
            reserved = eax >> 12;

            sprintf( BUFF_STR,"    Family: %d Model: %d [",family,model);
            Cyrix_CPU_IDENTITY_cnt = AddBufferLine(Cyrix_CPU_IDENTITY_cnt, Cyrix_CPU_IDENTITY, BUFF_STR);
            switch ( family )
            {
            case 4:
                switch ( model )
                {
                case 9:
	                Cyrix_CPU_IDENTITY_cnt = AddBufferLine(Cyrix_CPU_IDENTITY_cnt, Cyrix_CPU_IDENTITY, "5x86");
	                break;
                }
            break;
            case 5:
                switch ( model )
                {
                case 2:
	                Cyrix_CPU_IDENTITY_cnt = AddBufferLine(Cyrix_CPU_IDENTITY_cnt, Cyrix_CPU_IDENTITY, "6x86");
	                break;
                case 4:
	                Cyrix_CPU_IDENTITY_cnt = AddBufferLine(Cyrix_CPU_IDENTITY_cnt, Cyrix_CPU_IDENTITY, "GX, GXm");
	                break;
                }
            break;
            case 6:
                switch ( model )
                {
                case 0:
	                Cyrix_CPU_IDENTITY_cnt = AddBufferLine(Cyrix_CPU_IDENTITY_cnt, Cyrix_CPU_IDENTITY, "6x86MX");
	                break;
                }
            break;
            }
            Cyrix_CPU_IDENTITY_cnt = AddBufferLine(Cyrix_CPU_IDENTITY_cnt, Cyrix_CPU_IDENTITY, "]\n\n");

            if ( family == 5 && model == 0 )
            {
                int             i;

                for ( i=0 ; i<32 ; i++ )
                {
	                if ( edx & (1<<i) )
                    {
                        sprintf( BUFF_STR,"        %s\n",Cyrix_standard_feature_flags_5[i]);
                        Cyrix_CPU_FEATURES_cnt = AddBufferLine(Cyrix_CPU_FEATURES_cnt, Cyrix_CPU_FEATURES, BUFF_STR);
	                }
                }
            }
            else
            {
                int             i;

                for ( i=0 ; i<32 ; i++ )
                {
	                if ( edx & (1<<i) )
                    {
                        sprintf( BUFF_STR,"        %s\n",Cyrix_standard_feature_flags_not5[i]);
                        Cyrix_CPU_FEATURES_cnt = AddBufferLine(Cyrix_CPU_FEATURES_cnt, Cyrix_CPU_FEATURES, BUFF_STR);
	                }
                }
            }
        }

        if ( maxi >= 2 )                                                            /* TLB and L1 Cache info */
        {
            int                 ntlb = 255;
            int                 i;

            for( i=0 ; i<ntlb ; i++ )
            {
                unsigned long                   eax,edx,unused;

                i386_cpuid( 2, eax, unused, unused, edx);
                ntlb =  eax & 0xff;

                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode( eax >> 8, BUFF_STR ));
                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode( eax >> 16, BUFF_STR ));
                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode( eax >> 24, BUFF_STR ));
                  
                /* ebx and ecx are reserved */

                if (( edx & 0x80000000 ) == 0 )
                {
	                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode( edx, BUFF_STR ));
	                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode( edx >> 8, BUFF_STR ));
	                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode( edx >> 16, BUFF_STR ));
	                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode( edx >> 24, BUFF_STR ));
                }
            }
        }

        
        if ( maxei < 0x80000000 ) return;                                           /* Check for presence of extended info */


        Cyrix_CPU_IDENTITY_cnt = AddBufferLine(Cyrix_CPU_IDENTITY_cnt, Cyrix_CPU_IDENTITY, "\n    Extended info:\n");

        if ( maxei >= 0x80000001 )
        {
            unsigned long                   eax, ebx, ecx, edx;
            int                             stepping, model, family, reserved, i;

            i386_cpuid( 0x80000001, eax, ebx, ecx, edx );

            stepping = eax & 0xf;
            model = (eax >> 4) & 0xf;
            family = (eax >> 8) & 0xf;
            reserved = eax >> 12;

            sprintf( BUFF_STR,"    Family: %d Model: %d [",family,model);
            Cyrix_CPU_IDENTITY_cnt = AddBufferLine(Cyrix_CPU_IDENTITY_cnt, Cyrix_CPU_IDENTITY, BUFF_STR);
            switch ( family )
            {
            case 4:
                Cyrix_CPU_IDENTITY_cnt = AddBufferLine(Cyrix_CPU_IDENTITY_cnt, Cyrix_CPU_IDENTITY, "MediaGX");
                break;
            case 5:
                Cyrix_CPU_IDENTITY_cnt = AddBufferLine(Cyrix_CPU_IDENTITY_cnt, Cyrix_CPU_IDENTITY, "6x86/GXm");
                break;
            case 6:
                Cyrix_CPU_IDENTITY_cnt = AddBufferLine(Cyrix_CPU_IDENTITY_cnt, Cyrix_CPU_IDENTITY, "6x86/MX");
            }
            Cyrix_CPU_IDENTITY_cnt = AddBufferLine(Cyrix_CPU_IDENTITY_cnt, Cyrix_CPU_IDENTITY, "]\n\n");

            Cyrix_CPU_FEATURES_cnt = AddBufferLine(Cyrix_CPU_FEATURES_cnt, Cyrix_CPU_FEATURES, "Extended feature flags:\n");
            for( i=0 ; i<32 ; i++ )
            {
                if ( edx & (1<<i) )
                {
                    sprintf( BUFF_STR,"        %s\n",Cyrix_extended_feature_flags[i]);
                    Cyrix_CPU_FEATURES_cnt = AddBufferLine(Cyrix_CPU_FEATURES_cnt, Cyrix_CPU_FEATURES, BUFF_STR);
                }
            }
        }
        Cyrix_CPU_FEATURES_cnt = AddBufferLine(Cyrix_CPU_FEATURES_cnt, Cyrix_CPU_FEATURES, "\n");

        if ( maxei >= 0x80000002 )                                              /* Processor identification string */
        {
            char                namestring[49],*cp;
            int                 j;
            cp = namestring;
            
            Cyrix_CPU_NAME_cnt = AddBufferLine(Cyrix_CPU_NAME_cnt, Cyrix_CPU_NAME, "    Processor name string: ");
            for ( j=0x80000002 ; j<=0x80000004 ; j++ )
            {
                unsigned long           eax, ebx, ecx, edx;

                i386_cpuid( j, eax, ebx, ecx, edx );
                Cyrix_CPU_NAME_cnt = i386_sprintregs(Cyrix_CPU_NAME_cnt, Cyrix_CPU_NAME, eax,ebx,ecx,edx);
            }
        }

        if ( maxei >= 0x80000005 )                                              /* TLB and L1 Cache info */
        {
            int                 ntlb = 255;
            int                 i;

            for ( i=0 ; i<ntlb ; i++ )
            {
                unsigned long           eax, ebx, ecx, unused;

                i386_cpuid( 0x80000005, eax, ebx, ecx, unused);
                ntlb =  eax & 0xff;

                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode(  ebx >> 8, BUFF_STR ));
                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode(  ebx >> 16, BUFF_STR ));
                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode(  ebx >> 24, BUFF_STR ));
                  
                /* eax and edx are reserved */

                if (( ecx & 0x80000000 ) == 0 )
                {
	                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode(  ecx, BUFF_STR ));
	                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode(  ecx >> 8, BUFF_STR ));
	                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode(  ecx >> 16, BUFF_STR ));
	                Cyrix_CPU_CACHE_cnt = AddBufferLine(Cyrix_CPU_CACHE_cnt, Cyrix_CPU_CACHE, i386_cyrix_TLB_decode(  ecx >> 24, BUFF_STR ));
                }
            }
        }

        
        //for(i=0x80000000;i<=maxei;i++)                                          /* Dump extended info, if any, in raw hex */
        //{
        //    unsigned long           eax,ebx,ecx,edx;

        //    i386_cpuid(i,eax,ebx,ecx,edx);
        //    printf("eax in: 0x%x, eax = %08lx ebx = %08lx ecx = %08lx edx = %08lx\n",i,eax,ebx,ecx,edx);
        //}

        if ( Cyrix_CPU_NAME_cnt > 0 ) printf( Cyrix_CPU_NAME );             /* CPUs Retail Name     */

        speed = i386_approx_mhz();

        /* TODO : Calculate FSB ..     */

        if ( Cyrix_CPU_IDENTITY_cnt > 0 ) printf( Cyrix_CPU_IDENTITY );     /* CPUs Internal Name   */
        if ( Cyrix_CPU_FEATURES_cnt > 0 ) printf( Cyrix_CPU_FEATURES );     /* CPUs Feature Set     */
        if ( Cyrix_CPU_ADDR_cnt > 0 ) printf( Cyrix_CPU_ADDR );             /* CPUs Address Range   */
        if ( Cyrix_CPU_CACHE_cnt > 0 ) printf( Cyrix_CPU_CACHE );           /* CPUs Cache Details   */

    }
    else
    {
        printf( "ERROR: Couldn't allocate memory to parse CPU information .." );
        return;
    }
    FreeMem(global,sizeof(struct CPU_INTERN_DATA));
}
