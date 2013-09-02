/*
 * Copyright 2011 Red Hat Inc.
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
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs
 */

#include "drmP.h"
#include "nouveau_drv.h"
#include "nouveau_bios.h"
#include "nouveau_pm.h"
#include "nouveau_hw.h"

#define min2(a,b) ((a) < (b) ? (a) : (b))

static u32
read_pll_1(struct drm_device *dev, u32 reg)
{
	u32 ctrl = nv_rd32(dev, reg + 0x00);
	int P = (ctrl & 0x00070000) >> 16;
	int N = (ctrl & 0x0000ff00) >> 8;
	int M = (ctrl & 0x000000ff) >> 0;
	u32 ref = 27000, clk = 0;

	if (ctrl & 0x80000000)
		clk = ref * N / M;

	return clk >> P;
}

static u32
read_pll_2(struct drm_device *dev, u32 reg)
{
	u32 ctrl = nv_rd32(dev, reg + 0x00);
	u32 coef = nv_rd32(dev, reg + 0x04);
	int N2 = (coef & 0xff000000) >> 24;
	int M2 = (coef & 0x00ff0000) >> 16;
	int N1 = (coef & 0x0000ff00) >> 8;
	int M1 = (coef & 0x000000ff) >> 0;
	int P = (ctrl & 0x00070000) >> 16;
	u32 ref = 27000, clk = 0;

	if (ctrl & 0x80000000)
		clk = ref * N1 / M1;

	if (!(ctrl & 0x00000100)) {
		if (ctrl & 0x40000000)
			clk = clk * N2 / M2;
	}

	return clk >> P;
}

static u32
read_clk(struct drm_device *dev, u32 src)
{
	switch (src) {
	case 3:
		return read_pll_2(dev, 0x004000);
	case 2:
		return read_pll_1(dev, 0x004008);
	default:
		break;
	}

	return 0;
}

int
nv40_pm_clocks_get(struct drm_device *dev, struct nouveau_pm_level *perflvl)
{
	u32 ctrl = nv_rd32(dev, 0x00c040);

	perflvl->core   = read_clk(dev, (ctrl & 0x00000003) >> 0);
	perflvl->shader = read_clk(dev, (ctrl & 0x00000030) >> 4);
	perflvl->memory = read_pll_2(dev, 0x4020);
	return 0;
}

struct nv40_pm_state {
	u32 ctrl;
	u32 npll_ctrl;
	u32 npll_coef;
	u32 spll;
	u32 mpll_ctrl;
	u32 mpll_coef;
};

static int
nv40_calc_pll(struct drm_device *dev, u32 reg, struct pll_lims *pll,
	      u32 clk, int *N1, int *M1, int *N2, int *M2, int *log2P)
{
	struct nouveau_pll_vals coef;
	int ret;

	ret = get_pll_limits(dev, reg, pll);
	if (ret)
		return ret;

	if (clk < pll->vco1.maxfreq)
		pll->vco2.maxfreq = 0;

	ret = nouveau_calc_pll_mnp(dev, pll, clk, &coef);
	if (ret == 0)
		return -ERANGE;

	*N1 = coef.N1;
	*M1 = coef.M1;
	if (N2 && M2) {
		if (pll->vco2.maxfreq) {
			*N2 = coef.N2;
			*M2 = coef.M2;
		} else {
			*N2 = 1;
			*M2 = 1;
		}
	}
	*log2P = coef.log2P;
	return 0;
}

