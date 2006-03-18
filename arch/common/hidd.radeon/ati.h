#ifndef _ATI_H
#define _ATI_H

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <dos/bptr.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <hidd/graphics.h>

#include <oop/oop.h>

#include <aros/arossupportbase.h>
#include <exec/execbase.h>

#include "radeon.h"

#include LC_LIBDEFS_FILE

#define IID_Hidd_Gfx_Ati    "IRadeonDriver"
#define IID_Hidd_ATIBitMap  "IRadeonBitmap"
#define CLID_Hidd_Gfx_Ati   "RadeonDriver"

extern OOP_AttrBase HiddPCIDeviceAttrBase;
extern OOP_AttrBase HiddBitMapAttrBase;
extern OOP_AttrBase HiddPixFmtAttrBase;
extern OOP_AttrBase HiddSyncAttrBase;
extern OOP_AttrBase HiddGfxAttrBase;
extern OOP_AttrBase HiddATIBitMapAttrBase;
extern OOP_AttrBase HiddI2CAttrBase;
extern OOP_AttrBase HiddI2CDeviceAttrBase;
extern OOP_AttrBase __IHidd_PlanarBM;

enum {
    aoHidd_ATIBitMap_Drawable,

    num_Hidd_ATIBitMap_Attrs
};

#define aHidd_ATIBitMap_Drawable (HiddATIBitMapAttrBase + aoHidd_ATIBitMap_Drawable)

#define IS_BM_ATTR(attr, idx) (((idx)=(attr)-HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_ATIBM_ATTR(attr, idx) (((idx)=(attr)-HiddATIBitMapAttrBase) < num_Hidd_ATIBitMap_Attrs)

typedef struct __bm {
    struct SignalSemaphore bmLock;

    OOP_Object  *BitMap;    // BitMap OOP Object
    IPTR    framebuffer;    // Points to pixel data
    ULONG   width;      // Bitmap width
    ULONG   height;     // Bitmap height   
    ULONG   pitch;      // BytesPerRow aligned
    UBYTE   depth;      // Bitmap depth
    UBYTE   bpp;        // BytesPerPixel
    UBYTE   onbm;       // is onbitmap?
    UBYTE   fbgfx;      // is framebuffer in gfx memory
    ULONG   usecount;   // counts BitMap accesses

    ULONG   surface_format;
    ULONG   pattern_format;
    ULONG   rect_format;
    ULONG   line_format;

    struct CardState *state;
} atiBitMap;

struct ati_staticdata {
    struct ExecBase         *sysbase;

    struct SignalSemaphore  HWLock;     /* Hardware exclusive semaphore */
    struct SignalSemaphore  MultiBMLock;    /* To lock more than one bitmap at a time */

    APTR    memPool;

    struct MemHeader    CardMem;
    
    struct Card     Card;
    struct CardState *poweron_state;
    IPTR            scratch_buffer;
    
    OOP_Object      *AtiObject;
    OOP_Object      *PCIObject;
    OOP_Object      *PCIDevice;
    OOP_Object      *PCIDriver;
        
    OOP_Class       *AtiClass;
    OOP_Class       *AtiI2C;
    OOP_Class       *OnBMClass;
    OOP_Class       *OffBMClass;
    OOP_Class       *PlanarBMClass;
    
    OOP_AttrBase    pciAttrBase;
    OOP_AttrBase    atiBitMapAttrBase;
    OOP_AttrBase    bitMapAttrBase;
    OOP_AttrBase    pixFmtAttrBase;
    OOP_AttrBase    gfxAttrBase;
    OOP_AttrBase    syncAttrBase;
    OOP_AttrBase    i2cAttrBase;
    OOP_AttrBase    i2cDeviceAttrBase;
    OOP_AttrBase    planarAttrBase;

    OOP_MethodID    mid_ReadLong;
    OOP_MethodID    mid_CopyMemBox8;
    OOP_MethodID    mid_CopyMemBox16;
    OOP_MethodID    mid_CopyMemBox32;
    OOP_MethodID    mid_PutMem32Image8;
    OOP_MethodID    mid_PutMem32Image16;
    OOP_MethodID    mid_GetMem32Image8;
    OOP_MethodID    mid_GetMem32Image16;
    OOP_MethodID    mid_GetImage;
    OOP_MethodID    mid_Clear;
    OOP_MethodID    mid_PutMemTemplate8;
    OOP_MethodID    mid_PutMemTemplate16;
    OOP_MethodID    mid_PutMemTemplate32;
    OOP_MethodID    mid_PutMemPattern8;
    OOP_MethodID    mid_PutMemPattern16;
    OOP_MethodID    mid_PutMemPattern32;
    OOP_MethodID    mid_CopyLUTMemBox16;
    OOP_MethodID    mid_CopyLUTMemBox32;
  
    HIDDT_DPMSLevel dpms;
};

struct atibase {
    struct Library          LibNode;
    struct ExecBase         *sysbase;
    BPTR                    seglist;
    struct ati_staticdata   sd;
};

struct planarbm_data
{
    UBYTE   **planes;
    ULONG   planebuf_size;
    ULONG   bytesperrow;
    ULONG   rows;
    UBYTE   depth;
    BOOL    planes_alloced;
};

#define BASE(lib) ((struct atibase*)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define METHOD_NAME(base, id, name) \
  base ## __ ## id ## __ ## name

#define METHOD_NAME_S(base, id, name) \
  # base "__" # id "__" # name

 
#define LOCK_HW          { ObtainSemaphore(&sd->HWLock); }
#define UNLOCK_HW        { ReleaseSemaphore(&sd->HWLock); }

#define LOCK_BITMAP      { ObtainSemaphore(&bm->bmLock); }
#define UNLOCK_BITMAP        { ReleaseSemaphore(&bm->bmLock); }

#define LOCK_BITMAP_BM(bm)   { ObtainSemaphore(&(bm)->bmLock); }
#define UNLOCK_BITMAP_BM(bm) { ReleaseSemaphore(&(bm)->bmLock); }

#define LOCK_MULTI_BITMAP    { ObtainSemaphore(&SD(cl)->MultiBMLock); }
#define UNLOCK_MULTI_BITMAP  { ReleaseSemaphore(&SD(cl)->MultiBMLock); }

#endif /* _ATI_H */
