/* SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
 */
#include "priv.h"

#include <linux/elf.h>
#include <linux/crc32.h>

#include <subdev/fb.h>
#include <subdev/fsp.h>

#include <rm/r580/nvrm/gsp.h>

#include <nvhw/drf.h>
#include <nvhw/ref/gh100/dev_falcon_v4.h>
#include <nvhw/ref/gh100/dev_riscv_pri.h>

int
gh100_gsp_fini(struct nvkm_gsp *gsp, enum nvkm_suspend_state suspend)
{
	struct nvkm_falcon *falcon = &gsp->falcon;
	int ret, time = 4000;

	/* Shutdown RM. */
	ret = r535_gsp_fini(gsp, suspend);
	if (ret && suspend)
		return ret;

	/* Wait for RISC-V to halt. */
	do {
		u32 data = nvkm_falcon_rd32(falcon, falcon->addr2 + NV_PRISCV_RISCV_CPUCTL);

		if (NVVAL_GET(data, NV_PRISCV, RISCV_CPUCTL, HALTED))
			return 0;

		usleep_range(1000, 2000);
	} while(time--);

	return -ETIMEDOUT;
}

static bool
gh100_gsp_lockdown_released(struct nvkm_gsp *gsp, u32 *mbox0)
{
	u32 data;

	/* Wait for GSP access via BAR0 to be allowed. */
	*mbox0 = nvkm_falcon_rd32(&gsp->falcon, NV_PFALCON_FALCON_MAILBOX0);

	if (*mbox0 && (*mbox0 & 0xffffff00) == 0xbadf4100)
		return false;

	/* Check if an error code has been reported. */
	if (*mbox0) {
		u32 mbox1 = nvkm_falcon_rd32(&gsp->falcon, NV_PFALCON_FALCON_MAILBOX1);

		/* Any value that's not GSP_FMC_BOOT_PARAMS addr is an error. */
		if ((((u64)mbox1 << 32) | *mbox0) != gsp->fmc.args.addr)
			return true;
	}

	/* Check if lockdown has been released. */
	data = nvkm_falcon_rd32(&gsp->falcon, NV_PFALCON_FALCON_HWCFG2);
	return !NVVAL_GET(data, NV_PFALCON, FALCON_HWCFG2, RISCV_BR_PRIV_LOCKDOWN);
}

#if defined(__AROS__)
#include <core/pci.h>
#include <linux/pci.h>

static bool
gh100_gsp_aros_gfw_done(struct nvkm_device *device, u32 *pprogress, u32 *ppl)
{
	*ppl = nvkm_rd32(device, 0x118128);
	*pprogress = nvkm_rd32(device, 0x118234);
	return (*ppl & 1) && (*pprogress & 0xff) == 0xff;
}

static void
gh100_gsp_aros_dump(struct nvkm_gsp *gsp, const char *when)
{
	struct nvkm_subdev *subdev = &gsp->subdev;
	struct nvkm_device *device = subdev->device;
	u32 progress, pl;

	gh100_gsp_aros_gfw_done(device, &progress, &pl);
	nvkm_info(subdev, "%s: GFW boot progress %08x (plm %08x), fsp status %08x\n",
		  when, progress, pl, nvkm_rd32(device, 0x00ad00bc));
	nvkm_info(subdev, "fsp scratch2 %08x %08x %08x %08x scratch3 %08x %08x %08x %08x\n",
		  nvkm_rd32(device, 0x008f0320), nvkm_rd32(device, 0x008f0324),
		  nvkm_rd32(device, 0x008f0328), nvkm_rd32(device, 0x008f032c),
		  nvkm_rd32(device, 0x008f0330), nvkm_rd32(device, 0x008f0334),
		  nvkm_rd32(device, 0x008f0338), nvkm_rd32(device, 0x008f033c));
	nvkm_info(subdev, "aon scratch05 %08x %08x %08x %08x boot0 %08x strap %08x\n",
		  nvkm_rd32(device, 0x118234), nvkm_rd32(device, 0x118238),
		  nvkm_rd32(device, 0x11823c), nvkm_rd32(device, 0x118240),
		  nvkm_rd32(device, 0x000000), nvkm_rd32(device, 0x101000));
}

/*
 * The card's own firmware boot (devinit) did not finish, so RM will not
 * start. Reset the card and watch what its firmware does afterwards:
 * mode 1 is a secondary bus reset from the root port, mode 2 pulls the
 * root complex PHY reset the way the platform's own bring-up does. The
 * configuration header is put back afterwards.
 */
