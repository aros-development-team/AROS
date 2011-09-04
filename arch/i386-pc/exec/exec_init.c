/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Early bootup section
    Lang: english
*/

/*
    AROS MemoryMap:
     
    First 4KB of RAM (page 0x0000-0x0FFF) are READ ONLY!
          
    +------------+-----------+-------------------------------------------------+
    |    address |    length |                                     description |
    +------------+-----------+-------------------------------------------------+
    |   System page. Read only! Addresses undefined here are not allowed!!!!   |    
    +------------------------+-------------------------------------------------+
    | 0x00000004 | 0x0000004 |                               SysBase pointer. |
    | 0x00000020 | 0x0000020 |                                 CPU information |
    | 0x00000040 | 0x0000068 |                      TSS structure. Do not use! |
    | 0x000000a8 | 0x0000004 | INT server!                     cached_irq_mask |
    | 0x000000ac | 0x0000004 | INT server!                        io_apic_irqs |
    | 0x00000100 | 0x0000800 |                           256 interrupt vectors |
    | 0x00000900 | 0x0000040 |                         Global Descriptor Table |<---- needs changed! - must have space for GDT x No CPUs (NicJA)
    | 0x00000940 | 0x0000001 | INT server!                           softblock |
    | 0x00000a00 | 0x0000200 | INT server!                          irq_desc[] |
    +------------+-----------+-------------------------------------------------+
    | 0x00001000 | 0x0001000 |                           MultiBoot information |
    | 0x00002000 | ......... |                                      System RAM |
    | .......... | ......... |        Temporary stack frame for bootup process |
......0x0009e000   0x0001000                                                            SMP Trampoline  (NicJA)
    | 0x000a0000 | 0x0060000 | Data reserved for BIOS, VGA and some MMIO cards |
......0x000f7070                                                                        ACPI<-
 1MB| 0x00100000 | ......... |                                     AROS KERNEL |
    | .......... | ......... |           System RAM divided into DMA and rest. |
    +------------+-----------+-------------------------------------------------+ 
        0fef7f80
    WARNING!

        AROS Kernel will not be placed in first 1MB of ram anymore. Instead it
        will be either somewhere in DMA memory (eg at 2nd MByte), at the end of
        available memory or placed at Virtual address. Don't trust its position.

        When ROM module called from loader, the SysBase should be cleared.      

        This file has been completely rewritten from assembler source!
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
void    exec_DefaultTrap();
asmlinkage void Exec_SystemCall(struct pt_regs);
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


#undef memcpy
#define memcpy(_d, _s, _len)                     \
{                                                \
    int len = _len;                              \
    while (len)                                  \
    {                                            \
       ((char *)_d)[len-1] = ((char *)_s)[len-1];\
       len--;                                    \
    }                                            \
}


/* Temporary information */

#if 1

void clr();
void scr_RawPutChars(char *, int);

char tab[127];

#ifdef rkprintf
# undef rkprintf
#endif
#define rkprintf(x...)	scr_RawPutChars(tab, snprintf(tab,126, x))

#else

#define clr() /* eps */
#define rkprintf(x...) /* eps */

#endif

/*
 * Make SysBase global. This way we will have no problems with all crapy modules
 * which rely on global SysBase. They will get what they want - they will access
 * it :)
 */

asm(        ".globl aros_intern\n\t"
            ".globl SysBase     \n\t"
            ".set   aros_intern,0\n\t"
            ".set   SysBase,4       ");

/*
 * First, we will define exec.library (global) to make it usable outside this
 * file.
 */
const char exec_name[] __text       = "exec.library";

/* Now ID string as it will be used in a minute in resident structure. */
const char exec_idstring[] __text = VERSION_STRING;

/* We would need also version and revision fields placed somewhere here. */
const short exec_Version __text     = VERSION_NUMBER;
const short exec_Revision __text    = REVISION_NUMBER;

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

struct view { unsigned char sign; unsigned char attr; };

/*
 * Mixed stuff used to control TSS/GDT/IDT stuff. This is strictly machine
 * specific
 */

asm(".globl TSS\n\t"            /* Make these three global in case one would */
    ".globl SSP\n\t"            /* decide to use them directly */    
    ".globl EIP\n\t"
    ".set   TSS, 0x00000040\n\t"    /* See Memory Map at the top of the file */
    ".set   SSP, 0x00000044\n\t"
    ".set   EIP, 0x00000060");

