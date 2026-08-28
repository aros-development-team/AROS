#ifndef __NVC0_ACCEL_H__
#define __NVC0_ACCEL_H__

#include "hwdefs/nv_object.xml.h"
#include "hwdefs/nv50_2d.xml.h"
#include "hwdefs/nvc0_3d.xml.h"
#include "hwdefs/nvc0_m2mf.xml.h"
#include "hwdefs/nv50_defs.xml.h"
#include "hwdefs/nv50_texture.h"
#include "hwdefs/gm107_texture.xml.h"
#include "hwdefs/nv_3ddefs.xml.h"

/* subchannel assignments, compatible with kepler's fixed layout  */
#define SUBC_3D(mthd)    0, (mthd)
#define NVC0_3D(mthd)    SUBC_3D(NVC0_3D_##mthd)
#define NVE4_3D(mthd)    SUBC_3D(NVE4_3D_##mthd)
#define SUBC_M2MF(mthd)  2, (mthd)
#define SUBC_P2MF(mthd)  2, (mthd)
#define NVC0_M2MF(mthd)  SUBC_M2MF(NVC0_M2MF_##mthd)
#define SUBC_2D(mthd)    3, (mthd)
#define NV50_2D(mthd)    SUBC_2D(NV50_2D_##mthd)
#define NVC0_2D(mthd)    SUBC_2D(NVC0_2D_##mthd)
#define SUBC_COPY(mthd)  4, (mthd)
#define SUBC_NVSW(mthd)  5, (mthd)

/* scratch buffer offsets */
#define CODE_OFFSET 0x00000 /* Code */
#define PVP_DATA    0x01000 /* VP constants */
#define PFP_DATA    0x01100 /* FP constants */
#define TB_OFFSET   0x01800 /* Texture bindings (kepler) */
#define TIC_OFFSET  0x02000 /* Texture Image Control */
#define TSC_OFFSET  0x03000 /* Texture Sampler Control */
#define SOLID(i)   (0x04000 + (i) * 0x100)
#define NTFY_OFFSET 0x08000
#define SEMA_OFFSET 0x08100
#define MISC_OFFSET 0x10000

/* vertex/fragment programs */
#define SPO       ((pNv->Architecture < NV_KEPLER) ? 0x0000 : 0x0030)
#define PVP_PASS  (0x0000 + SPO) /* vertex pass-through shader */
#define PFP_S     (0x0200 + SPO) /* (src) */
#define PFP_C     (0x0400 + SPO) /* (src IN mask) */
#define PFP_CCA   (0x0600 + SPO) /* (src IN mask) component-alpha */
#define PFP_CCASA (0x0800 + SPO) /* (src IN mask) component-alpha src-alpha */
#define PFP_S_A8  (0x0a00 + SPO) /* (src) a8 rt */
#define PFP_C_A8  (0x0c00 + SPO) /* (src IN mask) a8 rt - same for CCA/CCASA */
#define PFP_NV12  (0x0e00 + SPO) /* NV12 YUV->RGB */


