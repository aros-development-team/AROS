#ifndef ASM_MPC5200B_H
#define ASM_MPC5200B_H

#include <inttypes.h>

typedef struct regs {
    uint32_t    gpr[32];
    uint32_t    srr0;
    uint32_t    srr1;
    uint32_t    ctr;
    uint32_t    lr;
    uint32_t    xer;
    uint32_t    ccr;
    uint32_t    dar;
    uint32_t    dsisr;
} regs_t;

typedef struct fpuregs {
    double      fpr[32];
    uint32_t    fpscr;
} fpuregs_t;

typedef struct context {
    regs_t      cpu;
    fpuregs_t   fpu;
} context_t;

#define SIZEOF_ALL_REGISTERS (sizeof(context_t))

static inline uint32_t rdmsr() {
    uint32_t msr; asm volatile("mfmsr %0":"=r"(msr)); return msr;
}

static inline void wrmsr(uint32_t msr) {
    asm volatile("mtmsr %0"::"r"(msr));
}

/* Machine State Register */
#define MSR_POW 0x00040000
#define MSR_CE  0x00020000
#define MSR_EE  0x00008000
#define MSR_PR  0x00004000
#define MSR_FP  0x00002000
#define MSR_ME  0x00001000
#define MSR_FE0 0x00000800
#define MSR_DWE 0x00000400
#define MSR_DE  0x00000200
#define MSR_FE1 0x00000100
#define MSR_IS  0x00000020
#define MSR_DS  0x00000010

#define rdspr(reg) \
    ({ unsigned long val; asm volatile("mfspr %0,%1":"=r"(val):"i"(reg)); val; })

#define wrspr(reg, val) \
    do { asm volatile("mtspr %0,%1"::"i"(reg),"r"(val)); } while(0)

/* SPR registers */
#define XER     0x001   /* Integer Exception Register */
#define LR      0x008   /* Link Register */
#define CTR     0x009   /* Count Register */
#define DEC     0x016   /* Decrementer */
#define SRR0    0x01A   /* Save/Restore Register 0 */
#define SRR1    0x01B   /* Save/Restore Register 1 */
#define TBLU    0x10C   /* Time Base Lower */
#define TBUU    0x10D   /* Time Base Upper */
#define SPRG0   0x110   /* Special Purpose Register General 0 */
#define SPRG1   0x111   /* Special Purpose Register General 1 */
#define SPRG2   0x112   /* Special Purpose Register General 2 */
#define SPRG3   0x113   /* Special Purpose Register General 3 */
#define SPRG4   0x114   /* Special Purpose Register General 4 */
#define SPRG5   0x115   /* Special Purpose Register General 5 */
#define SPRG6   0x116   /* Special Purpose Register General 6 */
#define SPRG7   0x117   /* Special Purpose Register General 7 */

#define DMISS	0x3d0
#define DCMP	0x3d1
#define HASH1	0xd32
#define HASH2	0x3d3
#define IMISS	0x3d4
#define ICMP	0x3d5
#define RPA		0x3d6

#endif /* ASM_MPC5200B_H */
