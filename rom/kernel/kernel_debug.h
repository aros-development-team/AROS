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

int __KrnBugBoot(const char *format, va_list args);

AROS_LD2(int, KrnBug,
	AROS_LDA(const char *, format, A0),
	AROS_LDA(va_list, args, A1),
	struct KernelBase *, KernelBase, 12, Kernel);

static inline void _bug(struct KernelBase *KernelBase, const char *format, ...)
{
    va_list args;

    va_start(args, format);

    /* KernelBase can be NULL, use __KrnBugBoot if it is */
    if (KernelBase != NULL) {
    	/* We use AROS_CALL2 here, since there are files that
    	 * include this that cannot tolerate <proto/kernel.h>
    	 */
    	AROS_CALL2(int, AROS_SLIB_ENTRY(KrnBug, Kernel),
    		AROS_LCA(const char *, format, A0),
    		AROS_LCA(va_list, args, A1),
    		struct KernelBase *, KernelBase);
    } else
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
