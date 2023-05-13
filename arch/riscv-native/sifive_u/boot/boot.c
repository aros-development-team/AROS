/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: boot.c
 */

#include <exec/types.h>
#include <aros/macros.h>
#include <inttypes.h>
#include <asm/cpu.h>
#include <utility/tagitem.h>
#include <aros/macros.h>
#include <aros/kernel.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

asm("   .section .aros.startup      \n"
"       .globl bootstrap            \n"
"       .type bootstrap,%function   \n"
"bootstrap:                         \n"
"       jal       boot              \n"
"       .section .text              \n"
".byte 0                            \n"
".string \"$VER: aros-sifive-riscv.img v40.0 (" __DATE__ ")\"\n"
".byte 0                            \n"
"\n\t\n\t"
);

// The bootstrap tmp stack is re-used by the reset handler so we store it at this fixed location
static __used void * tmp_stack_ptr __attribute__((used, section(".aros.startup" TARGET_SECTION_COMMENT))) = (void *)(0x1000 - 16);
static struct TagItem *boottag;
static unsigned long *mem_upper;
struct tag;

static const char bootstrapName[] = "Bootstrap/RISC-V 32bit";

void boot(uintptr_t dummy, uintptr_t arch, struct tag * atags, uintptr_t a)
{
    uint32_t tmp, initcr;
    void (*entry)(struct TagItem *);

    while(1) asm volatile("wfi");
}
