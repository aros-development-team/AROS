#include <bootconsole.h>
#include <stdarg.h>

/* This comes from librom.a */
int __vcformat(void *data, int(*outc)(int, void *), const char * format, va_list args);

#define __save_flags(x)		__asm__ __volatile__("pushfq ; popq %0":"=g" (x): /* no input */)
#define __restore_flags(x) 	__asm__ __volatile__("pushq %0 ; popfq": /* no output */ :"g" (x):"memory", "cc")
#define __cli() 		__asm__ __volatile__("cli": : :"memory")
#define __sti()			__asm__ __volatile__("sti": : :"memory")

static int kputc(int c, void *data)
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

int bug(const char *format, ...)
{
    va_list ap;
    int res;

    va_start(ap, format);
    res = __vcformat(0, kputc, format, ap);
    va_end(ap);

    return res;
}
