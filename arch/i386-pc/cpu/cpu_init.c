/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU Initialisation.
    Lang: english
*/

#include <aros/asmcall.h>
#include "cpu_intern.h"
#include LC_LIBDEFS_FILE

#ifdef SysBase
#undef SysBase
#endif

static const UBYTE name[];
static const UBYTE version[];
static const void * const LIBFUNCTABLE[];
extern const char LIBEND;

struct CPUBase *AROS_SLIB_ENTRY(init, BASENAME)();

void i386_CheckCPU_Type( struct i386_compat_intern *CPU_intern );

#define SMP_SUPPORT 1

int CPU_entry(void)
{
    return -1;
}

const struct Resident CPU_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&CPU_resident,
    (APTR)&LIBEND,
    RTF_COLDSTART,
    45,
    NT_RESOURCE,
    45,
    (UBYTE *)name,
    (UBYTE *)&version[6],
    (ULONG *)&AROS_SLIB_ENTRY(init,BASENAME)
};

static const UBYTE name[] = NAME_STRING;
static const UBYTE version[] = VERSION_STRING;

static const void * const LIBFUNCTABLE[] =
{
    //&AROS_SLIB_ENTRY(ResetBattClock,BASENAME),
    //&AROS_SLIB_ENTRY(ReadBattClock,BASENAME),
    //&AROS_SLIB_ENTRY(WriteBattClock,BASENAME),
    (void *)-1
};

static struct CPU_Definition *BootCPU;                      /* Pointer used by the exec launched boot cpu probe */ 

