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

#include "nv30_shaders.h"
#include "nv04_pushbuf.h"

void NV30_UploadFragProg(NVPtr pNv, nv_shader_t *shader, int *hw_offset)
{
	uint32_t data, i;
	uint32_t *map;

	shader->hw_id = *hw_offset;

	nouveau_bo_map(pNv->shader_mem, NOUVEAU_BO_WR);
	map = pNv->shader_mem->map + *hw_offset;
	for (i = 0; i < shader->size; i++) {
		data = shader->data[i];
#if (X_BYTE_ORDER != X_LITTLE_ENDIAN)
		data = ((data >> 16) | ((data & 0xffff) << 16));
#endif
		map[i] = data;
	}
	nouveau_bo_unmap(pNv->shader_mem);

	*hw_offset += (shader->size * sizeof(uint32_t));
	*hw_offset = (*hw_offset + 63) & ~63;
}

void NV40_UploadVtxProg(NVPtr pNv, nv_shader_t *shader, int *hw_id)
{
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *curie = pNv->Nv3D;
	int i;

	shader->hw_id = *hw_id;

	BEGIN_RING(chan, curie, NV40TCL_VP_UPLOAD_FROM_ID, 1);
	OUT_RING  (chan, (shader->hw_id));
	for (i=0; i<shader->size; i+=4) {
		BEGIN_RING(chan, curie, NV40TCL_VP_UPLOAD_INST(0), 4);
		OUT_RING  (chan, shader->data[i + 0]);
		OUT_RING  (chan, shader->data[i + 1]);
		OUT_RING  (chan, shader->data[i + 2]);
		OUT_RING  (chan, shader->data[i + 3]);
		(*hw_id)++;
	}
}

Bool
NV30_LoadFragProg(ScrnInfoPtr pScrn, nv_shader_t *shader)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *rankine = pNv->Nv3D;

	BEGIN_RING(chan, rankine, NV34TCL_FP_ACTIVE_PROGRAM, 1);
	if (OUT_RELOC(chan, pNv->shader_mem, shader->hw_id, NOUVEAU_BO_VRAM |
		      NOUVEAU_BO_RD | NOUVEAU_BO_LOW | NOUVEAU_BO_OR,
		      NV34TCL_FP_ACTIVE_PROGRAM_DMA0,
		      NV34TCL_FP_ACTIVE_PROGRAM_DMA1))
		return FALSE;
	BEGIN_RING(chan, rankine, NV34TCL_FP_REG_CONTROL, 1);
	OUT_RING  (chan, (1 << 16)| 0xf);
	BEGIN_RING(chan, rankine, NV34TCL_MULTISAMPLE_CONTROL, 1);
	OUT_RING  (chan, 0xffff0000);

	BEGIN_RING(chan, rankine, NV34TCL_FP_CONTROL,1);
	OUT_RING  (chan, (shader->card_priv.NV30FP.num_regs-1)/2);

	return TRUE;
}

void
NV40_LoadVtxProg(ScrnInfoPtr pScrn, nv_shader_t *shader)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *curie = pNv->Nv3D;

	BEGIN_RING(chan, curie, NV40TCL_VP_START_FROM_ID, 1);
	OUT_RING  (chan, (shader->hw_id));

	BEGIN_RING(chan, curie, NV40TCL_VP_ATTRIB_EN, 2);
	OUT_RING  (chan, shader->card_priv.NV30VP.vp_in_reg);
	OUT_RING  (chan, shader->card_priv.NV30VP.vp_out_reg);
}

Bool
NV40_LoadFragProg(ScrnInfoPtr pScrn, nv_shader_t *shader)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *curie = pNv->Nv3D;

	BEGIN_RING(chan, curie, NV40TCL_FP_ADDRESS, 1);
	if (OUT_RELOC(chan, pNv->shader_mem, shader->hw_id, NOUVEAU_BO_VRAM |
		      NOUVEAU_BO_GART | NOUVEAU_BO_RD | NOUVEAU_BO_LOW |
		      NOUVEAU_BO_OR,
		      NV40TCL_FP_ADDRESS_DMA0, NV40TCL_FP_ADDRESS_DMA1))
		return FALSE;
	BEGIN_RING(chan, curie, NV40TCL_FP_CONTROL, 1);
	OUT_RING  (chan, shader->card_priv.NV30FP.num_regs <<
			 NV40TCL_FP_CONTROL_TEMP_COUNT_SHIFT);

	return TRUE;
}

