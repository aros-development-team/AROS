/*
 * CPU-specific definitions.
 *
 * Architectures with the same CPU will likely share single kernel_cpu.h
 * in arch/$(CPU)-all/kernel/kernel_cpu.h
 *
 * As you can see, this file is just a sample.
 */

#ifndef KERNEL_CPU_H_
#define KERNEL_CPU_H_

#include <aros/i386/cpucontext.h>

/*
 * We handle all 255 exception vectors. However vectors starting from 0x20
 * are hardware IRQs which are handled separately. So - 32 raw exceptions.
 */
#define EXCEPTIONS_COUNT 32

/* We use native context format, no conversion needed */
#define regs_t struct ExceptionContext
/* There are no private add-ons */
#define AROSCPUContext ExceptionContext

/* We have no VBlank emulation */
#define NO_VBLANK_EMU

/* User/supervisor mode switching - not used */
#define cpumode_t __unused char

#define goSuper() 0
#define goUser()
#define goBack(mode)

/* A command to issue a syscall */
#define krnSysCall(num) asm volatile("int $0x80"::"a"(num):"memory")

#define IN_USER_MODE \
	({  short __value; \
	__asm__ __volatile__ ("mov %%cs,%%ax":"=a"(__value)); \
	(__value & 0x03);	})

#endif
