/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Probe installed AMD CPUs and display relevant information
    Lang: english
*/

/* BIG TO DO - SEPERATE THE INDIVIDUAL PROCESSOR FAMILY "PROBES" INTO RUNTIME SHARED LIBS OR SIMILAR */

/****************************************************************************************************
     Currently Supports:

        i386 compatable families...
            AMD 486/5x86/K5/K6/K6-II/K6-III/Athlon/Duron/Opteron/Athlon64

*****************************************************************************************************/

#include "../x86.h"

/********************************************
		         Structures
 ********************************************/

#define i386_amd_MAXBRANDS 7
char *i386_amd_Brands[i386_amd_MAXBRANDS] = {
    "engineering sample NN (NN=xxxxxb)",
    "AMD Athlon 64 XX00+ (XX=22+xxxxxb)",
    "AMD Athlon 64 XX00+ (XX=22+xxxxxb) mobile",
    "AMD Opteron UP 1YY (YY=38+2*xxxxxb)",
    "AMD Opteron DP 2YY (YY=38+2*xxxxxb)",
    "AMD Opteron MP 8YY (YY=38+2*xxxxxb)",
    "AMD Athlon 64 FX-ZZ (ZZ=24+xxxxxb)",
};

/********************************************
		      AMD CPU Features
 ********************************************/

char *AMD_feature_flags[] = {
    "FPU    : Floating Point Unit",
    "VME    : Virtual Mode Extensions",
    "DE     : Debugging Extensions",
    "PSE    : Page Size Extensions",
    "TSC    : Time Stamp Counter (with RDTSC and CR4 disable bit)",
    "MSR    : Model Specific Registers with i386_rdmsr & WRMSR",
    "PAE    : Page Address Extensions",
    "MCE    : Machine Check Exception",
    "CX8    : COMPXCHG8B Instruction",
    "APIC   : On-chip Advanced Programmable Interrupt Controller present and enabled",
    "10     : Reserved",
    "SEP    : SYSCALL/SYSRET or SYSENTER/SYSEXIT instructions",
    "MTRR   : Memory Type Range Registers",
    "PGE    : Global paging extension",
    "MCA    : Machine Check Architecture",
    "CMOV   : Conditional Move Instruction",
    "PAT    : Page Attribute Table",
    "PSE-36 : Page Size Extensions",
    "18     : Reserved",
    "19     : Reserved",
    "20     : Reserved",
    "21     : Reserved",
    "22     : AMD MMX Instruction Extensions",
    "MMX    : MMX instructions",
    "FXSR   : FXSAVE/FXRSTOR",
    "25     : Reserved",
    "26     : Reserved",
    "27     : Reserved",
    "28     : Reserved",
    "29     : Reserved",
    "30     : 3DNow! Instruction Extensions",
    "31     : 3DNow instructions",
};

char *Assoc[] = {
    "L2 off",
    "Direct mapped",
    "2-way",
    "Reserved",
    "4-way",
    "Reserved",
    "8-way",
    "Reserved",
    "16-way",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "FULL",
};

/********************************************
           Data for FSB detection
 ********************************************/

int     amd64cm[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x4, 0x6, 0xB, 0xA, 0xC, 0xE, 0x10, 0x12, 0x14};
float   athloncoef[] = {11, 11.5, 12.0, 12.5, 5.0, 5.5, 6.0, 6.5, 7, 7.5, 8, 8.5, 9, 9.5, 10, 10.5};
float   athloncoef2[] = {12, 19.0, 12, 20.0, 13.0, 13.5, 14.0, 21.0, 15, 22, 16, 16.5, 17, 18.0, 23, 24};

/********************************************
        AMD-specific information
 ********************************************/

