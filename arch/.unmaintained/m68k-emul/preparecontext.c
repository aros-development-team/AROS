/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <aros/libcall.h>

#include "exec_util.h"

#error "PrepareContext() has been changed. Additional tagList param, etc."
#error "This one here needs to be rewritten!"

BOOL PrepareContext(struct Task *task, APTR entryPoint, APTR fallBack,
                    struct TagItem *tagList, struct ExecBase *SysBase)
{
    UBYTE *sp=(UBYTE *)stackPointer;
    int i;

    /*
        mc68000 version. As long as no FPU is in use this works for the
        other mc680xx brands as well.
    */

    /* Push fallback address */
    sp-=sizeof(APTR);
    *(APTR *)sp=fallBack;

    /* Now push the context. Prepare a rts first (pc). */
    sp-=sizeof(APTR);
    *(APTR *)sp=entryPoint;

    /* Push 15 registers */
    for(i=0;i<15;i++)
    {
        sp-=sizeof(LONG);
        *(LONG *)sp=0;
    }

    return sp;
} /* PrepareContext */
