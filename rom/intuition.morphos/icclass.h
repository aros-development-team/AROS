#ifndef _ICCLASS_H
#define _ICCLASS_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Externally visible data for ICClass.
*/

/* Both ICCLASS and GADGETCLASS need this data, so we make it visible. */

struct ICData
{
    Object          *ic_Target;
    struct TagItem  *ic_Mapping;
    struct TagItem  *ic_CloneTags;
    ULONG            ic_LoopCounter;
};

#define ICMAGIC 1234

#endif /* _ICCLASS_H */