void    i386_poll_AMD32_FSB ( ULONG extclock, struct i386_compat_intern * CPUi386 )
{
	unsigned int                mcgsrl;
	unsigned int                mcgsth;
	unsigned long               temp;
	double                      dramclock;
	float                       coef;

    //if (CPUi386->x86_capability & X86_FEATURE_TSC)
    //{
	//    coef = 10;

	//    i386_rdmsr(MSR_K7_HWCR, mcgsrl, mcgsth);                            /* First, got the FID */
	//    temp = (mcgsrl >> 24)&0x0F;

	//    if ((mcgsrl >> 19)&1) { coef = athloncoef2[temp]; }
	//    else { coef = athloncoef[temp]; }

	//    if (coef == 0) { coef = 1; };
    	
	//    dramclock = (extclock /1000) / coef;                                /* Compute the final FSB Clock */

	    /* ...and print */
     //   printf("    Settings: FSB : %d Mhz (DDR) %d ", dramclock, (dramclock*2));
    //}
    //else    printf("\n WARNING! CPU doesnt support RDMSR - could not obtain FSB speed.\n");
}

/********************************************/

BOOL    i386_AMD_isMobile (  int maxi )
{
	unsigned long eax, ebx, ecx, edx;

    if ( maxi >= 0x80000007 )
    {
		i386_cpuid( 0x80000007, eax, ebx, ecx, edx );
		if ((edx & (1<<1|1<<2)) == 0) return FALSE;
		else return TRUE;
	}
    else return FALSE;
}

/********************************************/

void    i386_AMD_Athlon_Parse_MSR ( void )
{
	printf("\t\t\t\t31       23       15       7 \n");
	i386_Parse_MSR( 0x2A, 32 );
	i386_Parse_MSR( MSR_K6_EFER, 32 );
	i386_Parse_MSR( 0xC0010010, 32 );
	i386_Parse_MSR( MSR_K7_HWCR, 32 );
	i386_Parse_MSR( 0xC001001B, 32 );
	printf ("\n");
}

/********************************************/

void    i386_AMD_K6_Parse_MSR ( int family, int model, int stepping )
{
	unsigned long msrvala=0,msrvalb=0;

	printf("\t\t\t\t31       23       15       7 \n");
	i386_Parse_MSR( MSR_K6_WHCR, 32 );
	
	if ( ( model < 8 ) || ( ( model==8 ) && ( stepping <8 ) ) )               /* Original K6 or K6-2 (old core). */
    {
        i386_rdmsr( MSR_K6_WHCR, msrvala, msrvalb )
		if ( ( ( msrvalb << 32 ) | msrvala ) == 1 ) 
        {
			printf ("Write allocate enable limit: %dMbytes\n", (int) ( (  ( ( msrvalb << 32 ) | msrvala ) & 0x7e ) >>1 ) * 4 );
			printf ("Write allocate 15-16M bytes: %s\n",  ( ( msrvalb << 32 ) | msrvala ) & 1 ? "enabled" : "disabled" );
		}
        else printf ("Couldn't read WHCR register.\n");
	}

	
	if ( ( model > 8 ) || ( ( model == 8 ) && ( stepping >= 8 ) ) )               /* K6-2 core (Stepping 8-F), K6-III or later. */
    {   
        i386_rdmsr( MSR_K6_WHCR, msrvala, msrvalb );
		if ( ( ( msrvalb << 32 ) | msrvala ) == 1)
        {
			if (!( ( ( msrvalb << 32 ) | msrvala ) & ( 0x3ff << 22 ) ) )	printf ("    Write allocate disabled\n");
			else 
            {
				printf ("    Write allocate enable limit: %dMbytes\n", (int) ( (  ( ( msrvalb << 32 ) | msrvala ) >> 22) & 0x3ff ) * 4 );
				printf ("    Write allocate 15-16M bytes: %s\n",  ( ( msrvalb << 32 ) | msrvala ) & (1<<16) ? "enabled" : "disabled" );
			}
		}
        else printf ("  Couldn't read WHCR register.\n");
	}

	
	if ( ( family == 5 ) && ( model >= 8 ) )                         /* Dump EWBE register on K6-2 & K6-3 */
    {
        i386_rdmsr( MSR_K6_EFER, msrvala, msrvalb );
		if ( ( ( msrvalb << 32 ) | msrvala ) == 1 )
        {
			if ( ( ( msrvalb << 32 ) | msrvala ) & ( 1 << 0 ) ) printf ("System call extension present.\n");
			if ( ( ( msrvalb << 32 ) | msrvala ) & ( 1 << 1 ) ) printf ("Data prefetch enabled.\n");
			else printf ("    Data prefetch disabled.\n");

			printf ("  EWBE mode: ");
			switch ( ( ( ( msrvalb << 32 ) | msrvala ) & ( 1 << 2 | 1 << 3 | 1 << 4 ) ) >> 2 ) 
            {
				case 0:	printf ("    strong ordering (slowest performance)\n");
					break;
				case 1:	printf ("    speculative disable (close to best performance)\n");
					break;
				case 2:	printf ("    invalid\n");
					break;
				case 3:	printf ("    global disable (best performance)\n");
					break;
			}
		}
        else printf ("  Couldn't read EFER register.\n");
	}

	printf ("\n");
}

