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

        AROS_LH3(ULONG, KrnBacktraceFromRBP,

/*  SYNOPSIS */
        AROS_LHA(APTR, rbp_in, A0),
        AROS_LHA(APTR *, out_pcs, A1),
        AROS_LHA(ULONG, max_depth, D0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 69, Kernel)

/*  FUNCTION
        Walks the current call stack using frame-pointer chaining and stores
        the return addresses into the provided array. The function starts from
        the frame pointer value supplied in rbp_in and follows the saved RBP
        links until the maximum depth is reached or the frame chain becomes
        invalid.

        This is a low-level helper used to capture a stack backtrace for
        diagnostic and crash reporting purposes.

    INPUTS
        rbp_in     - Frame pointer of the starting stack frame. Typically this
                     should be the current RBP value.
        out_pcs    - Pointer to an array of addresses which will receive the
                     collected return addresses.
        max_depth  - Maximum number of entries to store in the output array.

    RESULT
        Returns the number of valid entries written into the output array.

    NOTES
        This function relies on standard frame-pointer conventions and requires
        code to be compiled with frame pointers enabled (for GCC or Clang, use
        -fno-omit-frame-pointer).

        The function performs basic safety checks on the frame chain and stops
        if the next frame pointer is invalid, unaligned, or lower than the
        previous one. It is safe to call from an exception or trap context.

    EXAMPLE
        APTR pcs[64];
        ULONG depth;

        depth = KrnBacktraceFromRBP((APTR)__builtin_frame_address(0), pcs, 64);
        KrnPrintBacktrace("[Kernel] ", pcs, depth);

    BUGS
        The function may produce incomplete results if compiler optimizations
        omit frame pointers, or if stack corruption has occurred prior to the
        call.

    SEE ALSO
        KrnPrintBacktrace(), KrnRegisterSymResolver(), KrnUnregisterSymResolver()

    INTERNALS
        Each frame is assumed to contain a saved RBP followed by the return RIP.
        No memory reads beyond these fields are performed. The function does
        not perform any symbol resolution or formatting of the output.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG n = 0;
    IPTR *rbp = (IPTR *)rbp_in;

    while (rbp && n < max_depth) {
        IPTR saved_rbp = rbp[0];
        IPTR ret       = rbp[1];

        if (!ret)
            break;

        out_pcs[n++] = (APTR)ret;

        /* Monotonicity & alignment to avoid loops/corruption */
        if (saved_rbp <= (IPTR)rbp || (saved_rbp & 0xF))
            break;

        rbp = (IPTR *)saved_rbp;
    }

    return n;

    AROS_LIBFUNC_EXIT
}
