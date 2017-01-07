#ifndef HIDD_GALLIUM_H
#define HIDD_GALLIUM_H

/*
    Copyright 2010-2017, The AROS Development Team. All rights reserved.
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

#ifndef _STDINT_H_
#   include <stdint.h>
#endif

#ifndef U_SIMPLE_SCREEN_H
#   include <gallium/util/u_simple_screen.h>
#endif

#ifndef PIPE_STATE_H
#   include <gallium/pipe/p_state.h>
#endif

#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
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
    moHidd_Gallium_DisplayResource,
    moHidd_Gallium_DestroyPipeScreen,

    NUM_GALLIUM_METHODS
};

enum
{
    aoHidd_Gallium_InterfaceVersion = 0,
    
    num_Hidd_Gallium_Attrs
};

#define aHidd_Gallium_InterfaceVersion  (HiddGalliumAttrBase + aoHidd_Gallium_InterfaceVersion)

#define IS_GALLIUM_ATTR(attr, idx) \
    (((idx) = (attr) - HiddGalliumAttrBase) < num_Hidd_Gallium_Attrs)

struct pHidd_Gallium_CreatePipeScreen
{
    STACKED OOP_MethodID    mID;
};

struct pHidd_Gallium_DestroyPipeScreen
{
    STACKED OOP_MethodID    mID;
    STACKED APTR    screen;
};

struct pHidd_Gallium_DisplayResource
{
    STACKED OOP_MethodID            mID;
    STACKED struct pipe_resource    *resource;
    STACKED ULONG                   srcx;
    STACKED ULONG                   srcy;

    STACKED struct BitMap           *bitmap;
    STACKED ULONG                   dstx;
    STACKED ULONG                   dsty;
    STACKED ULONG                   width;
    STACKED ULONG                   height;
};

struct HIDDT_WinSys
{
    struct pipe_winsys  base;
    OOP_Object          *driver;
};

#endif
