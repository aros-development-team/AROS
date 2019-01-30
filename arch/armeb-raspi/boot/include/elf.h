/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef ELF_H_
#define ELF_H_

#include <inttypes.h>

struct bss_tracker {
        void *addr;
        unsigned int length;
};

extern struct bss_tracker tracker[];

int getElfSize(void *elf_file, uint32_t *size_rw, uint32_t *size_ro);
void initAllocator(uintptr_t addr_ro, uintptr_t addr_rw, uintptr_t virtoffset);
int loadElf(void *elf_file);

#define ELF32_R_SYM(val)        ((val) >> 8)
#define ELF32_R_TYPE(val)       ((val) & 0xff)
#define ELF32_R_INFO(sym, type) (((sym) << 8) + ((type) & 0xff))

#endif /* ELF_H_ */
