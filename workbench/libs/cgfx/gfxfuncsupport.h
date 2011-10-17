#include <hidd/graphics.h>
#include <cybergraphx/cybergraphics.h>

#include "cybergraphics_intern.h"

/* These are the only bitmap internals on which we depend. */
#define IS_HIDD_BM(bitmap)  ((bitmap)->Flags & BMF_SPECIALFMT)
#define HIDD_BM_OBJ(bitmap) ((OOP_Object *)((bitmap)->Planes[0]))

extern BYTE hidd2cyber_pixfmt[];