#define VTX_ATTR(a, c, t, s)				\
	((NVC0_3D_VTX_ATTR_DEFINE_TYPE_##t) |		\
	 ((a) << NVC0_3D_VTX_ATTR_DEFINE_ATTR__SHIFT) |	\
	 ((c) << NVC0_3D_VTX_ATTR_DEFINE_COMP__SHIFT) |	\
	 ((s) << NVC0_3D_VTX_ATTR_DEFINE_SIZE__SHIFT))

static __inline__ void
PUSH_VTX1s(struct nouveau_pushbuf *push, float sx, float sy, int dx, int dy)
{
	BEGIN_NVC0(push, NVC0_3D(VTX_ATTR_DEFINE), 3);
	PUSH_DATA (push, VTX_ATTR(1, 2, FLOAT, 4));
	PUSH_DATAf(push, sx);
	PUSH_DATAf(push, sy);
	BEGIN_NVC0(push, NVC0_3D(VTX_ATTR_DEFINE), 3);
	PUSH_DATA (push, VTX_ATTR(0, 2, SSCALED, 4));
	PUSH_DATA (push, dx);
	PUSH_DATA (push, dy);
}

static __inline__ void
PUSH_VTX2s(struct nouveau_pushbuf *push,
	   int x0, int y0, int x1, int y1, int dx, int dy)
{
	BEGIN_NVC0(push, NVC0_3D(VTX_ATTR_DEFINE), 3);
	PUSH_DATA (push, VTX_ATTR(1, 2, SSCALED, 4));
	PUSH_DATA (push, x0);
	PUSH_DATA (push, y0);
	BEGIN_NVC0(push, NVC0_3D(VTX_ATTR_DEFINE), 3);
	PUSH_DATA (push, VTX_ATTR(2, 2, SSCALED, 4));
	PUSH_DATA (push, x1);
	PUSH_DATA (push, y1);
	BEGIN_NVC0(push, NVC0_3D(VTX_ATTR_DEFINE), 3);
	PUSH_DATA (push, VTX_ATTR(0, 2, SSCALED, 4));
	PUSH_DATA (push, dx);
	PUSH_DATA (push, dy);
}

static __inline__ void
PUSH_DATAu(struct nouveau_pushbuf *push, struct nouveau_bo *bo,
	   unsigned delta, unsigned dwords)
{
	if (push->client->device->chipset < 0xe0) {
		BEGIN_NVC0(push, NVC0_M2MF(OFFSET_OUT_HIGH), 2);
		PUSH_DATA (push, (bo->offset + delta) >> 32);
		PUSH_DATA (push, (bo->offset + delta));
		BEGIN_NVC0(push, NVC0_M2MF(LINE_LENGTH_IN), 2);
		PUSH_DATA (push, dwords * 4);
		PUSH_DATA (push, 1);
		BEGIN_NVC0(push, NVC0_M2MF(EXEC), 1);
		PUSH_DATA (push, 0x100111);
		BEGIN_NIC0(push, NVC0_M2MF(DATA), dwords);
	} else {
		BEGIN_NVC0(push, SUBC_P2MF(0x0180), 4);
		PUSH_DATA (push, dwords * 4);
		PUSH_DATA (push, 1);
		PUSH_DATA (push, (bo->offset + delta) >> 32);
		PUSH_DATA (push, (bo->offset + delta));
		BEGIN_1IC0(push, SUBC_P2MF(0x01b0), 1 + dwords);
		PUSH_DATA (push, 0x001001);
	}
}

static __inline__ void
PUSH_TIC(struct nouveau_pushbuf *push, struct nouveau_bo *bo, unsigned offset,
	 unsigned width, unsigned height, unsigned pitch, unsigned format)
{
	if (push->client->device->chipset < 0x110) {
		unsigned tic2 = 0xd0001000;
		if (pitch == 0)
			tic2 |= 0x00004000;
		else
			tic2 |= 0x0005c000;
		PUSH_DATA(push, format);
		PUSH_DATA(push, bo->offset + offset);
		PUSH_DATA(push, ((bo->offset + offset) >> 32) |
			        (bo->config.nvc0.tile_mode << 18) |
			        tic2);
		PUSH_DATA(push, 0x00300000);
		PUSH_DATA(push, 0x80000000 | width);
		PUSH_DATA(push, 0x00010000 | height);
		PUSH_DATA(push, 0x03000000);
		PUSH_DATA(push, 0x00000000);
	} else {
		unsigned tile_mode = bo->config.nvc0.tile_mode;
		PUSH_DATA(push, (format & 0x3f) | ((format & ~0x3f) << 1));
		PUSH_DATA(push, bo->offset + offset);
		if (pitch == 0) {
			PUSH_DATA(push, ((bo->offset + offset) >> 32) |
				  GM107_TIC2_2_HEADER_VERSION_BLOCKLINEAR);
			PUSH_DATA(push, GM107_TIC2_3_LOD_ANISO_QUALITY_2 |
				  ((tile_mode & 0x007)) |
				  ((tile_mode & 0x070) >> (4 - 3)) |
				  ((tile_mode & 0x700) >> (8 - 6)));
			PUSH_DATA(push, GM107_TIC2_4_SECTOR_PROMOTION_PROMOTE_TO_2_V |
				  GM107_TIC2_4_BORDER_SIZE_SAMPLER_COLOR |
				  GM107_TIC2_4_TEXTURE_TYPE_TWO_D |
				  (width - 1));
			PUSH_DATA(push, GM107_TIC2_5_NORMALIZED_COORDS |
				        ((height - 1) & 0xffff));
			PUSH_DATA(push, GM107_TIC2_6_ANISO_FINE_SPREAD_FUNC_TWO |
				        GM107_TIC2_6_ANISO_COARSE_SPREAD_FUNC_ONE);
			PUSH_DATA(push, 0x00000000);
		} else {
			PUSH_DATA(push, ((bo->offset + offset) >> 32) |
				        GM107_TIC2_2_HEADER_VERSION_PITCH);
			PUSH_DATA(push, GM107_TIC2_3_LOD_ANISO_QUALITY_2 |
				        (pitch >> 5));
			PUSH_DATA(push, GM107_TIC2_4_BORDER_SIZE_SAMPLER_COLOR |
				  GM107_TIC2_4_TEXTURE_TYPE_TWO_D_NO_MIPMAP |
				  (width - 1));
			PUSH_DATA(push, GM107_TIC2_5_NORMALIZED_COORDS | (height - 1));
			PUSH_DATA(push, 0x000000000);
			PUSH_DATA(push, 0x000000000);
		}
	}
}

#endif
