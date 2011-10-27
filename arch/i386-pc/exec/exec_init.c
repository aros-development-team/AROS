/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Early bootup section
    Lang: english
*/

/*
    i386-pc AROS MemoryMap:
     
    First 4KB of RAM (page 0x0000-0x0FFF) are READ ONLY!
          
    +------------+-----------+-------------------------------------------------+
    |    address |    length |                                     description |
    +------------+-----------+-------------------------------------------------+
    |   System page. Read only! Addresses undefined here are not allowed!!!!   |    
    +------------------------+-------------------------------------------------+
    | 0x00000000 | 0x0000100 |               Extended BIOS data area preserved |
    | 0x00000004 | 0x0000004 |                                 SysBase pointer |
    | 0x00000100 | 0x0000800 |                                        Not used |
    | 0x00000900 | 0x0000004 | INT server!                     cached_irq_mask |
    | 0x00000904 | 0x0000004 | INT server!                        io_apic_irqs |
    | 0x00000940 | 0x0000001 | INT server!                           softblock |
    | 0x00000a00 | 0x0000200 | INT server!                          irq_desc[] |
    | 0x00000c00 | 0x0000300 |                 Temporary stack for warm reboot |
    +------------+-----------+-------------------------------------------------+
    | 0x00001000 | ......... |   System RAM. Initially bootstrap resides here. |
    | 0x000a0000 | 0x0060000 | Data reserved for BIOS, VGA and some MMIO cards |
 1MB| 0x00100000 | ......... |           System RAM divided into DMA and rest. |
    +------------+-----------+-------------------------------------------------+ 
*/

#include <exec/resident.h>
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <hardware/intbits.h>
#include <asm/segments.h>
#include <asm/linkage.h>
#include <asm/ptrace.h>
#include <dos/dosextens.h>

#include <aros/arossupportbase.h>
#include <aros/asmcall.h>
#include <aros/config.h>

#include <aros/kernel.h>

#define DEBUG    1

#include <aros/debug.h>
#include <aros/multiboot.h>

#include <utility/tagitem.h>

#include <hardware/custom.h>
#include <resources/acpi.h>

#include <proto/alib.h>
#include <proto/exec.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h>

#include LC_LIBDEFS_FILE

#include "etask.h"
#include "exec_intern.h"
#include "exec_debug.h"
#include "exec_util.h"
#include "intservers.h"
#include "memory.h"
#include "traps.h"

/* As long as we don't have CPU detection routine, assume FPU to be present */

#define ASSUME_FPU 1

/*
 * Some macro definitions. __text will place given structure in .text section.
 * __no_ret will force function type to be no-return function. __packed will
 * force any structure to be non-aligned (we don't need alignment in native
 * AROS).
 */

#define	__text      __attribute__((section(".text")))
#define __no_ret    __attribute__((noreturn))
#define __packed    __attribute__((packed))

