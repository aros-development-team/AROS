#ifndef _ICCLASS_H
#define _ICCLASS_H

/*
    Copyright (C) 1997-2001 AROS - The Amiga Research OS
    $Id$
 
    Desc: Externally visible data for ICClass
    Lang: english
*/

/* Both ICCLASS and GADGETCLASS need this data, so we make it visible. */

struct ICData
{
    Object      *ic_Target;
    struct TagItem  *ic_Mapping;
    struct TagItem  *ic_CloneTags;
    ULONG        ic_LoopCounter;
};

#define ICMAGIC 1234

#endif _ICCLASS_H
