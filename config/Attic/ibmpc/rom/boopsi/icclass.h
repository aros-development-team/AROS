#ifndef _ICCLASS_H
#define _ICCLASS_H

/*
    Copyright (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Externally visible data for ICClass
    Lang: english
*/

/* Both ICCLASS and GADGETCLASS need this data, so we make it visible. */

struct ICData
{
    Object		*ic_Target;
    struct TagItem 	*ic_Mapping;
    struct TagItem	*ic_CloneTags;
    ULONG		 ic_LoopCounter;
};

#endif _ICCLASS_H
