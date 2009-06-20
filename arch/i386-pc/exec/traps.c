/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <asm/segments.h>
#include <asm/linkage.h>
#include <asm/ptrace.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include "traps.h"

#define __text __attribute__((section(".text")))

BUILD_COMMON_TRAP()

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

const void (*traps[0x14])(void) __text =
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

static const struct { long long a; } *idt_base = (struct { long long a; } *)0x100;

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
	_set_gate(idt_base+n,14,0,addr);
}

void set_system_gate(unsigned int n, void *addr)
{
	_set_gate(idt_base+n,14,3,addr);
}

void printException(struct pt_regs regs)
{
    kprintf("*** trap: eip = %x eflags = %04x  ds = %x sp ~= %x\n",
        regs.eip, regs.eflags, regs.xds, &regs.esp);
}

void handleException(ULONG exceptionNo)
{
    ULONG alert;
    struct Task *task;
    VOID (*trapHandler)(ULONG);

    // Determine alert number
    switch (exceptionNo)
    {
    case 0:
        alert = ACPU_DivZero;
        break;
    case 6:
        alert = ACPU_InstErr;
        break;
    default:
        alert = AT_DeadEnd | 0x100 | exceptionNo;
    }

    // Call task's trap handler
    task = FindTask(NULL);
    trapHandler = task->tc_TrapCode;
    trapHandler(alert);
}

void Init_Traps(void) {
int i;

	for (i=0;i<20;i++)
	{
		_set_gate(idt_base+i,14,0,traps[i]);
	}
}