static void
gh100_gsp_aros_watch(struct nvkm_gsp *gsp, struct pci_dev *pdev, u32 *cfg, int ms)
{
	struct nvkm_subdev *subdev = &gsp->subdev;
	struct nvkm_device *device = subdev->device;
	u32 last_id = ~0, last_fsp = ~0, last_gfw = ~0, cur, fsp, gfw, pl;
	bool restored = false;
	int t, i;

	for (t = 0; t < ms; t += 5) {
		pci_read_config_dword(pdev, 0, &cur);
		if (cur != last_id) {
			nvkm_info(subdev, "%5dms: id %08x\n", t, cur);
			last_id = cur;
		}
		if (cur == cfg[0] && !restored) {
			/* the header is gone with the reset; put it back so
			   the registers can be watched */
			for (i = 1; i < 64; i++)
				pci_write_config_dword(pdev, i * 4, cfg[i]);
			pci_write_config_dword(pdev, 4, cfg[1]);
			restored = true;
		}
		if (cur == cfg[0]) {
			fsp = nvkm_rd32(device, 0x00ad00bc);
			gh100_gsp_aros_gfw_done(device, &gfw, &pl);
			if (fsp != last_fsp || gfw != last_gfw) {
				nvkm_info(subdev, "%5dms: fsp %08x gfw %08x plm %08x\n", t, fsp, gfw, pl);
				last_fsp = fsp;
				last_gfw = gfw;
			}
			if (fsp == 0xff && (gfw & 0xff) == 0xff)
				break;
		}
		msleep(5);
	}
}

static void
gh100_gsp_aros_reset(struct nvkm_gsp *gsp, int mode)
{
	struct nvkm_subdev *subdev = &gsp->subdev;
	struct nvkm_device *device = subdev->device;
	struct pci_dev *pdev = container_of(device, struct nvkm_device_pci, device)->pdev;
	struct pci_bus bus0 = { .number = 0 };
	u32 cfg[64];
	u16 bctl;
	int i;

	for (i = 0; i < 64; i++)
		pci_read_config_dword(pdev, i * 4, &cfg[i]);

	if (mode == 2) {
		static const u32 rcs[] = { 0x21000000, 0x23000000, 0x24000000 };
		void __iomem *dbi = NULL;
		u32 core;
		u8 cls, probe;

		/* find the root complex this card hangs off: mark its config
		   header through the bus and look for the mark in each DBI */
		pci_bus_read_config_byte(&bus0, 0, 0x0c, &cls);
		pci_bus_write_config_byte(&bus0, 0, 0x0c, cls ^ 0x55);
		pci_bus_read_config_byte(&bus0, 0, 0x0c, &probe);
		for (i = 0; i < ARRAY_SIZE(rcs); i++) {
			dbi = ioremap(rcs[i], 0x401000);
			if (dbi && (readl(dbi + 0x0c) & 0xff) == probe)
				break;
			if (dbi)
				iounmap(dbi);
			dbi = NULL;
		}
		pci_bus_write_config_byte(&bus0, 0, 0x0c, cls);
		if (!dbi || probe == cls) {
			nvkm_error(subdev, "cannot find the root complex (mark %02x/%02x)\n", cls, probe);
			return;
		}
		nvkm_info(subdev, "root complex at %08x\n", rcs[i]);
		core = readl(dbi + 0x400000);
		nvkm_info(subdev, "phy reset (core %08x, debug1 %08x, hold %ldms)\n", core, readl(dbi + 0x72c),
			  nvkm_longopt(device->cfgopt, "NvGfwPhyHold", 3000));
		writel((core & ~0x00000080) | 0x00004000, dbi + 0x400000);
		msleep(nvkm_longopt(device->cfgopt, "NvGfwPhyHold", 3000));
		writel((core & ~0x00004000) | 0x00000080, dbi + 0x400000);
		for (i = 0; i < 2000; i += 5) {
			u32 dbg = readl(dbi + 0x72c);
			if ((dbg & 0x10) && !(dbg & 0x20000))
				break;
			msleep(5);
		}
		nvkm_info(subdev, "link %s after %dms (core %08x, debug1 %08x)\n",
			  i < 2000 ? "up" : "not up", i, readl(dbi + 0x400000), readl(dbi + 0x72c));
		iounmap(dbi);
	} else {
		pci_bus_read_config_word(&bus0, 0, 0x3e, &bctl);
		nvkm_info(subdev, "secondary bus reset (bridge control %04x)\n", bctl);
		pci_bus_write_config_word(&bus0, 0, 0x3e, bctl | 0x40);
		msleep(20);
		pci_bus_write_config_word(&bus0, 0, 0x3e, bctl);
	}

	gh100_gsp_aros_watch(gsp, pdev, cfg, 8000);
	gh100_gsp_aros_dump(gsp, "after reset");
}

int gh100_fsp_aros_cmd(struct nvkm_fsp *fsp, u8 nvdm_type);

/*
 * The slot's PERST# is a GPIO on this platform (port C of the DesignWare
 * APB GPIO block through a level shifter). Show the ports, and if exactly
 * one of C0..C3 is an output driven high, pulse it: a fundamental reset
 * with the root complex already up is the one thing the card has not had.
 */