#define rdcr(reg) \
    ({ long val; asm volatile("mov %%" #reg ",%0":"=r"(val)); val; })

#define wrcr(reg, val) \
    do { asm volatile("mov %0,%%" #reg::"r"(val)); } while(0)

#define cpuid(num, eax, ebx, ecx, edx) \
    do { asm volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(num)); } while(0)

/*
 * Some declarations
 */
AROS_UFP3S(struct ExecBase *, exec_init,
    AROS_UFPA(struct MemHeader *, mh, D0),
    AROS_UFPA(struct TagItem *, tagList, A0),
    AROS_UFPA(struct ExecBase *, sysBase, A6)); 

void    irqSetup(void);

extern const UBYTE LIBEND __text;             /* Somewhere in library */

extern void AROS_SLIB_ENTRY(SerialRawIOInit, Exec, 84)();
extern void AROS_SLIB_ENTRY(SerialRawPutChar, Exec, 86)(UBYTE chr);
extern void AROS_SLIB_ENTRY(MemoryRawIOInit, Exec, 84)();
extern void AROS_SLIB_ENTRY(MemoryRawPutChar, Exec, 86)(UBYTE chr);

extern void AROS_SLIB_ENTRY(Switch_FPU, Exec, 9)();
extern void AROS_SLIB_ENTRY(PrepareContext_FPU, Exec, 6)();
extern void AROS_SLIB_ENTRY(Dispatch_FPU, Exec, 10)();

extern void AROS_SLIB_ENTRY(Switch_SSE, Exec, 9)();
extern void AROS_SLIB_ENTRY(PrepareContext_SSE, Exec, 6)();
extern void AROS_SLIB_ENTRY(Dispatch_SSE, Exec, 10)();
extern void AROS_SLIB_ENTRY(CopyMem_SSE, Exec, 104)();

/*
 * First, we will define exec.library (global) to make it usable outside this
 * file.
 */
const char exec_name[] __text       = "exec.library";
/* Now ID string as it will be used in a minute in resident structure. */
const char exec_idstring[] __text = VERSION_STRING;

/*
 * The RomTag structure. It has to be placed inside .text block as there will
 * be no public RomTagList. In future we may change RTC_MATCHWORD to be machine
 * specific.
 */
const struct Resident Exec_resident __text=
{
    RTC_MATCHWORD,          /* Magic value used to find resident */
    &Exec_resident,         /* Points to Resident itself */
    (APTR)&LIBEND,          /* Where could we find next Resident? */
    0,                      /* There are no flags!! */
    VERSION_NUMBER,         /* Version */
    NT_LIBRARY,             /* Type */
    126,                    /* Very high startup priority. */
    (char *)exec_name,      /* Pointer to name string */
    (char *)exec_idstring,  /* Ditto */
    exec_init               /* Library initializer */
};


/* These defines used to keep control over interrupt server */
asm(".globl cached_irq_mask\n\t"
    ".globl io_apic_irqs\n\t"
    ".globl softblock\n\t"
    ".globl irq_desc\n\t"
    ".set   cached_irq_mask, 0x00000900\n\t"
    ".set   io_apic_irqs,    0x00000904\n\t"
    ".set   softblock,       0x00000940\n\t"
    ".set   irq_desc,        0x00000a00");

AROS_UFH3S(struct ExecBase *, exec_init,
    AROS_UFHA(struct MemHeader *, mh, D0),
    AROS_UFHA(struct TagItem *, tagList, A0),
    AROS_UFHA(struct ExecBase *, origSysBase, A6)
)
{
    AROS_USERFUNC_INIT

    if (!origSysBase)
    	return PrepareExecBase(mh, tagList);

    return NULL;

    AROS_USERFUNC_EXIT
}

void exec_boot(struct TagItem *msg)
{
    char *cmdline = (char *)LibGetTagData(KRN_CmdLine, 0, msg);
    int i;

    bug("Initializing library...");

    for (i=0; i<16; i++)
    {
        if( (1<<i) & (INTF_PORTS|INTF_COPER|INTF_VERTB|INTF_EXTER|INTF_SETCLR))
        {
            struct Interrupt *is;
            struct SoftIntList *sil;
            is = AllocMem
            (
                sizeof(struct Interrupt) + sizeof(struct SoftIntList),
                MEMF_CLEAR | MEMF_PUBLIC
            );
            if( is == NULL )
            {
                bug("ERROR: Cannot install Interrupt Servers!\n");
            }
            sil = (struct SoftIntList *)((struct Interrupt *)is + 1);

            is->is_Code = &IntServer;
            is->is_Data = sil;
            NEWLIST((struct List *)sil);
            SetIntVector(i,is);
        }
        else
        {
            struct Interrupt *is;
            switch (i)
            {
                case INTB_SOFTINT :
                    is = AllocMem
                    (
                        sizeof(struct Interrupt), 
                        MEMF_CLEAR | MEMF_PUBLIC
                    );
                    if (is == NULL)
                    {
                        bug("Error: Cannot install Interrupt Servers!\n");
                        // Alert(AT_DeadEnd | AN_IntrMem);
                    }
                    is->is_Node.ln_Type = NT_SOFTINT;   //INTERRUPT;
                    is->is_Node.ln_Pri = 0;
                    is->is_Node.ln_Name = "SW Interrupt Dispatcher";
                    is->is_Data = NULL;
                    is->is_Code = (void *)SoftIntDispatch;
                    SetIntVector(i,is);
                    break;
            }
        }
    }

    irqSetup();
    bug("IRQ services initialized\n");

    /* Enable interrupts and set int disable level to -1 */
    asm("sti");
    SysBase->TDNestCnt = -1;
    SysBase->IDNestCnt = -1;

    bug("Creating the very first task...");

    /* Complete boot task. */
    {
        struct Task *t = SysBase->ThisTask;
/*      struct MemList *ml;

        ml = (struct MemList *)AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR);

        if(!ml)
        {
            bug("ERROR: Cannot create Boot Task!\n");
	    for(;;);
        }
        ml->ml_NumEntries = 1;
        ml->ml_ME[0].me_Addr = t;
        ml->ml_ME[0].me_Length = sizeof(struct Task);

        AddHead(&t->tc_MemEntry,&ml->ml_Node);
*/
        t->tc_Flags |= TF_ETASK;
        t->tc_UnionETask.tc_ETask = AllocVec(sizeof(struct IntETask), MEMF_CLEAR);

        if (!t->tc_UnionETask.tc_ETask)
        {
            bug("Not enough memory for first task\n");
	    for(;;);
        }

        /* Initialise the ETask data. */
        InitETask(t, t->tc_UnionETask.tc_ETask);

        GetIntETask(t)->iet_Context = AllocTaskMem(t, SIZEOF_ALL_REGISTERS, MEMF_PUBLIC|MEMF_CLEAR);

        if (!GetIntETask(t)->iet_Context)
        {
            bug("Not enough memory for first task\n");
	    for(;;);
        }
	t->tc_UnionETask.tc_ETask->et_Parent = NULL;
    }

    bug("Done\n");

    /*
	Check whether the CPU supports SSE and FXSAVE.
	
	Dirty check, without use of any defines and human readable constants. The
	cpuid instruction with %eax=1 will return some essential cpu informations in %edx back, 
	including:
	- bit 24: CPU does support FXSAVE and FXRSTOR for MMX/FPU/SSE context saving and restoring
	- bit 25: CPU supports SSE
	- bit 26: CPU supports SSE2
    
    */
    ULONG v1,v2,v3,v4;
    cpuid(1, v1,v2,v3,v4);

    bug("cpuid 1 = %08x %08x %08x %08x\n", v1, v2, v3, v4);
    
    if (v4 & (1 << 24))
    {
	bug("The CPU supports FXSAVE and FXRSTOR. Good.\n");
	bug("CPU Supports ");
	
	switch ((v4 >> 25) & 3)
	{
	    case 3:
	    case 2:
		bug("SSE2 ");
	    case 1:
		bug("SSE\n");
		/* Patch exec with some SSE-aware functions */
        SetFunction(&SysBase->LibNode, -6*LIB_VECTSIZE, AROS_SLIB_ENTRY(PrepareContext_SSE, Exec, 6));
        SetFunction(&SysBase->LibNode, -9*LIB_VECTSIZE, AROS_SLIB_ENTRY(Switch_SSE, Exec, 9));
        SetFunction(&SysBase->LibNode, -10*LIB_VECTSIZE, AROS_SLIB_ENTRY(Dispatch_SSE, Exec, 10));
        SetFunction(&SysBase->LibNode, -104*LIB_VECTSIZE, AROS_SLIB_ENTRY(CopyMem_SSE, Exec, 104));
		SetFunction(&SysBase->LibNode, -105*LIB_VECTSIZE, AROS_SLIB_ENTRY(CopyMem_SSE, Exec, 104));
		/* tell the CPU that we will support SSE */
		wrcr(cr4, rdcr(cr4) | (3 << 9));
		/* Clear the EM and MP flags of CR0 */
		wrcr(cr0, rdcr(cr0) & ~6);	
		bug("SSE enabled.\n");
		break;

	    default:
		/* 
		    Ha! Bloody PentiumII does supports MMX/FPU/SSE saving instructions,
		    but it does not support SSE 
		*/
		bug("no SSE. Sorry :)\n");
		break;
	}	
    }
    else
    { 
#if ASSUME_FPU
        SetFunction(&SysBase->LibNode, -6*LIB_VECTSIZE, AROS_SLIB_ENTRY(PrepareContext_FPU, Exec, 6));
        SetFunction(&SysBase->LibNode, -9*LIB_VECTSIZE, AROS_SLIB_ENTRY(Switch_FPU, Exec, 9));
        SetFunction(&SysBase->LibNode, -10*LIB_VECTSIZE, AROS_SLIB_ENTRY(Dispatch_FPU, Exec, 10));

	bug("FPU enabled.\n");
#endif
    }


    bug("Jumping out from Supervisor mode...");

#if 0

    asm("mov %0,%%ds\n\t"   /* User DS */
	"mov %0,%%es\n\t"       /* User ES */
	"movl %%esp,%%ebx\n\t"  /* Hold the esp value before pushing! */
	"pushl %0\n\t"          /* User SS */
	"pushl %%ebx\n\t"       /* Stack frame */
	"pushl $0x3002\n\t"     /* IOPL:3 */
	"pushl %1\n\t"          /* User CS */
	"pushl $1f\n\t"         /* Entry address */
	"iret\n"                /* Go down to the user mode */
	"1:\tsti"               /* Enable interrupts */
	:
	: "eax"(USER_DS),"ecx"(USER_CS));
#else
#    define _stringify(x) #x
#    define stringify(x) _stringify(x)

    asm("movl $" stringify(USER_DS) ",%%eax\n\t"
        "mov %%eax,%%ds\n\t"                         /* User DS */
	"mov %%eax,%%es\n\t"                         /* User ES */
	"movl %%esp,%%ebx\n\t"			/* Hold the esp value before pushing! */
	"pushl %%eax\n\t"                           /* User SS */
	"pushl %%ebx\n\t"                           /* Stack frame */
	"pushl $0x3002\n\t"                        /* IOPL:3 */
	"pushl $" stringify(USER_CS) "\n\t"        /* User CS */
	"pushl $1f\n\t"                            /* Entry address */
	"iret\n"                                   /* Go down to the user mode */
	"1:\tsti":::"eax","ebx");                                /* Enable interrupts */

#   undef stringify
#   undef _stringify
#endif

    bug("Done\n");

    SysBase->TDNestCnt++;
    Permit();

    /*
     * Enable type of debug output chosen by user.
     * 'serial' is not handled in libbootconsole.
     */
    if (strstr(cmdline, "debug=memory"))
    {
        SetFunction(&SysBase->LibNode, -84 * LIB_VECTSIZE,
            AROS_SLIB_ENTRY(MemoryRawIOInit, Exec, 84));
        SetFunction(&SysBase->LibNode, -86 * LIB_VECTSIZE,
            AROS_SLIB_ENTRY(MemoryRawPutChar, Exec, 86));
    }
    RawIOInit();

    if (SysBase->CoolCapture)
    {
        void (*p)() = SysBase->CoolCapture;
 
	bug("Executing CoolCapture at 0x%p\n", p);
	(*p)();
    }

    InitCode(RTF_COLDSTART, 0);
}

/* Small delay routine used by exec_cinit initializer */
asm("\ndelay:\t.short   0x00eb\n\tret");

AROS_LH1(struct ExecBase *, open,
    AROS_LHA(ULONG, version, D0),
        struct ExecBase *, SysBase, 1, Exec)
{
    AROS_LIBFUNC_INIT

    /* I have one more opener. */
    SysBase->LibNode.lib_OpenCnt++;
    return SysBase;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
    struct ExecBase *, SysBase, 2, Exec)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    SysBase->LibNode.lib_OpenCnt--;
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
    struct ExecBase *, SysBase, 3, Exec)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
    struct ExecBase *, SysBase, 4, Exec)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
