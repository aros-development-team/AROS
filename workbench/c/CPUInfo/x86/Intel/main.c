/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Probe installed Intel CPUs and display relevant information
    Lang: english
*/

/* BIG TO DO - SEPERATE THE INDIVIDUAL PROCESSOR FAMILY "PROBES" INTO RUNTIME SHARED LIBS OR SIMILAR */

/****************************************************************************************************
     Currently Supports:

        i386 compatable families...
            Intel P5/P54C/P55C/P24T/P6/P2/P3/PM/Itanium(IA-64) 

*****************************************************************************************************/

#include "../x86.h"

/********************************************
		         Structures
 ********************************************/

#define i386_intel_MAXBRANDS 23
char *i386_intel_Brands[i386_intel_MAXBRANDS] = {
    "not supported",
    "0.18 µm Intel Celeron processor",
    "0.18 µm Intel Pentium III processor",
    "0.18 µm Intel Pentium III Xeon processor/0.13 µm Intel Celeron",
    "0.13 µm Intel Pentium III processor",
    "not supported",
    "not supported",
    "0.13 µm Intel Celeron mobile processor",
    "0.18 µm Intel Pentium 4 processor/0.13 µm Intel Celeron 4 mobile (0F24h)",
    "0.13 µm Intel Pentium 4 processor",
    "0.18 µm Intel Celeron 4 ",
    "0.18 µm Intel Pentium 4 Xeon MP/0.13 µm Intel Pentium 4 Xeon ",
    "0.13 µm Intel Pentium 4 Xeon MP",
    "not supported",
    "0.13 µm Intel Pentium 4 mobile (production)",
    "0.13 µm Intel Celeron 4 mobile (0F27h)/0.13 µm Intel Pentium 4 mobile (samples)",
    "not supported",
    "not supported",
    "not supported",    
    "not supported",
    "not supported",
    "not supported",
    "0.13 µm Intel Pentium M",
};

/********************************************
		     Intel CPU Features
 ********************************************/

char *Intel_feature_flags[] = {
    "FPU    : Floating Point Unit",
    "VME    : Virtual 8086 Mode Enhancements",
    "DE     : Debugging Extensions",
    "PSE    : Page Size Extensions",
    "TSC    : Time Stamp Counter",
    "MSR    : Model Specific Registers",
    "PAE    : Physical Address Extension",
    "MCE    : Machine Check Exception",
    "CX8    : COMPXCHG8B Instruction",
    "APIC   : On-chip Advanced Programmable Interrupt Controller present and enabled",
    "10     : Reserved",
    "SEP    : Fast System Call",
    "MTRR   : Memory Type Range Registers",
    "PGE    : PTE Global Flag",
    "MCA    : Machine Check Architecture",
    "CMOV   : Conditional Move and Compare Instructions",
    "FGPAT  : Page Attribute Table",
    "PSE-36 : 36-bit Page Size Extension",
    "PN     : Processor Serial Number present and enabled",
    "CLFSH  : CFLUSH instruction",
    "20     : reserved",
    "DS     : Debug store",
    "ACPI   : Thermal Monitor and Clock Ctrl",
    "MMX    : MMX instruction set",
    "FXSR   : Fast FP/MMX Streaming SIMD Extensions save/restore",
    "SSE    : Streaming SIMD Extensions instruction set",
    "SSE2   : SSE2 extensions",
    "SS     : Self Snoop",
    "HT     : Hyper Threading",
    "TM     : Thermal monitor",
    "30     : reserved",
    "31     : reserved",
};

/********************************************
        Intel specific information
 ********************************************/

/* Decode Intel TLB and cache info descriptors */

