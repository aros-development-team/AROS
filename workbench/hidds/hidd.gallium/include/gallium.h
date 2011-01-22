#ifndef HIDD_GALLIUM_H
#define HIDD_GALLIUM_H

/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef P_AROS_VERSION_H
/* Gallium3D interface version. This is separate from gallium.hidd versioning */
#   include <gallium/pipe/p_aros_version.h>
#endif

#ifndef U_SIMPLE_SCREEN_H
#   include <gallium/util/u_simple_screen.h>
#endif

/* Gallium interface */
#define CLID_Hidd_Gallium   "hidd.gallium"
#define IID_Hidd_Gallium    "hidd.gallium"

#define HiddGalliumAttrBase __IHidd_Gallium

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddGalliumAttrBase;
#endif

/* Gallium Class methods */

enum
{
    moHidd_Gallium_CreatePipeScreen = 0,
    moHidd_Gallium_DisplaySurface = 2,
    moHidd_Gallium_DisplayResource,

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

/* TODO: Use bitmap object instead of rast port. */
/* TODO: Give only one set of coordinates - bitmap relative */
struct pHidd_Gallium_DisplayResource
{
    OOP_MethodID    mID;
    APTR            rastport;
    ULONG           left;
    ULONG           top;
    ULONG           width;
    ULONG           height;
    APTR            resource;
    ULONG           absx;
    ULONG           absy;
    ULONG           relx;
    ULONG           rely;
};

struct HIDDT_WinSys
{
    struct pipe_winsys  base;
    OOP_Object          *driver;
};

#endif
