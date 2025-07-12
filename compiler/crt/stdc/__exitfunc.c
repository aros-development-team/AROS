/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.
*/

#include "__stdc_intbase.h"

#include <aros/symbolsets.h>
#include <exec/lists.h>
#include "__exitfunc.h"

int __addexitfunc(struct AtExitNode *aen)
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();
    
    ADDHEAD((struct List *)&StdCBase->atexit_list, (struct Node *)aen);

    return 0;
}

int __init_atexit(struct StdCIntBase *StdCBase)
{
    NEWLIST((struct List *)&StdCBase->atexit_list);

    return 1;
}

void __callexitfuncs(void)
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();
    struct AtExitNode *aen;

    while (
        (aen = (struct AtExitNode *) REMHEAD((struct List *) &StdCBase->atexit_list))
    )
    {
        switch (aen->node.ln_Type)
        {
        case AEN_VOID:
            aen->func.fn();
            break;

        case AEN_ON:
            {
                int *errorptr = __stdc_get_errorptr();
                aen->func.on.fn(errorptr != NULL ? *errorptr : 0, aen->func.on.arg);
            }
            break;

        case AEN_CXA:
            {
                int *errorptr = __stdc_get_errorptr();
                aen->func.cxa.fn(aen->func.cxa.arg, errorptr != NULL ? *errorptr : 0);
            }
            break;

        }
    }
}

ADD2OPENLIB(__init_atexit, 100);
