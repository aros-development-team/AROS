#ifndef ICCLASS_H
#define ICCLASS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#include "intuition_intern.h"

/* Both ICClass & GadgetClass use this */


struct ICData
{
    Object	   * ic_Target;
    struct TagItem * ic_Mapping;
    struct TagItem * ic_CloneTags;
    ULONG	     ic_LoopCounter;
};

VOID FreeICStuff(struct ICData *, struct IntuitionBase *);
ULONG DoNotification(Class *, Object *, struct ICData *, struct opUpdate *);

#endif /* ICCLASS_H */
