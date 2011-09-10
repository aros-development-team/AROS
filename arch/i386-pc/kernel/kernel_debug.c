#include <bootconsole.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"

#define __save_flags(x)		__asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */)
#define __restore_flags(x) 	__asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory", "cc")
#define __cli() 		__asm__ __volatile__("cli": : :"memory")
#define __sti()			__asm__ __volatile__("sti": : :"memory")

int krnPutC(int c, struct KernelBase *KernelBase)
{
    unsigned long flags;

    __save_flags(flags);

    /*
     * stegerg: Don't use Disable/Enable, because we want  interrupt enabled flag
     * to stay the same as it was before the Disable() call
     */
    __cli();

    con_Putc(c);

    /*
     * Interrupt flag is stored in flags - if it was enabled before,
     * it will be renabled when the flags are restored
     */
    __restore_flags(flags);

    return 1;
}