char    i386_intel_TLB_decode ( int tlb, char *BUFF_STR )
{
    tlb &= 0xff;
    if(tlb != 0)  sprintf( BUFF_STR,"%02x: ",tlb);

    sprintf( BUFF_STR,"        ");

    switch ( tlb )
    {
    case 0:
        break;
    case 0x1:
        sprintf( BUFF_STR,"Instruction TLB: 4KB pages, 4-way set assoc, 32 entries\n" );
        break;
    case 0x2:
        sprintf( BUFF_STR,"Instruction TLB: 4MB pages, 4-way set assoc, 2 entries\n" );
        break;
    case 0x3:
        sprintf( BUFF_STR,"Data TLB: 4KB pages, 4-way set assoc, 64 entries\n" );
        break;
    case 0x4:
        sprintf( BUFF_STR,"Data TLB: 4MB pages, 4-way set assoc, 8 entries\n" );
        break;
    case 0x6:
        sprintf( BUFF_STR,"1st-level instruction cache: 8KB, 4-way set assoc, 32 byte line size\n" );
        break;
    case 0x8:
        sprintf( BUFF_STR,"1st-level instruction cache: 16KB, 4-way set assoc, 32 byte line size\n" );
        break;
    case 0xa:
        sprintf( BUFF_STR,"1st-level data cache: 8KB, 2-way set assoc, 32 byte line size\n" );
        break;
    case 0xc:
        sprintf( BUFF_STR,"1st-level data cache: 16KB, 4-way set assoc, 32 byte line size\n" );
        break;


    case 0x40:
        sprintf( BUFF_STR,"No 2nd-level cache, or if 2nd-level cache exists, no 3rd-level cache\n" );
        break;
    case 0x41:
        sprintf( BUFF_STR,"2nd-level cache: 128KB, 4-way set assoc, 32 byte line size\n" );
        break;
    case 0x42:
        sprintf( BUFF_STR,"2nd-level cache: 256KB, 4-way set assoc, 32 byte line size\n" );
        break;
    case 0x43:
        sprintf( BUFF_STR,"2nd-level cache: 512KB, 4-way set assoc, 32 byte line size\n" );
        break;
    case 0x44:
        sprintf( BUFF_STR,"2nd-level cache: 1MB, 4-way set assoc, 32 byte line size\n" );
        break;
    case 0x45:
        sprintf( BUFF_STR,"2nd-level cache: 2MB, 4-way set assoc, 32 byte line size\n" );
        break;


    case 0x50:
        sprintf( BUFF_STR,"Instruction TLB: 4KB and 2MB or 4MB pages, 64 entries\n" );
        break;
    case 0x51:
        sprintf( BUFF_STR,"Instruction TLB: 4KB and 2MB or 4MB pages, 128 entries\n" );
        break;
    case 0x52:
        sprintf( BUFF_STR,"Instruction TLB: 4KB and 2MB or 4MB pages, 256 entries\n" );
        break;
    case 0x5b:
        sprintf( BUFF_STR,"Data TLB: 4KB and 4MB pages, 64 entries\n" );
        break;
    case 0x5c:
        sprintf( BUFF_STR,"Data TLB: 4KB and 4MB pages, 128 entries\n" );
        break;
    case 0x5d:
        sprintf( BUFF_STR,"Data TLB: 4KB and 4MB pages, 256 entries\n" );
        break;


    case 0x66:
        sprintf( BUFF_STR,"1st-level data cache: 8KB, 4-way set assoc, 64 byte line size\n" );
        break;
    case 0x67:
        sprintf( BUFF_STR,"1st-level data cache: 16KB, 4-way set assoc, 64 byte line size\n" );
        break;
    case 0x68:
        sprintf( BUFF_STR,"1st-level data cache: 32KB, 4-way set assoc, 64 byte line size\n" );
        break;


    case 0x70:
        sprintf( BUFF_STR,"Trace cache: 12K-micro-op, 4-way set assoc\n" );
        break;
    case 0x71:
        sprintf( BUFF_STR,"Trace cache: 16K-micro-op, 4-way set assoc\n" );
        break;
    case 0x72:
        sprintf( BUFF_STR,"Trace cache: 32K-micro-op, 4-way set assoc\n" );
        break;


    case 0x79:
        sprintf( BUFF_STR,"2nd-level cache: 128KB, 8-way set assoc, sectored, 64 byte line size\n" );
        break;
    case 0x7a:
        sprintf( BUFF_STR,"2nd-level cache: 256KB, 8-way set assoc, sectored, 64 byte line size\n" );    
        break;
    case 0x7b:
        sprintf( BUFF_STR,"2nd-level cache: 512KB, 8-way set assoc, sectored, 64 byte line size\n" );
        break;
    case 0x7c:
        sprintf( BUFF_STR,"2nd-level cache: 1MB, 8-way set assoc, sectored, 64 byte line size\n" );    
        break;


    case 0x82:
        sprintf( BUFF_STR,"2nd-level cache: 256KB, 8-way set assoc, 32 byte line size\n" );
        break;
    case 0x83:
        sprintf( BUFF_STR,"2nd-level cache: 512KB, 8-way set assoc 32 byte line size\n" );
        break;
    case 0x84:
        sprintf( BUFF_STR,"2nd-level cache: 1MB, 8-way set assoc, 32 byte line size\n" );
        break;
    case 0x85:
        sprintf( BUFF_STR,"2nd-level cache: 2MB, 8-way set assoc, 32 byte line size\n" );
        break;


    default:
        sprintf( BUFF_STR,"unknown TLB/cache descriptor\n" );
        break;
    }

    return BUFF_STR;
}

