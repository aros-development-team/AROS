#ifndef _GALLIUM_INTERN_H
#define _GALLIUM_INTERN_H

/*
    Copyright 2010-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "state_tracker/sw_winsys.h"

#include LC_LIBDEFS_FILE

struct HiddGalliumData
{
    APTR  pad;
};

struct galliumstaticdata 
{
    OOP_Class       *galliumclass;
    OOP_AttrBase    galliumAttrBase;
};

LIBBASETYPE 
{
    struct Library              LibNode;
    struct galliumstaticdata    sd;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib) ((LIBBASETYPEPTR)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

#endif
