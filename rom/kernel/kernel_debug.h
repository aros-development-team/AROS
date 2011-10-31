/*
 * This file contains useful redefinition of bug() macro which uses
 * kernel.resource's own debugging facilities. Include it if you
 * need bug() in your code.
 */

#include <aros/asmcall.h>
#include <aros/libcall.h>
#include <stdarg.h>

#ifdef bug
#undef bug
#endif

int krnPutC(int chr, struct KernelBase *KernelBase);
int krnBug(const char *format, va_list args, APTR kernelBase);
void krnDisplayAlert(const char *text, struct KernelBase *KernelBase);
void krnPanic(struct KernelBase *KernelBase, const char *fmt, ...);

static inline void _bug(APTR kernelBase, const char *format, ...)
{
    va_list args;

    va_start(args, format);

    /* 
     * We use iternal entry here. This is done because this function can be used
     * during early boot, while KernelBase is NULL. However it's still passed,
     * just in case.
     */
    krnBug(format, args, kernelBase);

    va_end(args);
}

#define bug(...) _bug(KernelBase, __VA_ARGS__)
