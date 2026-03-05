/*
 * Copyright (c) 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (C) 2005-2026 The AROS Dev Team
 *
 * This code is derived from the Stanford/CMU enet packet filter,
 * (net/enet.c) distributed as part of 4.3BSD, and code contributed
 * to Berkeley by Steven McCanne and Van Jacobson both of Lawrence
 * Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.
 *
 * @(#)bpf_filter.c	8.1 (Berkeley) 5/21/93
 */

#include <conf.h>
#include <sys/param.h>
#include <sys/types.h>
#include <net/bpf.h>

#define EXTRACT_SHORT(p) \
	((u_int16_t) \
	 ((u_int16_t)*((const u_char *)(p) + 0) << 8 | \
	  (u_int16_t)*((const u_char *)(p) + 1)))

#define EXTRACT_LONG(p) \
	((u_int32_t) \
	 ((u_int32_t)*((const u_char *)(p) + 0) << 24 | \
	  (u_int32_t)*((const u_char *)(p) + 1) << 16 | \
	  (u_int32_t)*((const u_char *)(p) + 2) <<  8 | \
	  (u_int32_t)*((const u_char *)(p) + 3)))

/*
 * Execute the filter program starting at pc on the packet p
 * wirelen is the length of the original packet
 * buflen is the amount of data present
 *
 * Returns the return value of the filter program (0 = reject,
 * non-zero = number of bytes to accept).
 */
u_int
bpf_filter(const struct bpf_insn *pc, const u_char *p,
           u_int wirelen, u_int buflen)
{
    u_int32_t A = 0, X = 0;
    u_int32_t mem[BPF_MEMWORDS];
    int k;

    if(pc == NULL)
        return (u_int) - 1;	/* no filter means accept all */

    --pc;
    while(1) {
        ++pc;
        switch(pc->code) {

        default:
            return 0;

        /* RET instructions */
        case BPF_RET|BPF_K:
            return (u_int)pc->k;

        case BPF_RET|BPF_A:
            return (u_int)A;

        /* LD instructions: load into accumulator */
        case BPF_LD|BPF_W|BPF_ABS:
            k = pc->k;
            if(k + 4 > (int)buflen)
                return 0;
            A = EXTRACT_LONG(&p[k]);
            continue;

        case BPF_LD|BPF_H|BPF_ABS:
            k = pc->k;
            if(k + 2 > (int)buflen)
                return 0;
            A = EXTRACT_SHORT(&p[k]);
            continue;

        case BPF_LD|BPF_B|BPF_ABS:
            k = pc->k;
            if(k >= (int)buflen)
                return 0;
            A = p[k];
            continue;

        case BPF_LD|BPF_W|BPF_LEN:
            A = wirelen;
            continue;

        case BPF_LD|BPF_W|BPF_IND:
            k = X + pc->k;
            if(k + 4 > (int)buflen || k < 0)
                return 0;
            A = EXTRACT_LONG(&p[k]);
            continue;

        case BPF_LD|BPF_H|BPF_IND:
            k = X + pc->k;
            if(k + 2 > (int)buflen || k < 0)
                return 0;
            A = EXTRACT_SHORT(&p[k]);
            continue;

        case BPF_LD|BPF_B|BPF_IND:
            k = X + pc->k;
            if(k >= (int)buflen || k < 0)
                return 0;
            A = p[k];
            continue;

        case BPF_LD|BPF_IMM:
            A = pc->k;
            continue;

        case BPF_LD|BPF_MEM:
            A = mem[pc->k];
            continue;

        /* LDX instructions: load into index register */
        case BPF_LDX|BPF_W|BPF_IMM:
            X = pc->k;
            continue;

        case BPF_LDX|BPF_W|BPF_MEM:
            X = mem[pc->k];
            continue;

        case BPF_LDX|BPF_W|BPF_LEN:
            X = wirelen;
            continue;

        case BPF_LDX|BPF_B|BPF_MSH:
            k = pc->k;
            if(k >= (int)buflen)
                return 0;
            X = (p[k] & 0xf) << 2;
            continue;

        /* ST/STX: store accumulator/index into scratch memory */
        case BPF_ST:
            mem[pc->k] = A;
            continue;

        case BPF_STX:
            mem[pc->k] = X;
            continue;

        /* ALU instructions */
        case BPF_ALU|BPF_ADD|BPF_X:
            A += X;
            continue;

        case BPF_ALU|BPF_SUB|BPF_X:
            A -= X;
            continue;

        case BPF_ALU|BPF_MUL|BPF_X:
            A *= X;
            continue;

        case BPF_ALU|BPF_DIV|BPF_X:
            if(X == 0)
                return 0;
            A /= X;
            continue;

        case BPF_ALU|BPF_MOD|BPF_X:
            if(X == 0)
                return 0;
            A %= X;
            continue;

        case BPF_ALU|BPF_AND|BPF_X:
            A &= X;
            continue;

        case BPF_ALU|BPF_OR|BPF_X:
            A |= X;
            continue;

        case BPF_ALU|BPF_XOR|BPF_X:
            A ^= X;
            continue;

        case BPF_ALU|BPF_LSH|BPF_X:
            A <<= X;
            continue;

        case BPF_ALU|BPF_RSH|BPF_X:
            A >>= X;
            continue;

        case BPF_ALU|BPF_ADD|BPF_K:
            A += pc->k;
            continue;

        case BPF_ALU|BPF_SUB|BPF_K:
            A -= pc->k;
            continue;

        case BPF_ALU|BPF_MUL|BPF_K:
            A *= pc->k;
            continue;

        case BPF_ALU|BPF_DIV|BPF_K:
            A /= pc->k;
            continue;

        case BPF_ALU|BPF_MOD|BPF_K:
            A %= pc->k;
            continue;

        case BPF_ALU|BPF_AND|BPF_K:
            A &= pc->k;
            continue;

        case BPF_ALU|BPF_OR|BPF_K:
            A |= pc->k;
            continue;

        case BPF_ALU|BPF_XOR|BPF_K:
            A ^= pc->k;
            continue;

        case BPF_ALU|BPF_LSH|BPF_K:
            A <<= pc->k;
            continue;

        case BPF_ALU|BPF_RSH|BPF_K:
            A >>= pc->k;
            continue;

        case BPF_ALU|BPF_NEG:
            A = (u_int32_t)(-(int32_t)A);
            continue;

        /* JMP instructions */
        case BPF_JMP|BPF_JA:
            pc += pc->k;
            continue;

        case BPF_JMP|BPF_JGT|BPF_K:
            pc += (A > pc->k) ? pc->jt : pc->jf;
            continue;

        case BPF_JMP|BPF_JGE|BPF_K:
            pc += (A >= pc->k) ? pc->jt : pc->jf;
            continue;

        case BPF_JMP|BPF_JEQ|BPF_K:
            pc += (A == pc->k) ? pc->jt : pc->jf;
            continue;

        case BPF_JMP|BPF_JSET|BPF_K:
            pc += (A & pc->k) ? pc->jt : pc->jf;
            continue;

        case BPF_JMP|BPF_JGT|BPF_X:
            pc += (A > X) ? pc->jt : pc->jf;
            continue;

        case BPF_JMP|BPF_JGE|BPF_X:
            pc += (A >= X) ? pc->jt : pc->jf;
            continue;

        case BPF_JMP|BPF_JEQ|BPF_X:
            pc += (A == X) ? pc->jt : pc->jf;
            continue;

        case BPF_JMP|BPF_JSET|BPF_X:
            pc += (A & X) ? pc->jt : pc->jf;
            continue;

        /* MISC instructions */
        case BPF_MISC|BPF_TAX:
            X = A;
            continue;

        case BPF_MISC|BPF_TXA:
            A = X;
            continue;
        }
    }
}

