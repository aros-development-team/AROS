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

#include <kernel_base.h>
#include <kernel_debug.h>

asm(".section .aros.init,\"ax\"\n\t"
    ".globl start\n\t"
    ".type start,%function\n"
	"start: ldr r12, 3f				\n"		// Load address of top of stack pointer
	"		ldr sp, [r12]			\n"		// Load temporary stack pointer
	"		mov r4, r0				\n"
	"		bl	clear_bss			\n"		// clear bss regions
	"		mov r0, r4				\n"		// restore boot msg parameter
	"		ldr r12, 4f				\n"		// load supervisor stack
	"		ldr sp, [r12]			\n"
	"		ldr	pc, 2f				\n"		// jump to the kernel
	"1:		b	1b					\n"		// we never return from here
	"2:		.word startup			\n"
	"3:		.word tmp_stack_end		\n"
	"4: 	.word ssp_stack_end		\n"
);

/*
 * Temporary stack for very early init. It's used only during clearing of
 * .bss sections in all modules. Rest of this space is occupied by TagItem
 * boot message. prepared by the bootstrap.
 */
union {
	uint32_t 		stack[128];
	struct TagItem 	tags[64]
} temporary __attribute__((aligned(32),used,section(".data")));

static uint32_t ssp_stack[4096];
static const uint32_t *ssp_stack_end __used = &ssp_stack[4096];
static const uint32_t *tmp_stack_end __used = &temporary.stack[128];

struct TagItem *BootMsg;

static void __used clear_bss(struct TagItem *msg)
{
	struct TagItem *tag = LibFindTagItem(KRN_KernelBss, msg);

	if (tag)
	{
		struct KernelBSS *bss = (struct KernelbSS *)tag->ti_Data;

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
