/*
 * Copyright 2007 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __NV30_SHADERS_H__
#define __NV30_SHADERS_H__

#define NV_SHADER_MAX_PROGRAM_LENGTH 256

#include "nv_include.h"

typedef struct nv_shader {
	uint32_t hw_id;
	uint32_t size;
	union {
		struct {
			uint32_t vp_in_reg;
			uint32_t vp_out_reg;
		} NV30VP;
		struct  {
			uint32_t num_regs;
		} NV30FP;
	} card_priv;
	uint32_t data[NV_SHADER_MAX_PROGRAM_LENGTH];
} nv_shader_t;

void NV30_UploadFragProg(NVPtr pNv, nv_shader_t *shader, int *hw_offset);
void NV40_UploadVtxProg(NVPtr pNv, nv_shader_t *shader, int *hw_id);
void NV40_LoadVtxProg(ScrnInfoPtr pScrn, nv_shader_t *shader);
Bool NV40_LoadFragProg(ScrnInfoPtr pScrn, nv_shader_t *shader);
Bool NV30_LoadFragProg(ScrnInfoPtr pScrn, nv_shader_t *shader);


/*******************************************************************************
 * NV40/G70 vertex shaders
 */

nv_shader_t nv40_vp_exa_render;
nv_shader_t nv40_vp_video;

/*******************************************************************************
 * NV30/NV40/G70 fragment shaders
 */

nv_shader_t nv30_fp_pass_col0;
nv_shader_t nv30_fp_pass_tex0;
nv_shader_t nv30_fp_composite_mask;
nv_shader_t nv30_fp_composite_mask_sa_ca;
nv_shader_t nv30_fp_composite_mask_ca;
nv_shader_t nv30_fp_yv12_bicubic;
nv_shader_t nv30_fp_yv12_bilinear;
nv_shader_t nv40_fp_yv12_bicubic;

#endif
