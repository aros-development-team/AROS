/* Copyright (C) 2026, The AROS Development Team. All rights reserved.
 * VirtIO GPU 1.3 wire layouts for the x86_64 manual diagnostic.
 * VirGL numeric definitions are a subset of Mesa 26.2.5 virgl_protocol.h
 * and virgl_hw.h; their notice follows. No Linux driver code is imported.
 */
/*
 * Copyright 2014, 2015 Red Hat.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef AROS_TEST_VIRTIOGPU_PROTOCOL_H
#define AROS_TEST_VIRTIOGPU_PROTOCOL_H

#include <exec/types.h>

#define GPU_F_VIRGL 1ULL
#define GPU_FLAG_FENCE 1
#define GPU_GET_DISPLAY 0x100
#define GPU_RESOURCE_UNREF 0x102
#define GPU_ATTACH_BACKING 0x106
#define GPU_DETACH_BACKING 0x107
#define GPU_GET_CAPSET_INFO 0x108
#define GPU_GET_CAPSET 0x109
#define GPU_CTX_CREATE 0x200
#define GPU_CTX_DESTROY 0x201
#define GPU_CTX_ATTACH_RESOURCE 0x202
#define GPU_CTX_DETACH_RESOURCE 0x203
#define GPU_RESOURCE_CREATE_3D 0x204
#define GPU_TRANSFER_TO_HOST_3D 0x205
#define GPU_TRANSFER_FROM_HOST_3D 0x206
#define GPU_SUBMIT_3D 0x207
#define GPU_OK_NODATA 0x1100
#define GPU_OK_DISPLAY 0x1101
#define GPU_OK_CAPSET_INFO 0x1102
#define GPU_OK_CAPSET 0x1103

#define VIRGL_CREATE_OBJECT 1
#define VIRGL_DESTROY_OBJECT 3
#define VIRGL_SET_FRAMEBUFFER 5
#define VIRGL_CLEAR 7
#define VIRGL_BLIT 16
#define VIRGL_COPY_REGION 17
#define VIRGL_OBJECT_SURFACE 8
#define VIRGL_TEXTURE_2D 2
#define VIRGL_FORMAT_RGBA8 67
#define VIRGL_BIND_RENDER_TARGET 2
#define VIRGL_CLEAR_COLOR0 4
#define VIRGL_MASK_RGBA 15
#define FLOAT_ONE_BITS 0x3f800000

struct gpu_header {
    ULONG type, flags;
    UQUAD fence;
    ULONG context;
    UBYTE ring, padding[3];
};
struct capset_request {
    struct gpu_header header;
    ULONG id, version;
};
struct capset_info {
    struct gpu_header header;
    ULONG id, version, size, padding;
};
struct gpu_rect { ULONG x, y, width, height; };
struct gpu_display { struct gpu_rect rect; ULONG enabled, flags; };
struct gpu_display_info {
    struct gpu_header header;
    struct gpu_display display[16];
};

struct render_create {
    struct gpu_header header;
    ULONG resource, target, format, bind, width, height, depth, array_size;
    ULONG last_level, samples, flags, padding;
};
struct render_context { struct gpu_header header; ULONG length, init; char name[64]; };
struct render_resource { struct gpu_header header; ULONG resource, padding; };
struct render_backing {
    struct gpu_header header;
    ULONG resource, count;
    UQUAD address;
    ULONG length, padding;
};
struct render_transfer {
    struct gpu_header header;
    ULONG x, y, z, width, height, depth;
    UQUAD offset;
    ULONG resource, level, stride, layer_stride;
};
struct render_submit { struct gpu_header header; ULONG size, padding; ULONG words[32]; };
_Static_assert(sizeof(struct render_create) == 72, "Create3D wire size");
_Static_assert(sizeof(struct render_context) == 96, "Context wire size");
_Static_assert(sizeof(struct render_backing) == 48, "Backing wire size");
_Static_assert(sizeof(struct render_transfer) == 72, "Transfer wire size");
_Static_assert(sizeof(struct gpu_header) == 24, "GPU header wire size");
_Static_assert(sizeof(struct capset_info) == 40, "Capset info wire size");

#endif
