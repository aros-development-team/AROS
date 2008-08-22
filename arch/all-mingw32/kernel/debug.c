#include <aros/kernel.h>
#include <aros/libcall.h>
#include <stdarg.h>

#include "kernel_intern.h"

AROS_LH2(int, KrnBug,
         AROS_LHA(const char *, format, A0),
         AROS_LHA(va_list, args, A1),
         struct KernelBase *, KernelBase, 11, Kernel)
{
    AROS_LIBFUNC_INIT

    return HostIFace->VKPrintF(format, args);
    
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, KrnRawPutChar,

/*  SYNOPSIS */
        AROS_LHA(UBYTE, chr, D0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 12, Kernel)

/*  FUNCTION
        Emits a single character.

    INPUTS
        chr - The character to emit

    RESULT
        None.

    NOTES
        This function is for very low level debugging only.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    HostIFace->PutChar(chr);

    AROS_LIBFUNC_EXIT
} /* RawPutChar */