static void
gh100_gsp_aros_perst(struct nvkm_gsp *gsp)
{
	struct nvkm_subdev *subdev = &gsp->subdev;
	struct nvkm_device *device = subdev->device;
	struct pci_dev *pdev = container_of(device, struct nvkm_device_pci, device)->pdev;
	void *compat_map_mmio(unsigned long pa, unsigned long size);
	void __iomem *gpio = compat_map_mmio(0x20200000, 0x1000);
	u32 cfg[64], dr, ddr, ext, cand;
	int i, bit = -1;

	if (!gpio) {
		nvkm_error(subdev, "cannot map the gpio block\n");
		return;
	}
	nvkm_info(subdev, "gpio a dr %08x ddr %08x ext %08x, b dr %08x ddr %08x ext %08x\n",
		  readl(gpio + 0x00), readl(gpio + 0x04), readl(gpio + 0x50),
		  readl(gpio + 0x0c), readl(gpio + 0x10), readl(gpio + 0x54));
	dr = readl(gpio + 0x18);
	ddr = readl(gpio + 0x1c);
	ext = readl(gpio + 0x58);
	nvkm_info(subdev, "gpio c dr %08x ddr %08x ext %08x, d dr %08x ddr %08x ext %08x\n",
		  dr, ddr, ext, readl(gpio + 0x24), readl(gpio + 0x28), readl(gpio + 0x5c));

	/*
	 * Every port is an input here, so PERST# is held high by a pull-up
	 * and asserted by driving the pin low. The slot's low-speed lines
	 * (PERST#, WAKE#, PRSNT#s) sit on C0..C3; the ones reading high are
	 * the candidates. Try each: the one that makes the card vanish from
	 * the bus is PERST#.
	 */
	if (!nvkm_boolopt(device->cfgopt, "NvGfwPerst", true)) {
		nvkm_info(subdev, "perst: disabled\n");
		return;
	}
	cand = nvkm_longopt(device->cfgopt, "NvGfwPerstMask", (ext & ~ddr) & 0x0f);

	for (i = 0; i < 64; i++)
		pci_read_config_dword(pdev, i * 4, &cfg[i]);

	for (bit = 0; bit < 4; bit++) {
		u32 id;
		int t;

		if (!(cand & (1 << bit)))
			continue;
		nvkm_info(subdev, "perst: pulsing gpio c%d\n", bit);
		writel(dr & ~(1 << bit), gpio + 0x18);
		writel(ddr | (1 << bit), gpio + 0x1c);
		msleep(100);
		writel(ddr, gpio + 0x1c);
		writel(dr, gpio + 0x18);
		for (t = 0; t < 300; t += 5) {
			pci_read_config_dword(pdev, 0, &id);
			if (id != cfg[0])
				break;
			msleep(5);
		}
		nvkm_info(subdev, "perst: gpio c%d: card %s\n", bit, t < 300 ? "reset" : "unaffected");
		if (t < 300)
			break;
	}
	if (bit == 4)
		return;
	gh100_gsp_aros_watch(gsp, pdev, cfg, 12000);
	gh100_gsp_aros_dump(gsp, "after perst");
}

/*
 * A function level reset is the one reset NVIDIA's own driver expects to
 * be followed by a devinit ("FLR devinit timeout"); try it and watch.
 */
static void
gh100_gsp_aros_flr(struct nvkm_gsp *gsp)
{
	struct nvkm_subdev *subdev = &gsp->subdev;
	struct nvkm_device *device = subdev->device;
	struct pci_dev *pdev = container_of(device, struct nvkm_device_pci, device)->pdev;
	u32 cfg[64], devcap;
	u16 devctl;
	u8 pos = pdev->pcie_cap;
	int i;

	if (!pos) {
		u8 next, id;
		pci_read_config_byte(pdev, 0x34, &next);
		for (i = 0; next && i < 32; i++) {
			pci_read_config_byte(pdev, next, &id);
			if (id == 0x10) {
				pos = next;
				break;
			}
			pci_read_config_byte(pdev, next + 1, &next);
		}
	}
	if (!pos) {
		nvkm_info(subdev, "flr: no pcie capability\n");
		return;
	}
	pci_read_config_dword(pdev, pos + 4, &devcap);
	pci_read_config_word(pdev, pos + 8, &devctl);
	nvkm_info(subdev, "flr: pcie cap at %02x, devcap %08x (flr %s), devctl %04x\n",
		  pos, devcap, (devcap & (1 << 28)) ? "supported" : "not supported", devctl);
	if (!(devcap & (1 << 28)))
		return;

	for (i = 0; i < 64; i++)
		pci_read_config_dword(pdev, i * 4, &cfg[i]);

	pci_write_config_word(pdev, pos + 8, devctl | 0x8000);
	msleep(100);
	gh100_gsp_aros_watch(gsp, pdev, cfg, 12000);
	gh100_gsp_aros_dump(gsp, "after flr");
}

/*
 * The board's embedded controller owns the peripheral reset (and slot
 * power); the platform tells it about suspend/resume over its UART with a
 * three byte message. Say "suspending", then "resumed", and watch what
 * that does to the card - the hope is a fundamental reset or power cycle
 * while the root complex is up.
 */
