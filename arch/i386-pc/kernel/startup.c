#define DEBUG 1
#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>

#include <utility/tagitem.h>

#include <strings.h>
#include <inttypes.h>

#include "kernel_intern.h"

kerncall void start32(struct TagItem *msg);

#if 0
asm(
		".section .aros.init,\"ax\"\n\t"
		".globl start32\n\t"
		".type start32,@function\n"
		"start32:\n\t"
		"movl tmp_stack_end,%esp\n\t"
		"pushl %eax\n\t"
		"call __clear_bss_tags\n\t"
		"popl %eax\n\t"
		"movl stack_end,%esp\n\t"
		"call *target_address\n\t"
);
#endif