/*******************************************************************************
 * NV40/G70 vertex shaders
 */

nv_shader_t nv40_vp_exa_render = {
	.card_priv.NV30VP.vp_in_reg  = 0x00000309,
	.card_priv.NV30VP.vp_out_reg = 0x0000c001,
	.size = (3*4),
	.data = {
		/* MOV result.position, vertex.position */
		0x40041c6c, 0x0040000d, 0x8106c083, 0x6041ff80,
		/* MOV result.texcoord[0], vertex.texcoord[0] */
		0x401f9c6c, 0x0040080d, 0x8106c083, 0x6041ff9c,
		/* MOV result.texcoord[1], vertex.texcoord[1] */
		0x401f9c6c, 0x0040090d, 0x8106c083, 0x6041ffa1,
	}
};

/*******************************************************************************
 * NV30/NV40/G70 fragment shaders
 */

nv_shader_t nv30_fp_pass_col0 = {
	.card_priv.NV30FP.num_regs = 2,
	.size = (1*4),
	.data = {
		/* MOV R0, fragment.color */
		0x01403e81, 0x1c9dc801, 0x0001c800, 0x3fe1c800, 
	}
};

nv_shader_t nv30_fp_pass_tex0 = {
	.card_priv.NV30FP.num_regs = 2,
	.size = (2*4),
	.data = {
		/* TEX R0, fragment.texcoord[0], texture[0], 2D */
		0x17009e00, 0x1c9dc801, 0x0001c800, 0x3fe1c800,
		/* MOV R0, R0 */
		0x01401e81, 0x1c9dc800, 0x0001c800, 0x0001c800,
	}
};

nv_shader_t nv30_fp_composite_mask = {
	.card_priv.NV30FP.num_regs = 2,
	.size = (3*4),
	.data = {
		/* TEXC0 R1.w         , fragment.texcoord[1], texture[1], 2D */
		0x1702b102, 0x1c9dc801, 0x0001c800, 0x3fe1c800,
		/* TEX   R0 (NE0.wwww), fragment.texcoord[0], texture[0], 2D */
		0x17009e00, 0x1ff5c801, 0x0001c800, 0x3fe1c800,
		/* MUL   R0           , R0, R1.w */
		0x02001e81, 0x1c9dc800, 0x0001fe04, 0x0001c800,
	}
};

nv_shader_t nv30_fp_composite_mask_sa_ca = {
	.card_priv.NV30FP.num_regs = 2,
	.size = (3*4),
	.data = {
		/* TEXC0 R1.w         , fragment.texcoord[0], texture[0], 2D */
		0x17009102, 0x1c9dc801, 0x0001c800, 0x3fe1c800,
		/* TEX   R0 (NE0.wwww), fragment.texcoord[1], texture[1], 2D */
		0x1702be00, 0x1ff5c801, 0x0001c800, 0x3fe1c800,
		/* MUL   R0           , R1,wwww, R0 */
		0x02001e81, 0x1c9dfe04, 0x0001c800, 0x0001c800,
	}
};

nv_shader_t nv30_fp_composite_mask_ca = {
	.card_priv.NV30FP.num_regs = 2,
	.size = (3*4),
	.data = {
		/* TEXC0 R0           , fragment.texcoord[0], texture[0], 2D */
		0x17009f00, 0x1c9dc801, 0x0001c800, 0x3fe1c800,
		/* TEX   R1 (NE0.xyzw), fragment.texcoord[1], texture[1], 2D */
		0x1702be02, 0x1c95c801, 0x0001c800, 0x3fe1c800,
		/* MUL   R0           , R0, R1 */
		0x02001e81, 0x1c9dc800, 0x0001c804, 0x0001c800,
	}
};

