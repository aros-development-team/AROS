#ifndef GRAPHICS_HIDD_INTERN_H
#define GRAPHICS_HIDD_INTERN_H

/* Include files */
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif



/*
    This is the default bitmap structure for a graphics hidd. If you
    use this or an extension auf this structure(eg. hGfx_bitMapInt)
    you must not override the Set() and Get() functions and methods
    for bitmaps of the default graphics hidd.
*/
struct hGfx_bitMap
{
    ULONG width;         /* width of the bitmap in pixel  */
    ULONG height;        /* height of the bitmap in pixel */
    ULONG depth;         /* depth of the bitmap in        */
    ULONG flags;         /* see hidd/graphic.h 'flags for */
                         /* HIDD_Graphics_CreateBitMap'   */
    ULONG bytesPerRow;   /* bytes per row                 */
    ULONG bytesPerPixel; /* bytes per pixel               */
    APTR colorTab;       /* color table of the bitmap     */
};

/*
    This is the default graphics context structure for a graphics hidd. If
    you use this or an extension auf this structure(eg. hGfx_gcInt) you must
    not override the Set() and Get() functions and methods for graphics
    contextes of the default graphics hidd.
*/
struct hGfx_gc
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
    /* WARNING: structure could be extented in the future                */
};


struct GfxHidd
{
};


/* GfxHiddClass definitions */


/* GfxHiddBitMapClass definitions */
struct GfxHiddBitMapData
{
    struct hGfx_bitMap *bitMap;
};


/* GfxHiddClass definitions */
struct GfxHiddGCData
{
    struct hGfx_gc *gc;
};


struct class_static_data
{
    struct ExecBase * sysbase;
    struct Library       * utilitybase;
    struct Library       * oopbase;

    Class                *gfxhiddclass;       /* graphics hidd class    */
    Class                *bitmapclass; /* bitmap class           */
    Class                *gcclass;     /* graphics context class */
};


#define CSD(x) ((struct class_static_data *)x)


#undef SysBase
#define SysBase (CSD(cl->UserData)->sysbase)

#undef UtilityBase
#define UtilityBase (CSD(cl->UserData)->utilitybase)


#undef OOPBase
#define OOPBase (CSD(cl->UserData)->oopbase)

Class *init_gfxhiddclass(struct class_static_data *);
Class *init_bitmapclass(struct class_static_data *);
Class *init_gcclass(struct class_static_data *);

#endif /* GRAPHICS_HIDD_INTERN_H */
