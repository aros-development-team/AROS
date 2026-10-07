#ifndef AMDGPU_HIDD_H
#define AMDGPU_HIDD_H

/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Desc: Amdgpu Gfx Hidd data.
*/

#include <exec/interrupts.h>

#include "amdgpu_bitmap.h"

#define IID_Hidd_Amdgpu  "hidd.gfx.amdgpu"
#define CLID_Hidd_Amdgpu "hidd.gfx.amdgpu"
#define CLID_Hidd_Display_Amdgpu "hidd.display.amdgpu"
#define CLID_Hidd_Gallium_Amdgpu  "hidd.gallium.amdgpu"

struct AmdgpuHiddData
{
    struct Interrupt ResetInterrupt;
};

struct AmdgpuDisplayData
{
};

#endif /* AMDGPU_HIDD_H */
