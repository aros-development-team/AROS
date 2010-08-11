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

/* Some common definitions */
#define cpumode_t __unused char

#define goSuper() 0
#define goUser()
#define goBack(mode)

struct ExceptionTranslation
{
    unsigned long ExceptionCode; /* Windows exception code		       */
    char	  AmigaTrap;	 /* m68k trap number for exec.library	       */
    char	  CPUTrap;	 /* Native CPU trap number for kernel.resource */
};

extern struct ExceptionTranslation Traps[];

#endif
