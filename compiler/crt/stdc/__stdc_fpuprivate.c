/*
    Copyright (C) 2018, The AROS Development Team. All rights reserved.

    Platforms that use softfloat support should implement these
    functions to correctly share interpretations between the softfloat implementation
    and fenv.
*/
#include <libraries/stdc.h>

/*****i**********************************************************************
    NAME
        void * __stdc_get_fpuprivate(

    SYNOPSIS
        void)

    FUNCTION
        Provide the default hook for retrieving private
        softfloat state.

    INPUTS
        None.

    RESULT
        Always NULL in this default implementation.

    NOTES
        Internal stdc.library support hook, not an application API.
        The default implementation maintains no private state
        and does not change the floating-point environment.

    SEE ALSO
        __stdc_set_fpuprivate()

*****************************************************************************/
void *__stdc_get_fpuprivate(void)
{
    return NULL;
}

/*****i**********************************************************************
    NAME
        void * __stdc_set_fpuprivate(

    SYNOPSIS
        void *_fpuprivate)

    FUNCTION
        Provide the default no-op hook for installing private
        softfloat state.

    INPUTS
        _fpuprivate - Ignored pointer; may be NULL. The pointed-to
                      object is neither accessed nor retained.

    RESULT
        Always NULL in this default implementation.

    NOTES
        Internal stdc.library support hook, not an application API.
        The default implementation maintains no private state
        and does not change the floating-point environment.

        Ownership remains with the caller. The return value
        does not identify a previous state or confirm installation.

    SEE ALSO
        __stdc_get_fpuprivate()

*****************************************************************************/
void *__stdc_set_fpuprivate(void *_fpuprivate)
{
    return NULL;
}