static void
gh100_gsp_aros_ec_cycle(struct nvkm_gsp *gsp)
{
	struct nvkm_subdev *subdev = &gsp->subdev;
	struct nvkm_device *device = subdev->device;
	struct pci_dev *pdev = container_of(device, struct nvkm_device_pci, device)->pdev;
	void *compat_map_mmio(unsigned long pa, unsigned long size);
	void __iomem *uart = compat_map_mmio(0x20310000, 0x1000);
	static const u8 pre[] = { 0x05, 0x01, 0x88 }, post[] = { 0x05, 0x01, 0x89 };
	u32 cfg[64], id;
	int i, t;

	if (!uart) {
		nvkm_error(subdev, "cannot map the ec uart\n");
		return;
	}
	for (i = 0; i < 64; i++)
		pci_read_config_dword(pdev, i * 4, &cfg[i]);

	/* 115200 8N1 from the 62.5 MHz uart clock, fifo on */
	writel(0x83, uart + (3 << 2));
	writel(34, uart + (0 << 2));
	writel(0, uart + (1 << 2));
	writel(0x03, uart + (3 << 2));
	writel(0x07, uart + (2 << 2));

	nvkm_info(subdev, "ec: suspend message\n");
	for (i = 0; i < 3; i++) {
		for (t = 0; t < 1000 && !(readl(uart + (5 << 2)) & 0x20); t++)
			udelay(10);
		writel(pre[i], uart + (0 << 2));
	}
	for (t = 0; t < 3000; t += 10) {
		pci_read_config_dword(pdev, 0, &id);
		if (id != cfg[0])
			break;
		msleep(10);
	}
	nvkm_info(subdev, "ec: card %s after %dms\n", t < 3000 ? "gone" : "still there", t);

	nvkm_info(subdev, "ec: resume message\n");
	for (i = 0; i < 3; i++) {
		for (t = 0; t < 1000 && !(readl(uart + (5 << 2)) & 0x20); t++)
			udelay(10);
		writel(post[i], uart + (0 << 2));
	}
	gh100_gsp_aros_watch(gsp, pdev, cfg, 15000);
	gh100_gsp_aros_dump(gsp, "after ec cycle");
}

/* D3hot and back: a device-side soft reset on cards that allow it */
static void
gh100_gsp_aros_d3(struct nvkm_gsp *gsp)
{
	struct nvkm_subdev *subdev = &gsp->subdev;
	struct nvkm_device *device = subdev->device;
	struct pci_dev *pdev = container_of(device, struct nvkm_device_pci, device)->pdev;
	u32 cfg[64];
	u16 pmcsr;
	u8 pos = 0, next, id;
	int i;

	pci_read_config_byte(pdev, 0x34, &next);
	for (i = 0; next && i < 32; i++) {
		pci_read_config_byte(pdev, next, &id);
		if (id == 0x01) {
			pos = next;
			break;
		}
		pci_read_config_byte(pdev, next + 1, &next);
	}
	if (!pos) {
		nvkm_info(subdev, "d3: no pm capability\n");
		return;
	}
	for (i = 0; i < 64; i++)
		pci_read_config_dword(pdev, i * 4, &cfg[i]);
	pci_read_config_word(pdev, pos + 4, &pmcsr);
	nvkm_info(subdev, "d3: pm cap at %02x, pmcsr %04x -> D3hot\n", pos, pmcsr);
	pci_write_config_word(pdev, pos + 4, (pmcsr & ~3) | 3);
	msleep(200);
	pci_read_config_word(pdev, pos + 4, &pmcsr);
	nvkm_info(subdev, "d3: pmcsr %04x -> D0\n", pmcsr);
	pci_write_config_word(pdev, pos + 4, pmcsr & ~3);
	msleep(100);
	gh100_gsp_aros_watch(gsp, pdev, cfg, 12000);
	gh100_gsp_aros_dump(gsp, "after d3");
}

/* ask the FSP itself to reset, then watch what its boot does */
static void
gh100_gsp_aros_fsp_reset(struct nvkm_gsp *gsp)
{
	struct nvkm_subdev *subdev = &gsp->subdev;
	struct nvkm_device *device = subdev->device;
	struct pci_dev *pdev = container_of(device, struct nvkm_device_pci, device)->pdev;
	u32 cfg[64];
	int i;

	for (i = 0; i < 64; i++)
		pci_read_config_dword(pdev, i * 4, &cfg[i]);

	nvkm_info(subdev, "fsp reset command\n");
	gh100_fsp_aros_cmd(device->fsp, 0x04);
	gh100_gsp_aros_watch(gsp, pdev, cfg, 10000);
	gh100_gsp_aros_dump(gsp, "after fsp reset");
}
#endif

int compat_dump_file(const char *name, const void *data, unsigned long size);

/*
 * Leave what the card and RM had to say on the boot volume for offline
 * decoding: the libos log queues, the VBIOS and a few register windows.
 */
