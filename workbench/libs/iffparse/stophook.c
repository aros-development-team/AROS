/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Handler installed by StopChunk() and StopOnExit()
*/

#include "iffparse_intern.h"

/***************************/
/* StopChunk entry-handler */
/***************************/

LONG StopFunc
(
    struct Hook *hook,
     APTR         obj, 
     APTR         p
)
{
    return (IFF_RETURN2CLIENT);
}

