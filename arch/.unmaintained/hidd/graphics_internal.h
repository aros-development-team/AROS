#ifndef GRAPHICS_INTERNAL_H
#define GRAPHICS_INTERNAL_H


/*
    (C) 1995-98 AROS - The Amiga Research OS
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


HIDDT_StdPixFmt cyber2hidd_pixfmt(UWORD cpf, struct GfxBase *GfxBase);
UWORD hidd2cyber_pixfmt(HIDDT_StdPixFmt stdpf, struct GfxBase *GfxBase);

VOID activatebm_callback(APTR data, Object *bmobj, BOOL activated);
BOOL init_activescreen_stuff(struct GfxBase *GfxBase);
VOID cleanup_activescreen_stuff(struct GfxBase *GfxBase);

APTR build_dispinfo_db(struct List *gfxmodes, struct GfxBase *GfxBase);
VOID destroy_dispinfo_db(APTR dispinfo_db, struct GfxBase *GfxBase);


/* A Pointer to this struct is stored in each RastPort->longreserved[0] */

struct gfx_driverdata {
    UWORD	* dd_AreaPtrn;		/* Amiga current AreaPtrn	*/
    BYTE	  dd_AreaPtSz;		/* Amiga AreaPtSz		*/
    UWORD	  dd_LinePtrn;		/* Amiga current LinePtrn	*/
    Object	* dd_GC;
    struct RastPort * dd_RastPort;	/* This RastPort		*/
    
};


#define PRIV_GFXBASE(base) ((struct GfxBase_intern *)base)

#define SDD(base)  ((struct shared_driverdata *)PRIV_GFXBASE(base)->shared_driverdata)

#define OOPBase (SDD(GfxBase)->oopbase)

typedef struct {
    int just_for_type_checking;
} ObjectCache;

ObjectCache *create_object_cache(Class *classPtr, STRPTR classID, struct TagItem *createTags, struct GfxBase *GfxBase);
VOID delete_object_cache(ObjectCache *objectCache, struct GfxBase *GfxBase);
Object *obtain_cache_object(ObjectCache *objectCache, struct GfxBase *GfxBase);
VOID release_cache_object(ObjectCache *objectCache, Object *object, struct GfxBase *GfxBase);



struct shared_driverdata
{
    Object *gfxhidd;
    struct Library *oopbase;
    ObjectCache *gc_cache;
    ObjectCache *planarbm_cache;
    
    /* Has the code to handle active screens been activated ? */
    BOOL activescreen_inited;
    APTR dispinfo_db; /* Display info database */
    struct List *queried_modes; /* gfxmodes gotten from the hidd */
};

#include "macros.h"
#endif /* GRAPHICS_INTERNAL_H */
