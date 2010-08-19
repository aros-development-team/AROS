#include <aros/asmcall.h>
#include <proto/kernel.h>

#ifdef bug
#undef bug
#endif

AROS_LD2(int, KrnBug,
         AROS_LDA(const char *, format, A0),
         AROS_LDA(va_list, args, A1),
         struct KernelBase *, KernelBase, 12, Kernel);

static inline void _bug(struct KernelBase *KernelBase, const char *format, ...)
{
    va_list args;

    va_start(args, format);

    /* We call the function directly, not using vector table,
       because KernelBase can be NULL here (during very early
       startup) */
    AROS_UFC3(int, AROS_SLIB_ENTRY(KrnBug, Kernel),
	      AROS_UFCA(const char *, format, A0),
	      AROS_UFCA(va_list, args, A1),
	      AROS_UFCA(struct KernelBase *, KernelBase, A6));

    va_end(args);
}

#define bug(...) _bug(KernelBase, __VA_ARGS__)

int krnPutC(int chr, struct KernelBase *KernelBase);
