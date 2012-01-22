/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef __AROS__

#include "arossupport.h"
#include <dos/dostags.h>

#include <datatypes/datatypesclass.h>
#ifndef STM_DONE
#define STM_DONE 0
#endif

#include <utility/tagitem.h>

#include <proto/utility.h>

ULONG *FindMethod(ULONG *methods, ULONG searchmethodid)
{
    if(methods == NULL)
        return NULL;

    while(((LONG)(*methods)) != -1)
    {
        if(*methods == searchmethodid)
            return methods;

        methods++;
    }

    return NULL;

} /* FindMethod */

struct DTMethod *FindTriggerMethod(struct DTMethod *methods, STRPTR command, ULONG method)
{
    struct DTMethod *retval = NULL;

    if (methods)
    {
        while(methods->dtm_Method != STM_DONE)
        {
            if(command != NULL)
            {
                if(Stricmp(methods->dtm_Command, command) == 0)
                {
                    retval = methods;
                    break;
                }
            }

            if(method != ~0)
            {
                if(methods->dtm_Method == method)
                {
                    retval = methods;
                    break;
                }
            }

            methods++;
        }
    }

    return retval;

} /* FindTriggerMethod */
#endif /* __AROS__ */

