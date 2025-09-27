#ifndef HIDD_AMIGAVIDEO_H
#define HIDD_AMIGAVIDEO_H

/* Amiga Gfx hidd interface */
#include <interface/Hidd_AmigaGfx.h>

#define CLID_Hidd_Gfx_AmigaVideo    "hidd.gfx.amigavideo"

#define IS_AMIGAVIDEO_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HiddAmigaGfxAttrBase, num_Hidd_AmigaGfx_Attrs)

#endif /* !HIDD_AMIGAVIDEO_H */
