/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _AMDGPU_2D_H
#define _AMDGPU_2D_H

#include <exec/types.h>
#include <oop/oop.h>

/* smaller operations stay on the CPU, below the cost of a GPU round trip */
#define AMDGPU_2D_MIN_FILL  16384
#define AMDGPU_2D_MIN_READ  512

struct Amdgpu_staticdata;
struct Amdgpu_FB;

/* Operations on a scan-out bitmap done by the SDMA engine (amdgpu_2d.c);
   FALSE when it cannot, and the caller does them itself. Sync waits for
   them, before the CPU touches a bitmap. */
void Amdgpu_2D_Init(struct Amdgpu_staticdata *sd);
void Amdgpu_2D_Exit(void);
void Amdgpu_2D_ReleaseFB(struct Amdgpu_FB *fb);
void Amdgpu_2D_Sync(void);
BOOL Amdgpu_2D_CopyBox(OOP_Object *bm, LONG sx, LONG sy, LONG dx, LONG dy, LONG w, LONG h);
BOOL Amdgpu_2D_Fill(OOP_Object *bm, LONG x, LONG y, LONG w, LONG h, ULONG pixel);
BOOL Amdgpu_2D_Read(OOP_Object *bm, LONG x, LONG y, LONG w, LONG h, UBYTE *dst, ULONG dstmod);

#endif /* _AMDGPU_2D_H */
