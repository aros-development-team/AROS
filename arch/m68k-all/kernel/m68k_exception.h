/*
    copyright © 1995-2010, the aros development team. all rights reserved.
    $id$

    desc: M68K Exceptions
    lang: english
 */
#ifndef M68K_EXCEPTION_H
#define M68K_EXCEPTION_H

#include <aros/kernel.h>
#include <exec/execbase.h>

#include <kernel_cpu.h>

/* Here's how it's all laid out:
 *    M68K Exception
 *      0	Reset: Initial SP
 *      1	Reset: Initial PC (NOTE: Really is SysBase!)
 *      2	Bus Error
 *      3	Address Error
 *      4	Illegal Instruction
 *      5	Divide by Zero
 *      6	CHK Instruction
 *      7	TRAPV Instruction
 *      8	Privileged Instruction
 *      9	Trace
 *      10	Line 1010 Emulator
 *      11	Line 1111 Emulator
 *      12	-
 *      13	-
 *      14	Format Error
 *      15	Uninitilaized Interrupt Vector
 *      16	-
 *      ..
 *      23	-
 *      24	Spurious Interrupt
 *      25	Level 1 Interrupt
 *      26	Level 2 Interrupt
 *      27	Level 3 Interrupt
 *      28	Level 4 Interrupt
 *      29	Level 5 Interrupt
 *      30	Level 6 Interrupt
 *      31	Level 7 Interrupt
 *      32	TRAP #0
 *      ..
 *      47	TRAP #15
 *      48	-
 *      ..
 *      63	-
 *      64	User 1
 *      ..
 *      255	User 191
 */
struct M68KException {
	UWORD Id;
	/* tc_TrapCode style handler - MUST PRESERVE ALL REGISTERS! */
	void (*Handler)(ULONG id);
};

void M68KExceptionInit(const struct M68KException *Table, struct ExecBase *SysBase);

#endif /* M68K_EXCEPTION_H */
