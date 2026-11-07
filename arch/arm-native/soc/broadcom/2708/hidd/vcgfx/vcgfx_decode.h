/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The decode interface and decode-target bitmaps (vcgfx_decode.c).
*/

#ifndef VCGFX_DECODE_H
#define VCGFX_DECODE_H

#include <hidd/gfx.h>
#include <oop/oop.h>

struct vcgfx_dectgt;
struct VideoCoreGfx_staticdata;

extern const struct HIDD_DecodeInterface vcgfx_decode_vectors;
extern const ULONG vcgfx_decode_datasize;

/* OffBM hooks: a bitmap created with aHidd_BitMap_DecodeTarget */
OOP_Object *vcgfx_dectgt_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg, BOOL *handled);
void vcgfx_dectgt_dispose(struct VideoCoreGfx_staticdata *xsd, struct vcgfx_dectgt *t);
BOOL vcgfx_dectgt_lock(struct vcgfx_dectgt *t, struct pHidd_BitMap_LockBuffer *msg);
void vcgfx_dectgt_unlock(struct vcgfx_dectgt *t, struct pHidd_BitMap_UnlockBuffer *msg);

/* Planes: a buffer's pixels (after its decode), SAND128 column pitch and
 * chroma offset; TRUE for 10-bit columns */
UBYTE *vcgfx_dectgt_page(struct vcgfx_dectgt *t, ULONG b);
BOOL vcgfx_dectgt_sand(struct vcgfx_dectgt *t, ULONG *pitch, ULONG *chroma);

#endif /* VCGFX_DECODE_H */
