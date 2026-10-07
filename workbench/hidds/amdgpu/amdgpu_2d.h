/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _AMDGPU_2D_H
#define _AMDGPU_2D_H

#include <exec/types.h>

/* smaller operations stay on the CPU, below the cost of a GPU round trip */
#define AMDGPU_2D_MIN_FILL  16384
#define AMDGPU_2D_MIN_READ  512

struct Amdgpu_KMS;
struct BitmapData;

void Amdgpu_2D_Init(void);
BOOL Amdgpu_2D_CopyBox(struct Amdgpu_KMS *kms, struct BitmapData *bm, LONG sx, LONG sy,
                       LONG dx, LONG dy, LONG w, LONG h);
BOOL Amdgpu_2D_Fill(struct Amdgpu_KMS *kms, struct BitmapData *bm, LONG x, LONG y, LONG w, LONG h, ULONG pixel);
BOOL Amdgpu_2D_Read(struct Amdgpu_KMS *kms, struct BitmapData *bm, LONG x, LONG y, LONG w, LONG h,
                    UBYTE *dst, ULONG dstmod);

#endif /* _AMDGPU_2D_H */
