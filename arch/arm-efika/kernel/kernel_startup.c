/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <utility/tagitem.h>
#include <proto/arossupport.h>
#include <strings.h>
#include <inttypes.h>

#include <aros/arm/cpucontext.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#define _STR(x) #x
#define STR(x) _STR(x)

#define SYS_STACK_SIZE  (16384)

asm(".section .aros.init,\"ax\"\n\t"
    ".globl start\n\t"
    ".type start,%function\n"
    "start: ldr sp, tmp_stack_end   \n"     // Load address of top of stack pointer
    "       mov r4, r0              \n"
    "       bl  clear_bss           \n"     // clear bss regions
    "       mov r0, r4              \n"     // restore boot msg parameter
    "       cps #" STR(CPUMODE_SYSTEM) "\n"     // Enter System mode
    "       ldr sp, sys_stack_end   \n"     // Load system mode stack
    "       cps #" STR(CPUMODE_IRQ) "\n"
    "       ldr sp, irq_stack_end   \n"
    "       cps #" STR(CPUMODE_ABORT) "\n"
    "       ldr sp, abt_stack_end   \n"
    "       cps #" STR(CPUMODE_SUPERVISOR) "\n"
    "       ldr sp, svc_stack_end   \n"
    "       ldr pc, 2f              \n"     // jump to the kernel
    "1:     b   1b                  \n"     // we never return from here
    "2:     .word startup           \n"
    "tmp_stack_end: .word temporary + " STR(127*4) "\n"
    "sys_stack_end: .word sys_stack + " STR((SYS_STACK_SIZE - 4)) "\n"
    "svc_stack_end: .word svc_stack + " STR((SYS_STACK_SIZE - 4)) "\n"
    "irq_stack_end: .word irq_stack + " STR((SYS_STACK_SIZE - 4)) "\n"
    "abt_stack_end: .word abt_stack + " STR((SYS_STACK_SIZE - 4)) "\n"
);

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

//static uint32_t * const sys_stack_end __attribute__((used,section(".aros.init"))) = &sys_stack[SYS_STACK_SIZE-1];
//static uint32_t * const svc_stack_end __attribute__((used,section(".aros.init"))) = &svc_stack[SYS_STACK_SIZE-1];
//static uint32_t * const abt_stack_end __attribute__((used,section(".aros.init"))) = &abt_stack[SYS_STACK_SIZE-1];
//static uint32_t * const irq_stack_end __attribute__((used,section(".aros.init"))) = &irq_stack[SYS_STACK_SIZE-1];

//static uint32_t * const tmp_stack_end __attribute__((used,section(".aros.init"))) = &temporary.stack[128 - 1];

struct TagItem *BootMsg;

static void __used __attribute__((section(".aros.init"))) clear_bss(struct TagItem *msg)
{
	struct TagItem *tag = LibFindTagItem(KRN_KernelBss, msg);

	if (tag)
	{
		struct KernelBSS *bss = (struct KernelBSS *)tag->ti_Data;

		if (bss)
		{
			__clear_bss(bss);
		}
	}
}

void startup(struct TagItem *tags)
{
	bug("\n[KRN] AROS for EfikaMX built on %s starting...\n", __DATE__);
	bug("[KRN] BootMsg @ %08x\n", tags);

	while(1);
}
