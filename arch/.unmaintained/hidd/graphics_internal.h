#ifndef GRAPHICS_INTERNAL_H
#define GRAPHICS_INTERNAL_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GfxHIDD specific Internal Information Management
    Lang: english
*/

#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif
#ifndef HIDD_GRAPHICS_H
#   include <hidd/graphics.h>
#endif

#include "fakegfxhidd.h"

HIDDT_StdPixFmt cyber2hidd_pixfmt(UWORD cpf, struct GfxBase *GfxBase);
UWORD hidd2cyber_pixfmt(HIDDT_StdPixFmt stdpf, struct GfxBase *GfxBase);

#if 0
VOID activatebm_callback(APTR data, OOP_Object *bmobj, BOOL activated);
BOOL init_activescreen_stuff(struct GfxBase *GfxBase);
VOID cleanup_activescreen_stuff(struct GfxBase *GfxBase);
#endif

APTR build_dispinfo_db(struct GfxBase *GfxBase);
VOID destroy_dispinfo_db(APTR dispinfo_db, struct GfxBase *GfxBase);
HIDDT_ModeID get_hiddmode_for_amigamodeid(ULONG modeid, struct GfxBase *GfxBase);
HIDDT_ModeID get_best_resolution_and_depth(struct GfxBase *GfxBase);


/* A Pointer to this struct is stored in each RastPort->longreserved[0] */

struct gfx_driverdata {
    UWORD	* dd_AreaPtrn;		/* Amiga current AreaPtrn	*/
    BYTE	  dd_AreaPtSz;		/* Amiga AreaPtSz		*/
    UWORD	  dd_LinePtrn;		/* Amiga current LinePtrn	*/
    OOP_Object	* dd_GC;
    struct RastPort * dd_RastPort;	/* This RastPort		*/
    
};


#define PRIV_GFXBASE(base) ((struct GfxBase_intern *)base)

#define SDD(base)  ((struct shared_driverdata *)PRIV_GFXBASE(base)->shared_driverdata)

#define OOPBase (SDD(GfxBase)->oopbase)

typedef struct {
    int just_for_type_checking;
} ObjectCache;

ObjectCache *create_object_cache(OOP_Class *classPtr, STRPTR classID, struct TagItem *createTags, struct GfxBase *GfxBase);
VOID delete_object_cache(ObjectCache *objectCache, struct GfxBase *GfxBase);
OOP_Object *obtain_cache_object(ObjectCache *objectCache, struct GfxBase *GfxBase);
VOID release_cache_object(ObjectCache *objectCache, OOP_Object *object, struct GfxBase *GfxBase);


struct shared_driverdata
{
    OOP_Object *gfxhidd;
    OOP_Object *gfxhidd_orig;
    OOP_Object *gfxhidd_fake;
    
    struct Library *oopbase;
    ObjectCache *gc_cache;
    ObjectCache *planarbm_cache;
    
    OOP_Object	*framebuffer;
    
    OOP_Object	*bm_bak;
    OOP_Object	*colmap_bak;
    HIDDT_ColorModel colmod_bak;
    
    
    /* The frontmost screen's bitmap */
    struct BitMap *frontbm;

    /* Does the gfx hidd support hardware pointers ? */    
    BOOL has_hw_cursor;

    /* This is used if the gfx hidd does not support hardware mouse pointers */
    OOP_Object  *pointerbm;
    LONG pointer_x;
    LONG pointer_y;
    
    struct class_static_data fakegfx_staticdata;
    BOOL fakegfx_inited;
    
#if 0    
    /* Has the code to handle active screens been activated ? */
    BOOL activescreen_inited;
#endif    
    APTR dispinfo_db; /* Display info database */
};

#include "macros.h"
#endif /* GRAPHICS_INTERNAL_H */
