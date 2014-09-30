/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/pertask.h>
#include "pertaskbase.h"

void PertaskSetValue(int value)
{
    struct PertaskBase *PertaskBase = (struct PertaskBase *)__aros_getbase_PertaskBase();

    PertaskBase->value = value;
}


int PertaskGetValue(void)
{
    struct PertaskBase *PertaskBase = (struct PertaskBase *)__aros_getbase_PertaskBase();

    return PertaskBase->value;
}