nv_shader_t nv40_vp_video = {
	.card_priv.NV30VP.vp_in_reg  = 0x00000309,
	.card_priv.NV30VP.vp_out_reg = 0x0000c001,
	.size = (3*4),
	.data = {
		/* MOV result.position, vertex.position */
		0x40041c6c, 0x0040000d, 0x8106c083, 0x6041ff80,
		/* MOV result.texcoord[0], vertex.texcoord[0] */
		0x401f9c6c, 0x0040080d, 0x8106c083, 0x6041ff9c,
		/* MOV result.texcoord[1], vertex.texcoord[1] */
		0x401f9c6c, 0x0040090d, 0x8106c083, 0x6041ffa1,
	}
};

nv_shader_t nv40_fp_yv12_bicubic = {
	.card_priv.NV30FP.num_regs = 4,
	.size = (29*4),
	.data = {
		/* INST 0: MOVR R0.xy (TR0.xyzw), attrib.texcoord[0] */
		0x01008600, 0x1c9dc801, 0x0001c800, 0x3fe1c800,
		/* INST 1: ADDR R0.z (TR0.xyzw), R0.yyyy, { 0.50, 0.00, 0.00, 0.00 }.xxxx */
		0x03000800, 0x1c9caa00, 0x00000002, 0x0001c800,
		0x3f000000, 0x00000000, 0x00000000, 0x00000000,
		/* INST 2: ADDR R1.x (TR0.xyzw), R0, { 0.50, 0.00, 0.00, 0.00 }.xxxx */
		0x03000202, 0x1c9dc800, 0x00000002, 0x0001c800,
		0x3f000000, 0x00000000, 0x00000000, 0x00000000,
		/* INST 3: TEXRC0 R1.xyz (TR0.xyzw), R0.zzzz, texture[0] */
		0x17000f82, 0x1c9d5400, 0x0001c800, 0x0001c800,
		/* INST 4: MULR R2.yw (TR0.xyzw), R1.xxyy, { -1.00, 1.00, 0.00, 0.00 }.xxyy */
		0x02001404, 0x1c9ca104, 0x0000a002, 0x0001c800,
		0xbf800000, 0x3f800000, 0x00000000, 0x00000000,
		/* INST 5: TEXR R3.xyz (TR0.xyzw), R1, texture[0] */
		0x17000e86, 0x1c9dc804, 0x0001c800, 0x0001c800,
		/* INST 6: MULR R2.xz (TR0.xyzw), R3.xxyy, { -1.00, 1.00, 0.00, 0.00 }.xxyy */
		0x02000a04, 0x1c9ca10c, 0x0000a002, 0x0001c800,
		0xbf800000, 0x3f800000, 0x00000000, 0x00000000,
		/* INST 7: ADDR R2 (TR0.xyzw), R0.xyxy, R2 */
		0x03001e04, 0x1c9c8800, 0x0001c808, 0x0001c800,
		/* INST 8: TEXR R1.y (TR0.xyzw), R2.zwzz, -texture[1] */
		0x17020402, 0x1c9d5c08, 0x0001c800, 0x0001c800,
		/* INST 9: MADH R1.x (TR0.xyzw), -R1.zzzz, R1.yyyy, R1.yyyy */
		0x04400282, 0x1c9f5504, 0x0000aa04, 0x0000aa04,
		/* INST 10: TEXR R0.y (TR0.xyzw), R2.xwxw, -texture[1] */
		0x17020400, 0x1c9d9808, 0x0001c800, 0x0001c800,
		/* INST 11: MADH R0.w (TR0.xyzw), -R1.zzzz, R0.yyyy, R0.yyyy */
		0x04401080, 0x1c9f5504, 0x0000aa00, 0x0000aa00,
		/* INST 12: TEXR R0.x (TR0.xyzw), R2.zyxy, texture[1] */
		0x17020200, 0x1c9c8c08, 0x0001c800, 0x0001c800,
		/* INST 13: MADH R1.x (TR0.xyzw), R1.zzzz, R0, R1 */
		0x04400282, 0x1c9d5504, 0x0001c800, 0x0001c904,
		/* INST 14: TEXR R0.x (NE0.zzzz), R2, texture[1] */
		0x17020200, 0x1555c808, 0x0001c800, 0x0001c800,
		/* INST 15: MADH R0.x (TR0.xyzw), R1.zzzz, R0, R0.wwww */
		0x04400280, 0x1c9d5504, 0x0001c800, 0x0001ff00,
		/* INST 16: MADH R0.w (TR0.xyzw), -R3.zzzz, R1.xxxx, R1.xxxx */
		0x04401080, 0x1c9f550c, 0x00000104, 0x00000104,
		/* INST 17: TEXR R0.yz (TR0.xyzw), attrib.texcoord[1], abs(texture[2]) */
		0x1704ac80, 0x1c9dc801, 0x0001c800, 0x3fe1c800,
		/* INST 18: MADH R0.x (TR0.xyzw), R3.zzzz, R0, R0.wwww */
		0x04400280, 0x1c9d550c, 0x0001c900, 0x0001ff00,
		/* INST 19: MADH R1.xyz (TR0.xyzw), R0.xxxx, { 1.16, -0.87, 0.53, -1.08 }.xxxx, { 1.16, -0.87, 0.53, -1.08 }.yzww */
		0x04400e82, 0x1c9c0100, 0x00000002, 0x0001f202,
		0x3f9507c8, 0xbf5ee393, 0x3f078fef, 0xbf8a6762,
		/* INST 20: MADH R1.xyz (TR0.xyzw), R0.yyyy, { 0.00, -0.39, 2.02, 0.00 }, R1 */
		0x04400e82, 0x1c9cab00, 0x0001c802, 0x0001c904,
		0x00000000, 0xbec890d6, 0x40011687, 0x00000000,
		/* INST 21: MADH R0.xyz (TR0.xyzw), R0.zzzz, { 1.60, -0.81, 0.00, 0.00 }, R1 + END */
		0x04400e81, 0x1c9d5500, 0x0001c802, 0x0001c904,
		0x3fcc432d, 0xbf501a37, 0x00000000, 0x00000000,
	}
};

