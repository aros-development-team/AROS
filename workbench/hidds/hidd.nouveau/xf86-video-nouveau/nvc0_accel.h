#ifndef __NVC0_ACCEL_H__
#define __NVC0_ACCEL_H__

#include "nvc0_pushbuf.h"

/* scratch buffer offsets */
#define CODE_OFFSET 0x00000 /* Code */
#define TIC_OFFSET  0x02000 /* Texture Image Control */
#define TSC_OFFSET  0x03000 /* Texture Sampler Control */
#define NTFY_OFFSET 0x08000
#define MISC_OFFSET 0x10000

/* fragment programs */
#define PFP_S     0x0000 /* (src) */
#define PFP_C     0x0100 /* (src IN mask) */
#define PFP_CCA   0x0200 /* (src IN mask) component-alpha */
#define PFP_CCASA 0x0300 /* (src IN mask) component-alpha src-alpha */
#define PFP_S_A8  0x0400 /* (src) a8 rt */
#define PFP_C_A8  0x0500 /* (src IN mask) a8 rt - same for CA and CA_SA */
#define PFP_NV12  0x0600 /* NV12 YUV->RGB */

/* vertex programs */
#define PVP_PASS  0x0700 /* vertex pass-through shader */

/* shader constants */
#define CB_OFFSET 0x1000

#define VTX_ATTR(a, c, t, s)				\
	((NVC0_3D_VTX_ATTR_DEFINE_TYPE_##t) |		\
	 ((a) << NVC0_3D_VTX_ATTR_DEFINE_ATTR__SHIFT) |	\
	 ((c) << NVC0_3D_VTX_ATTR_DEFINE_COMP__SHIFT) |	\
	 ((s) << NVC0_3D_VTX_ATTR_DEFINE_SIZE__SHIFT))

static __inline__ void
VTX1s(NVPtr pNv, float sx, float sy, unsigned dx, unsigned dy)
{
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *fermi = pNv->Nv3D;

	BEGIN_RING(chan, fermi, NVC0_3D_VTX_ATTR_DEFINE, 3);
	OUT_RING  (chan, VTX_ATTR(1, 2, FLOAT, 4));
	OUT_RINGf (chan, sx);
	OUT_RINGf (chan, sy);
#if 1
	BEGIN_RING(chan, fermi, NVC0_3D_VTX_ATTR_DEFINE, 2);
	OUT_RING  (chan, VTX_ATTR(0, 2, USCALED, 2));
	OUT_RING  (chan, (dy << 16) | dx);
#else
	BEGIN_RING(chan, fermi, NVC0_3D_VTX_ATTR_DEFINE, 3);
	OUT_RING  (chan, VTX_ATTR(0, 2, FLOAT, 4));
	OUT_RINGf (chan, (float)dx);
	OUT_RINGf (chan, (float)dy);
#endif
}

static __inline__ void
VTX2s(NVPtr pNv, float s1x, float s1y, float s2x, float s2y,
      unsigned dx, unsigned dy)
{
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *fermi = pNv->Nv3D;

	BEGIN_RING(chan, fermi, NVC0_3D_VTX_ATTR_DEFINE, 3);
	OUT_RING  (chan, VTX_ATTR(1, 2, FLOAT, 4));
	OUT_RINGf (chan, s1x);
	OUT_RINGf (chan, s1y);
	BEGIN_RING(chan, fermi, NVC0_3D_VTX_ATTR_DEFINE, 3);
	OUT_RING  (chan, VTX_ATTR(2, 2, FLOAT, 4));
	OUT_RINGf (chan, s2x);
	OUT_RINGf (chan, s2y);
#if 1
	BEGIN_RING(chan, fermi, NVC0_3D_VTX_ATTR_DEFINE, 2);
	OUT_RING  (chan, VTX_ATTR(0, 2, USCALED, 2));
	OUT_RING  (chan, (dy << 16) | dx);
#else
	BEGIN_RING(chan, fermi, NVC0_3D_VTX_ATTR_DEFINE, 3);
	OUT_RING  (chan, VTX_ATTR(0, 2, FLOAT, 4));
	OUT_RINGf (chan, (float)dx);
	OUT_RINGf (chan, (float)dy);
#endif
}

#endif