/*
 * Validate a BPF filter program.
 * Returns 1 if valid, 0 if invalid.
 *
 * The kernel relies on this to verify that a filter loaded by userspace
 * will not crash the system.
 */
int
bpf_validate(const struct bpf_insn *f, int len)
{
    int i;

    if(len < 1 || len > BPF_MAXINSNS)
        return 0;

    for(i = 0; i < len; ++i) {
        const struct bpf_insn *p = &f[i];

        switch(BPF_CLASS(p->code)) {
        case BPF_LD:
        case BPF_LDX:
            switch(BPF_MODE(p->code)) {
            case BPF_IMM:
                break;
            case BPF_ABS:
            case BPF_IND:
            case BPF_MSH:
                break;
            case BPF_MEM:
                if(p->k >= BPF_MEMWORDS)
                    return 0;
                break;
            case BPF_LEN:
                break;
            default:
                return 0;
            }
            break;
        case BPF_ST:
        case BPF_STX:
            if(p->k >= BPF_MEMWORDS)
                return 0;
            break;
        case BPF_ALU:
            switch(BPF_OP(p->code)) {
            case BPF_ADD:
            case BPF_SUB:
            case BPF_MUL:
            case BPF_OR:
            case BPF_AND:
            case BPF_XOR:
            case BPF_LSH:
            case BPF_RSH:
            case BPF_MOD:
            case BPF_NEG:
                break;
            case BPF_DIV:
                if(BPF_SRC(p->code) == BPF_K && p->k == 0)
                    return 0;
                break;
            default:
                return 0;
            }
            break;
        case BPF_JMP:
            switch(BPF_OP(p->code)) {
            case BPF_JA:
                if((int)p->k >= len - i - 1)
                    return 0;
                break;
            case BPF_JEQ:
            case BPF_JGT:
            case BPF_JGE:
            case BPF_JSET:
                if(p->jt >= len - i - 1 ||
                        p->jf >= len - i - 1)
                    return 0;
                break;
            default:
                return 0;
            }
            break;
        case BPF_RET:
            break;
        case BPF_MISC:
            switch(BPF_MISCOP(p->code)) {
            case BPF_TAX:
            case BPF_TXA:
                break;
            default:
                return 0;
            }
            break;
        default:
            return 0;
        }
    }

    /* Last instruction must be a RET */
    return (BPF_CLASS(f[len - 1].code) == BPF_RET);
}