nv_shader_t nv30_fp_yv12_bicubic = {
	.card_priv.NV30FP.num_regs = 4,
	.size = (24*4),
	.data = {
		/* INST 0: MOVR R2.xy (TR0.xyzw), attrib.texcoord[0] */
		0x01008604, 0x1c9dc801, 0x0001c800, 0x0001c800,
		/* INST 1: ADDR R0.xy (TR0.xyzw), R2, { 0.50, 0.00, 0.00, 0.00 }.xxxx */
		0x03000600, 0x1c9dc808, 0x00000002, 0x0001c800,
		0x3f000000, 0x00000000, 0x00000000, 0x00000000,
		/* INST 2: TEXR R3.xyz (TR0.xyzw), R0, texture[0] */
		0x17000e06, 0x1c9dc800, 0x0001c800, 0x0001c800,
		/* INST 3: TEXR R0.xyz (TR0.xyzw), R0.yyyy, texture[0] */
		0x17000e00, 0x1c9caa00, 0x0001c800, 0x0001c800,
		/* INST 4: MULR R1.xz (TR0.xyzw), R3.xxyy, { -1.00, 1.00, 0.00, 0.00 }.xxyy */
		0x02000a02, 0x1c9ca00c, 0x0000a002, 0x0001c800,
		0xbf800000, 0x3f800000, 0x00000000, 0x00000000,
		/* INST 5: MULR R1.yw (TR0.xyzw), R0.xxyy, { -1.00, 1.00, 0.00, 0.00 }.xxyy */
		0x02001402, 0x1c9ca000, 0x0000a002, 0x0001c800,
		0xbf800000, 0x3f800000, 0x00000000, 0x00000000,
		/* INST 6: ADDR R2 (TR0.xyzw), R2.xyxy, R1 */
		0x03001e04, 0x1c9c8808, 0x0001c804, 0x0001c800,
		/* INST 7: TEXR R0.x (TR0.xyzw), R2, texture[1] */
		0x17020200, 0x1c9dc808, 0x0001c800, 0x0001c800,
		/* INST 8: TEXR R1.y (TR0.xyzw), R2.xwxw, texture[1] */
		0x17020402, 0x1c9d9808, 0x0001c800, 0x0001c800,
		/* INST 9: TEXR R1.x (TR0.xyzw), R2.zyxy, texture[1] */
		0x17020202, 0x1c9c8c08, 0x0001c800, 0x0001c800,
		/* INST 10: LRPH R0.x (TR0.xyzw), R0.zzzz, R0, R1.yyyy */
		0x1f400280, 0x1c9d5400, 0x0001c800, 0x0000aa04,
		/* INST 11: TEXR R0.y (TR0.xyzw), R2.zwzz, texture[1] */
		0x17020400, 0x1c9d5c08, 0x0001c800, 0x0001c800,
		/* INST 12: LRPH R0.y (TR0.xyzw), R0.zzzz, R1.xxxx, R0 */
		0x1f400480, 0x1c9d5400, 0x00000004, 0x0001c800,
		/* INST 13: LRPH R0.x (TR0.xyzw), R3.zzzz, R0, R0.yyyy */
		0x1f400280, 0x1c9d540c, 0x0001c900, 0x0000ab00,
		/* INST 14: MADH R0.xyz (TR0.xyzw), R0.xxxx, { 1.16, -0.87, 0.53, -1.08 }.xxxx, { 1.16, -0.87, 0.53, -1.08 }.yzww */
		0x04400e80, 0x1c9c0100, 0x00000002, 0x0001f202,
		0x3f9507c8, 0xbf5ee393, 0x3f078fef, 0xbf8a6762,
		/* INST 15: TEXR R1.yz (TR0.xyzw), attrib.texcoord[1], abs(texture[2]) */
		0x1704ac02, 0x1c9dc801, 0x0001c800, 0x0001c800,
		/* INST 16: MADH R0.xyz (TR0.xyzw), R1.yyyy, { 0.00, -0.39, 2.02, 0.00 }, R0 */
		0x04400e80, 0x1c9caa04, 0x0001c802, 0x0001c900,
		0x00000000, 0xbec890d6, 0x40011687, 0x00000000,
		/* INST 17: MADH R0.xyz (TR0.xyzw), R1.zzzz, { 1.60, -0.81, 0.00, 0.00 }, R0 + END */
		0x04400e81, 0x1c9d5404, 0x0001c802, 0x0001c900,
		0x3fcc432d, 0xbf501a37, 0x00000000, 0x00000000,
	}
};

