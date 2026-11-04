/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    NVK's two OpenCL C kernels (src/nouveau/vulkan/cl/*.cl), as NIR
    builder code. Upstream compiles them with mesa_clc into SPIR-V and
    vtn_bindgen2 writes this header for them; neither tool exists on the
    build host, so the kernels are written out by hand with the same
    entry points, so that the two users (nvk_query_pool.c and
    nvk_cmd_indirect.c) are unchanged.
*/

#ifndef NVKCL_H
#define NVKCL_H

#include "compiler/nir/nir.h"
#include "compiler/nir/nir_builder.h"

#ifdef __cplusplus
extern "C" {
#endif

/* cl/nvk_query.cl: vkCmdCopyQueryPoolResults, one query per invocation */
void
nvk_copy_queries(nir_builder *b, nir_def *pool_addr, nir_def *available_stride,
                 nir_def *reports_start, nir_def *report_count,
                 nir_def *query_stride, nir_def *first_query,
                 nir_def *query_count, nir_def *dst_addr, nir_def *dst_stride,
                 nir_def *flags);

/* cl/nvk_copy_indirect.cl: one VkCopyMemoryIndirectCommandKHR per
 * invocation, turned into the copy engine's method words */
void
nvk_copy_indirect(nir_builder *b, nir_def *in, nir_def *in_stride,
                  nir_def *out, nir_def *count);

#ifdef __cplusplus
}
#endif

#endif /* NVKCL_H */
