#ifndef KERNEL_DEBUG_XMM_H
#define KERNEL_DEBUG_XMM_H
/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
*/
#include <exec/types.h>

#define DEBUG_XMM 0

#if DEBUG_XMM
extern UBYTE *pseudorsp;
#define SAVE_XMM_INTO_AREA(area)                \
    asm volatile (                              \
        "       movaps %%xmm0, (%0)\n"          \
        "       movaps %%xmm1, 16(%0)\n"        \
        "       movaps %%xmm2, 32(%0)\n"        \
        "       movaps %%xmm3, 48(%0)\n"        \
        "       movaps %%xmm4, 64(%0)\n"        \
        "       movaps %%xmm5, 80(%0)\n"        \
        "       movaps %%xmm6, 96(%0)\n"        \
        "       movaps %%xmm7, 112(%0)\n"       \
        ::"r"(area));

#define SAVE_XMM_AND_CHECK                      \
UQUAD xmmpost[16] __attribute__((aligned(16))); \
SAVE_XMM_INTO_AREA(xmmpost)                     \
UQUAD *xmmpre = (UQUAD *)localarea;             \
for (int i = 0; i < 15; i++)                    \
    if (xmmpre[i] != xmmpost[i]) bug("diff in %s (%d) %lx vs %lx!!\n", __func__, i, xmmpre[i], xmmpost[i]);

#define PSEUDOSTACK_POPFRAME                    \
pseudorsp -= 16 * 8;

#define PSEUDOSTACK_MAKEFRAME                   \
pseudorsp += 16 * 8;

#define DEFINEPSEUDOSTACK                       \
/* Pseudo-stack to support nesting */           \
UBYTE pseudostack[16 * 8 * 8 + 15];             \
UBYTE *pseudorsp = NULL;

#define CREATEPSEUDOSTACK                       \
if (pseudorsp == NULL)                          \
    pseudorsp = (UBYTE *)AROS_ROUNDUP2((IPTR)pseudostack, 16);  \

#define SETLOCALAREA                            \
APTR localarea = pseudorsp - (16 * 8);

#endif


#endif /* KERNEL_DEBUG_XMM_H */
