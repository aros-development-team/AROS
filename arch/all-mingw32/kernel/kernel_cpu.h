#ifndef KERNEL_CPU_H_
#define KERNEL_CPU_H_

/* First include CPU-dependent definitions */

#ifdef __i386__
#include "cpu_i386.h"
#endif
#ifdef __x86_64__
#include "cpu_x86_64.h"
#endif

/* This macro serves as an indicator of included file */
#ifndef PRINT_CPUCONTEXT
#error Unsupported CPU
#endif

#ifdef __AROS__

/* Some common definitions */
#define cpumode_t __unused char

#define goSuper() 0
#define goUser()
#define goBack(mode)

#define AROS_EXCEPTION_SYSCALL 0x00080001
#define AROS_EXCEPTION_RESUME  0x00080002

/* On AROS side we save also LastError code */
struct AROSCPUContext
{
    CONTEXT regs;
    ULONG LastError;
};

struct ExceptionTranslation
{
    ULONG ExceptionCode; /* Windows exception code		       */
    char  AmigaTrap;	 /* m68k trap number for exec.library	       */
    char  CPUTrap;	 /* Native CPU trap number for kernel.resource */
};

extern struct ExceptionTranslation Traps[];

/* I don't want to include too much */
struct CoreInterface;

struct KernelInterface
{
    int (*core_init)(unsigned int TimerPeriod, char *IDNestCnt, struct CoreInterface *CoreIFace);
    void (*core_intr_disable)(void);
    void (*core_intr_enable)(void);
    void (*core_raise)(ULONG num, const IPTR n);
    unsigned char (*core_is_super)(void);
    unsigned int (*core_protect)(void *addr, unsigned int len, unsigned int prot);
    void (*core_putc)(char c);
    int (*core_getc)(void);
    unsigned char *SleepState;
};

extern struct KernelInterface KernelIFace;

#define krnSysCall(n) KernelIFace.core_raise(AROS_EXCEPTION_SYSCALL, n)

#endif

#endif