static void
gh100_gsp_aros_dumpfiles(struct nvkm_gsp *gsp)
{
	struct nvkm_subdev *subdev = &gsp->subdev;
	struct nvkm_device *device = subdev->device;
	struct pci_dev *pdev = container_of(device, struct nvkm_device_pci, device)->pdev;
	u32 *buf;
	int i, ret;

	if (!nvkm_boolopt(device->cfgopt, "NvDump", true))
		return;

	if (gsp->loginit.data)
		compat_dump_file("loginit.bin", gsp->loginit.data, gsp->loginit.size);
	if (gsp->logintr.data)
		compat_dump_file("logintr.bin", gsp->logintr.data, gsp->logintr.size);
	if (gsp->logrm.data)
		compat_dump_file("logrm.bin", gsp->logrm.data, gsp->logrm.size);
	if (gsp->rmargs.data)
		compat_dump_file("rmargs.bin", gsp->rmargs.data, gsp->rmargs.size);
	if (gsp->wpr_meta.data)
		compat_dump_file("wprmeta.bin", gsp->wpr_meta.data, gsp->wpr_meta.size);

	buf = vmalloc(0x100000);
	if (!buf)
		return;

	/* register windows: 0x000000-0x001fff, 0x088000-0x08bfff (config mirror),
	   0x118000-0x118fff (aon), 0x8f0000-0x8f0fff (fsp), 0xad0000-0xad0fff (therm) */
	{
		static const struct { u32 base, size; } win[] = {
			{ 0x000000, 0x2000 }, { 0x088000, 0x4000 }, { 0x118000, 0x1000 },
			{ 0x8f0000, 0x1000 }, { 0xad0000, 0x1000 }, { 0x101000, 0x1000 },
			{ 0x110000, 0x2000 }, { 0x00e000, 0x2000 },
		};
		u32 off = 0;
		for (i = 0; i < ARRAY_SIZE(win); i++) {
			u32 j;
			buf[off++] = 0x77696e00 | i;
			buf[off++] = win[i].base;
			buf[off++] = win[i].size;
			buf[off++] = 0;
			for (j = 0; j < win[i].size; j += 4)
				buf[off++] = nvkm_rd32(device, win[i].base + j);
		}
		compat_dump_file("regs.bin", buf, off * 4);
	}

	/* PROM window */
	for (i = 0; i < 0x100000; i += 4)
		buf[i / 4] = nvkm_rd32(device, 0x300000 + i);
	nvkm_info(subdev, "prom: %08x %08x %08x %08x\n", buf[0], buf[1], buf[2], buf[3]);
	compat_dump_file("vbios-prom.rom", buf, 0x100000);

	/* expansion ROM BAR */
	{
		size_t romsize = 0;
		void __iomem *rom;
		u32 romaddr;

		struct pci_bus bus0 = { .number = 0 }, bus1 = { .number = 1 };
		u32 romorig, mask, base = 0, top = 0, bar, sz;
		u16 mem;

		pci_read_config_dword(pdev, 0x30, &romorig);
		pci_write_config_dword(pdev, 0x30, 0xfffff800);
		pci_read_config_dword(pdev, 0x30, &mask);
		pci_write_config_dword(pdev, 0x30, romorig);
		romsize = mask ? (~(mask & ~0x7ff) + 1) : 0;
		if (romsize > 0x100000)
			romsize = 0x100000;

		/* bridge non-prefetchable window; take its top, unless BAR0/BAR of fn1 sit there */
		pci_bus_read_config_word(&bus0, 0, 0x20, &mem);
		base = (u32)(mem & 0xfff0) << 16;
		pci_bus_read_config_word(&bus0, 0, 0x22, &mem);
		top = ((u32)(mem & 0xfff0) << 16) + 0x100000;
		nvkm_info(subdev, "rom bar %08x size %zx window %08x-%08x\n", romorig, romsize, base, top);
		romaddr = top - romsize;
		pci_read_config_dword(pdev, 0x10, &bar);
		bar &= ~0xf;
		sz = 0x1000000;
		if (bar >= base && bar < top && bar + sz > romaddr)
			romaddr = bar - romsize;
		pci_bus_read_config_dword(&bus1, 1, 0x10, &bar);
		bar &= ~0xf;
		if (bar >= base && bar < top && bar + 0x4000 > romaddr && bar < romaddr + romsize)
			romaddr = bar - romsize;
		if (romsize && romaddr >= base && romaddr + romsize <= top) {
			pci_write_config_dword(pdev, 0x30, romaddr | 1);
			rom = ioremap(romaddr, romsize);
			if (rom) {
				for (i = 0; i < romsize; i += 4)
					buf[i / 4] = readl(rom + i);
				nvkm_info(subdev, "rom @%08x: %08x %08x %08x %08x (%zx)\n", romaddr, buf[0], buf[1], buf[2], buf[3], romsize);
				compat_dump_file("vbios-bar.rom", buf, romsize);
				iounmap(rom);
			}
			pci_write_config_dword(pdev, 0x30, romorig);
		}
	}
	vfree(buf);
	ret = 0;
	(void)ret;
	nvkm_info(subdev, "dump written to SYS:nouveau-dump\n");
}