void *
nv40_pm_clocks_pre(struct drm_device *dev, struct nouveau_pm_level *perflvl)
{
	struct nv40_pm_state *info;
	struct pll_lims pll;
	int N1, N2, M1, M2, log2P;
	int ret;

	info = kmalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return ERR_PTR(-ENOMEM);

	/* core/geometric clock */
	ret = nv40_calc_pll(dev, 0x004000, &pll, perflvl->core,
			    &N1, &M1, &N2, &M2, &log2P);
	if (ret < 0)
		goto out;

	if (N2 == M2) {
		info->npll_ctrl = 0x80000100 | (log2P << 16);
		info->npll_coef = (N1 << 8) | M1;
	} else {
		info->npll_ctrl = 0xc0000000 | (log2P << 16);
		info->npll_coef = (N2 << 24) | (M2 << 16) | (N1 << 8) | M1;
	}

	/* use the second PLL for shader/rop clock, if it differs from core */
	if (perflvl->shader && perflvl->shader != perflvl->core) {
		ret = nv40_calc_pll(dev, 0x004008, &pll, perflvl->shader,
				    &N1, &M1, NULL, NULL, &log2P);
		if (ret < 0)
			goto out;

		info->spll = 0xc0000000 | (log2P << 16) | (N1 << 8) | M1;
		info->ctrl = 0x00000223;
	} else {
		info->spll = 0x00000000;
		info->ctrl = 0x00000333;
	}

	/* memory clock */
	ret = nv40_calc_pll(dev, 0x004020, &pll, perflvl->memory,
			    &N1, &M1, &N2, &M2, &log2P);
	if (ret < 0)
		goto out;

	info->mpll_ctrl  = 0x80000000 | (log2P << 16);
	info->mpll_ctrl |= min2(pll.log2p_bias + log2P, pll.max_log2p) << 20;
	if (N2 == M2) {
		info->mpll_ctrl |= 0x00000100;
		info->mpll_coef  = (N1 << 8) | M1;
	} else {
		info->mpll_ctrl |= 0x40000000;
		info->mpll_coef  = (N2 << 24) | (M2 << 16) | (N1 << 8) | M1;
	}

out:
	if (ret < 0) {
		kfree(info);
		info = ERR_PTR(ret);
	}
	return info;
}

static bool
nv40_pm_gr_idle(void *data)
{
	struct drm_device *dev = data;

	if ((nv_rd32(dev, 0x400760) & 0x000000f0) >> 4 !=
	    (nv_rd32(dev, 0x400760) & 0x0000000f))
		return false;

	if (nv_rd32(dev, 0x400700))
		return false;

	return true;
}

void
nv40_pm_clocks_set(struct drm_device *dev, void *pre_state)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nv40_pm_state *info = pre_state;
	unsigned long flags;
	struct bit_entry M;
	u32 crtc_mask = 0;
	u8 sr1[2];
	int i;

	/* determine which CRTCs are active, fetch VGA_SR1 for each */
	for (i = 0; i < 2; i++) {
		u32 vbl = nv_rd32(dev, 0x600808 + (i * 0x2000));
		u32 cnt = 0;
		do {
			if (vbl != nv_rd32(dev, 0x600808 + (i * 0x2000))) {
				nv_wr08(dev, 0x0c03c4 + (i * 0x2000), 0x01);
				sr1[i] = nv_rd08(dev, 0x0c03c5 + (i * 0x2000));
				if (!(sr1[i] & 0x20))
					crtc_mask |= (1 << i);
				break;
			}
			udelay(1);
		} while (cnt++ < 32);
	}

	/* halt and idle engines */
	spin_lock_irqsave(&dev_priv->context_switch_lock, flags);
	nv_mask(dev, 0x002500, 0x00000001, 0x00000000);
	if (!nv_wait(dev, 0x002500, 0x00000010, 0x00000000))
		goto resume;
	nv_mask(dev, 0x003220, 0x00000001, 0x00000000);
	if (!nv_wait(dev, 0x003220, 0x00000010, 0x00000000))
		goto resume;
	nv_mask(dev, 0x003200, 0x00000001, 0x00000000);
	nv04_fifo_cache_pull(dev, false);

	if (!nv_wait_cb(dev, nv40_pm_gr_idle, dev))
		goto resume;

	/* set engine clocks */
	nv_mask(dev, 0x00c040, 0x00000333, 0x00000000);
	nv_wr32(dev, 0x004004, info->npll_coef);
	nv_mask(dev, 0x004000, 0xc0070100, info->npll_ctrl);
	nv_mask(dev, 0x004008, 0xc007ffff, info->spll);
	mdelay(5);
	nv_mask(dev, 0x00c040, 0x00000333, info->ctrl);

	/* wait for vblank start on active crtcs, disable memory access */
	for (i = 0; i < 2; i++) {
		if (!(crtc_mask & (1 << i)))
			continue;
		nv_wait(dev, 0x600808 + (i * 0x2000), 0x00010000, 0x00000000);
		nv_wait(dev, 0x600808 + (i * 0x2000), 0x00010000, 0x00010000);
		nv_wr08(dev, 0x0c03c4 + (i * 0x2000), 0x01);
		nv_wr08(dev, 0x0c03c5 + (i * 0x2000), sr1[i] | 0x20);
	}

	/* prepare ram for reclocking */
	nv_wr32(dev, 0x1002d4, 0x00000001); /* precharge */
	nv_wr32(dev, 0x1002d0, 0x00000001); /* refresh */
	nv_wr32(dev, 0x1002d0, 0x00000001); /* refresh */
	nv_mask(dev, 0x100210, 0x80000000, 0x00000000); /* no auto refresh */
	nv_wr32(dev, 0x1002dc, 0x00000001); /* enable self-refresh */

	/* change the PLL of each memory partition */
	nv_mask(dev, 0x00c040, 0x0000c000, 0x00000000);
	switch (dev_priv->chipset) {
	case 0x40:
	case 0x45:
	case 0x41:
	case 0x42:
	case 0x47:
		nv_mask(dev, 0x004044, 0xc0771100, info->mpll_ctrl);
		nv_mask(dev, 0x00402c, 0xc0771100, info->mpll_ctrl);
		nv_wr32(dev, 0x004048, info->mpll_coef);
		nv_wr32(dev, 0x004030, info->mpll_coef);
	case 0x43:
	case 0x49:
	case 0x4b:
		nv_mask(dev, 0x004038, 0xc0771100, info->mpll_ctrl);
		nv_wr32(dev, 0x00403c, info->mpll_coef);
	default:
		nv_mask(dev, 0x004020, 0xc0771100, info->mpll_ctrl);
		nv_wr32(dev, 0x004024, info->mpll_coef);
		break;
	}
	udelay(100);
	nv_mask(dev, 0x00c040, 0x0000c000, 0x0000c000);

	/* re-enable normal operation of memory controller */
	nv_wr32(dev, 0x1002dc, 0x00000000);
	nv_mask(dev, 0x100210, 0x80000000, 0x80000000);
	udelay(100);

	/* execute memory reset script from vbios */
	if (!bit_table(dev, 'M', &M))
		nouveau_bios_init_exec(dev, ROM16(M.data[0]));

	/* make sure we're in vblank (hopefully the same one as before), and
	 * then re-enable crtc memory access
	 */
	for (i = 0; i < 2; i++) {
		if (!(crtc_mask & (1 << i)))
			continue;
		nv_wait(dev, 0x600808 + (i * 0x2000), 0x00010000, 0x00010000);
		nv_wr08(dev, 0x0c03c4 + (i * 0x2000), 0x01);
		nv_wr08(dev, 0x0c03c5 + (i * 0x2000), sr1[i]);
	}

	/* resume engines */
