#ifndef GRAPHICS_RENDERFUNC_H
#define GRAPHICS_RENDERFUNC_H

#include <graphics/gfxbase.h>
#include <oop/oop.h>

/* The stuff defined in this file is in fact private. Don't use it. */

typedef ULONG (*RENDERFUNC)(APTR, LONG, LONG, OOP_Object *, OOP_Object *, LONG, LONG, LONG, LONG, struct GfxBase *);
typedef LONG (*PIXELFUNC)(APTR, OOP_Object *, OOP_Object *, LONG, LONG, struct GfxBase *);

#endif
