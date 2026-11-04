#ifndef __NV04_ACCEL_H__
#define __NV04_ACCEL_H__

#include "hwdefs/nv_object.xml.h"
#include "hwdefs/nv01_2d.xml.h"

#define XV_TABLE_SIZE 512

/* scratch buffer offsets */
#define PFP_PASS          0x00000000
#define PFP_S             0x00000100
#define PFP_C             0x00000200
#define PFP_CCA           0x00000300
#define PFP_CCASA         0x00000400
#define PFP_S_A8          0x00000500
#define PFP_C_A8          0x00000600
#define PFP_NV12_BILINEAR 0x00000700
#define PFP_NV12_BICUBIC  0x00000800
#define XV_TABLE          0x00001000
#define SOLID(i)         (0x00002000 + (i) * 0x100)

/* subchannel assignments */
#define SUBC_M2MF(mthd)  0, (mthd)
#define NV03_M2MF(mthd)  SUBC_M2MF(NV03_M2MF_##mthd)
#define SUBC_NVSW(mthd)  1, (mthd)
#define SUBC_SF2D(mthd)  2, (mthd)
#define NV04_SF2D(mthd)  SUBC_SF2D(NV04_SURFACE_2D_##mthd)
#define NV10_SF2D(mthd)  SUBC_SF2D(NV10_SURFACE_2D_##mthd)
#define SUBC_RECT(mthd)  3, (mthd)
#define NV04_RECT(mthd)  SUBC_RECT(NV04_GDI_##mthd)
#define SUBC_BLIT(mthd)  4, (mthd)
#define NV01_BLIT(mthd)  SUBC_BLIT(NV01_BLIT_##mthd)
#define NV04_BLIT(mthd)  SUBC_BLIT(NV04_BLIT_##mthd)
#define NV15_BLIT(mthd)  SUBC_BLIT(NV15_BLIT_##mthd)
#define SUBC_IFC(mthd)   5, (mthd)
#define NV01_IFC(mthd)   SUBC_IFC(NV01_IFC_##mthd)
#define NV04_IFC(mthd)   SUBC_IFC(NV04_IFC_##mthd)
#define SUBC_MISC(mthd)  6, (mthd)
#define NV03_SIFM(mthd)  SUBC_MISC(NV03_SIFM_##mthd)
#define NV05_SIFM(mthd)  SUBC_MISC(NV05_SIFM_##mthd)
#define NV01_BETA(mthd)  SUBC_MISC(NV01_BETA_##mthd)
#define NV04_BETA4(mthd) SUBC_MISC(NV04_BETA4_##mthd)
#define NV01_PATT(mthd)  SUBC_MISC(NV01_PATTERN_##mthd)
#define NV04_PATT(mthd)  SUBC_MISC(NV04_PATTERN_##mthd)
#define NV01_ROP(mthd)   SUBC_MISC(NV01_ROP_##mthd)
#define NV01_CLIP(mthd)  SUBC_MISC(NV01_CLIP_##mthd)
#define SUBC_3D(mthd)    7, (mthd)
#define NV10_3D(mthd)    SUBC_3D(NV10_3D_##mthd)
#define NV30_3D(mthd)    SUBC_3D(NV30_3D_##mthd)
#define NV40_3D(mthd)    SUBC_3D(NV40_3D_##mthd)

static __inline__ void
PUSH_DATAu(struct nouveau_pushbuf *push, struct nouveau_bo *bo,
	   unsigned delta, unsigned dwords)
{
	const uint32_t domain = NOUVEAU_BO_VRAM | NOUVEAU_BO_WR;
	struct nouveau_pushbuf_refn refs[] = { { bo, domain } };
	unsigned pitch = ((dwords * 4) + 63) & ~63;

	if (nouveau_pushbuf_space(push, 32 + dwords, 2, 0) ||
	    nouveau_pushbuf_refn (push, refs, 1))
		return;

	BEGIN_NV04(push, NV01_SUBC(MISC, OBJECT), 1);
	PUSH_DATA (push, NvClipRectangle);
	BEGIN_NV04(push, NV01_CLIP(POINT), 2);
	PUSH_DATA (push, (0 << 16) | 0);
	PUSH_DATA (push, (1 << 16) | dwords);
	BEGIN_NV04(push, NV04_SF2D(FORMAT), 4);
	PUSH_DATA (push, NV04_SURFACE_2D_FORMAT_A8R8G8B8);
	PUSH_DATA (push, (pitch << 16) | pitch);
	PUSH_RELOC(push, bo, delta, NOUVEAU_BO_LOW, 0, 0);
	PUSH_RELOC(push, bo, delta, NOUVEAU_BO_LOW, 0, 0);
	BEGIN_NV04(push, NV01_IFC(OPERATION), 5);
	PUSH_DATA (push, NV01_IFC_OPERATION_SRCCOPY);
	PUSH_DATA (push, NV01_IFC_COLOR_FORMAT_A8R8G8B8);
	PUSH_DATA (push, (0 << 16) | 0);
	PUSH_DATA (push, (1 << 16) | dwords);
	PUSH_DATA (push, (1 << 16) | dwords);
	BEGIN_NV04(push, NV01_IFC(COLOR(0)), dwords);
}

/* For NV40 FP upload, deal with the weird-arse big-endian swap */
static __inline__ void
PUSH_DATAs(struct nouveau_pushbuf *push, unsigned data)
{
#if (X_BYTE_ORDER != X_LITTLE_ENDIAN)
	data = (data >> 16) | ((data & 0xffff) << 16);
#endif
	PUSH_DATA(push, data);
}

#endif