resume:
	nv_wr32(dev, 0x003250, 0x00000001);
	nv_mask(dev, 0x003220, 0x00000001, 0x00000001);
	nv_wr32(dev, 0x003200, 0x00000001);
	nv_wr32(dev, 0x002500, 0x00000001);
	spin_unlock_irqrestore(&dev_priv->context_switch_lock, flags);

	kfree(info);
}

int
nv40_pm_fanspeed_get(struct drm_device *dev)
{
	u32 reg = nv_rd32(dev, 0x0010f0);
	if (reg & 0x80000000) {
		u32 duty = (reg & 0x7fff0000) >> 16;
		u32 divs = (reg & 0x00007fff);
		if (divs && divs >= duty)
			return ((divs - duty) * 100) / divs;
	}

	return 100;
}

int
nv40_pm_fanspeed_set(struct drm_device *dev, int percent)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_pm_engine *pm = &dev_priv->engine.pm;
	u32 divs = pm->pwm_divisor;
	u32 duty = ((100 - percent) * divs) / 100;

	nv_wr32(dev, 0x0010f0, 0x80000000 | (duty << 16) | divs);
	return 0;
}

int
nv41_pm_fanspeed_get(struct drm_device *dev)
{
	u32 reg = nv_rd32(dev, 0x0015f4);
	if (reg & 0x80000000) {
		u32 divs = nv_rd32(dev, 0x0015f8);
		u32 duty = (reg & 0x7fffffff);
		if (divs && divs >= duty)
			return ((divs - duty) * 100) / divs;
	}

	return 100;
}

int
nv41_pm_fanspeed_set(struct drm_device *dev, int percent)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_pm_engine *pm = &dev_priv->engine.pm;
	u32 divs = pm->pwm_divisor;
	u32 duty = ((100 - percent) * divs) / 100;

	nv_wr32(dev, 0x0015f8, divs);
	nv_wr32(dev, 0x0015f4, duty | 0x80000000);
	return 0;
}
