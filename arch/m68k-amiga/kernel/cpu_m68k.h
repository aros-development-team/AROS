/*
 * CPU-specific definitions.
 *
 * Architectures with the same CPU will likely share single kernel_cpu.h
 * in arch/$(CPU)-all/kernel/kernel_cpu.h
 *
 * As you can see, this file is just a sample.
 */

#ifndef CPU_M68K_H_
#define CPU_M68K_H_

#ifndef __AROS_EXEC_LIBRARY__

/* Number of exceptions supported by the CPU. Needed by kernel_base.h */
#define EXCEPTIONS_COUNT 256

#endif

/* CPU context stored in task's iet_Context. */
struct AROSCPUContext
{
	ULONG d[8];	/* Manually saved */
	IPTR  a[8];
	UWORD sr;
	IPTR  pc;	/* Automatically created on entry */
};

typedef struct AROSCPUContext regs_t;

#define GET_PC(cc)	((APTR)(cc)->pc)
#define SET_PC(cc, val)	((cc)->pc = (IPTR)(val))

/*
 * Only Exec/PrepareContext needs this.
 */
#define PREPARE_INITIAL_FRAME(cc, sp, startpc) \
	do { \
		memset(cc, 0, sizeof(*cc)); \
		cc->pc   = startpc; \
		cc->a[7] = sp; \
		cc->sr   = 0x0000; \
		*(ULONG *)(cc->a[7] - 4) = startpc; \
		*(UWORD *)(cc->a[7] - 6) = 0x0000; \
	} while (0)

#endif /* _CPU_M68K_H */
