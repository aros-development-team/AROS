/*
    Copyright © 2009-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Copy memory.
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/libcall.h>
#include <proto/exec.h>

#define SSE_REG_SIZE 16
#define SSE_REG_MASK 0xF

#define MEMFENCE __asm__ __volatile__ ("sfence":::"memory")
#define MMENABLE __asm__ __volatile__ ("emms":::"memory")

#define __byte_memcpy(src,dst,size)                             \
{                                                               \
    __asm__ __volatile__(                                       \
        "    rep; movsb"                                        \
        : "=&D" (dst), "=&S" (src), "=&c" (dummy)               \
        : "0" (dst), "1" (src), "2" (size)                      \
        : "memory");                                            \
}

#define __long_memcpy(src,dst,size)                             \
{                                                               \
    __asm__ __volatile__(                                       \
        "    rep; movsl" "\n\t"                                 \
        "    testb $2,%b6" "\n\t"                               \
        "    je 1f" "\n\t"                                      \
        "    movsw" "\n"                                        \
        "1:  testb $1,%b6" "\n\t"                               \
        "    je 2f" "\n\t"                                      \
        "    movsb" "\n"                                        \
        "2:"                                                    \
        : "=&D" (dst), "=&S" (src), "=&c" (dummy)               \
        : "0" (dst), "1" (src), "2" (size >> 2), "q" (size)     \
        : "memory");                                            \
}

static __inline__ void __small_memcpy(const void * src, void * dst, ULONG size)
{
    register unsigned long int dummy;
    if( size < 4 ) {
D(bug("[Exec] __byte_memcpy(%p, %p, %ld)\n", src, dst, size));

        __byte_memcpy(src, dst, size);
    }
    else
    {
D(bug("[Exec] __long_memcpy(%p, %p, %ld)\n", src, dst, size));

        __long_memcpy(src, dst, size);
    }
}

/*****************************************************************************

    NAME */

	AROS_LH3I(void, CopyMem,

/*  SYNOPSIS */
	AROS_LHA(CONST_APTR, source, A0),
	AROS_LHA(APTR, dest, A1),
	AROS_LHA(IPTR, size, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 104, Exec)

/*  FUNCTION
	Copy some data from one location to another in memory using
	SSE optimised copying method if enough data.

    INPUTS
	source - Pointer to source area
	dest   - Pointer to destination
	size   - number of bytes to copy

    RESULT

    NOTES
	The source and destination area are not allowed to overlap.
        If the src isn't on a 16-byte boundary, it is aligned
        first (so long as there's enough data)
        Copies using 4x16-byte registers = 64 bytes at a time.

    EXAMPLE

    BUGS

    SEE ALSO
	CopyMemQuick()

    INTERNALS
	64-bit sizes are not handled yet.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!size) return;

    ULONG lcnt = (size >> 6);                   /* size / 64       */

    const void *src = source;
    void *dst = dest;

D(bug("[Exec] CopyMem(%p, %p, %ld)\n", src, dst, size));

    __asm__ __volatile__ (
                "    prefetchnta (%0)\n"
                "    prefetchnta 32(%0)\n"
                "    prefetchnta 64(%0)\n"
                "    prefetchnta 96(%0)\n"
                "    prefetchnta 128(%0)\n"
                "    prefetchnta 160(%0)\n"
                "    prefetchnta 192(%0)\n"
                "    prefetchnta 256(%0)\n"
                "    prefetchnta 288(%0)\n"
                :
                : "r" (src) );

    if ((lcnt > 0) && (size >= (SSE_REG_SIZE * 4)))
    {
D(bug("[Exec] CopyMem: Using SSE Copy.\n"));
        ULONG alignsize = ((SSE_REG_SIZE - ((IPTR)src & SSE_REG_MASK)));

        if ((((IPTR)src & SSE_REG_MASK) != 0) && (((IPTR)(dst + alignsize) & SSE_REG_MASK) == 0))
        {
D(bug("[Exec] CopyMem: Aligning src to %d byte boundary (%d bytes) .. \n", SSE_REG_SIZE, alignsize));

            __small_memcpy(src, dst, alignsize);

            size -= alignsize;
            lcnt = (size >> 6);                 /* size / 64       */
            src += alignsize;
            dst += alignsize;
        }
        if (lcnt > 0) {
            if ((((IPTR)src & SSE_REG_MASK) == 0) && (((IPTR)dst & SSE_REG_MASK) == 0))
            {
                /*
                    # SRC and DST aligned on 16-byte boundary.
                    We can use movaps instead of movups since we meet
                    the alignment constraints (a general-protection fault
                    would be triggered otherwise)
                */
                size -= (lcnt << 6);
                for( ; lcnt > 0; lcnt--)
                {
D(bug("[Exec] CopyMem: SSE Aligned-Copy %p to %p.\n", src, dst));

                    __asm__ __volatile__ (
                        "    prefetchnta 320(%0)\n"
                        "    prefetchnta 352(%0)\n"
                        "    movaps (%0), %%xmm0\n"
                        "    movaps 16(%0), %%xmm1\n"
                        "    movaps 32(%0), %%xmm2\n"
                        "    movaps 48(%0), %%xmm3\n"
                        "    movntps %%xmm0, (%1)\n"
                        "    movntps %%xmm1, 16(%1)\n"
                        "    movntps %%xmm2, 32(%1)\n"
                        "    movntps %%xmm3, 48(%1)\n"
                        :
                        : "r" (src), "r" (dst)
                        : "memory");

                    src += (SSE_REG_SIZE * 4);
                    dst += (SSE_REG_SIZE * 4);
                }
            }
            else if (((IPTR)dst & SSE_REG_MASK) == 0)
            {
                /*
                    # SRC is unaligned and DST aligned on 16-byte boundary.
                */
                size -= (lcnt << 6);
                for( ; lcnt > 0; lcnt--)
                {
D(bug("[Exec] CopyMem: SSE Unaligned-Copy %p to %p.\n", src, dst));

                    __asm__ __volatile__ (
                        "    prefetchnta 320(%0)\n"
                        "    prefetchnta 352(%0)\n"
                        "    movups (%0), %%xmm0\n"
                        "    movups 16(%0), %%xmm1\n"
                        "    movups 32(%0), %%xmm2\n"
                        "    movups 48(%0), %%xmm3\n"
                        "    movntps %%xmm0, (%1)\n"
                        "    movntps %%xmm1, 16(%1)\n"
                        "    movntps %%xmm2, 32(%1)\n"
                        "    movntps %%xmm3, 48(%1)\n"
                        :
                        : "r" (src), "r" (dst)
                        : "memory");

                    src += (SSE_REG_SIZE * 4);
                    dst += (SSE_REG_SIZE * 4);
                }
            }
        }
    }

    if (size > 0)
    {
D(bug("[Exec] CopyMem: Copy remaining %ld bytes.\n", size));
        __small_memcpy(src, dst, size);
    }

    /* 
        FENCE Memory to re-order again since movntq is weakly-ordered ?
    */
    MEMFENCE;
    /*
        enable FPU use ?
    */
    MMENABLE;

    D(bug("[Exec] CopyMem: Finished.\n"));

    AROS_LIBFUNC_EXIT
} /* CopyMem */

