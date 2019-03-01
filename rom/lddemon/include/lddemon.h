/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _RESOURCES_LDDEMON_H
#define _RESOURCES_LDDEMON_H

struct LDDemonBase
{
    /* Public*/
    struct Node		        node;
    struct Library          *dl_DOSBase;
    struct List             dl_Flavours; // List of the flavours (extensions) supported by LDLoadSeg Function.
};

#endif /* _RESOURCES_LDDEMON_H */
