/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH3(ULONG, KrnBacktraceFromFrame,

/*  SYNOPSIS */
        AROS_LHA(APTR, frame_in, A0),
        AROS_LHA(APTR *, out_pcs, A1),
        AROS_LHA(ULONG, max_depth, D0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 69, Kernel)

/*  FUNCTION
        Walks the current call stack using frame-pointer chaining and stores
        the return addresses into the provided array. The function starts from
        the frame pointer value supplied in frame_in and follows the saved
        link fields until the maximum depth is reached or the chain becomes
        invalid.

        This is a low-level helper used to capture a stack backtrace for
        diagnostic and crash reporting purposes.

    INPUTS
        frame_in   - Frame pointer of the starting stack frame. Typically this
                     should be the current function's frame pointer.
        out_pcs    - Pointer to an array of addresses which will receive the
                     collected return addresses.
        max_depth  - Maximum number of entries to store in the output array.

    RESULT
        Returns the number of valid entries written into the output array.

    NOTES
        This function relies on standard frame-link conventions and requires
        code to be compiled with frame pointers enabled (for GCC or Clang,
        use -fno-omit-frame-pointer).

        The function performs basic safety checks on the frame chain and stops
        if the next frame pointer is invalid, unaligned, or lower than the
        previous one. It is safe to call from an exception or trap context.

    EXAMPLE
        APTR pcs[64];
        ULONG depth;

        depth = KrnBacktraceFromFrame((APTR)__builtin_frame_address(0), pcs, 64);
        KrnPrintBacktrace("[Kernel] ", pcs, depth);

    BUGS
        The function may produce incomplete results if compiler optimizations
        omit frame pointers, or if stack corruption has occurred prior to the
        call.

    SEE ALSO
        KrnPrintBacktrace(), KrnRegisterSymResolver(), KrnUnregisterSymResolver()

    INTERNALS
        Each frame is assumed to contain a saved frame pointer followed by a
        return address. No memory reads beyond these fields are performed. The
        function does not perform any symbol resolution or formatting.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */
    return 0;

    AROS_LIBFUNC_EXIT
}