/********************************************/
/*

/********************************************/

void    parse_i386_AMD ( int maxi, struct i386_compat_intern * CPUi386 )
{
    struct  CPU_INTERN_DATA *global;
    ULONG                   speed, maxei,unused,connection;
    char                    *BUFF_STR, *AMD_CPU_NAME, *AMD_CPU_IDENTITY, *AMD_CPU_FEATURES, *AMD_CPU_CACHE, *AMD_CPU_ADDR;
    int                     AMD_CPU_NAME_cnt, AMD_CPU_IDENTITY_cnt, AMD_CPU_FEATURES_cnt, AMD_CPU_CACHE_cnt, AMD_CPU_ADDR_cnt;
    int                     stepping,model,reserved;
    int                     family = 0;

    if ( ( global = AllocMem( sizeof( struct CPU_INTERN_DATA ), MEMF_PUBLIC|MEMF_CLEAR ) ) )
    {
        AMD_CPU_NAME           = global->CPU_NAME;
        AMD_CPU_IDENTITY       = global->CPU_IDENTITY;
        AMD_CPU_FEATURES       = global->CPU_FEATURES;
        AMD_CPU_CACHE          = global->CPU_CACHE;
        AMD_CPU_ADDR           = global->CPU_ADDR;
        BUFF_STR               = global->CPU_BUFF;
        
        AMD_CPU_NAME_cnt = AMD_CPU_IDENTITY_cnt = AMD_CPU_FEATURES_cnt = AMD_CPU_CACHE_cnt = AMD_CPU_ADDR_cnt = 0;

        AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "    AMD-specific functions\n");
        
        if ( maxi >= 1 )    /* Do standard stuff */
        {
            unsigned long   eax,ebx,edx,unused,extended_family;

            i386_cpuid( 1, eax, ebx, unused, edx);

            stepping    = eax & 0xf;
            model       = ( eax >> 4 ) & 0xf;
            family      = ( eax >> 8 ) & 0xf;
            reserved    = eax >> 12;

            if ( family == 15 )
            {
                extended_family = (eax >> 20) & 0xff;
                sprintf( BUFF_STR,"    Extended family %d",extended_family);
                AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, BUFF_STR);

                switch ( extended_family )
                {
                case 0:
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "AMD K8" );
                    break;
                }
            }


            sprintf( BUFF_STR,"    Version %08lx:\n",eax);
            AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, BUFF_STR );
            sprintf( BUFF_STR,"    Family: %d Model: %d [",family,model);
            AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, BUFF_STR );
            switch(family)
            {
            case 4:
                connection = CONN_SOCKET_3;
                switch ( model )
                {
			    case 3:
				    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "486DX2" );
				    break;
			    case 7:
				    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "486DX2-WB" );
				    break;
			    case 8:
				    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "486DX4");
				    break;
			    case 9:
				    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "486DX4-WB");
				    break;
			    case 10:
				    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Elan SC400");
				    break;
			    case 14:
				    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "5x86");
				    break;
			    case 15:
				    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "5x86-WB");
				    break;
                }
                break;
            case 5:
                connection = CONN_SOCKET_7;
                switch ( model )
                {
                case 0:
	                connection = CONN_SOCKET_5_7;
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "SSA5 (PR75/90/100)");
	                break;
                case 1:
	                connection = CONN_SOCKET_5_7;
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "5k86 (PR120/133)");
	                break;
                case 2:
	                connection = CONN_SOCKET_5_7;
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "5k86 (PR166)");
	                break;
                case 3:
	                connection = CONN_SOCKET_5_7;
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "5k86 (PR200)");
	                break;
                case 6:
	                AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "K6 Model 6 (0.30 µm)");
	                break;
                case 7:
	                AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "K6 Model 7 (0.25 µm)");
	                break;
                case 8:
	                connection = CONN_SUPER_SOCKET_7;
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "K6-2");
	                break;
                case 9:
	                connection = CONN_SUPER_SOCKET_7;
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "K6-III");
	                break;
                case 11:
	                connection = CONN_SUPER_SOCKET_7;
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "K6-2+ (0.18 µm)");
	                break;
                case 13:
	                connection = CONN_SUPER_SOCKET_7;
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "K6-2+ or K6-III+ (0.18 µm)");
	                break;
                default:
                    sprintf( BUFF_STR,"K5/K6 model %d",model);
	                AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, BUFF_STR);
	                break;
                }
                break;
            case 6:
                connection = CONN_SOCKET_A;
                if ( ( model > 4) && ( i386_AMD_isMobile( maxi ) == TRUE ) ) AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Mobile ");
                switch ( model )
                {
                case 1:
                    connection = CONN_SLOT_A;
	                switch ( CPUi386->x86 )
                    {
                    case 1:
                        AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Athlon Model [C1] (0.25 µm)");
                        break;
                    case 2:
                        AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Athlon Model [C2] (0.25 µm)");
                        break;
                    }
                    break;
                case 2:
                    connection = CONN_SLOT_A;
	                switch ( CPUi386->x86 )
                    {
                    case 1:
                        AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Athlon Model [A1] (0.18 µm)");
                        break;
                    case 2:
                        AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Athlon Model [A2] (0.18 µm)");
                        break;
                    }
                    break;
                case 3:
	                switch ( CPUi386->x86 )
                    {
                    case 0:
                        AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Duron Model [A0] (0.18 µm)(SF Core)");
                        break;
                    case 1:
                        AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Duron Model [A2] (0.18 µm)(SF Core)");
                        break;
                    }
	                break;
                case 4:
	                switch ( CPUi386->x86 )
                    {
                    case 0:
                        AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Athlon Model [A1] (0.18 µm) (TB Core)");
                        break;
                    case 1:
                        AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Athlon Model [A2] (0.18 µm) (TB Core)");
                        break;
                    case 2:
                        AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Athlon Model [A4-A8] (0.18 µm) (TB Core)");
                        break;
                    case 3:
                        AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Athlon Model [A9] (0.18 µm) (TB Core)");
                        break;
                    }
	                break;
                case 6:
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Athlon MP/Mobile Athlon Model 6 (0.18 µm) (PM Core)");
	                break;
                case 7:
                    switch ( CPUi386->x86 )
                    {
                    case 0:
                        AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Duron Model [A0] (MG Core)");
                        break;
                    case 1:
                        AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Duron Model [A1] (MG Core)");
                        break;
                    }
	                break;
                case 8:
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Athlon Model 8 (TH/AP Core)");
	                break;
                case 10:
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Athlon Model 10 (BT Core)");
	                break;
                default:
                    sprintf( BUFF_STR,"Duron/Athlon model %d",model);
	                AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, BUFF_STR);
	                break;
                }
                break;
            case 15:
                switch ( model )
                {
                case 4:
	                connection = CONN_SOCKET_754;
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Athlon 64 (0.13 µm)");
	                break;
                case 5:
	                connection = CONN_SOCKET_754;
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Athlon 64 FX or Opteron (0.13 µm)");
	                break;
                default:
                    AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Unknown 64bit Model!");
                    break;
                }
                break;

            default:
                AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "Unknown Model!");
                break;
            }

            AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, "]\n");
            {
                int i;

                sprintf( BUFF_STR,"    Standard feature flags [%08lx]:\n",edx);
                AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, BUFF_STR);
                for(i=0;i<32;i++)
                {
	                if(family == 5 && model == 0)
                    {
	                    if(i == 9) AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "        Global Paging Extensions\n");
                        else if(i == 13) AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "13 - reserved\n");
                    }
                    else if(edx & (1<<i)) 
                    {
                        sprintf( BUFF_STR,"        %s\n",AMD_feature_flags[i]);
                        AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, BUFF_STR);
                    }
                }
            }
            AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "\n");
        }
        
        
        i386_cpuid( 0x80000000, maxei, unused, unused, unused );                /* Check for presence of extended info */

        if ( maxei == 0 ) return;

        int stepping,model,generation,reserved;

        if ( maxei >= 0x80000001 )
        {
            unsigned long eax,ebx,ecx,edx;
            int i;

            i386_cpuid(0x80000001,eax,ebx,ecx,edx);
            stepping    = eax & 0xf;
            model       = (eax >> 4) & 0xf;
            generation  = (eax >> 8) & 0xf;
            reserved    = eax >> 12;

            sprintf( BUFF_STR,"    Generation: %d Model: %d\n\n",generation,model);
            AMD_CPU_IDENTITY_cnt = AddBufferLine( AMD_CPU_IDENTITY_cnt, AMD_CPU_IDENTITY, BUFF_STR);
            sprintf( BUFF_STR,"    Extended feature flags [%08lx]:\n",edx);
            AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, BUFF_STR);
            for(i=0;i<32;i++)
            {
                if(family == 5 && model == 0 && i == 9) AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "        Global Paging Extensions\n");
                else if(edx & (1<<i))
                {
                    sprintf( BUFF_STR,"        %s\n",AMD_feature_flags[i]);
                    AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, BUFF_STR);
                }
            }
        }

        AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "\n");
        if ( maxei >= 0x80000002 )                 /* Processor identification string */
        {
            int j;
            AMD_CPU_NAME_cnt = AddBufferLine( AMD_CPU_NAME_cnt, AMD_CPU_NAME, "    Processor name string: ");
            for(j=0x80000002;j<=0x80000004;j++)
            {
                unsigned long eax,ebx,ecx,edx;

                i386_cpuid(j,eax,ebx,ecx,edx);
                AMD_CPU_NAME_cnt = i386_sprintregs(AMD_CPU_NAME_cnt, AMD_CPU_NAME, eax,ebx,ecx,edx);
            }
            AMD_CPU_NAME_cnt = AddBufferLine( AMD_CPU_NAME_cnt, AMD_CPU_NAME, "\n");
        }

        if ( maxei >= 0x80000005 )                 /* TLB and cache info */
        {
            
            unsigned long eax,ebx,ecx,edx;

            i386_cpuid(0x80000005,eax,ebx,ecx,edx);
            AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, "    L1 Cache Information:\n");

            if(family >= 6)
            {
                AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, "    2/4-MB Pages:\n");
                sprintf( BUFF_STR,"       Data TLB: associativity %ld-way #entries %ld\n", (eax >> 24) & 0xff,(eax >> 16) & 0xff);
                AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, BUFF_STR);
                sprintf( BUFF_STR,"       Instruction TLB: associativity %ld-way #entries %ld\n", (eax >> 8) & 0xff,eax & 0xff);
                AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, BUFF_STR);
            }

            AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, "    4-KB Pages:\n");    
            sprintf( BUFF_STR,"       Data TLB: associativity %ld-way #entries %ld\n", (ebx >> 24) & 0xff,(ebx >> 16) & 0xff);
            AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, BUFF_STR);
            sprintf( BUFF_STR,"       Instruction TLB: associativity %ld-way #entries %ld\n", (ebx >> 8) & 0xff,ebx & 0xff);
            AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, BUFF_STR);

            AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, "    L1 Data cache:\n");
            sprintf( BUFF_STR,"       size %ld KB associativity %lx-way lines per tag %ld line size %ld\n", ecx >> 24,(ecx>>16) & 0xff,(ecx >> 8)&0xff,ecx&0xff);
            AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, BUFF_STR);

            AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, "    L1 Instruction cache:\n");
            sprintf( BUFF_STR,"       size %ld KB associativity %lx-way lines per tag %ld line size %ld\n\n", edx >> 24,(edx>>16) & 0xff,(edx >> 8)&0xff,edx&0xff);
            AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, BUFF_STR);
        }
          
        if ( maxei >= 0x80000006 )                /* check K6-III (and later?) on-chip L2 cache size */
        {
            unsigned long eax,ebx,ecx,unused;
            int assoc;

            i386_cpuid(0x80000006,eax,ebx,ecx,unused);
            AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, "    L2 Cache Information:\n");

            if(family >= 6)
            {
                AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, "    2/4-MB Pages:\n");
                assoc = (eax >> 24) & 0xff;

                if(assoc == 6) assoc = 8;

                sprintf( BUFF_STR,"       Data TLB: associativity %s #entries %ld\n", Assoc[(eax >> 24) & 0xf],(eax >> 16) & 0xff);
                AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, BUFF_STR);
                assoc = (eax >> 16) & 0xff;
                sprintf( BUFF_STR,"       Instruction TLB: associativity %s #entries %ld\n", Assoc[(eax >> 8) & 0xf],eax & 0xff);
                AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, BUFF_STR);
                  
                AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, "    4-KB Pages:\n");    
                sprintf( BUFF_STR,"       Data TLB: associativity %s #entries %ld\n", Assoc[(ebx >> 24) & 0xf],(ebx >> 16) & 0xff);
                AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, BUFF_STR);
                sprintf( BUFF_STR,"       Instruction TLB: associativity %s #entries %ld\n", Assoc[(ebx >> 8) & 0xf],ebx & 0xff);
                AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, BUFF_STR);
            }
            sprintf( BUFF_STR,"    size %ld KB associativity %s lines per tag %ld line size %ld\n", ecx >> 24,Assoc[(ecx>>16) & 0xf],(ecx >> 8)&0xff,ecx&0xff);
            AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, BUFF_STR);

            AMD_CPU_CACHE_cnt = AddBufferLine( AMD_CPU_CACHE_cnt, AMD_CPU_CACHE, "\n");
        }

        if ( maxei >= 0x80000007 )                 /* Check power management feature flags */
        {
            unsigned long unused,edx;

            AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "    Advanced Power Management (PowerNOW!) Feature Flags\n");

            i386_cpuid(0x80000007,unused,unused,unused,edx);
            if(edx & 1) AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "        Temperature Sensing Diode\n");
            if(edx & 2) AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "        Supports Frequency ID control\n");
            if(edx & 4) AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "        Supports Voltage ID control\n");
            if(edx & 8) AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "        Supports Thermal Trip\n");
            if(edx & 16) AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "        Supports Thermal Monitoring\n");
            if(edx & 31) AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "        Supports Software Thermal Control\n");
            AMD_CPU_FEATURES_cnt = AddBufferLine( AMD_CPU_FEATURES_cnt, AMD_CPU_FEATURES, "\n");
        }

        if ( maxei >= 0x80000008 )                 /* Check phys address & linear address size */
        {
            unsigned long unused,eax;

            i386_cpuid(0x80000008,eax,unused,unused,unused);
            sprintf( BUFF_STR,"    Maximum LINEAR address: %ld; Maximum PHYSICAL address %ld\n\n", (eax>>8) & 0xff,eax&0xff);
            AMD_CPU_ADDR_cnt = AddBufferLine( AMD_CPU_ADDR_cnt, AMD_CPU_ADDR, BUFF_STR);
        }

    /** DUMP ALL THE INFORMATION WE HAVE GATHERED **/

        if ( AMD_CPU_NAME_cnt > 0 ) printf( AMD_CPU_NAME );             /* CPUs Retail Name     */

        speed = i386_approx_mhz();

        i386_poll_AMD32_FSB( speed, CPUi386 );                          /* Calculate FSB ..     */


        if ( AMD_CPU_IDENTITY_cnt > 0 ) printf( AMD_CPU_IDENTITY );     /* CPUs Internal Name   */
        if ( AMD_CPU_FEATURES_cnt > 0 ) printf( AMD_CPU_FEATURES );     /* CPUs Feature Set     */
        if ( AMD_CPU_ADDR_cnt > 0 ) printf( AMD_CPU_ADDR );             /* CPUs Address Range   */
        if ( AMD_CPU_CACHE_cnt > 0 ) printf( AMD_CPU_CACHE );           /* CPUs Cache Details   */

        /*if ( family == 5 ) i386_AMD_K6_Parse_MSR( family, model, stepping );
		if ( family == 6 ) i386_AMD_Athlon_Parse_MSR(); */
    }
    else
    {
        printf( "ERROR: Couldn't allocate memory to parse CPU information .." );
        return;
    }
    FreeMem(global,sizeof(struct CPU_INTERN_DATA));
}



