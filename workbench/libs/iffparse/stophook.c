/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Handler installed by StopChunk() and StopOnExit()
    Lang: English.
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

