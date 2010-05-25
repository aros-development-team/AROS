#ifndef _SOFTPIPE_H
#define _SOFTPIPE_H

/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/


#include LC_LIBDEFS_FILE

#define CLID_Hidd_GalliumSoftpipeDriver   "hidd.gallium.softpipedriver"

struct HIDDGalliumSoftpipeDriverData
{
};

struct softpipestaticdata 
{
    OOP_Class       *galliumSoftpipeDriverClass;
    OOP_AttrBase    hiddGalliumBaseDriverAB;
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
