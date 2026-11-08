#ifndef __NV50_ACCEL_H__
#define __NV50_ACCEL_H__


#include "hwdefs/nv50_2d.xml.h"
#include "hwdefs/nv50_3d.xml.h"
#include "hwdefs/nv50_defs.xml.h"
#include "hwdefs/nv50_texture.h"
#include "hwdefs/nv_3ddefs.xml.h"
#include "hwdefs/nv_m2mf.xml.h"
#include "hwdefs/nv_object.xml.h"

/* subchannel assignments - graphics channel */
#define SUBC_M2MF(mthd)  0, (mthd)
#define NV03_M2MF(mthd)  SUBC_M2MF(NV03_M2MF_##mthd)
#define NV50_M2MF(mthd)  SUBC_M2MF(NV50_M2MF_##mthd)
#define SUBC_NVSW(mthd)  1, (mthd)
#define SUBC_2D(mthd)    2, (mthd)
#define NV50_2D(mthd)    SUBC_2D(NV50_2D_##mthd)
#define NVC0_2D(mthd)    SUBC_2D(NVC0_2D_##mthd)
#define SUBC_3D(mthd)    7, (mthd)
#define NV50_3D(mthd)    SUBC_3D(NV50_3D_##mthd)

/* subchannel assignments - copy engine channel */
#define SUBC_COPY(mthd)  2, (mthd)

/* scratch buffer offsets */
#define PVP_OFFSET  0x00000000 /* Vertex program */
#define PFP_OFFSET  0x00001000 /* Fragment program */
#define TIC_OFFSET  0x00002000 /* Texture Image Control */
#define TSC_OFFSET  0x00003000 /* Texture Sampler Control */
#define PVP_DATA    0x00004000 /* VP constbuf */
#define PFP_DATA    0x00004100 /* FP constbuf */
#define SOLID(i)   (0x00006000 + (i) * 0x100)

/* Fragment programs */
#define PFP_S     0x0000 /* (src) */
#define PFP_C     0x0100 /* (src IN mask) */
#define PFP_CCA   0x0200 /* (src IN mask) component-alpha */
#define PFP_CCASA 0x0300 /* (src IN mask) component-alpha src-alpha */
#define PFP_S_A8  0x0400 /* (src) a8 rt */
#define PFP_C_A8  0x0500 /* (src IN mask) a8 rt - same for CA and CA_SA */
#define PFP_NV12  0x0600 /* NV12 YUV->RGB */

/* Constant buffer assignments */
#define CB_PSH 0
#define CB_PVP 1
#define CB_PFP 2

static __inline__ void
PUSH_VTX1s(struct nouveau_pushbuf *push, float sx, float sy, int dx, int dy)
{
	BEGIN_NV04(push, NV50_3D(VTX_ATTR_2F_X(8)), 2);
	PUSH_DATAf(push, sx);
	PUSH_DATAf(push, sy);
	BEGIN_NV04(push, NV50_3D(VTX_ATTR_2I(0)), 1);
	PUSH_DATA (push, ((dy & 0xffff) << 16) | (dx & 0xffff));
}

static __inline__ void
PUSH_VTX2s(struct nouveau_pushbuf *push,
	   int x1, int y1, int x2, int y2, int dx, int dy)
{
	BEGIN_NV04(push, NV50_3D(VTX_ATTR_2I(8)), 2);
	PUSH_DATA (push, ((y1 & 0xffff) << 16) | (x1 & 0xffff));
	PUSH_DATA (push, ((y2 & 0xffff) << 16) | (x2 & 0xffff));
	BEGIN_NV04(push, NV50_3D(VTX_ATTR_2I(0)), 1);
	PUSH_DATA (push, ((dy & 0xffff) << 16) | (dx & 0xffff));
}

static __inline__ void
PUSH_DATAu(struct nouveau_pushbuf *push, struct nouveau_bo *bo,
	   unsigned delta, unsigned dwords)
{
	const unsigned idx = (delta & 0x000000fc) >> 2;
	const unsigned off = (delta & 0xffffff00);
	BEGIN_NV04(push, NV50_3D(CB_DEF_ADDRESS_HIGH), 3);
	PUSH_DATA (push, (bo->offset + off) >> 32);
	PUSH_DATA (push, (bo->offset + off));
	PUSH_DATA (push, (CB_PSH << NV50_3D_CB_DEF_SET_BUFFER__SHIFT) | 0x2000);
	BEGIN_NV04(push, NV50_3D(CB_ADDR), 1);
	PUSH_DATA (push, CB_PSH | (idx << NV50_3D_CB_ADDR_ID__SHIFT));
	BEGIN_NI04(push, NV50_3D(CB_DATA(0)), dwords);
}

#endif