nv_shader_t nv30_fp_yv12_bilinear = {
	.card_priv.NV30FP.num_regs = 2,
	.size = (8*4),
	.data = {
		/* INST 0: TEXR R0.x (TR0.xyzw), attrib.texcoord[0], abs(texture[1]) */
		0x17028200, 0x1c9dc801, 0x0001c800, 0x3fe1c800,
		/* INST 1: MADR R1.xyz (TR0.xyzw), R0.xxxx, { 1.16, -0.87, 0.53, -1.08 }.xxxx, { 1.16, -0.87, 0.53, -1.08 }.yzww */
		0x04000e02, 0x1c9c0000, 0x00000002, 0x0001f202,
		0x3f9507c8, 0xbf5ee393, 0x3f078fef, 0xbf8a6762,
		/* INST 2: TEXR R0.yz (TR0.xyzw), attrib.texcoord[1], abs(texture[2]) */
		0x1704ac80, 0x1c9dc801, 0x0001c800, 0x3fe1c800,
		/* INST 3: MADR R1.xyz (TR0.xyzw), R0.yyyy, { 0.00, -0.39, 2.02, 0.00 }, R1 */
		0x04000e02, 0x1c9cab00, 0x0001c802, 0x0001c804,
		0x00000000, 0xbec890d6, 0x40011687, 0x00000000,
		/* INST 4: MADR R0.xyz (TR0.xyzw), R0.zzzz, { 1.60, -0.81, 0.00, 0.00 }, R1 + END */
		0x04000e81, 0x1c9d5500, 0x0001c802, 0x0001c804,
		0x3fcc432d, 0xbf501a37, 0x00000000, 0x00000000,
	}
};


