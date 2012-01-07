/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "__arosc_privdata.h"

#include <aros/symbolsets.h>
#include <aros/startup.h>
#include <exec/lists.h>
#include "__exitfunc.h"

int __addexitfunc(struct AtExitNode *aen)
{
    struct aroscbase *aroscbase = __GM_GetBase();
    
    ADDHEAD((struct List *)&aroscbase->acb_atexit_list, (struct Node *)aen);

    return 0;
}

int __init_atexit(struct aroscbase *aroscbase)
{
    NEWLIST((struct List *)&aroscbase->acb_atexit_list);

    return 1;
}

void __callexitfuncs(void)
{
    struct aroscbase *aroscbase = __GM_GetBase();
    struct AtExitNode *aen;

    while (
        (aen = (struct AtExitNode *) REMHEAD((struct List *) &aroscbase->acb_atexit_list))
    )
    {
        switch (aen->node.ln_Type)
        {
        case AEN_VOID:
            aen->func.fvoid();
            break;

        case AEN_PTR:
            aen->func.fptr(__arosc_startup_error, aen->ptr);
            break;
        }
    }
}

ADD2OPENLIB(__init_atexit, 100);
