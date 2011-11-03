#ifndef KERNEL_CPU_H_
#define KERNEL_CPU_H_

/* First include CPU-dependent definitions */

#ifdef __i386__
#include <cpu_i386.h>
#endif
#ifdef __ppc__
#include <cpu_ppc.h>
#endif
#ifdef __arm__
#include <cpu_arm.h>
#endif
#ifdef __x86_64__
#include <cpu_x86_64.h>
#endif

/* This macro serves as an indicator of included file */
#ifndef EXCEPTIONS_COUNT
#error Unsupported CPU
#endif

/* Some common definitions */
#define cpumode_t __unused char

#define goSuper() 0
#define goUser()
#define goBack(mode)

void unix_SysCall(unsigned char n, struct KernelBase *KernelBase);
#define krnSysCall(n) unix_SysCall(n, KernelBase)

#endif
