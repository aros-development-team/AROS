/*
    Copyright (C) 2024-2026, The AROS Development Team. All rights reserved.

    Desc: ELF definitions for AArch64 boot loader
*/

#ifndef ELF_H_
#define ELF_H_

#include <inttypes.h>

struct bss_tracker {
        void *addr;
        unsigned long length;
};

extern struct bss_tracker tracker[];

int getElfSize(void *elf_file, uint32_t *size_rw, uint32_t *size_ro);
void initAllocator(uintptr_t addr_ro, uintptr_t addr_rw, uintptr_t virtoffset);
int loadElf(void *elf_file);

/* AArch64 relocation types */
#define R_AARCH64_NONE          0
#define R_AARCH64_ABS64         257     /* S + A */
#define R_AARCH64_ABS32         258     /* S + A */
#define R_AARCH64_ABS16         259     /* S + A */
#define R_AARCH64_PREL64        260     /* S + A - P */
#define R_AARCH64_PREL32        261     /* S + A - P */
#define R_AARCH64_PREL16        262     /* S + A - P */
#define R_AARCH64_MOVW_UABS_G0         263
#define R_AARCH64_MOVW_UABS_G0_NC      264
#define R_AARCH64_MOVW_UABS_G1         265
#define R_AARCH64_MOVW_UABS_G1_NC      266
#define R_AARCH64_MOVW_UABS_G2         267
#define R_AARCH64_MOVW_UABS_G2_NC      268
#define R_AARCH64_MOVW_UABS_G3         269
#define R_AARCH64_MOVW_SABS_G0         270
#define R_AARCH64_MOVW_SABS_G1         271
#define R_AARCH64_MOVW_SABS_G2         272
#define R_AARCH64_ADR_PREL_LO21        274     /* ADR: S + A - P */
#define R_AARCH64_ADR_PREL_PG_HI21     275     /* ADRP: Page(S+A) - Page(P) */
#define R_AARCH64_ADR_PREL_PG_HI21_NC  276
#define R_AARCH64_ADD_ABS_LO12_NC      277     /* ADD: (S + A) & 0xFFF */
#define R_AARCH64_LDST8_ABS_LO12_NC    278
#define R_AARCH64_LDST16_ABS_LO12_NC   284
#define R_AARCH64_LDST32_ABS_LO12_NC   285
#define R_AARCH64_LDST64_ABS_LO12_NC   286
#define R_AARCH64_LDST128_ABS_LO12_NC  299
#define R_AARCH64_JUMP26        282     /* B: S + A - P */
#define R_AARCH64_CALL26        283     /* BL: S + A - P */
#define R_AARCH64_CONDBR19      280     /* B.cond: S + A - P */

/* ELF64 relocation macros */
#define ELF64_R_SYM(i)         ((uint32_t)((i) >> 32))
#define ELF64_R_TYPE(i)        ((uint32_t)((i) & 0xffffffffULL))

#endif /* ELF_H_ */
