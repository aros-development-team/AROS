/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal data structures for potgo.resource
    Lang: english
*/

#ifndef POTGO_INTERN_H
#define POTGO_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

struct PotgoBase
{
    struct Library pb_LibNode;
    UWORD allocated;
    UWORD data;
};

#endif //POTGO_INTERN_H
