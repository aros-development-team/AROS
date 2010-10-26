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
		*(ULONG *)(_sp - (4)) = (ULONG)(startpc); \
		*(UWORD *)(_sp - (4 + 2)) = 0x0009; \
	} while (0)

#endif /* _KERNEL_CPU_H */