/*
 * Two defines used to keep control over interrupt server
 */

asm(".globl cached_irq_mask\n\t"
    ".globl io_apic_irqs\n\t"
    ".globl softblock\n\t"
    ".globl irq_desc\n\t"
    ".set   cached_irq_mask, 0x000000a8\n\t"
    ".set   io_apic_irqs,    0x000000ac\n\t"
    ".set   softblock,       0x00000940\n\t"
    ".set   irq_desc,        0x00000a00");

/*
 * TaskStateStructure, defined only in matter of making life (setup)
 * more human-readable
 */
struct _tss {
    ULONG   link,               /* link to previous task        - UNUSED        */
            ssp,                /* Supervisor Stack Pointer                     */
            ssp_seg,            /* SSP descriptor                               */
            t0,t1,              /* Stack for CPL1 code          - USE IN FUTURE */
            t2,t3,              /* Stack for CPL2 code          - UNUSED        */
            cr3,                /* used in paging                               */
            eip,                /* Instruction pointer                          */
            eflags,             /* Flags for given task                         */
            r0,r1,r2,r3,        /* 8 general purpouse registers                 */
            r4,r5,r6,r7,
            es,cs,ss,ds,fs,gs,  /* segment descriptors                          */
            ldt;                /* LocalDescriptorTable         - UNUSED        */
    UWORD   trap,iomap;         /* trap flag and iomap pointer                  */
};

/*
 * Nice macro used for setting given gate to specified type.
 * dpl is used to control access of this gate - setting to something
 * lower than 3 makes gate impossible to use from User Mode
 */
#define set_gate(gate_addr,type,dpl,addr) \
do { \
  int __d0, __d1; \
  __asm__ __volatile__ ("movw %%dx,%%ax\n\t" \
    "movw %4,%%dx\n\t" \
    "movl %%eax,%0\n\t" \
    "movl %%edx,%1" \
    :"=m" (*((long *) (gate_addr))), \
     "=m" (*(1+(long *) (gate_addr))), "=&a" (__d0), "=&d" (__d1) \
    :"i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
     "3" ((char *) (addr)),"2" (KERNEL_CS << 16)); \
} while (0)

/* Empty interrupt. Used for short while */
void exec_DummyInt();
asm("\nexec_DummyInt:   iret");

/*
 * RO copy of GlobalDescriptorTable. It's easier to copy this table than
 * to generate completely new one
 */
const struct {UWORD l1, l2, l3, l4;}
    GDT_Table[] __text = {
        { 0x0000, 0x0000, 0x0000, 0x0000 },
        { 0xffff, 0x0000, 0x9a00, 0x00cf },
        { 0xffff, 0x0000, 0x9200, 0x00cf },
        { 0xffff, 0x0000, 0xfa00, 0x00cf },
        { 0xffff, 0x0000, 0xf200, 0x00cf },
        { 0x0000, 0x0000, 0x0000, 0x0000 },
        { 0x0067, 0x0040, 0x8900, 0x0000 },
        { 0x0000, 0x0000, 0x0000, 0x0000 }};

/* Two magic registers pointing to Global Descriptor Table and Interrupt
 * Descriptor Table */
const struct
{
    UWORD l1 __packed;
    ULONG l3 __packed;
}
GDT_reg __text = {0x3f,  0x900},
IDT_reg __text = {0x7ff, 0x100};

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

/*
 * Init the system. This routine has to prepare
 * all needed structures, GDT and IDT tables, TSS structure and many other.
 */
