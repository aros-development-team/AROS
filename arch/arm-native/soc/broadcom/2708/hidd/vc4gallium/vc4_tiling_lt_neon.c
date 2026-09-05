/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The second build of Mesa's vc4_tiling_lt.c, the one that defines the
    _neon tagged entry points vc4_tiling.h calls when the CPU reports NEON;
    without it the driver does not link. Upstream's meson build compiles the
    same source a second time into a library of its own, so include it here
    and let the build compile this file instead. The raspi target already
    carries -mfpu=neon-vfpv4, so the assembly paths in it are available.
*/

#define V3D_BUILD_NEON
#include "vc4_tiling_lt.c"