/********************************************/

void    parse_i386_Intel ( int maxi, struct i386_compat_intern * CPUi386 )
{
    struct  CPU_INTERN_DATA *global;
    ULONG                   speed, maxei,unused;
    int                     family = 0;
    char                    *BUFF_STR, *Intel_CPU_NAME, *Intel_CPU_IDENTITY, *Intel_CPU_FEATURES, *Intel_CPU_CACHE, *Intel_CPU_ADDR;
    int                     Intel_CPU_NAME_cnt, Intel_CPU_IDENTITY_cnt, Intel_CPU_FEATURES_cnt, Intel_CPU_CACHE_cnt, Intel_CPU_ADDR_cnt;

    if ((global = AllocMem(sizeof(struct CPU_INTERN_DATA),MEMF_PUBLIC|MEMF_CLEAR)))
    {
        Intel_CPU_NAME          = global->CPU_NAME;
        Intel_CPU_IDENTITY      = global->CPU_IDENTITY;
        Intel_CPU_FEATURES      = global->CPU_FEATURES;
        Intel_CPU_CACHE         = global->CPU_CACHE;
        Intel_CPU_ADDR          = global->CPU_ADDR;
        BUFF_STR                = global->CPU_BUFF;

        Intel_CPU_NAME_cnt = Intel_CPU_IDENTITY_cnt = Intel_CPU_FEATURES_cnt = Intel_CPU_CACHE_cnt = Intel_CPU_ADDR_cnt = 0;

        Intel_CPU_FEATURES_cnt = AddBufferLine( Intel_CPU_FEATURES_cnt, Intel_CPU_FEATURES, "    Intel-specific functions\n" );

        if ( maxi >= 1 )                                                                    /* Family/model/type etc */
        {
            int clf,apic_id,feature_flags;
            int extended_model = -1,extended_family = -1;
            unsigned long eax,ebx,edx,unused;
            int stepping,model,family,type,reserved,brand,siblings;
            int i;

            i386_cpuid( 1, eax, ebx, unused, edx );

            sprintf( BUFF_STR,"    Version %08lx:\n",eax);
            Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, BUFF_STR);

            stepping    = eax & 0xf;
            model       = (eax >> 4) & 0xf;
            family      = (eax >> 8) & 0xf;
            type        = (eax >> 12) & 0x3;
            reserved    = eax >> 14;

            clf         = (ebx >> 8) & 0xff;
            siblings    = (ebx >> 16) & 0xff;
            apic_id     = (ebx >> 24) & 0xff;

            feature_flags = edx;

            sprintf( BUFF_STR,"    Type [%d] - ",type );
            Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, BUFF_STR);
            switch ( type )
            {
            case 0:
                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Primary");
                break;
            case 1:
                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Overdrive");
                break;
            case 2:
                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Secondary");
                break;
            case 3:
                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Reserved");
                break;
            }

            sprintf( BUFF_STR," Family [%d] - ",family);
            Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, BUFF_STR);
            switch ( family )
            {
            case 3:
                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "i386" );
                break;
            case 4:
                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "i486" );
                break;
            case 5:
                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium" );
                break;
            case 6:
                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium Pro" );
                break;
            case 15:
                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium 4" );
            }
            Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "\n");

            if ( family == 15 )
            {
                extended_family = (eax >> 20) & 0xff;
                sprintf( BUFF_STR,"    Extended family %d",extended_family);
                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, BUFF_STR);

                switch ( extended_family )
                {
                case 0:
                    Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Intel P4" );
                    break;
                case 1:
                    Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Intel Itanium 2 (IA-64)" );
                    break;
                }
            }

            sprintf( BUFF_STR,"    Model %d - ",model);
            Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, BUFF_STR);
            switch ( family )
            {
            case 3:
                break;
            case 4:
                switch ( model )
                {
                case 0:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "486DX-25/33");
	                break;
                case 1:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "486DX-50");
	                break;
                case 2:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "486SX");
	                break;
                case 3:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "486DX2");
	                break;
                case 4:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "486SL");
	                break;
                case 5:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "486SX2");
	                break;
                case 7:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "486DX2-WB (write-back enhanced)");
	                break;
                case 8:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "486DX4");
	                break;
                case 9:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "486DX4-WB (write-back enhanced)");
	                break;
                }
            break;
            case 5:
                switch ( model )
                {
                case 0:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "P5 A-step");
	                break;
                case 1:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "P5");
	                break;
                case 2:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "P54C");
	                break;
                case 3:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "P24T Overdrive");
	                break;
                case 4:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "P55C");
	                break;
                case 7:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "P54C");
	                break;
                case 8:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "P55C (0.25µm)");
	                break;
                }
            break;
            case 6:
                switch ( model )
                {
                case 0:
                    Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium Pro (P6 A-step)");
	                break;
                case 1:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium Pro (P6)");
	                break;
                case 3:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium II Model 3 (0.28 µm)");
	                break;
                case 5:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium II Model 5/Xeon/Celeron (0.25 µm)");
	                break;
                case 6:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Celeron (PII with on-die L2 cache)");
	                break;
                case 7:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium III/Pentium III Xeon external L2 cache (0.25 µm)");
	                break;
                case 8:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium III/Pentium III Xeon with 256 KB on-die L2 cache (0.18 µm)");
	                break;
                case 9:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "PM - with 1 MB on-die L2 cache  (0.13 µm)");
	                break;
                case 10:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium III with 1 or 2 MB on-die L2 cache (0.18 µm)");
	                break;
                case 11:
	                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium III with 256 or 512 KB on-die L2 cache (0.13 µm)");
	                break;
                }
            break;
		    case 15:
			    switch ( model )
                {
			    case 0:
			    case 1:			
                    Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium 4/Celeron (0.18)");
				    break;
			    case 2:
                    Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium 4/Celeron (0.13)");
                    break;
			    case 3:
                    Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "Pentium 4 (0.09)");
                    break;
			    }
	            break;

            }
            Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "\n");

            if ( model == 15 )
            {
                extended_model = (eax >> 16) & 0xf;
                sprintf( BUFF_STR,"    Extended model %d\n",extended_model);
                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, BUFF_STR);
            }

            sprintf( BUFF_STR,"    Stepping %d\n",stepping);
            Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, BUFF_STR);

            sprintf( BUFF_STR,"    Reserved %d\n\n",reserved);
            Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, BUFF_STR);

            brand = ebx & 0xff;
            if ( brand > 0 )
            {
                sprintf( BUFF_STR,"    Brand index: %d [",brand);
                Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, BUFF_STR);
                if (brand  < i386_intel_MAXBRANDS)
                {
                    sprintf( BUFF_STR,"%s]\n",i386_intel_Brands[brand]);
                    Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, BUFF_STR);
                }
                else Intel_CPU_IDENTITY_cnt = AddBufferLine(Intel_CPU_IDENTITY_cnt, Intel_CPU_IDENTITY, "not in table]\n");
            }

            i386_cpuid( 0x80000000, eax, ebx, unused, edx );
            if ( eax & 0x80000000 )                                                            /* Extended feature/signature bits supported */
            {
                int maxe = eax;
                if( maxe >= 0x80000004 )
                {
	                int i;

	                Intel_CPU_NAME_cnt = AddBufferLine(Intel_CPU_NAME_cnt, Intel_CPU_NAME, "    Extended brand string: \"" );
	                for( i=0x80000002; i<=0x80000004 ;i++ )
                    {
	                    unsigned long eax,ebx,ecx,edx;

	                    i386_cpuid(i,eax,ebx,ecx,edx);	
	                    Intel_CPU_NAME_cnt = i386_sprintregs(Intel_CPU_NAME_cnt, Intel_CPU_NAME, eax,ebx,ecx,edx);
	                }
	                Intel_CPU_NAME_cnt = AddBufferLine(Intel_CPU_NAME_cnt, Intel_CPU_NAME, "\"\n");
                }
            }
            if ( clf )
            {
                sprintf( BUFF_STR,"    CLFLUSH instruction cache line size: %d\n", clf );
                Intel_CPU_FEATURES_cnt = AddBufferLine(Intel_CPU_FEATURES_cnt, Intel_CPU_FEATURES, BUFF_STR);
            }
            
            if ( apic_id ) 
            {
                sprintf( BUFF_STR,"    Initial APIC ID: %d\n", apic_id );
                Intel_CPU_FEATURES_cnt = AddBufferLine(Intel_CPU_FEATURES_cnt, Intel_CPU_FEATURES, BUFF_STR);
            }
            
            if ( feature_flags & (1<<28) )
            {
                sprintf( BUFF_STR,"    Hyper threading siblings: %d\n", siblings );
                Intel_CPU_FEATURES_cnt = AddBufferLine(Intel_CPU_FEATURES_cnt, Intel_CPU_FEATURES, BUFF_STR);
            }

            sprintf( BUFF_STR,"\n    Feature flags [%08x]:\n", feature_flags );
            Intel_CPU_FEATURES_cnt = AddBufferLine(Intel_CPU_FEATURES_cnt, Intel_CPU_FEATURES, BUFF_STR);
            for(i=0;i<32;i++)
            {
                if( feature_flags & (1<<i) )
                {
                    sprintf( BUFF_STR,"        %s\n", Intel_feature_flags[i] );
                    Intel_CPU_FEATURES_cnt = AddBufferLine(Intel_CPU_FEATURES_cnt, Intel_CPU_FEATURES, BUFF_STR);
                }
            }
            Intel_CPU_FEATURES_cnt = AddBufferLine(Intel_CPU_FEATURES_cnt, Intel_CPU_FEATURES, "\n");
        }

        if ( maxi >= 2 )                                                                /* Decode TLB and cache info */
        {
            int ntlb,i;

            ntlb = 255;
            Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, "    TLB and cache info:\n");
            for( i=0; i<ntlb ;i++ )
            {
                unsigned long eax,ebx,ecx,edx;

                i386_cpuid(2,eax,ebx,ecx,edx);
                ntlb =  eax & 0xff;
                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( eax >> 8, BUFF_STR ));
                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( eax >> 16, BUFF_STR ));
                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( eax >> 24, BUFF_STR ));
                  
                if (( ebx & 0x80000000 ) == 0 )
                {
	                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( ebx, BUFF_STR ));
	                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( ebx >> 8, BUFF_STR ));
	                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( ebx >> 16, BUFF_STR ));
	                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( ebx >> 24, BUFF_STR ));
                }
                if (( ecx & 0x80000000 ) == 0 )
                {
	                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( ecx, BUFF_STR ));
	                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( ecx >> 8, BUFF_STR ));
	                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( ecx >> 16, BUFF_STR ));
	                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( ecx >> 24, BUFF_STR ));
                }
                if (( edx & 0x80000000 ) == 0 )
                {
	                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( edx, BUFF_STR ));
	                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( edx >> 8, BUFF_STR ));
	                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( edx >> 16, BUFF_STR ));
	                Intel_CPU_CACHE_cnt = AddBufferLine(Intel_CPU_CACHE_cnt, Intel_CPU_CACHE, i386_intel_TLB_decode( edx >> 24, BUFF_STR ));
                }
            }
        }

        if ( maxi >= 3 )                                                                   /* Pentium III CPU serial number */
        {
            unsigned long signature,unused,ecx,edx;

            i386_cpuid(1,signature,unused,unused,unused);
            i386_cpuid(3,unused,unused,ecx,edx);
            
            Intel_CPU_NAME_cnt = AddBufferLine(Intel_CPU_NAME_cnt, Intel_CPU_NAME, "    Processor serial: ");

            sprintf( BUFF_STR,"%04lX",signature >> 16);
            Intel_CPU_NAME_cnt = AddBufferLine(Intel_CPU_NAME_cnt, Intel_CPU_NAME, BUFF_STR);
            sprintf( BUFF_STR,"-%04lX",signature & 0xffff);
            Intel_CPU_NAME_cnt = AddBufferLine(Intel_CPU_NAME_cnt, Intel_CPU_NAME, BUFF_STR);
            sprintf( BUFF_STR,"-%04lX",edx >> 16);
            Intel_CPU_NAME_cnt = AddBufferLine(Intel_CPU_NAME_cnt, Intel_CPU_NAME, BUFF_STR);
            sprintf( BUFF_STR,"-%04lX",edx & 0xffff);
            Intel_CPU_NAME_cnt = AddBufferLine(Intel_CPU_NAME_cnt, Intel_CPU_NAME, BUFF_STR);
            sprintf( BUFF_STR,"-%04lX",ecx >> 16);
            Intel_CPU_NAME_cnt = AddBufferLine(Intel_CPU_NAME_cnt, Intel_CPU_NAME, BUFF_STR);
            sprintf( BUFF_STR,"-%04lX\n",ecx & 0xffff);
            Intel_CPU_NAME_cnt = AddBufferLine(Intel_CPU_NAME_cnt, Intel_CPU_NAME, BUFF_STR);
        }

    /** DUMP ALL THE INFORMATION WE HAVE GATHERED **/

        if ( Intel_CPU_NAME_cnt > 0 ) printf( Intel_CPU_NAME );             /* CPUs Retail Name     */

        speed = i386_approx_mhz();

        /* TODO : Calculate FSB ..     */

        if ( Intel_CPU_IDENTITY_cnt > 0 ) printf( Intel_CPU_IDENTITY );     /* CPUs Internal Name   */
        if ( Intel_CPU_FEATURES_cnt > 0 ) printf( Intel_CPU_FEATURES );     /* CPUs Feature Set     */
        if ( Intel_CPU_ADDR_cnt > 0 ) printf( Intel_CPU_ADDR );             /* CPUs Address Range   */
        if ( Intel_CPU_CACHE_cnt > 0 ) printf( Intel_CPU_CACHE );           /* CPUs Cache Details   */
    }
    else
    {
        printf( "ERROR: Couldn't allocate memory to parse CPU information .." );
        return;
    }
    FreeMem(global,sizeof(struct CPU_INTERN_DATA));
}