void exec_cinit()
{
    struct _tss *tss = (struct _tss *)0x40; /* Dummy pointer making life easier */
    long long *idt = (long long *)0x100;    /* Ditto */

    int i;                                  /* Whatever? Counter? Erm... */
    
    /*
     * We will have to build new tables for our CPU to make it work nicely.
     */
    tss->ssp = 0x00090000;      /* temporary Supervisor Stack Pointer */
    tss->ssp_seg = KERNEL_DS;   /* SSP segment descriptor */
    tss->cs = USER_CS;
    tss->ds = USER_DS;
    tss->es = USER_DS;
    tss->ss = USER_DS;
    tss->iomap = 104;

    /* Restore IDT structure. */
    for (i=0; i<256; i++)
    {
        /* Feed all vectors with dummy interrupt */
        set_gate(idt + i, 14, 0, exec_DummyInt);
    }

    /*
     * Fix Global Descriptor Table. Because I'm too lazy I'll just copy one from
     * here. I've already prepared such a nice one :)
     */
    memcpy((void *)0x900, &GDT_Table, 64);

    /*
     * As we prepared all necessary stuff, we can hopefully load GDT and LDT
     * into CPU. We may also play a bit with TSS
     */
    asm("lgdt   %0\n\t"
        "lidt   %1"
        :
        :"m"(GDT_reg),"m"(IDT_reg));

    asm("mov    %0,%%ds\n\t"    /* Now it's high time to set segment   */
        "mov    %0,%%es\n\t"    /* registers (segment descriptors).    */
        "mov    %0,%%ss\n\t"    /* AROS uses only %CS %SS %DS and %ES  */
        "mov    %1,%%fs\n\t"    /* %FS and %GS are set to 0 so we can  */
        "mov    %1,%%gs\n\t"    /* generate GP if someone uses them.   */
        "ljmp   %2,$1f\n1:\n\t" /* And finally, set the %CS!!!         */
        :
        :"a"(KERNEL_DS),"b"(0),"i"(KERNEL_CS));

    asm("ltr    %%ax"::"ax"(0x30));

    rkprintf("OK\nSystem restored\n");

    /*
     * Wew have to do some stuff from setup.S due to remove of it.
     *
     * Remap IRQs. We really HAVE TO do that because IBM did it in a very bad
     * way - they've ignored what Intel said about interrupts. See intel manual
     * for further information.
     * For now all irq's are placed in range 0x20 - 0x30 as vectors 0x00 - 0x1f
     * are reserved by Intel
     */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(0x20)); /* Initialization sequence for 8259A-1 */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(0xa0)); /* Initialization sequence for 8259A-2 */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x20),"i"(0x21)); /* IRQs at 0x20 - 0x27 */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x28),"i"(0xa1)); /* IRQs at 0x28 - 0x2f */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x04),"i"(0x21)); /* 8259A-1 is master */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x02),"i"(0xa1)); /* 8259A-2 is slave */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(0x21)); /* 8086 mode for both */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(0xa1));
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xff),"i"(0x21)); /* Enable cascade int */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xff),"i"(0xa1)); /* Mask all interrupts */

    rkprintf("Interrupts redirected\n");
}

