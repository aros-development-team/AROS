/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <utility/tagitem.h>
#include <proto/arossupport.h>
#include <strings.h>
#include <inttypes.h>

#include <string.h>

#include <aros/arm/cpucontext.h>
#include <aros/arm/cpu.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <hardware/sun4i/platform.h>

#define D(x) x

#define _STR(x) #x
#define STR(x) _STR(x)

#define SYS_STACK_SIZE  (16384)

extern void start();

asm("	.section .aros.init,\"ax\"								\n"
"		.globl start											\n"
"		.type start,%function									\n"
"																\n"
"start:															\n"
"		cps		#"STR(CPUMODE_SYSTEM)"							\n"     // Enter System mode
"       ldr		sp, sys_stack_end								\n"     // Load system mode stack
"       cps		#"STR(CPUMODE_IRQ)"								\n"
"       ldr		sp, irq_stack_end								\n"
"       cps		#"STR(CPUMODE_ABORT)"							\n"
"       ldr		sp, abt_stack_end								\n"
"       cps		#"STR(CPUMODE_SUPERVISOR)"						\n"
"       ldr		sp, svc_stack_end								\n"
"       ldr		pc, 2f											\n"     // jump to the kernel
"		b		.												\n"     // we never return from here
"2:     .word startup											\n"
"tmp_stack_end: .word temporary + " STR(127*4)"					\n"
"sys_stack_end: .word sys_stack + " STR((SYS_STACK_SIZE - 4))"	\n"
"svc_stack_end: .word svc_stack + " STR((SYS_STACK_SIZE - 4))"	\n"
"irq_stack_end: .word irq_stack + " STR((SYS_STACK_SIZE - 4))"	\n"
"abt_stack_end: .word abt_stack + " STR((SYS_STACK_SIZE - 4))"	\n"
);

static uintptr_t    early_mmu __attribute__((used, section(".data"))) = 0;

/*
 * Temporary stack for very early init. It's used only during clearing of
 * .bss sections in all modules. Rest of this space is occupied by TagItem
 * boot message. prepared by the bootstrap.
 */
static union {
	uint32_t 		stack[128];
	struct TagItem 	tags[64];
} temporary __attribute__((aligned(32),used,section(".data")));

static uint8_t sys_stack[SYS_STACK_SIZE] __attribute__((used,aligned(16)));
static uint8_t svc_stack[SYS_STACK_SIZE] __attribute__((used,aligned(16)));
static uint8_t abt_stack[SYS_STACK_SIZE] __attribute__((used,aligned(16)));
static uint8_t irq_stack[SYS_STACK_SIZE] __attribute__((used,aligned(16)));

static char  CmdLine[200] __attribute__((used, section(".data")));

__attribute__((section(".data"))) struct ExecBase *SysBase = NULL;

struct TagItem *BootMsg;

static void __used __attribute__((section(".aros.init"))) clear_bss(struct TagItem *msg) {
    struct TagItem *tag = LibFindTagItem(KRN_KernelBss, msg);

    if (tag) {
        struct KernelBSS *bss = (struct KernelBSS *)tag->ti_Data;

        if (bss) {
            __clear_bss(bss);
        }
    }
}

void startup(struct TagItem *tags) {
    bug("\n[KRN] AROS for sun4i (" SUN4I_PLATFORM_NAME ") built on %s starting...\n", __DATE__);
    bug("[KRN] BootMsg @ %08x\n", tags);
    bug("[KRN] Kernel entry @ %08x\n", start);
    bug("[KRN] Kernel entry @ %08x\n", startup);
    bug("[KRN] Early MMU @ %08x\n", early_mmu);

    /* Check if the taglist is copied into safe place */
    if (tags != temporary.tags) {
        /* Nope, copy taglist... */
        struct TagItem *msg = tags;
        struct TagItem *tmp = temporary.tags;

        while(msg->ti_Tag != TAG_DONE) {
            /* Copy the tag */
            *tmp = *msg;

            if (tmp->ti_Tag == KRN_CmdLine) {
                strcpy(CmdLine, (char*) msg->ti_Data);
                tmp->ti_Data = (STACKIPTR) CmdLine;
                D(bug("[KRN] CmdLine: %s\n", tmp->ti_Data));
            } else if (tmp->ti_Tag == KRN_MEMLower) {
                D(bug("[KRN] MemLower: %08x\n", tmp->ti_Data));
            } else if (tmp->ti_Tag == KRN_MEMUpper) {
                D(bug("[KRN] MemUpper: %08x\n", tmp->ti_Data));
            }

            else if (tmp->ti_Tag == KRN_KernelLowest) {
                D(bug("[KRN] KernelLowest: %08x\n", tmp->ti_Data));
            } else if (tmp->ti_Tag == KRN_KernelHighest) {
                D(bug("[KRN] KernelHighest: %08x\n", tmp->ti_Data));
            }

            else if (tmp->ti_Tag == KRN_KernelBss) {
                D(bug("[KRN] kernelBSS: %08x\n", tmp->ti_Data));
            }

            tmp++;
            msg++;
        }
    }

    while(1);
}
