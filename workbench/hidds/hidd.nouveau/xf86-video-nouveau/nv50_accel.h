#ifndef __NV50_ACCEL_H__
#define __NV50_ACCEL_H__

#include "nv04_pushbuf.h"

/* "Tesla scratch buffer" offsets */
#define PVP_OFFSET  0x00000000 /* Vertex program */
#define PFP_OFFSET  0x00001000 /* Fragment program */
#define TIC_OFFSET  0x00002000 /* Texture Image Control */
#define TSC_OFFSET  0x00003000 /* Texture Sampler Control */
#define PFP_DATA    0x00004000 /* FP constbuf */

/* Fragment programs */
#define PFP_S     0x0000 /* (src) */
#define PFP_C     0x0100 /* (src IN mask) */
#define PFP_CCA   0x0200 /* (src IN mask) component-alpha */
#define PFP_CCASA 0x0300 /* (src IN mask) component-alpha src-alpha */
#define PFP_S_A8  0x0400 /* (src) a8 rt */
#define PFP_C_A8  0x0500 /* (src IN mask) a8 rt - same for CA and CA_SA */
#define PFP_NV12  0x0600 /* NV12 YUV->RGB */

/* Constant buffer assignments */
#define CB_TSC 0
#define CB_TIC 1
#define CB_PFP 2

static __inline__ void
VTX1s(NVPtr pNv, float sx, float sy, unsigned dx, unsigned dy)
{
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *tesla = pNv->Nv3D;

	BEGIN_RING(chan, tesla, NV50TCL_VTX_ATTR_2F_X(8), 2);
	OUT_RINGf (chan, sx);
	OUT_RINGf (chan, sy);
	BEGIN_RING(chan, tesla, NV50TCL_VTX_ATTR_2I(0), 1);
 	OUT_RING  (chan, (dy << 16) | dx);
}

static __inline__ void
VTX2s(NVPtr pNv, float s1x, float s1y, float s2x, float s2y,
		 unsigned dx, unsigned dy)
{
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *tesla = pNv->Nv3D;

	BEGIN_RING(chan, tesla, NV50TCL_VTX_ATTR_2F_X(8), 4);
	OUT_RINGf (chan, s1x);
	OUT_RINGf (chan, s1y);
	OUT_RINGf (chan, s2x);
	OUT_RINGf (chan, s2y);
	BEGIN_RING(chan, tesla, NV50TCL_VTX_ATTR_2I(0), 1);
 	OUT_RING  (chan, (dy << 16) | dx);
}

#endif
