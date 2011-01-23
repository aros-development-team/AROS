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

/* Number of exceptions supported by the CPU. Needed by kernel_base.h */
#define EXCEPTIONS_COUNT 256

/* CPU context stored in task's iet_Context. */
struct AROSCPUContext
{
	ULONG d[8];	/* Manually saved */
	IPTR  a[8];
	IPTR  trapcode;	/* Trap Code to call as the exception return */
	ULONG traparg;	/* Argument to the TrapCode */
	UWORD sr;
	IPTR  pc;	/* Automatically created on entry */
};

typedef struct AROSCPUContext regs_t;

/*
 * Only Exec/PrepareContext needs this.
 */
#define PREPARE_INITIAL_FRAME(cc, sp, startpc) \
	do { \
		memset(cc, 0, sizeof(*cc)); \
		cc->pc   = (IPTR)startpc; \
		cc->a[7] = (IPTR)sp; \
		cc->sr   = 0x0000; \
	} while (0)

/*
 * Only used by Exec/Debug()
 */
#define PRINT_CPU_CONTEXT(ctx) \
    do { \
        int i; \
        UWORD sr = (ctx)->sr; \
        for (i = 0; i < 8; i++) { \
            bug("D%d: %08x%s", i, (ctx)->d[i], ((i%4) == 3) ? "\n" : " "); \
        } \
        for (i = 0; i < 8; i++) { \
            bug("A%d: %08x%s", i, (ctx)->a[i], ((i%4) == 3) ? "\n" : " "); \
        } \
        bug("SR: T=%02d S=%d M=%d X=%d N=%d Z=%d V=%d C=%d IMASK=%d\n", \
                (sr >> 14) & 3, (sr >> 13) & 1, (sr >> 5) & 1, \
                (sr >>  4) & 1, (sr >>  3) & 1, (sr >> 2) & 1, \
                (sr >>  1) & 1, (sr >>  0) & 1, (sr >> 8) & 7); \
        bug("PC: %08x\n", (ctx)->pc); \
    } while (0)


#endif /* _CPU_M68K_H */
