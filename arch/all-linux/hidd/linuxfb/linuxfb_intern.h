#ifndef LINUX_INTERN_H
#define LINUX_INTERN_H

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux framebuffer hidd for AROS
    Lang: English.
*/

#include <exec/interrupts.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <oop/oop.h>
#include <hidd/graphics.h>

/* hack: prevent linux include header <bits/time.h> to re-define timeval struct */
#  define _STRUCT_TIMEVAL 1

#include <linux/fb.h>
#include <linux/kd.h>

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

#define IID_Hidd_LinuxFBBitmap  "hidd.bitmap.linuxfb"

#define HiddLinuxFBBitmapAttrBase  __abHidd_LinuxFBBitmap
extern OOP_AttrBase HiddLinuxFBBitmapAttrBase;

enum
{
    aoHidd_LinuxFBBitmap_FBDevInfo,
    num_Hidd_LinuxFBBitmap_Attrs
};

#define aHidd_LinuxFBBitmap_FBDevInfo   HiddLinuxFBBitmapAttrBase + aoHidd_LinuxFBBitmap_FBDevInfo

struct FBDevInfo
{
    int fbdev;
    UBYTE fbtype;
    BYTE *baseaddr;
    LONG pitch;
    LONG bpp;
    LONG xres;
    LONG yres;
};

struct LinuxFB_data
{
    unsigned long mem_len;

    OOP_Object *visible;
    OOP_Object *unixio;
    int confd;
    long kbmode;
    BOOL gamma;

    /* FBDev info */
    struct FBDevInfo fbdevinfo;
    struct Interrupt resetHandler;
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
    OOP_AttrBase cmAttrBase;
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

#define LSD(cl) (&((struct LinuxFB_base *)cl->UserData)->lsd)

#undef HiddGfxAttrBase
#undef HiddBitMapAttrBase
#undef HiddSyncAttrBase
#undef HiddPixFmtAttrBase
#undef HiddColorMapAttrBase
#undef HiddChunkyBMAttrBase
#undef HiddLinuxFBAttrBase
#undef HiddLinuxFBBitmapAttrBase
#define HiddGfxAttrBase           LSD(cl)->gfxAttrBase
#define HiddBitMapAttrBase        LSD(cl)->bmAttrBase
#define HiddSyncAttrBase          LSD(cl)->syncAttrBase
#define HiddPixFmtAttrBase        LSD(cl)->pfAttrBase
#define HiddColorMapAttrBase      LSD(cl)->cmAttrBase
#define HiddChunkyBMAttrBase      LSD(cl)->chunkyAttrBase
#define HiddLinuxFBAttrBase       LSD(cl)->linuxFBAttrBase
#define HiddLinuxFBBitmapAttrBase LSD(cl)->linuxBMAttrBase
#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#endif /* LINUX_INTERN_H */
