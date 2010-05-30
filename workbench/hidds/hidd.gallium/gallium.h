#ifndef _GALLIUM_H
#define _GALLIUM_H

/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/


#include LC_LIBDEFS_FILE

struct HIDDGalliumBaseDriverData
{
};

struct HIDDGalliumDriverFactoryData
{
};

struct galliumstaticdata 
{
    OOP_Class       *galliumDriverFactoryClass;
    OOP_AttrBase    hiddGalliumBaseDriverAB;
    struct Library  *loadedDriverHidd;
    OOP_Object      *driver;
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
