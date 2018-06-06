/*
    Copyright © 2018, The AROS Development Team. All rights reserved.
    $Id$

    Platforms that use softfloat support should implement these 
    functions to correctly share interpretations between the softfloat implementation
    and fenv.
*/
#include <libraries/stdc.h>

void *__stdc_get_fpuprivate(void)
{
    return NULL;
}

void *__stdc_set_fpuprivate(void *_fpuprivate)
{
    return NULL;
}
