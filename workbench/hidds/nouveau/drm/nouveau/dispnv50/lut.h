#ifndef __NV50_KMS_LUT_H__
#define __NV50_KMS_LUT_H__
#include <nvif/mem.h>
#include <linux/minmax.h>
#include <uapi/drm/drm_mode.h>
struct drm_property_blob;
struct nv50_disp;

/* drm_color_mgmt.h's helper: a 16-bit LUT component scaled and rounded
 * to the hardware's bit depth (the header is not part of this port) */
static inline u32 drm_color_lut_extract(u32 user_input, u32 bit_precision)
{
	u32 val = user_input;
	u32 max = 0xffff >> (16 - bit_precision);

	if (bit_precision < 16) {
		val += 1UL << (16 - bit_precision - 1);
		val >>= 16 - bit_precision;
	}

	return clamp_val(val, 0, max);
}

struct nv50_lut {
	struct nvif_mem mem[2];
};

int nv50_lut_init(struct nv50_disp *, struct nvif_mmu *, struct nv50_lut *);
void nv50_lut_fini(struct nv50_lut *);
u32 nv50_lut_load(struct nv50_lut *, int buffer, struct drm_property_blob *,
		  void (*)(struct drm_color_lut *, int size, void __iomem *));
#endif
