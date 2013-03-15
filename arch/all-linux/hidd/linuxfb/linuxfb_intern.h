#ifndef LINUX_INTERN_H
#define LINUX_INTERN_H

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux framebuffer hidd for AROS
    Lang: English.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#ifndef HIDD_GRAPHICS_H
#   include <hidd/graphics.h>
#endif

/* hack: prevent linux include header <bits/time.h> to re-define timeval struct */
#  define _STRUCT_TIMEVAL 1

#include <linux/fb.h>
#include <linux/kd.h>

#define BUFFERED_VRAM 1

/* Private Attrs and methods for the LinuxFB Hidd */

#define CLID_Hidd_LinuxFB "hidd.gfx.linuxfb"
#define IID_Hidd_LinuxFB  "hidd.gfx.linuxfb"

#define HiddLinuxFBAttrBase  __abHidd_LinuxFB
extern OOP_AttrBase HiddLinuxFBAttrBase;

enum
{
    aoHidd_LinuxFB_File,
    num_Hidd_LinuxFB_Attrs   
};

#define aHidd_LinuxFB_File HiddLinuxFBAttrBase + aoHidd_LinuxFB_File

enum
{
    moHidd_LinuxFB_FBChanged
};

struct pHidd_LinuxFB_FBChanged
{
    OOP_MethodID mID;
    WORD x;
    WORD y;
    WORD width;
    WORD height;
    APTR src;
    LONG srcpitch;
};

#define CLID_Hidd_LinuxFBBitmap "hidd.bitmap.linuxfb"
#define IID_Hidd_LinuxFBBitmap  "hidd.bitmap.linuxfb"

#define HiddLinuxFBBitmapAttrBase  __abHidd_LinuxFBBitmap
extern OOP_AttrBase HiddLinuxFBBitmapAttrBase;

enum
{
    aoHidd_LinuxFBBitmap_FBDevInfo,
    num_Hidd_LinuxFBBitmap_Attrs
};

#define aHidd_LinuxFBBitmap_FBDevInfo   HiddLinuxFBBitmapAttrBase + aoHidd_LinuxFBBitmap_FBDevInfo

struct FBImage
{
    APTR buffer;
    LONG width;
    LONG height;
    LONG bpp;
};
struct CursorInfo
{
    struct FBImage img;
    BOOL            visible;
    LONG            currentx;
    LONG            currenty;
};

struct FBDevInfo
{
    BYTE *baseaddr;
    LONG pitch;
    LONG bpp;
    LONG xres;
    LONG yres;
};

struct SavedFB
{
    struct FBImage img;
    LONG            x;
    LONG            y;
    LONG            width;
    LONG            height;
    BOOL            active;
};

struct LinuxFB_data
{
    /* The device file */
    int fbdev;
    unsigned long mem_len;

    struct SignalSemaphore framebufferlock;

    /* FBDev info */
    struct FBDevInfo fbdevinfo;

    /* Cursor handling */
    struct CursorInfo   cinfo;
    struct SavedFB      sfb;
};

/*** Shared data ***/
struct LinuxFB_staticdata
{
    struct SignalSemaphore sema;
    
    OOP_Class *gfxclass;
    OOP_Class *bmclass;
    OOP_Object *unixio;

    OOP_AttrBase gfxAttrBase;
    OOP_AttrBase bmAttrBase;
    OOP_AttrBase syncAttrBase;
    OOP_AttrBase pfAttrBase;
    OOP_AttrBase chunkyAttrBase;
    OOP_AttrBase linuxFBAttrBase;
    OOP_AttrBase linuxBMAttrBase;
};

struct LinuxFB_base
{
    struct Library library;
    struct LinuxFB_staticdata lsd;
};

struct BitmapData;

#define LOCK_FRAMEBUFFER(data)    ObtainSemaphore(&data->framebufferlock)
#define UNLOCK_FRAMEBUFFER(data) ReleaseSemaphore(&data->framebufferlock)

#define LSD(cl) (&((struct LinuxFB_base *)cl->UserData)->lsd)

#undef HiddGfxAttrBase
#undef HiddBitMapAttrBase
#undef HiddSyncAttrBase
#undef HiddPixFmtAttrBase
#undef HiddChunkyBMAttrBase
#undef HiddLinuxFBAttrBase
#undef HiddLinuxFBBitmapAttrBase
#define HiddGfxAttrBase           LSD(cl)->gfxAttrBase
#define HiddBitMapAttrBase        LSD(cl)->bmAttrBase
#define HiddSyncAttrBase          LSD(cl)->syncAttrBase
#define HiddPixFmtAttrBase        LSD(cl)->pfAttrBase
#define HiddChunkyBMAttrBase      LSD(cl)->chunkyAttrBase
#define HiddLinuxFBAttrBase       LSD(cl)->linuxFBAttrBase
#define HiddLinuxFBBitmapAttrBase LSD(cl)->linuxBMAttrBase
#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#endif /* LINUX_INTERN_H */
