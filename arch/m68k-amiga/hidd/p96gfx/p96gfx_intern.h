#ifndef P96GFX_INTERN_H
#define P96GFX_INTERN_H

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <oop/oop.h>

#include <hidd/gfx.h>

struct RTGMode
{
    struct Node                                         node;
    ULONG                                               modeid;
    UWORD                                               width, height;
    OOP_Object                                          *pf;
    OOP_Object                                          *sync;
};

struct p96gfx_staticdata
{
    struct ExecBase                                     *cs_SysBase;
    struct Library                                      *cs_GfxBase;
    struct Library                                      *cs_IntuitionBase;
    struct Library                                      *cs_UtilityBase;
    struct Library                                      *cs_OOPBase;

    IPTR                                                cs_SegList;

    OOP_Class                                           *basebm;            /* baseclass for CreateObject */
    OOP_Class                                           *gfxclass;
    OOP_Class                                           *bmclass;

    OOP_AttrBase                                        hiddAttrBase;
    OOP_AttrBase                                        hiddBitMapAttrBase;  
    OOP_AttrBase                                        hiddP96GFXBitMapAttrBase;
    OOP_AttrBase                                        hiddGCAttrBase;
    OOP_AttrBase                                        hiddSyncAttrBase;
    OOP_AttrBase                                        hiddPixFmtAttrBase;
    OOP_AttrBase                                        hiddGfxAttrBase;
    OOP_AttrBase                                        hiddP96GfxAttrBase;
    OOP_AttrBase                                        hiddColorMapAttrBase;

    OOP_MethodID                                        hiddBitMapBase;
    OOP_MethodID                                        hiddColorMapBase;
    OOP_MethodID                                        hiddGfxBase;
    OOP_MethodID                                        hiddP96GfxBase;

    struct List                                         foundCards;
};

struct p96gfx_carddata
{
    struct Node                                         p96gfx_Node;
    struct TagItem                                      *p96gfxcd_Tags;
    char                                                *p96gfx_HWName;
    char                                                *p96gfx_HWResTmplt;

    struct List                                         rtglist;
    struct List                                         bitmaplist;
    struct Library                                      *CardBase;

    struct P96GfxBitMapData                             *disp;
    APTR                                                p96romvector;
    ULONG                                               *rgbformat;
    struct ModeInfo                                     *modeinfo;
    struct ModeInfo                                     *fakemodeinfo;
    UBYTE                                               *boardinfo;
    UBYTE                                               *bitmapextra;
    UBYTE                                               *vram_start;
    ULONG                                               vram_size;
    ULONG                                               vram_used, fram_used;
    struct MemHeader                                    *vmem;

    WORD                                                sprite_width, sprite_height;
    BOOL                                                hardwaresprite;
    WORD                                                spritepencnt;

    BOOL                                                initialized;
    BOOL                                                superforward; 

    UWORD                                               dwidth, dheight;
    ULONG                                               dmodeid;

    UWORD                                               maxwidth[5];
    UWORD                                               maxheight[5];

    struct ViewPort                                     *viewport;
    void                                                (*acb)(void *data, void *bm);
    APTR                                                acbdata;

    struct SignalSemaphore                              HWLock;
    struct SignalSemaphore                              MultiBMLock;    
};

struct P96GFXclbase
{
    struct Library                                      library;

    struct p96gfx_staticdata                            csd;
};

#undef CSD
#define CSD(cl)                                         (&((struct P96GFXclbase *)cl->UserData)->csd)

#define SysBase                                         (csd->cs_SysBase)
#define OOPBase                                         (csd->cs_OOPBase)
#define UtilityBase                                     (csd->cs_UtilityBase)

#define __IHidd		                                    (csd->hiddAttrBase)
#define __IHidd_BitMap	                                (csd->hiddBitMapAttrBase)
#define __IHidd_BitMap_P96                              (csd->hiddP96GFXBitMapAttrBase)
#define __IHidd_GC                                      (csd->hiddGCAttrBase)
#define __IHidd_Sync	                                (csd->hiddSyncAttrBase)
#define __IHidd_PixFmt		                            (csd->hiddPixFmtAttrBase)
#define __IHidd_Gfx 	                                (csd->hiddGfxAttrBase)
#define __IHidd_P96Gfx 	                                (csd->hiddP96GfxAttrBase)
//#define __IHidd_Attr		                            (csd->hiddAttrBase)
#define __IHidd_ColorMap	                            (csd->hiddColorMapAttrBase)

#define HiddBitMapBase		                            (csd->hiddBitMapBase)
#define HiddColorMapBase	                            (csd->hiddColorMapBase)
#define HiddGfxBase		                                (csd->hiddGfxBase)
#define HiddP96GfxBase                                  (csd->hiddP96GfxBase)

#define LOCK_HW                                         {ObtainSemaphore(&(cid)->HWLock);}
#define UNLOCK_HW                                       {ReleaseSemaphore(&(cid)->HWLock);}

#define LOCK_MULTI_BITMAP                               {ObtainSemaphore(&(cid)->MultiBMLock);}
#define UNLOCK_MULTI_BITMAP                             {ReleaseSemaphore(&(cid)->MultiBMLock);}

#endif
