/*
   (C) 1997-98 AROS - The Amiga Replacement OS
   $Id$

   Desc: Default method funtions.
   Lang: english
*/

#include <oop/oop.h>
#include "intern.h"

/* Default function for calling DoMethod() on a local function */
IPTR LocalDoMethod(Object *object, Msg msg)
{
    struct IntClass *cl = (struct IntClass *)OCLASS(object);
    
    IntCallMethod(cl, object, msg);

}
