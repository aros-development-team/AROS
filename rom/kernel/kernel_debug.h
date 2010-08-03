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
    AROS_SLIB_ENTRY(KrnBug, Kernel)(format, args, KernelBase);
    va_end(args);
}

#define bug(...) _bug(KernelBase, __VA_ARGS__)

int krnPutC(int chr, void *data);
