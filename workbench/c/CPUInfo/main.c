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

     Soon....

        PPC?

*****************************************************************************************************/

#include "cpuinfo.h"

/********************************************
		    Version Information
 ********************************************/

static const char version[] = VERSTAG;
BOOL    VERBOSE=FALSE;

/********************************************
		       C Functions
 ********************************************/

BOOL    isLastNode ( struct MinNode *CurrNode )
{
    struct      MinNode             *NextNode;

    if ( CurrNode->mln_Succ == NULL )    return TRUE;

    NextNode = ( struct MinNode *)CurrNode->mln_Succ;
    if ( NextNode->mln_Succ != NULL )    return FALSE;

    return  TRUE;                                                                   /* yes we are the last .. */
}

/********************************************/

int     AddBufferLine ( int buffpos, char *buffer, char *line )
{
	ULONG                   size = strlen( line );

    sprintf ( buffer + buffpos, line );
    return  ( buffpos += size );

}

/********************************************
		Actual Code... Main Entry
 ********************************************/

int     main ( void )
{
    struct      CPUBase             *CPUResBase;
    struct      CPU_Definition      *CPUList, *FoundCPUs;
    struct      RDArgs              *rda;
    struct      MinNode             *CPUNode;
    int                             cpu_count;
    int                             result;
    int                             error = RETURN_OK;
    IPTR                            args[NOOFARGS] = { (IPTR)FALSE, };
    ULONG  buffers = 0;

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);

    if (rda != NULL)
    {
        VERBOSE = (BOOL)args[ARG_VERBOSE];
        FreeArgs(rda);
    }

    printf( APPNAME " - CPU Information tool v" VERSSTRING ".\n" );
    printf( "© Copyright the AROS Dev Team.\n");
    printf( "-------------------------------------\n\n" );

    if (VERBOSE) printf( " Performing VERBOSE Probe...\n\n" );

    if ( CPUResBase = OpenResource( "cpu.resource" ) )
    {
        cpu_count = 0;  /* Initialise the list counter */
        CPUList = (struct CPU_Definition *)CPUResBase->CPUB_Processors;
        FoundCPUs = (struct CPU_Definition *)CPUList->CPU_CPUList.mlh_Head;

        printf( "CPUBase->CPUB_Processors contains CPU list @ %p\n\n", CPUList );
        printf( "Found CPU(s) @ %p\n\n", FoundCPUs );

        if ( CPUList->CPU_Physical == 1) printf("1 Processor is " );
        else printf( "%u Processor's are ", CPUList->CPU_Physical );
        printf( "present in this system.\n\n" );

    /* . Main loop . */
        CPUNode = (struct MinNode *)&CPUList->CPU_CPUList.mlh_Head;
        while ( isLastNode( CPUNode ) == FALSE )
        {
            printf( "Processor ID : %u [PhysicalID = %u] @ %p ", FoundCPUs->CPU_ID, FoundCPUs->CPU_Physical, FoundCPUs );
            if ( FoundCPUs->CPU_BootCPU == TRUE) printf( "[BOOT PROCESSOR]" );

            if ( FoundCPUs->CPU_IsOnline == TRUE) printf(" (online)\n");
            else  printf( " (offline)\n" );

            printf( "Processor Family : [%p]\n", FoundCPUs->CPU_Family );
            printf( "Processor Model  : [%p]\n", FoundCPUs->CPU_Model );

            if (VERBOSE)                                                                     /* Perform a thorough CPU list? */
            {

                if (FoundCPUs->CPU_Family == CPU_Family_i386) parse_i386((struct i386_compat_intern *)FoundCPUs->CPU_Private1,FoundCPUs->CPU_ID);

            }

            if (FoundCPUs->CPU_SMPGroup)
            {
                struct  SMP_Definition          *CPUsSMPGrp;

                CPUsSMPGrp   = FoundCPUs->CPU_SMPGroup;
                
                /* . This CPU is a member of an SMP group .. */
                printf("  Member of SMP Group @ %p\n", CPUsSMPGrp);
                printf("  SMP Group Member No. %d of %d\n", FoundCPUs->CPU_Physical + 1, CPUsSMPGrp->SMP_CPUCount);
            }

            printf("\n");
            cpu_count++;                                                                    /* . All done with this CPU .. */

            CPUNode = (struct MinNode *)&FoundCPUs->CPU_CPUList.mlh_Head;
            FoundCPUs = (struct CPU_Definition *)CPUNode->mln_Succ;                         /* get the next cpu in the list .. */

            if ( cpu_count > MAX_CPU )
            { 
                 printf("WARNING: Number of CPUs in list exceeds MAX no of CPUS [%d]\n", MAX_CPU);
                 error = RETURN_FAIL; 
                 break;
            }
        }
    /* .. */     
        printf("Processor List Complete..\n");

        if (cpu_count != (CPUList->CPU_Physical))
        {
            error = RETURN_FAIL;        
            printf("WARNING: Number of CPU's in list != registered number (list = %p,Registered=  %p).\n",cpu_count,CPUList->CPU_Physical);
        }
    }
    else 
    {
        error = RETURN_FAIL;
        printf("ERROR: Couldnt open cpu.resource.\n");
    }

    return error;
}