int
gh100_gsp_init(struct nvkm_gsp *gsp)
{
	struct nvkm_subdev *subdev = &gsp->subdev;
	struct nvkm_device *device = subdev->device;
	const bool resume = gsp->sr.meta.data != NULL;
	struct nvkm_gsp_mem *meta;
	GSP_FMC_BOOT_PARAMS *args;
	int ret, time = 4000;
	u32 rsvd_size;
	u32 mbox0;

	if (!resume) {
		ret = nvkm_gsp_mem_ctor(gsp, sizeof(*args), &gsp->fmc.args);
		if (ret)
			return ret;

		meta = &gsp->wpr_meta;
	} else {
		gsp->rm->api->gsp->set_rmargs(gsp, true);
		meta = &gsp->sr.meta;
	}

	args = gsp->fmc.args.data;

	args->bootGspRmParams.gspRmDescOffset = meta->addr;
	args->bootGspRmParams.gspRmDescSize = meta->size;
	args->bootGspRmParams.target = GSP_DMA_TARGET_COHERENT_SYSTEM;
	args->bootGspRmParams.bIsGspRmBoot = 1;

	args->gspRmParams.target = GSP_DMA_TARGET_NONCOHERENT_SYSTEM;
	args->gspRmParams.bootArgsOffset = gsp->libos.addr;

	rsvd_size = gsp->fb.heap.size;
	if (gsp->rm->wpr->rsvd_size_pmu)
		rsvd_size = ALIGN(rsvd_size + gsp->rm->wpr->rsvd_size_pmu, 0x200000);

#if defined(__AROS__)
	/*
	 * The GPU's own firmware boot (devinit) has to be complete before RM
	 * is started, or RM gives up on it. Give it a moment, say where it
	 * got to, and if it is not there try a bus reset to restart it.
	 */
	{
		int gfw = 4000;
		u32 pl, progress;

		while (!gh100_gsp_aros_gfw_done(device, &progress, &pl) && gfw--)
			usleep_range(1000, 2000);
		gh100_gsp_aros_dump(gsp, gfw < 0 ? "GFW boot timed out" : "GFW boot complete");
		if (gfw < 0) {
			int mode = nvkm_longopt(device->cfgopt, "NvGfwReset", 0);
			if (mode & 64)
				gh100_gsp_aros_d3(gsp);
			if ((mode & 32) && !gh100_gsp_aros_gfw_done(device, &progress, &pl))
				gh100_gsp_aros_ec_cycle(gsp);
			if ((mode & 16) && !gh100_gsp_aros_gfw_done(device, &progress, &pl))
				gh100_gsp_aros_flr(gsp);
			if ((mode & 8) && !gh100_gsp_aros_gfw_done(device, &progress, &pl))
				gh100_gsp_aros_perst(gsp);
			if ((mode & 4) && !gh100_gsp_aros_gfw_done(device, &progress, &pl))
				gh100_gsp_aros_fsp_reset(gsp);
			if ((mode & 1) && !gh100_gsp_aros_gfw_done(device, &progress, &pl))
				gh100_gsp_aros_reset(gsp, 1);
			if ((mode & 2) && !gh100_gsp_aros_gfw_done(device, &progress, &pl))
				gh100_gsp_aros_reset(gsp, 2);
		}
	}
	compat_dma_sync_all_coherent();
	/*
	 * The FMC/ACR occasionally refuses the first boot on this platform
	 * (mailbox 0xb) although a fresh attempt a moment later succeeds;
	 * give it a few tries before giving up.
	 */
	{
		int attempt;

		for (attempt = 0; attempt < 4; attempt++) {
			if (attempt) {
				nvkm_warn(subdev, "GSP-FMC boot retry %d\n", attempt);
				/* the mailbox keeps the last verdict; clear it so a
				   fresh answer (or none) is what we wait for */
				nvkm_falcon_wr32(&gsp->falcon, NV_PFALCON_FALCON_MAILBOX0, 0);
				nvkm_falcon_wr32(&gsp->falcon, NV_PFALCON_FALCON_MAILBOX1, 0);
				msleep(1000);
				compat_dma_sync_all_coherent();
			}
			ret = nvkm_fsp_boot_gsp_fmc(device->fsp, gsp->fmc.args.addr, rsvd_size, resume,
						    gsp->fmc.fw.addr, gsp->fmc.hash, gsp->fmc.pkey, gsp->fmc.sig);
			if (ret) {
				nvkm_error(subdev, "GSP-FMC boot request failed: %d\n", ret);
				continue;
			}

			time = 4000;
			mbox0 = 0;
			do {
				if (gh100_gsp_lockdown_released(gsp, &mbox0))
					break;
				usleep_range(1000, 2000);
			} while (time--);

			if (time < 0) {
				nvkm_error(subdev, "GSP-FMC boot timed out\n");
				ret = -ETIMEDOUT;
				continue;
			}
			if (mbox0) {
				nvkm_error(subdev, "GSP-FMC boot failed (mbox: 0x%08x)\n", mbox0);
				ret = -EIO;
				continue;
			}
			ret = 0;
			break;
		}
		if (ret) {
			gh100_gsp_aros_dumpfiles(gsp);
			return ret;
		}
	}
#else
	ret = nvkm_fsp_boot_gsp_fmc(device->fsp, gsp->fmc.args.addr, rsvd_size, resume,
				    gsp->fmc.fw.addr, gsp->fmc.hash, gsp->fmc.pkey, gsp->fmc.sig);
	if (ret)
		return ret;

	do {
		if (gh100_gsp_lockdown_released(gsp, &mbox0))
			break;

		usleep_range(1000, 2000);
	} while(time--);

	if (time < 0) {
		nvkm_error(subdev, "GSP-FMC boot timed out\n");
		return -ETIMEDOUT;
	}

	if (mbox0) {
		nvkm_error(subdev, "GSP-FMC boot failed (mbox: 0x%08x)\n", mbox0);
		return -EIO;
	}
#endif

	return r535_gsp_init(gsp);
}

