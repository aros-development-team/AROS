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

typedef ObjectCache;

VOID activatebm_callback(APTR data, Object *bmobj, BOOL activated);
BOOL init_activescreen_stuff(struct GfxBase *GfxBase);
VOID cleanup_activescreen_stuff(struct GfxBase *GfxBase);

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


struct shared_driverdata
{
    Object *gfxhidd;
    struct Library *oopbase;
    ObjectCache *gc_cache;
    ObjectCache *planarbm_cache;
    
    
    BOOL activescreen_inited;


};


#endif /* GRAPHICS_INTERNAL_H */
