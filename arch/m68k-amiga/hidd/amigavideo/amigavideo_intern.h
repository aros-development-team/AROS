#ifndef AMIGAVIDEO_INTERN_H
#define AMIGAVIDEO_INTERN_H

/* Private definitions for AmigaVideo class */

#define USE_FAST_BMSTACKCHANGE		1
#define USE_FAST_BMPOSCHANGE		1
//#define USE_ALIEN_DISPLAYMODES		1
#define USE_UCOP_DIRECT             1

#define AMIGAVIDEO_LIBNAME          "amigavideo.hidd"

#define CLID_Hidd_Gfx_AmigaVideo    "hidd.gfx.amigavideo"

#define COPPEROUT(cop_ptr,val_a,val_b) \
    *cop_ptr++ = val_a; \
    *cop_ptr++ = val_b;

struct copper2data
{
    struct MinNode            cnode;

    UWORD                       *copper2_palette;
    UWORD                       *copper2_palette_aga_lo;
    UWORD                       *copper2_scroll;
    UWORD                       *copper2_bpl;
    UWORD                       *copper2_fmode;
    UWORD                       *copper2_tail;

    UBYTE                       extralines;
};


struct NativeChipsetMode
{
    struct Node                 node;
    ULONG                       modeid;
    UWORD                       width, height, depth;
    OOP_Object                  *pf;
    OOP_Object                  *sync;
    UBYTE                       special;
};

struct amigavideo_staticdata
{
    OOP_Class 	    	        *cs_basebm;           /* baseclass for CreateObject */

    OOP_Class 	    	        *amigagfxclass;
    OOP_Class 	    	        *amigacompositorclass;
    OOP_Class 	    	        *amigabmclass;

    OOP_AttrBase                hiddAttrBase;
    OOP_AttrBase                hiddBitMapAttrBase;
    OOP_AttrBase                hiddPlanarBitMapAttrBase;
    OOP_AttrBase                hiddAmigaVideoBitMapAttrBase;
    OOP_AttrBase                hiddGCAttrBase;
    OOP_AttrBase                hiddSyncAttrBase;
    OOP_AttrBase                hiddPixFmtAttrBase;
    OOP_AttrBase                hiddGfxAttrBase;
    OOP_AttrBase                hiddCompositorAttrBase;
    OOP_AttrBase                hiddColorMapAttrBase;

    struct List                 nativemodelist;

    struct List                 *compositedbms;
    struct List                 *obscuredbms;
    struct MinList              c2fragments;
    struct MinList              c2ifragments;

    struct Interrupt            inter;
    volatile UWORD              framecounter;
    struct amigabm_data         *updatescroll;

    WORD                        width_alignment;
    WORD                        startx, starty;

    UWORD                       *copper1;
    UWORD                       *copper1_pt2;
    UWORD                       *copper1_spritept;
    UWORD                       *copper2_backup;
    UWORD                       spritedatasize;
    WORD                        sprite_width, sprite_height;
    UWORD                       spritepos, spritectl;
    UWORD                       *sprite_null;
    UWORD                       *sprite;
    WORD                        spritex, spritey;
    BYTE                        sprite_offset_x, sprite_offset_y;
    UWORD                       bplcon0_null,
                                bplcon1, bplcon2, bplcon3, bplcon4;

    UBYTE                       fmode_bpl, fmode_spr;
    UBYTE                       initialized;

    UWORD                       max_colors;

    void                        (*acb)(void *data, void *bm);
    APTR                        acbdata;

    BPTR                        cs_SegList;

    struct Library              *cs_OOPBase;
    struct Library              *cs_GfxBase;
    struct Library              *cs_UtilityBase;

    OOP_MethodID                cs_HiddGfxBase;
    OOP_MethodID                cs_HiddBitMapBase;

    OOP_MethodID                mid_BitMapStackChanged;
    OOP_MethodID                mid_BitMapPositionChanged;
    OOP_MethodID                mid_BitMapRectChanged;
    OOP_MethodID                mid_ValidateBitMapPositionChange;

    /* flags */
    BOOL                        superforward;
    BOOL                        ecs_agnus, ecs_denise, aga;
    BOOL                        aga_enabled;
    BOOL                        cursorvisible;
    BOOL                        palmode;
    BOOL                        interlaced;
};

struct amigavideoclbase
{
    struct Library              library;
    
    struct amigavideo_staticdata csd;
};

#undef CSD
#define CSD(cl)     	                (&((struct amigavideoclbase *)cl->UserData)->csd)

#define __IHidd 	                (csd->hiddAttrBase)
#define __IHidd_BitMap	                (csd->hiddBitMapAttrBase)
#define __IHidd_Compositor              (csd->hiddCompositorAttrBase)
#define __IHidd_PlanarBM	        (csd->hiddPlanarBitMapAttrBase)
#define __IHidd_BitMap_AmigaVideo       (csd->hiddAmigaVideoBitMapAttrBase)
#define __IHidd_GC			(csd->hiddGCAttrBase)
#define __IHidd_Sync	                (csd->hiddSyncAttrBase)
#define __IHidd_PixFmt		        (csd->hiddPixFmtAttrBase)
#define __IHidd_Gfx 	                (csd->hiddGfxAttrBase)
//#define __IHidd_Attr		        (csd->hiddAttrBase)
#define __IHidd_ColorMap	        (csd->hiddColorMapAttrBase)

#endif /* ! AMIGAVIDEO_INTERN_H */
