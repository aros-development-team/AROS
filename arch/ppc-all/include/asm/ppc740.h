#ifndef ASM_PPC_PPC740_H
#define ASM_PPC_PPC740_H

#include <asm/cpu.h>

/* Machine State Register */
#define MSR_LE  0x80000000
#define MSR_RI  0x40000000
#define MSR_PM  0x20000000
#define MSR_DR  0x08000000
#define MSR_IR  0x04000000
#define MSR_IP  0x02000000
#define MSR_FE1 0x00800000
#define MSR_BE  0x00400000
#define MSR_SE  0x00200000
#define MSR_FE0 0x00100000
#define MSR_ME  0x00080000
#define MSR_FP  0x00040000
#define MSR_PR  0x00020000
#define MST_EE  0x00010000
#define MSR_ILE 0x00008000
#define MSR_POW 0x00002000

#endif
