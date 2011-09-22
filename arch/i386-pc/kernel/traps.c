/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <asm/segments.h>
#include <asm/linkage.h>
#include <asm/ptrace.h>
#include <exec/alerts.h>
#include <proto/exec.h>

#include "cpu_traps.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "traps.h"
#include "exec_extern.h"

/* 0,1,5-7,9-17,19:
		return address of these exceptions is the address of faulting instr
   1,3,4:
		return address is address of instruction followed by trapping instr
		(1 can be FAULT and TRAP)
	others:
		ABORT = ??? (no information = no return address)
*/

BUILD_TRAP(0x00)
BUILD_TRAP(0x01)
BUILD_TRAP(0x02)
BUILD_TRAP(0x03)
BUILD_TRAP(0x04)
BUILD_TRAP(0x05)
BUILD_TRAP(0x06)
BUILD_TRAP(0x07)
BUILD_TRAP(0x08)
BUILD_TRAP(0x09)
BUILD_TRAP(0x0a)
BUILD_TRAP(0x0b)
BUILD_TRAP(0x0c)
BUILD_TRAP(0x0d)
BUILD_TRAP(0x0e)
BUILD_TRAP(0x0f)
BUILD_TRAP(0x10)
BUILD_TRAP(0x11)
BUILD_TRAP(0x12)
BUILD_TRAP(0x13)

typedef void (*trap_type)(void);

const trap_type traps[0x14] =
{
	TRAP0x00_trap,
	TRAP0x01_trap,
	TRAP0x02_trap,
	TRAP0x03_trap,
	TRAP0x04_trap,
	TRAP0x05_trap,
	TRAP0x06_trap,
	TRAP0x07_trap,
	TRAP0x08_trap,
	TRAP0x09_trap,
	TRAP0x0a_trap,
	TRAP0x0b_trap,
	TRAP0x0c_trap,
	TRAP0x0d_trap,
	TRAP0x0e_trap,
	TRAP0x0f_trap,
	TRAP0x10_trap,
	TRAP0x11_trap,
	TRAP0x12_trap,
	TRAP0x13_trap
};

#define _set_gate(gate_addr,type,dpl,addr) \
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

void set_intr_gate(unsigned int n, void *addr)
{
    struct PlatformData *data = KernelBase->kb_PlatformData;
 
    _set_gate(&data->idt[n], 14, 0, addr);
}

void set_system_gate(unsigned int n, void *addr)
{
    struct PlatformData *data = KernelBase->kb_PlatformData;

    _set_gate(&data->idt[n], 14, 3, addr);
}

void handleException(struct ExceptionContext *ctx, unsigned long error_code, unsigned long irq_number)
{
    /* Currently we support only traps here */
    cpu_Trap(ctx, error_code, irq_number);
}

void Init_Traps(struct PlatformData *data)
{
    int i;

    for (i = 0; i < 20; i++)
    {
	_set_gate(&data->idt[i], 14, 0, traps[i]);
    }
    /* Set all unused vectors to dummy interrupt */
    for (i = 20; i < 256; i++)
    {
	_set_gate(&data->idt[i], 14, 0, core_Unused_Int);
    }

    /* Create user interrupt used to enter supervisor mode */
    _set_gate(&data->idt[0x80], 14, 3, Exec_SystemCall);
}
