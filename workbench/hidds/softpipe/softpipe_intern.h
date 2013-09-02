#ifndef _SOFTPIPE_INTERN_H
#define _SOFTPIPE_INTERN_H

/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/


#include LC_LIBDEFS_FILE

#define CLID_Hidd_Gallium_Softpipe  "hidd.gallium.softpipe"

struct HIDDGalliumSoftpipeData
{
};

struct softpipestaticdata 
{
    OOP_Class       *galliumclass;
    OOP_AttrBase    hiddGalliumAB;
    struct Library  *SoftpipeCyberGfxBase;
};

LIBBASETYPE 
{
    struct Library              LibNode;
    struct softpipestaticdata   sd;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib) ((LIBBASETYPEPTR)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

#endif
