/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/penmap.h
*/

#ifndef IMAGES_PENMAP_H
#define IMAGES_PENMAP_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif
#ifndef INTUITION_IMAGECLASS_H
#include <intuition/imageclass.h>
#endif

#define PENMAP_CLASSNAME    "penmap.image"
#define PENMAP_VERSION      44

#define PENMAP_Dummy        (REACTION_Dummy + 0x18000)

#define PENMAP_SelectBGPen  (PENMAP_Dummy + 1)  /* (WORD) Selected render background pen */
#define PENMAP_SelectData   (PENMAP_Dummy + 2)  /* Optional selected state render data */
#define PENMAP_RenderBGPen  IA_BGPen            /* (WORD) Background pen */
#define PENMAP_RenderData   IA_Data             /* Normal state render data */
#define PENMAP_Palette      (PENMAP_Dummy + 3)  /* Palette data */
#define PENMAP_Screen       (PENMAP_Dummy + 4)  /* (struct Screen *) Display screen */
#define PENMAP_ImageType    (PENMAP_Dummy + 5)  /* (UWORD) Image type (unsupported) */
#define PENMAP_Transparent  (PENMAP_Dummy + 6)  /* (UWORD) Map color 0 to background pen */
#define PENMAP_Precision    (PENMAP_Dummy + 8)  /* (UWORD) ObtainBestPen precision */
#define PENMAP_ColorMap     (PENMAP_Dummy + 9)  /* (struct ColorMap *) Colormap for remap */
#define PENMAP_MaskBlit     (PENMAP_Dummy + 10) /* (BOOL) Use blitmask for transparency */

/* PENMAP_ImageType values */
#define IMAGE_CHUNKY    0   /* Chunky pixel data (default) */
#define IMAGE_IMAGE     1   /* Image struct (unsupported) */
#define IMAGE_DRAWLIST  2   /* DrawList data (unsupported) */

/* Macros to extract source dimensions from penmap data */
#ifndef IMAGE_WIDTH
#define IMAGE_WIDTH(i)  (((UWORD *)(i))[0])
#endif
#ifndef IMAGE_HEIGHT
#define IMAGE_HEIGHT(i) (((UWORD *)(i))[1])
#endif

#ifndef PenMapObject
#define PenMapObject    NewObject(NULL, PENMAP_CLASSNAME
#endif
#ifndef PenMapEnd
#define PenMapEnd       TAG_END)
#endif

#endif /* IMAGES_PENMAP_H */