AROS_UFH3(struct CPUBase *, AROS_SLIB_ENTRY(init,BASENAME),
    AROS_UFHA(ULONG,	dummy,	D0),
    AROS_UFHA(ULONG,	slist,	A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct intel_mp_confblock       *mpcfb;
    struct CPU_Definition           *AvailCPUs;
    struct i386_compat_intern       *BootCPU_intern;
    struct  ACPIBase                *ACPIBase;
    CPUBase = NULL;

    UWORD neg = AROS_ALIGN(LIB_VECTSIZE * 3);
    
    CPUBase = (struct CPUBase *)(((UBYTE *)	AllocMem( neg + sizeof(struct CPUBase), MEMF_CLEAR | MEMF_PUBLIC)) + neg);

    if( CPUBase )
    {
        CPUBase->CPUB_SysBase = SysBase;
	    CPUBase->CPUB_Node.ln_Pri = 0;
	    CPUBase->CPUB_Node.ln_Type = NT_RESOURCE;
	    CPUBase->CPUB_Node.ln_Name = (STRPTR)name;
        CPUBase->CPUB_UtilBase = OpenLibrary("utility.library", 0);

        CPUBase->CPUB_BOOT_Physical = -1;                                                   /* set to a single cpu for now          */
        CPUBase->CPUB_BOOT_Logical = -1;

	    MakeFunctions( CPUBase, (APTR)LIBFUNCTABLE, NULL );
	    AddResource( CPUBase );

        mpcfb = find_smp_config();

        if ( mpcfb )
        {
            kprintf(DEBUG_NAME_STR ": Found SMP 1.4 MP table = 0x%p\n", mpcfb);
            AllocAbs( 4096, &mpcfb);
	        if (mpcfb->mpcf_physptr) AllocAbs( 4096, mpcfb->mpcf_physptr );
            kprintf(DEBUG_NAME_STR ": SMP Table(s) protected\n");
        }
        else kprintf(DEBUG_NAME_STR ": NO compatable SMP hardware found.\n");

        AvailCPUs = AllocMem( sizeof(struct CPU_Definition), MEMF_CLEAR | MEMF_PUBLIC );    /* Create our new CPU List              */

        if( AvailCPUs )
        {

            NEWLIST((struct List *)&(AvailCPUs->CPU_CPUList));
            AvailCPUs->CPU_Physical = 1;

            InitSemaphore( &CPUBase->CPU_ListLock);
            kprintf(DEBUG_NAME_STR ": Initialised CPU List Semaphore\n");
        }
        else
        {
            kprintf(DEBUG_NAME_STR ": ERROR - Couldnt allocate CPU list memory!\n");
            return NULL;
        }

#warning TODO: Patch functions with suitable replacements (bug fixes/speed ups) - BEFORE SMP SETUP!

        if ( mpcfb )  /* SMP? */
        {
            AllocAbs( 4096, smp_alloc_memory());                                    /* Create The Trampoline page..
	                                                                                Has to be in very low memory so we can execute
                                                                                    real-mode AP code.                               */
            BootCPU->CPU_Private2 = mpcfb;                                          /* Store the pointer to the SMP config block        */
            kprintf(DEBUG_NAME_STR ": SMP Trampoline page protected, SMP config stored.\n");
        }

        CPUBase->CPUB_Processors = AvailCPUs; // done !

        AddTail(&AvailCPUs->CPU_CPUList,&BootCPU->CPU_CPUList);
        kprintf(DEBUG_NAME_STR ": CPU List created = 0x%p, Boot CPU inserted..0x%p\n",AvailCPUs,BootCPU);

	    /*  Parse the ACPI tables for possible boot-time SMP configuration. */
	    CPUBase->CPUB_ACPIBase = OpenResource("acpi.resource");
        kprintf(DEBUG_NAME_STR ": acpi.resource @ %p\n",CPUBase->CPUB_ACPIBase);
        
        ACPIBase = CPUBase->CPUB_ACPIBase;
        ACPIBase->ACPIB_CPUBase = CPUBase;                                          /* pass our base poiner over   */

        //ACPI_Init();                                                                /* make sure ACPI is online .. */

        if (mpcfb) get_smp_config( mpcfb, CPUBase );
    }
    return CPUBase;

    AROS_USERFUNC_EXIT
}

#ifdef SysBase  /* WARNING!!! THIS NEXT FUNCTION RUS IN KERNEL LAND _ NO DEBUG OUTPUT ETC>> BE CAREFULL! */
#undef SysBase
#endif

void prepare_primary_cpu(struct ExecBase *SysBase)
{
    struct i386_compat_intern       *BootCPU_intern;

    BootCPU = AllocMem( sizeof(struct CPU_Definition), MEMF_CLEAR | MEMF_PUBLIC );                  /* Add the boot CPU to the CPU List     */
    if( BootCPU )
    {

        BootCPU->CPU_Family = CPU_Family_i386;                                                      /* we are i386 compatable..             */
        BootCPU->CPU_Model = CPU_i386_386;
        BootCPU->CPU_ID = 1;                                                                        /* we are the only CPU at this time ..  */
        BootCPU->CPU_IsOnline = TRUE;                                                               /* CPU is online ..                     */
        BootCPU->CPU_BootCPU = TRUE;                                                                /* CPU bootd system..                   */

        BootCPU_intern = AllocMem(sizeof(struct i386_compat_intern), MEMF_CLEAR | MEMF_PUBLIC );    /* Create its per CPU internal struct   */

        if( BootCPU_intern )
        {

#warning TODO: The next line is broken - fix the checkcpu function
            i386_CheckCPU_Type( BootCPU_intern );

            BootCPU->CPU_Private1 = BootCPU_intern;

            switch (BootCPU_intern->x86_model)
            {    
                case 4:
                    BootCPU->CPU_Model = CPU_i386_486;
                    break;
                case 5:
                    BootCPU->CPU_Model = CPU_i386_586;
                    break;
                case 6:
                    BootCPU->CPU_Model = CPU_i386_686;
                    break;
                case 7:
                    BootCPU->CPU_Model = CPU_i386_786;
                    break;
                case 8:
                    BootCPU->CPU_Model = CPU_i386_886;
                    break;
            }
        }
    }
}
