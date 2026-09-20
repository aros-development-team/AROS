/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/*
 * Part of norootrel.library, a pertaskbase library built with
 * norootrellibs: its rellib pertask.library is opened only for the bases
 * OpenLib creates. Reading the value through it checks that path.
 */

#include <proto/pertask.h>

int NorootrelGetValue(void)
{
    return PertaskGetValue();
}
