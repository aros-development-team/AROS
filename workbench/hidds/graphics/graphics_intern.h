#ifndef GRAPHICS_HIDD_INTERN_H
#define GRAPHICS_HIDD_INTERN_H

/* Include files */

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#ifndef HIDD_GRAPHICS_H
#   include <hidd/graphics.h>
#endif
#include <dos/dos.h>


/* Instance data of GfxMode objects. We have it defined here so we can
   access GfxMode objects' instance data directly (like BOOPSI gadgets)
   in GraphicsClass
*/
struct gfxmode_data {
    ULONG width;
    ULONG height;
    
    Object *pixfmt;
    
    UWORD depth;
    
};

struct HIDDGraphicsData
{
	struct MinList modelist;
};

struct HIDDBitMapData
{
    struct _hidd_bitmap_protected prot;
    
    ULONG width;         /* width of the bitmap in pixel  */
    ULONG height;        /* height of the bitmap in pixel */
    ULONG reqdepth;	 /* Depth as requested by user */
    BOOL  displayable;   /* bitmap displayable?           */
    ULONG format;        /* planar or chunky              */
    ULONG flags;         /* see hidd/graphic.h 'flags for */
                         /* HIDD_Graphics_CreateBitMap'   */
    ULONG bytesPerRow;   /* bytes per row                 */
    ULONG bytesPerPixel; /* bytes per pixel               */
    APTR  colorTab;      /* color table of the bitmap     */
    HIDDT_Color *coltab;

    HIDDT_Pixel fg;        /* foreground color                                 */
    HIDDT_Pixel bg;        /* background color                                 */
    ULONG drMode;    /* drawmode                                         */
    /* WARNING: type of font could be change */
    APTR  font;      /* current fonts                                    */
    ULONG colMask;   /* ColorMask prevents some color bits from changing */
    UWORD linePat;   /* LinePattern                                      */
    APTR  planeMask; /* Pointer to a shape bitMap                        */
    Object *gc;
    Object *bitMap;
    /* WARNING: structure could be extented in the future                */
    ULONG colExp;	/* Color expansion mode	*/
    
    Object *friend;	/* Friend bitmap */

};


struct HIDDGCData
{
    APTR bitMap;     /* bitmap to which this gc is connected             */
    ULONG fg;        /* foreground color                                 */
    ULONG bg;        /* background color                                 */
    ULONG drMode;    /* drawmode                                         */
    /* WARNING: type of font could be change */
    APTR  font;      /* current fonts                                    */
    ULONG colMask;   /* ColorMask prevents some color bits from changing */
    UWORD linePat;   /* LinePattern                                      */
    APTR  planeMask; /* Pointer to a shape bitMap                        */
    APTR  userData;  /* pointer to own data                              */
    ULONG colExp;
    /* WARNING: structure could be extented in the future                */
};


struct class_static_data
{
    struct ExecBase      * sysbase;
    struct Library       * utilitybase;
    struct Library       * oopbase;

    Class                *gfxhiddclass; /* graphics hidd class    */
    Class                *bitmapclass;  /* bitmap class           */
    Class                *gcclass;      /* graphics context class */
    Class		 *colormapclass; /* colormap class	  */
    
    Class		 *gfxmodeclass;
    Class		 *pixfmtclass;
    
    
    Class		 *planarbmclass;
    Class		 *chunkybmclass;
    
    Object		*std_pixfmts[num_Hidd_StdPixFmt];
};


/* Library base */

struct IntHIDDGraphicsBase
{
    struct Library            hdg_LibNode;
    BPTR                      hdg_SegList;
    struct ExecBase          *hdg_SysBase;
    struct Library           *hdg_UtilityBase;

    struct class_static_data *hdg_csd;
};


#define CSD(x) ((struct class_static_data *)x)

#undef SysBase
#define SysBase (CSD(cl->UserData)->sysbase)

#undef UtilityBase
#define UtilityBase (CSD(cl->UserData)->utilitybase)

#undef OOPBase
#define OOPBase (CSD(cl->UserData)->oopbase)


/* pre declarations */

Class *init_gfxhiddclass(struct class_static_data *csd);
void   free_gfxhiddclass(struct class_static_data *csd);

Class *init_bitmapclass(struct class_static_data *csd);
void   free_bitmapclass(struct class_static_data *csd);

Class *init_gcclass(struct class_static_data *csd);
void   free_gcclass(struct class_static_data *csd);

Class *init_gfxmodeclass(struct class_static_data *csd);
void   free_gfxmodeclass(struct class_static_data *csd);

Class *init_pixfmtclass(struct class_static_data *csd);
void   free_pixfmtclass(struct class_static_data *csd);



VOID  bitmap_putpixel(Class *cl, Object *obj, struct pHidd_BitMap_PutPixel *msg);
ULONG bitmap_getpixel(Class *cl, Object *obj, struct pHidd_BitMap_GetPixel *msg);
VOID bitmap_convertpixels(Class *cl, Object *o, struct pHidd_BitMap_ConvertPixels *msg);


Class *init_planarbmclass(struct class_static_data *csd);
void   free_planarbmclass(struct class_static_data *csd);

Class *init_chunkybmclass(struct class_static_data *csd);
void   free_chunkybmclass(struct class_static_data *csd);


#endif /* GRAPHICS_HIDD_INTERN_H */
