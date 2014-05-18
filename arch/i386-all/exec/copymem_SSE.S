/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME

        AROS_LH3(void, CopyMem_SSE,

    SYNOPSIS
        AROS_LHA(CONST_APTR, source, A0),
        AROS_LHA(APTR, destination, A1),
        AROS_LHA(IPTR, len, D0),

    LOCATION
        struct ExecBase *, SysBase, 104, Exec) (if we're lucky)

    FUNCTION
        Copy some data from one location to another in memory using
        SSE optimised copying method if enough data.

    INPUTS
        source - Pointer to source area
        dest   - Pointer to destination
        size   - number of bytes to copy

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        The source and destination area *ARE* allowed to overlap.

******************************************************************************/
#include "aros/i386/asm.h"
#include <aros/config.h>

    .text
    .globl  AROS_SLIB_ENTRY(CopyMem_SSE,Exec,104)
    _FUNCTION(AROS_SLIB_ENTRY(CopyMem_SSE,Exec,104))
AROS_SLIB_ENTRY(CopyMem_SSE,Exec,104):
    movl        12(%esp),%eax
    cmpl        $4096,%eax
    jge         .begin
    jmp         AROS_SLIB_ENTRY(CopyMem,Exec,104)    /* non-SSE version is faster for small copies */

    .globl  AROS_SLIB_ENTRY(CopyMemQuick_SSE,Exec,105)
    _FUNCTION(AROS_SLIB_ENTRY(CopyMemQuick_SSE,Exec,105))
AROS_SLIB_ENTRY(CopyMemQuick_SSE,Exec,105):
    movl        12(%esp),%eax
    cmpl        $4096,%eax
    jge         .begin
    jmp         AROS_SLIB_ENTRY(CopyMemQuick,Exec,105)    /* non-SSE version is faster for small copies */

.begin:
    pushl       %edi
    pushl       %esi
    pushl       %ecx

    movl        16(%esp),%esi
    movl        20(%esp),%edi
    movl        24(%esp),%eax

    /*
    ** okay, so the user wants to copy at least 4096 bytes.
    ** let's go!
    */
    pushl       %ebx
    pushl       %ebp
    movl        %esp,%ebp

    /*
    ** align memory to save xmm regs
    */
    subl        $16*4,%esp
    andl        $-16,%esp
    movntps     %xmm0,(%esp)
    movntps     %xmm1,16(%esp)
    movntps     %xmm2,32(%esp)
    movntps     %xmm3,48(%esp)
    movl        %eax,%ebx

    /*
    ** compute step
    cmp    %edi,%esi
    jl    .copy_backward
    */

.copy_forward:
    /*
    ** prefetch area 
    */
    prefetchnta (%esi)
    prefetchnta 32(%esi)
    prefetchnta 64(%esi)
    prefetchnta 96(%esi)
    prefetchnta 128(%esi)
    prefetchnta 160(%esi)

    /*
    ** check memory alignment
    ** a little trick ;)
    */
    movl        $16,%eax
    sub         %edi,%eax
    andl        $15,%eax
    je          .noalign

    movl        %eax,%ecx        /* set count register */
    subl        %eax,%ebx        /* update "to-do" length */
    cld
    rep         movsb

.noalign:
    movl        %esi,%eax        /* start checking alignment here */
    subl        $64,%ebx        /* another trick here - don't use cmp, but neg flag instead */
    jl          .fast_copy_done
    andl        $15,%eax
    je          .aligned_copy

.unaligned_copy:
    prefetchnta 160(%esi)        /* it's true we prefetched "something". but we may have moved during alignment */
    movups      (%esi),%xmm0    /* transfer block 16 bytes by 16 bytes */
    movups      16(%esi),%xmm1
    movups      32(%esi),%xmm2
    movups      48(%esi),%xmm3
    movntps     %xmm0,(%edi)
    movntps     %xmm1,16(%edi)
    movntps     %xmm2,32(%edi)
    movntps     %xmm3,48(%edi)
    addl        $64,%esi
    addl        $64,%edi
    subl        $64,%ebx        /* update count */
    jge         .unaligned_copy /* continue */
    jmp         .fast_copy_done

.aligned_copy:
    prefetchnta 160(%esi)        /* it's true we prefetched "something". but we may have moved during alignment */
    movaps      (%esi),%xmm0    /* transfer block 16 bytes by 16 bytes */
    movaps      16(%esi),%xmm1
    movaps      32(%esi),%xmm2
    movaps      48(%esi),%xmm3
    movntps     %xmm0,(%edi)
    movntps     %xmm1,16(%edi)
    movntps     %xmm2,32(%edi)
    movntps     %xmm3,48(%edi)
    addl        $64,%esi
    addl        $64,%edi
    subl        $64,%ebx        /* update count */
    jge         .aligned_copy   /* continue */
    jmp         .fast_copy_done

.copy_backward:
    /*
    ** adjust pointers first
    */
    add         %eax,%esi
    add         %eax,%edi
    subl        $1,%esi
    subl        $1,%edi

    /*
    ** prefetch area 
    */
    prefetchnta -191(%esi)
    prefetchnta -159(%esi)
    prefetchnta -127(%esi)
    prefetchnta -95(%esi)
    prefetchnta -63(%esi)
    prefetchnta -31(%esi)

    /*
    ** check memory alignment
    ** a little trick ;)
    */
    mov         %edi,%eax
    add         $1,%eax
    andl        $15,%eax
    je          .noalign

    movl        %eax,%ecx        /* set count register */
    subl        %eax,%ebx        /* update "to-do" length */
    std                /* indicate we want to copy backwards */
    rep         movsb

.noalign_back:
    movl        %esi,%eax        /* start checking alignment here */
    subl        $64,%ebx        /* another trick here - don't use cmp, but neg flag instead */
    jl          .fast_copy_done
    andl        $15,%eax
    cmp         $15,%eax
    je          .aligned_reverse_copy

.unaligned_reverse_copy:
    prefetchnta -191(%esi)        /* it's true we prefetched "something". but we may have moved during alignment */
    movups      -63(%esi),%xmm0    /* transfer block 16 bytes by 16 bytes */
    movups      -47(%esi),%xmm1
    movups      -31(%esi),%xmm2
    movups      -15(%esi),%xmm3
    movntps     %xmm0,-63(%edi)
    movntps     %xmm1,-47(%edi)
    movntps     %xmm2,-31(%edi)
    movntps     %xmm3,-15(%edi)
    subl        $64,%esi
    subl        $64,%edi
    subl        $64,%ebx        /* update count */
    jge         .unaligned_reverse_copy /* continue */
    jmp         .fast_copy_done

.aligned_reverse_copy:
    prefetchnta -192(%esi)        /* it's true we prefetched "something". but we may have moved during alignment */
    movaps      -63(%esi),%xmm0    /* transfer block 16 bytes by 16 bytes */
    movaps      -47(%esi),%xmm1
    movaps      -31(%esi),%xmm2
    movaps      -15(%esi),%xmm3
    movntps     %xmm0,-63(%edi)
    movntps     %xmm1,-47(%edi)
    movntps     %xmm2,-31(%edi)
    movntps     %xmm3,-15(%edi)
    subl        $64,%esi
    subl        $64,%edi
    subl        $64,%ebx        /* update count */
    jge         .aligned_reverse_copy   /* continue */
    jmp         .fast_copy_done



.fast_copy_done:
    /*
    ** prefetch stack. we'll be leaving soon
    */
    prefetchnta (%esp)
    prefetchnta 32(%esp)
    prefetchnta 64(%esp)

    /*
    ** copy remaining bytes
    ** note: std not needed for reverse resume
    */
    addl        $64,%ebx
    movl        %ebx,%ecx
    rep         movsb
    
    /*
    ** restore everything
    */
    movaps      (%esp),%xmm0
    movaps      16(%esp),%xmm1
    movaps      32(%esp),%xmm2
    movaps      48(%esp),%xmm3
    movl        %ebp,%esp
    popl        %ebp
    popl        %ebx
    popl        %ecx
    popl        %esi
    popl        %edi

    /*
    ** just cleanup..
    */
    cld
    ret