static int
gh100_gsp_wpr_meta_init(struct nvkm_gsp *gsp)
{
	GspFwWprMeta *meta;
	int ret;

	ret = nvkm_gsp_mem_ctor(gsp, sizeof(*meta), &gsp->wpr_meta);
	if (ret)
		return ret;

	gsp->fb.size = nvkm_fb_vidmem_size(gsp->subdev.device);
	gsp->fb.bios.vga_workspace.size = 128 * 1024;
	gsp->fb.heap.size = gsp->rm->wpr->heap_size_non_wpr;

	meta = gsp->wpr_meta.data;

	meta->magic = GSP_FW_WPR_META_MAGIC;
	meta->revision = GSP_FW_WPR_META_REVISION;

	meta->sizeOfRadix3Elf = gsp->fw.len;
	meta->sysmemAddrOfRadix3Elf = gsp->radix3.lvl0.addr;

	meta->sizeOfBootloader = gsp->boot.fw.size;
	meta->sysmemAddrOfBootloader = gsp->boot.fw.addr;
	meta->bootloaderCodeOffset = gsp->boot.code_offset;
	meta->bootloaderDataOffset = gsp->boot.data_offset;
	meta->bootloaderManifestOffset = gsp->boot.manifest_offset;

	meta->sysmemAddrOfSignature = gsp->sig.addr;
	meta->sizeOfSignature = gsp->sig.size;

	meta->nonWprHeapSize = gsp->fb.heap.size;
	meta->gspFwHeapSize = tu102_gsp_wpr_heap_size(gsp);
	meta->frtsSize = 0x100000;
	meta->vgaWorkspaceSize = gsp->fb.bios.vga_workspace.size;
	meta->pmuReservedSize = gsp->rm->wpr->rsvd_size_pmu;
	return 0;
}

/* The sh_flags value for the binary blobs in the ELF image */
#define FMC_SHF_FLAGS (SHF_MASKPROC | SHF_MASKOS | SHF_OS_NONCONFORMING | SHF_ALLOC)

#define ELF_HDR_SIZE ((u8)sizeof(struct elf32_hdr))
#define ELF_SHDR_SIZE ((u8)sizeof(struct elf32_shdr))

/* The FMC ELF header must be exactly this */
static const u8 elf_header[] = {
	0x7f, 'E', 'L', 'F', 1, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 1, 0, 0, 0, /* e_type, e_machine, e_version */
	0, 0, 0, 0, 0, 0, 0, 0, /* e_entry, e_phoff */

	ELF_HDR_SIZE, 0, 0, 0, 0, 0, 0, 0, /* e_shoff, e_flags */
	ELF_HDR_SIZE, 0, 0, 0, /* e_ehsize, e_phentsize */
	0, 0, ELF_SHDR_SIZE, 0, /* e_phnum, e_shentsize */

	6, 0, 1, 0, /* e_shnum, e_shstrndx */
};

/**
 * elf_validate_sections - validate each section in the FMC ELF image
 * @elf: ELF image
 * @length: size of the entire ELF image
 */
static bool
elf_validate_sections(const void *elf, size_t length)
{
	const struct elf32_hdr *ehdr = elf;
	const struct elf32_shdr *shdr = elf + ehdr->e_shoff;

	/* The offset of the first section */
	Elf32_Off section_begin = ehdr->e_shoff + ehdr->e_shnum * ehdr->e_shentsize;

	if (section_begin > length)
		return false;

	/* The first section header is the null section, so skip it */
	for (unsigned int i = 1; i < ehdr->e_shnum; i++) {
		if (i == ehdr->e_shstrndx) {
			if (shdr[i].sh_type != SHT_STRTAB)
				return false;
			if (shdr[i].sh_flags != SHF_STRINGS)
				return false;
		} else {
			if (shdr[i].sh_type != SHT_PROGBITS)
				return false;
			if (shdr[i].sh_flags != FMC_SHF_FLAGS)
				return false;
		}

		/* Ensure that each section is inside the image */
		if (shdr[i].sh_offset < section_begin ||
		    (u64)shdr[i].sh_offset + shdr[i].sh_size > length)
			return false;

		/* Non-zero sh_info is a CRC */
		if (shdr[i].sh_info) {
			/* The kernel's CRC32 needs a pre- and post-xor to match standard CRCs */
			u32 crc32 = crc32_le(~0, elf + shdr[i].sh_offset, shdr[i].sh_size) ^ ~0;

			if (shdr[i].sh_info != crc32)
				return false;
		}
	}

	return true;
}

