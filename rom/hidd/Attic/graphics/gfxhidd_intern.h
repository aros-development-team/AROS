#ifndef GRAPHICS_HIDD_INTERN_H
#define GRAPHICS_HIDD_INTERN_H

/* Include files */
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef PROTO_GRAPHICS_H
#   include <proto/graphics.h>
#endif
#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif
#ifndef GRAPHICS_GFX_H
#   include <graphics/gfx.h>
#endif
#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif
#ifndef PROTO_INTUITION_H
#   include <proto/intuition.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif
#ifndef INTUITION_SCREENS_H
#   include <intuition/screens.h>
#endif
#ifndef PROTO_UTILITY_H
#   include <proto/utility.h>
#endif

/* Use intuition boopsi on amigaos systems */
#ifndef __amigaos__
#ifndef PROTO_BOOPSI_H
#   include <proto/boopsi.h>
#endif
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
struct GfxHiddData
{
    Class *bitmap;  /* bitmap class     */
    Class *gc;      /* graphics context */
};


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


struct GfxHiddBase_intern
{
    struct Library    library;
    struct ExecBase * sysbase;
    BPTR              seglist;

    struct IntuitionBase * intuibase;
    struct Library       * dosbase;
    struct GfxBase       * gfxbase;
    struct Library       * utilitybase;
    struct Library       * boopsibase;

    Class                *classptr;       /* graphics hidd class    */
    Class                *bitMapClassptr; /* bitmap class           */
    Class                *gcClassptr;     /* graphics context class */
};

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers  and the corresponding
   structs are equal.
   This is a hack, of course. */
typedef struct IntuitionBase IntuiBase;
typedef struct GfxBase GraphicsBase;

#define GTB(gtb)        ((struct GfxHiddBase_intern *)gtb)
/*
#undef SysBase
#define SysBase (GTB(GfxHiddBase)->sysbase)
*/
extern struct ExecBase * SysBase;
#undef IntuitionBase
#define IntuitionBase (GTB(GfxHiddBase)->intuibase)
#undef DOSBase
#define DOSBase (GTB(GfxHiddBase)->dosbase)
#undef GfxBase
#define GfxBase (GTB(GfxHiddBase)->gfxbase)
#undef UtilityBase
#define UtilityBase (GTB(GfxHiddBase)->utilitybase)
#undef BOOPSIBase
#define BOOPSIBase (GTB(GfxHiddBase)->boopsibase)

#define expunge() \
AROS_LC0(BPTR, expunge, struct GfxHiddBase_intern *, GfxHiddBase, 3, GfxHidd)

#endif /* GRAPHICS_HIDD_INTERN_H */
