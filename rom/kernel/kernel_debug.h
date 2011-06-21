/*
 * This file contains useful redefinition of bug() macro which uses
 * kernel.resource's own debugging facilities. Include it if you
 * need bug() in your code.
 */

#include <aros/asmcall.h>
#include <aros/libcall.h>
#include <stdarg.h>

#include <proto/kernel.h>

#ifdef bug
#undef bug
#endif

int __KrnBugBoot(format, args);

static inline void _bug(struct KernelBase *KernelBase, const char *format, ...)
{
    va_list args;

    va_start(args, format);

    /* KernelBase can be NULL, use __KrnBugBoot if it is */
    if (KernelBase != NULL)
        KrnBug(format, args);
    else
        __KrnBugBoot(format, args);

    va_end(args);
}

#define bug(...) _bug(KernelBase, __VA_ARGS__)

/*
 * Character output function. All debug output ends up there.
 * This function needs to be implemented for every supported
 * architecture.
 */
int krnPutC(int chr, struct KernelBase *KernelBase);