void exec_boot(struct TagItem *msg)
{
    struct _tss *tss = (struct _tss *)0x40;
    long long *idt = (long long *)0x100;
    char *cmdline = (char *)LibGetTagData(KRN_CmdLine, 0, msg);
    int i;

    rkprintf("Initializing library...");

    /* Set up system stack */
    SysBase->SysStkLower = AllocMem(0x10000, MEMF_PUBLIC);  /* 64KB of system stack */
    SysBase->SysStkUpper = SysBase->SysStkLower + 0x10000;

    tss->ssp = (IPTR)SysBase->SysStkUpper;

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
                rkprintf("ERROR: Cannot install Interrupt Servers!\n");
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
                        rkprintf("Error: Cannot install Interrupt Servers!\n");
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

/* TODO: Write CPU detailed detection scheme. Patch proper functions?? */

    Init_Traps();
    irqSetup();
    rkprintf("IRQ services initialized\n");

    /* Create user interrupt used to enter supervisor mode */
    set_gate(idt + 0x80, 14, 3, Exec_SystemCall);

    /* Enable interrupts and set int disable level to -1 */
    asm("sti");
    SysBase->TDNestCnt = -1;
    SysBase->IDNestCnt = -1;

    rkprintf("Creating the very first task...");

    /* Complete boot task. */
    {
        struct Task *t = SysBase->ThisTask;
        struct MemList *ml;

        ml = (struct MemList *)AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR);

        if(!ml)
        {
            rkprintf("ERROR: Cannot create Boot Task!\n");
	    for(;;);
        }
        ml->ml_NumEntries = 1;
        ml->ml_ME[0].me_Addr = t;
        ml->ml_ME[0].me_Length = sizeof(struct Process);

        AddHead(&t->tc_MemEntry,&ml->ml_Node);
<<<<<<< .mine
=======

        t->tc_Node.ln_Name = (char *)exec_name;
        t->tc_Node.ln_Pri = 0;
        t->tc_Node.ln_Type = NT_TASK;
        t->tc_State = TS_RUN;
        t->tc_SigAlloc = 0xFFFF;
        t->tc_SPLower = stack_base;
        t->tc_SPUpper = stack_base + stack_size - 1;
>>>>>>> .r41117
        t->tc_Flags |= TF_ETASK;
<<<<<<< .mine
        t->tc_UnionETask.tc_ETask = AllocVec(sizeof(struct IntETask), MEMF_CLEAR);
=======
        aros_init_altstack(t);
>>>>>>> .r41117

        if (!t->tc_UnionETask.tc_ETask)
        {
            rkprintf("Not enough memory for first task\n");
	    for(;;);
        }

        /* Initialise the ETask data. */
        InitETask(t, t->tc_UnionETask.tc_ETask);

        GetIntETask(t)->iet_Context = AllocTaskMem(t, SIZEOF_ALL_REGISTERS, MEMF_PUBLIC|MEMF_CLEAR);

        if (!GetIntETask(t)->iet_Context)
        {
            rkprintf("Not enough memory for first task\n");
	    for(;;);
        }
	t->tc_UnionETask.tc_ETask->et_Parent = NULL;
    }

    rkprintf("Done\n");

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

    rkprintf("cpuid 1 = %08x %08x %08x %08x\n", v1, v2, v3, v4);
    
    if (v4 & (1 << 24))
    {
	rkprintf("The CPU supports FXSAVE and FXRSTOR. Good.\n");
	rkprintf("CPU Supports ");
	
	switch ((v4 >> 25) & 3)
	{
	    case 3:
	    case 2:
		rkprintf("SSE2 ");
	    case 1:
		rkprintf("SSE\n");
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
		rkprintf("SSE enabled.\n");
		break;

	    default:
		/* 
		    Ha! Bloody PentiumII does supports MMX/FPU/SSE saving instructions,
		    but it does not support SSE 
		*/
		rkprintf("no SSE. Sorry :)\n");
		break;
	}	
    }
    else
    { 
#if ASSUME_FPU
        SetFunction(&SysBase->LibNode, -6*LIB_VECTSIZE, AROS_SLIB_ENTRY(PrepareContext_FPU, Exec, 6));
        SetFunction(&SysBase->LibNode, -9*LIB_VECTSIZE, AROS_SLIB_ENTRY(Switch_FPU, Exec, 9));
        SetFunction(&SysBase->LibNode, -10*LIB_VECTSIZE, AROS_SLIB_ENTRY(Dispatch_FPU, Exec, 10));

	rkprintf("FPU enabled.\n");
#endif
    }


    rkprintf("Jumping out from Supervisor mode...");

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

    rkprintf("Done\n");

    SysBase->TDNestCnt++;
    Permit();

    /* Enable type of debug output chosen by user */
    if (strstr(cmdline, "debug=serial"))
    {
        SetFunction(&SysBase->LibNode, -84 * LIB_VECTSIZE,
            AROS_SLIB_ENTRY(SerialRawIOInit, Exec, 84));
        SetFunction(&SysBase->LibNode, -86 * LIB_VECTSIZE,
            AROS_SLIB_ENTRY(SerialRawPutChar, Exec, 86));
    }
    else if (strstr(cmdline, "debug=memory"))
    {
        SetFunction(&SysBase->LibNode, -84 * LIB_VECTSIZE,
            AROS_SLIB_ENTRY(MemoryRawIOInit, Exec, 84));
        SetFunction(&SysBase->LibNode, -86 * LIB_VECTSIZE,
            AROS_SLIB_ENTRY(MemoryRawPutChar, Exec, 86));
    }
    PrivExecBase(SysBase)->PageSize = MEMCHUNK_TOTAL; /* temp fix for debug=memory */
    RawIOInit();

    if (SysBase->CoolCapture)
    {
        void (*p)() = SysBase->CoolCapture;
 
	rkprintf("Executing CoolCapture at 0x%p\n", p);
	(*p)();
    }

    InitCode(RTF_SINGLETASK, 0);
    InitCode(RTF_COLDSTART, 0);
    
    /*
     * We suppose that at this point dos.library has already taken over.
     * The last thing to do is to call WarmCapture vector. After that this
     * task has completely nothing to do so it may execute Debug() in
     * forever loop
     */
/* InitCode(RTF_COLDSTART) never returns!!!
    if (SysBase->WarmCapture)
    {
        void (*p)() = SysBase->WarmCapture;

        (*p)();
    } */

    rkprintf("Failed to start up the system!");
    do { Debug(0); } while(1);
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
