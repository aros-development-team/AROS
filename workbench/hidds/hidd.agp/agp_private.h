#ifndef _AGP_PRIVATE_H
#define _AGP_PRIVATE_H

/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/


#include LC_LIBDEFS_FILE

struct HIDDAGPData
{
};

struct agpstaticdata 
{
    OOP_Class       *AGPClass;
    OOP_Class       *genericBridgeDeviceClass;
    
    OOP_AttrBase    hiddAGPBridgeDeviceAB;

    OOP_Object      *bridgedevice;
};

LIBBASETYPE 
{
    struct Library              LibNode;
    struct agpstaticdata        sd;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib) ((LIBBASETYPEPTR)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

/* This is an abstract class. Contains usefull code but is not functional */
#define CLID_Hidd_GenericBridgeDevice   "hidd.agp.genericbridgedevice"

struct HIDDGenericBridgeDeviceData
{
};

#endif
