/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "__arosc_privdata.h"

#include <aros/symbolsets.h>
#include <aros/startup.h>
#include <exec/lists.h>
#include "__exitfunc.h"

int __addexitfunc(struct AtExitNode *aen)
{
    struct aroscbase *aroscbase = __get_aroscbase();
    
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
    struct aroscbase *aroscbase = __get_aroscbase();
    struct AtExitNode *aen;

    while (
        (aen = (struct AtExitNode *) REMHEAD((struct List *) &aroscbase->acb_atexit_list))
    )
    {
        int _error = __arosc_startup_error;
        void *oldrelbase = AROS_SET_RELBASE(aen->relbase);
        switch (aen->node.ln_Type)
        {
        case AEN_VOID:
            aen->func.fvoid();
            break;

        case AEN_PTR:
            aen->func.fptr(_error, aen->ptr);
            break;
        }
        AROS_SET_RELBASE(oldrelbase);
    }
}

ADD2OPENLIB(__init_atexit, 100);
