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

AROS_LD2(int, KrnBug,
	AROS_LDA(const char *, format, A0),
	AROS_LDA(va_list, args, A1),
	struct KernelBase *, KernelBase, 12, Kernel);

/*
 * Character output function. All debug output ends up there.
 * This function needs to be implemented for every supported
 * architecture.
 */
int krnPutC(int chr, struct KernelBase *KernelBase);

static inline void _bug(struct KernelBase *KernelBase, const char *format, ...)
{
    va_list args;

    va_start(args, format);

    /* 
     * We use AROS_CALL2 here, since there are files that
     * include this that cannot tolerate <proto/kernel.h>
     * Note that this is a direct call, not using an LVO. This is done
     * in such a manner because this function can be user during early boot,
     * while KernelBase is NULL.
     */
    AROS_CALL2(int, AROS_SLIB_ENTRY(KrnBug, Kernel, 12),
               AROS_LCA(const char *, format, A0),
               AROS_LCA(va_list, args, A1),
               struct KernelBase *, KernelBase);

    va_end(args);
}

#define bug(...) _bug(KernelBase, __VA_ARGS__)