/**
 * elf_section - return a pointer to the data for a given section
 * @elf: ELF image
 * @name: section name to search for
 * @len: pointer to returned length of found section
 */
static const void *
elf_section(const void *elf, const char *name, unsigned int *len)
{
	const struct elf32_hdr *ehdr = elf;
	const struct elf32_shdr *shdr = elf + ehdr->e_shoff;
	const char *names = elf + shdr[ehdr->e_shstrndx].sh_offset;

	for (unsigned int i = 1; i < ehdr->e_shnum; i++) {
		if (!strcmp(&names[shdr[i].sh_name], name)) {
			*len = shdr[i].sh_size;
			return elf + shdr[i].sh_offset;
		}
	}

	return NULL;
}

int
gh100_gsp_oneinit(struct nvkm_gsp *gsp)
{
	struct nvkm_subdev *subdev = &gsp->subdev;
	struct nvkm_device *device = subdev->device;
	struct nvkm_fsp *fsp = device->fsp;
	const void *fw = gsp->fws.fmc->data;
	const void *hash, *sig, *pkey, *img;
	unsigned int img_len = 0, hash_len = 0, pkey_len = 0, sig_len = 0;
	int ret;

	if (gsp->fws.fmc->size < ELF_HDR_SIZE ||
	    memcmp(fw, elf_header, sizeof(elf_header)) ||
	    !elf_validate_sections(fw, gsp->fws.fmc->size)) {
		nvkm_error(subdev, "fmc firmware image is invalid\n");
		return -ENODATA;
	}

	hash = elf_section(fw, "hash", &hash_len);
	sig = elf_section(fw, "signature", &sig_len);
	pkey = elf_section(fw, "publickey", &pkey_len);
	img = elf_section(fw, "image", &img_len);

	if (!hash || !sig || !pkey || !img) {
		nvkm_error(subdev, "fmc firmware image is invalid\n");
		return -ENODATA;
	}

	if (!nvkm_fsp_verify_gsp_fmc(fsp, hash_len, pkey_len, sig_len))
		return -EINVAL;

	/* Load GSP-FMC FW into memory. */
	ret = nvkm_gsp_mem_ctor(gsp, img_len, &gsp->fmc.fw);
	if (ret)
		return ret;

	memcpy(gsp->fmc.fw.data, img, img_len);

	gsp->fmc.hash = kmemdup(hash, hash_len, GFP_KERNEL);
	gsp->fmc.pkey = kmemdup(pkey, pkey_len, GFP_KERNEL);
	gsp->fmc.sig = kmemdup(sig, sig_len, GFP_KERNEL);
	if (!gsp->fmc.hash || !gsp->fmc.pkey || !gsp->fmc.sig)
		return -ENOMEM;

	ret = r535_gsp_oneinit(gsp);
	if (ret)
		return ret;

	return gh100_gsp_wpr_meta_init(gsp);
}

static const struct nvkm_gsp_func
gh100_gsp = {
	.flcn = &ga102_gsp_flcn,

	.sig_section = ".fwsignature_gh100",

	.dtor = r535_gsp_dtor,
	.oneinit = gh100_gsp_oneinit,
	.init = gh100_gsp_init,
	.fini = gh100_gsp_fini,

	.rm.gpu = &gh100_gpu,
};

int
gh100_gsp_load(struct nvkm_gsp *gsp, int ver, const struct nvkm_gsp_fwif *fwif)
{
	int ret;

	ret = tu102_gsp_load_rm(gsp, fwif);
	if (ret)
		goto done;

	ret = nvkm_gsp_load_fw(gsp, "fmc", fwif->ver, &gsp->fws.fmc);

done:
	if (ret)
		nvkm_gsp_dtor_fws(gsp);

	return ret;
}

static struct nvkm_gsp_fwif
gh100_gsps[] = {
	{ 2, gh100_gsp_load, &gh100_gsp, &r580_rm_gh100, "580.178.04" },
	{ 0, gh100_gsp_load, &gh100_gsp, &r570_rm_gh100, "570.144" },
	{}
};

int
gh100_gsp_new(struct nvkm_device *device, enum nvkm_subdev_type type, int inst,
	      struct nvkm_gsp **pgsp)
{
	return nvkm_gsp_new_(gh100_gsps, device, type, inst, pgsp);
}

NVKM_GSP_FIRMWARE_FMC(gh100, 570.144);
NVKM_GSP_FIRMWARE_FMC(gh100, 580.178.04);
