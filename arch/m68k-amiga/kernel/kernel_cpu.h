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
 
/* Number of exceptions supported by the CPU. Needed by kernel_base.h */
#define EXCEPTIONS_COUNT 1

/* CPU context stored in task's iet_Context. Just a dummy sample definition. */
struct AROSCPUContext
{
	ULONG pc;
};

typedef struct AROSCPUContext regs_t;

/* User/supervisor mode switching */
#define cpumode_t __unused UWORD

/* Only used on protected memory systems. On the
 * m68k-amiga, these are unneeded.
 */
#define goSuper() (0)
#define goUser()  do { } while (0)
#define goBack(mode)  do { } while (0)
		                   

/*
 * Only Exec/PrepareContext needs this file.
 */

#define PREPARE_INITIAL_FRAME(cc, sp, startpc) \
	do { \
		void *_sp = (sp); \
		_sp -= 4; \
		*(ULONG *)(_sp) = (ULONG)(startpc); \
		_sp -= 2; \
		*(UWORD *)(_sp) = 0x0000; \
	} while (0)

#endif /* _KERNEL_CPU_H */
