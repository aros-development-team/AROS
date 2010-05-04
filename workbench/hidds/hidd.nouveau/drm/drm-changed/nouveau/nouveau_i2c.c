/*
 * Copyright 2009 Red Hat Inc.
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
#include "nouveau_i2c.h"
#include "nouveau_hw.h"

#if !defined(__AROS__)
static void
nv04_i2c_setscl(void *data, int state)
{
	struct nouveau_i2c_chan *i2c = data;
	struct drm_device *dev = i2c->dev;
	uint8_t val;

	val = (NVReadVgaCrtc(dev, 0, i2c->wr) & 0xd0) | (state ? 0x20 : 0);
	NVWriteVgaCrtc(dev, 0, i2c->wr, val | 0x01);
}

static void
nv04_i2c_setsda(void *data, int state)
{
	struct nouveau_i2c_chan *i2c = data;
	struct drm_device *dev = i2c->dev;
	uint8_t val;

	val = (NVReadVgaCrtc(dev, 0, i2c->wr) & 0xe0) | (state ? 0x10 : 0);
	NVWriteVgaCrtc(dev, 0, i2c->wr, val | 0x01);
}

static int
nv04_i2c_getscl(void *data)
{
	struct nouveau_i2c_chan *i2c = data;
	struct drm_device *dev = i2c->dev;

	return !!(NVReadVgaCrtc(dev, 0, i2c->rd) & 4);
}

static int
nv04_i2c_getsda(void *data)
{
	struct nouveau_i2c_chan *i2c = data;
	struct drm_device *dev = i2c->dev;

	return !!(NVReadVgaCrtc(dev, 0, i2c->rd) & 8);
}

static void
nv4e_i2c_setscl(void *data, int state)
{
	struct nouveau_i2c_chan *i2c = data;
	struct drm_device *dev = i2c->dev;
	uint8_t val;

	val = (nv_rd32(dev, i2c->wr) & 0xd0) | (state ? 0x20 : 0);
	nv_wr32(dev, i2c->wr, val | 0x01);
}

static void
nv4e_i2c_setsda(void *data, int state)
{
	struct nouveau_i2c_chan *i2c = data;
	struct drm_device *dev = i2c->dev;
	uint8_t val;

	val = (nv_rd32(dev, i2c->wr) & 0xe0) | (state ? 0x10 : 0);
	nv_wr32(dev, i2c->wr, val | 0x01);
}

static int
nv4e_i2c_getscl(void *data)
{
	struct nouveau_i2c_chan *i2c = data;
	struct drm_device *dev = i2c->dev;

	return !!((nv_rd32(dev, i2c->rd) >> 16) & 4);
}

static int
nv4e_i2c_getsda(void *data)
{
	struct nouveau_i2c_chan *i2c = data;
	struct drm_device *dev = i2c->dev;

	return !!((nv_rd32(dev, i2c->rd) >> 16) & 8);
}

static int
nv50_i2c_getscl(void *data)
{
	struct nouveau_i2c_chan *i2c = data;
	struct drm_device *dev = i2c->dev;

	return !!(nv_rd32(dev, i2c->rd) & 1);
}


static int
nv50_i2c_getsda(void *data)
{
	struct nouveau_i2c_chan *i2c = data;
	struct drm_device *dev = i2c->dev;

	return !!(nv_rd32(dev, i2c->rd) & 2);
}

static void
nv50_i2c_setscl(void *data, int state)
{
	struct nouveau_i2c_chan *i2c = data;
	struct drm_device *dev = i2c->dev;

	nv_wr32(dev, i2c->wr, 4 | (i2c->data ? 2 : 0) | (state ? 1 : 0));
}

static void
nv50_i2c_setsda(void *data, int state)
{
	struct nouveau_i2c_chan *i2c = data;
	struct drm_device *dev = i2c->dev;

	nv_wr32(dev, i2c->wr,
			(nv_rd32(dev, i2c->rd) & 1) | 4 | (state ? 2 : 0));
	i2c->data = state;
}

static const uint32_t nv50_i2c_port[] = {
	0x00e138, 0x00e150, 0x00e168, 0x00e180,
	0x00e254, 0x00e274, 0x00e764, 0x00e780,
	0x00e79c, 0x00e7b8
};
#define NV50_I2C_PORTS ARRAY_SIZE(nv50_i2c_port)
#endif

int
nouveau_i2c_init(struct drm_device *dev, struct dcb_i2c_entry *entry, int index)
{
#if !defined(__AROS__)
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_i2c_chan *i2c;
	int ret;

	if (entry->chan)
		return -EEXIST;

	if (dev_priv->card_type == NV_50 && entry->read >= NV50_I2C_PORTS) {
		NV_ERROR(dev, "unknown i2c port %d\n", entry->read);
		return -EINVAL;
	}

	i2c = kzalloc(sizeof(*i2c), GFP_KERNEL);
	if (i2c == NULL)
		return -ENOMEM;

	switch (entry->port_type) {
	case 0:
		i2c->algo.bit.setsda = nv04_i2c_setsda;
		i2c->algo.bit.setscl = nv04_i2c_setscl;
		i2c->algo.bit.getsda = nv04_i2c_getsda;
		i2c->algo.bit.getscl = nv04_i2c_getscl;
		i2c->rd = entry->read;
		i2c->wr = entry->write;
		break;
	case 4:
		i2c->algo.bit.setsda = nv4e_i2c_setsda;
		i2c->algo.bit.setscl = nv4e_i2c_setscl;
		i2c->algo.bit.getsda = nv4e_i2c_getsda;
		i2c->algo.bit.getscl = nv4e_i2c_getscl;
		i2c->rd = 0x600800 + entry->read;
		i2c->wr = 0x600800 + entry->write;
		break;
	case 5:
		i2c->algo.bit.setsda = nv50_i2c_setsda;
		i2c->algo.bit.setscl = nv50_i2c_setscl;
		i2c->algo.bit.getsda = nv50_i2c_getsda;
		i2c->algo.bit.getscl = nv50_i2c_getscl;
		i2c->rd = nv50_i2c_port[entry->read];
		i2c->wr = i2c->rd;
		break;
	case 6:
		i2c->rd = entry->read;
		i2c->wr = entry->write;
		break;
	default:
		NV_ERROR(dev, "DCB I2C port type %d unknown\n",
			 entry->port_type);
		kfree(i2c);
		return -EINVAL;
	}

	snprintf(i2c->adapter.name, sizeof(i2c->adapter.name),
		 "nouveau-%s-%d", pci_name(dev->pdev), index);
	i2c->adapter.owner = THIS_MODULE;
	i2c->adapter.dev.parent = &dev->pdev->dev;
	i2c->dev = dev;
	i2c_set_adapdata(&i2c->adapter, i2c);

	if (entry->port_type < 6) {
		i2c->adapter.algo_data = &i2c->algo.bit;
		i2c->algo.bit.udelay = 40;
		i2c->algo.bit.timeout = usecs_to_jiffies(5000);
		i2c->algo.bit.data = i2c;
		ret = i2c_bit_add_bus(&i2c->adapter);
	} else {
		i2c->adapter.algo_data = &i2c->algo.dp;
		i2c->algo.dp.running = false;
		i2c->algo.dp.address = 0;
		i2c->algo.dp.aux_ch = nouveau_dp_i2c_aux_ch;
		ret = i2c_dp_aux_add_bus(&i2c->adapter);
	}

	if (ret) {
		NV_ERROR(dev, "Failed to register i2c %d\n", index);
		kfree(i2c);
		return ret;
	}

	entry->chan = i2c;
#else
IMPLEMENT("\n");
#endif
	return 0;
}

void
nouveau_i2c_fini(struct drm_device *dev, struct dcb_i2c_entry *entry)
{
	if (!entry->chan)
		return;

#if !defined(__AROS__)
	i2c_del_adapter(&entry->chan->adapter);
#else
IMPLEMENT("Calling i2c_del_adapter\n");
#endif
	kfree(entry->chan);
	entry->chan = NULL;
}

struct nouveau_i2c_chan *
nouveau_i2c_find(struct drm_device *dev, int index)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nvbios *bios = &dev_priv->vbios;

	if (index >= DCB_MAX_NUM_I2C_ENTRIES)
		return NULL;

	if (!bios->dcb.i2c[index].chan) {
		if (nouveau_i2c_init(dev, &bios->dcb.i2c[index], index))
			return NULL;
	}

	return bios->dcb.i2c[index].chan;
}

