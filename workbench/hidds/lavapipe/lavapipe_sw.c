/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    The software rasterizer selection for lavapipe's pipe loader: the
    sw_screen_create_vk() the loader's null-winsys device calls, which is
    llvmpipe here (GALLIUM_LLVMPIPE). Mesa's own lavapipe target does the
    same with this header.
*/

/* p_screen.h first: sw_helper_public.h only forward-declares the config */
#include "pipe/p_screen.h"
#include "target-helpers/sw_helper.h"
