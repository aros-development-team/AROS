#ifndef HIDD_GALLIUM_H
#define HIDD_GALLIUM_H

/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef HIDD_HIDD_H
#include <hidd/hidd.h>
#endif

#ifndef OOP_OOP_H
#include <oop/oop.h>
#endif

#ifndef P_AROS_VERSION_H
/* Gallium3D interface version. This is separate from gallium.hidd versioning */
#include <gallium/pipe/p_aros_version.h>
#endif

/* Gallium interface */

#define IID_Hidd_Gallium    "hidd.gallium"

#define HiddGalliumAttrBase __IHidd_Gallium

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddGalliumAttrBase;
#endif

/* Gallium Class methods */

enum
{
    moHidd_Gallium_CreatePipeScreen = 0,
    moHidd_Gallium_QueryDepthStencil,
    moHidd_Gallium_DisplaySurface,
    moHidd_Gallium_DestroyPipeScreen,

    NUM_GALLIUM_METHODS
};

enum
{
    aoHidd_Gallium_GalliumInterfaceVersion = 0,
    
    num_Hidd_Gallium_Attrs
};

#define aHidd_Gallium_GalliumInterfaceVersion  (HiddGalliumAttrBase + aoHidd_Gallium_GalliumInterfaceVersion)

#define IS_GALLIUM_ATTR(attr, idx) \
    (((idx) = (attr) - HiddGalliumAttrBase) < num_Hidd_Gallium_Attrs)

struct pHidd_Gallium_CreatePipeScreen
{
    OOP_MethodID    mID;
};

struct pHidd_Gallium_QueryDepthStencil
{
    OOP_MethodID    mID;
    UBYTE           *depthbits;
    UBYTE           *stencilbits;
};

struct pHidd_Gallium_DisplaySurface
{
    OOP_MethodID    mID;
    APTR            context;
    APTR            rastport;
    ULONG           left;
    ULONG           top;
    ULONG           width;
    ULONG           height;
    APTR            surface;
    ULONG           absx;
    ULONG           absy;
    ULONG           relx;
    ULONG           rely;
};

struct pHidd_Gallium_DestroyPipeScreen
{
    OOP_MethodID    mID;
    APTR            screen;
};

/* Gallium DriverFactory class */

#define CLID_Hidd_GalliumDriverFactory    "hidd.gallium.driverfactory"
#define IID_Hidd_GalliumDriverFactory     "hidd.gallium.driverfactory"

#define HiddGalliumDriverFactoryAttrBase  __IHidd_GalliumDriverFactory

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddGalliumDriverFactoryAttrBase;
#endif

/* Gallium DriverFactory Class methods */
enum
{
    moHidd_GalliumDriverFactory_GetDriver = 0,

    NUM_GALLIUMDRIVERFACTORY_METHODS
};

struct pHidd_GalliumDriverFactory_GetDriver
{
    OOP_MethodID    mID;
    ULONG           galliuminterfaceversion;
};

/* Stub that uses predefined Gallium interface version */
#define HIDD_GalliumDriverFactory_GetDriver(__o)                                                        \
    ({                                                                                                  \
        struct pHidd_GalliumDriverFactory_GetDriver gdmsg = {                                           \
        mID : OOP_GetMethodID(IID_Hidd_GalliumDriverFactory, moHidd_GalliumDriverFactory_GetDriver),    \
        galliuminterfaceversion : GALLIUM_INTERFACE_VERSION                                             \
        };                                                                                              \
        (OOP_Object*)OOP_DoMethod(__o, (OOP_Msg)&gdmsg);                                                \
    })

#endif
