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
#include <subdev/timer.h>

#ifndef GSP_FW_FLAGS_RECOVERY_MARGIN_PRESENT
#define GSP_FW_FLAGS_RECOVERY_MARGIN_PRESENT (1 << 1)
#endif
#include <nvhw/ref/gh100/dev_riscv_pri.h>

int
gh100_gsp_fini(struct nvkm_gsp *gsp, enum nvkm_suspend_state suspend)
{
	struct nvkm_falcon *falcon = &gsp->falcon;
	struct nvkm_device *device = gsp->subdev.device;
	int ret, time = 2000;

	/* Shutdown RM. */
	ret = r535_gsp_fini(gsp, suspend);
	if (ret && suspend)
		return ret;

	/* Wait for RISC-V to halt, servicing display supervisors so RM's
	   teardown can make progress on this card. */
	do {
		u32 sv = nvkm_rd32(device, 0x611860) & 7;
		u32 data = nvkm_falcon_rd32(falcon, falcon->addr2 + NV_PRISCV_RISCV_CPUCTL);

		if (sv) {
			nvkm_wr32(device, 0x611860, sv);
			nvkm_wr32(device, 0x6107a8, 0x80000000);
		}
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

/*
 * kgspResetHw_GH100: the resource manager resets the GSP falcon before
 * every RISC-V bootstrap. A falcon left halted by a previous instance is
 * refused by the FMC (mailbox 0xb) until it has been through this.
 * NV_PGSP_FALCON_ENGINE: bit 0 RESET (1 assert), bits 10:8 RESET_STATUS
 * (0 asserted, 2 deasserted).
 */
static void
gh100_gsp_reset_hw(struct nvkm_gsp *gsp)
{
	struct nvkm_subdev *subdev = &gsp->subdev;
	struct nvkm_falcon *falcon = &gsp->falcon;
	u32 before, after, plm;
	int i;

	/* kgspResetHw_GB100: the reset register sits behind a priv level
	 * mask that GSP-RM lowers on its way out; wait for level-0 access
	 * (NV_PGSP_FALCON_RESET_PRIV_LEVEL_MASK read bit 0, write bit 4). */
	for (i = 0; i < 500; i++) {
		plm = nvkm_falcon_rd32(falcon, 0x3c4);
		if ((plm & 0x00000011) == 0x00000011)
			break;
		/* Target still locked behind the BAR0 decoupler: nothing in
		 * the falcon window can be reached yet, the reset is moot. */
		if ((plm & 0xffffff00) == 0xbadf4100) {
			nvkm_debug(subdev, "GSP target locked, reset not available\n");
			return;
		}
		usleep_range(1000, 2000);
	}
	if (i == 500)
		nvkm_warn(subdev, "GSP reset PLM not lowered (%08x)\n", plm);

	before = nvkm_falcon_rd32(falcon, 0x3c0);
	nvkm_falcon_mask(falcon, 0x3c0, 0x00000001, 0x00000001);
	for (i = 0; i < 1000; i++) {
		if (((nvkm_falcon_rd32(falcon, 0x3c0) >> 8) & 7) == 0)
			break;
		udelay(1);
	}
	if (i == 1000)
		nvkm_warn(subdev, "GSP falcon reset did not assert\n");

	nvkm_falcon_mask(falcon, 0x3c0, 0x00000001, 0x00000000);
	for (i = 0; i < 1000; i++) {
		if (((nvkm_falcon_rd32(falcon, 0x3c0) >> 8) & 7) == 2)
			break;
		udelay(1);
	}
	after = nvkm_falcon_rd32(falcon, 0x3c0);
	if (i == 1000)
		nvkm_warn(subdev, "GSP falcon reset did not deassert\n");
	nvkm_debug(subdev, "GSP falcon reset: plm %08x engine %08x -> %08x\n", plm, before, after);
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

#if defined(__AROS__)
/*
 * One line of GSP falcon / FSP state, logged on every boot: before the
 * driver touches the falcon and right before each GSP-FMC boot request.
 * The FMC occasionally refuses a cold boot (mbox 0xb); these lines let an
 * accepting and a refusing boot be compared. 0x110000 reads badf4100 when
 * the GSP falcon block is still disabled (power-on), badf56xx when it is
 * powered and priv-locked.
 */
static void
gh100_gsp_aros_snapshot(struct nvkm_gsp *gsp, const char *when)
{
	struct nvkm_device *device = gsp->subdev.device;
	struct nvkm_falcon *falcon = &gsp->falcon;

	nvkm_debug(&gsp->subdev, "%s: falcon %08x engine %08x mbox %08x/%08x plm %08x "
		    "cpuctl %08x hwcfg2 %08x wpr2 %08x/%08x fsp %08x GPU up %llu ms\n", when,
		    nvkm_rd32(device, 0x110000), nvkm_rd32(device, 0x1103c0),
		    nvkm_falcon_rd32(falcon, NV_PFALCON_FALCON_MAILBOX0),
		    nvkm_falcon_rd32(falcon, NV_PFALCON_FALCON_MAILBOX1),
		    nvkm_falcon_rd32(falcon, 0x3c4),
		    nvkm_falcon_rd32(falcon, falcon->addr2 + NV_PRISCV_RISCV_CPUCTL),
		    nvkm_falcon_rd32(falcon, NV_PFALCON_FALCON_HWCFG2),
		    nvkm_rd32(device, 0x1fa824), nvkm_rd32(device, 0x1fa828),
		    nvkm_rd32(device, 0xad00bc),
		    device->timer ? (unsigned long long)(nvkm_timer_read(device->timer) / 1000000) : 0ULL);
}
#endif

#if defined(__AROS__)
#include "../fsp/priv.h"

/*
 * After the FMC has refused a boot it leaves the GSP locked (reset PLM
 * 0x00800f8f): the host can no longer reset it, and a repeated COT request
 * just finds the old verdict. A function level reset takes the whole GPU,
 * FSP included, back through its own boot; then a retry is a real one.
 */
static int
gh100_gsp_fmc_recover(struct nvkm_gsp *gsp)
{
	struct nvkm_subdev *subdev = &gsp->subdev;
	struct nvkm_device *device = subdev->device;
	struct pci_dev *pdev = container_of(device, struct nvkm_device_pci, device)->pdev;
	u32 cfg[64], devcap, id;
	u16 devctl;
	u8 pos = pdev->pcie_cap;
	int i, t, ret;

	if (!pos) {
		u8 next, cap;

		pci_read_config_byte(pdev, 0x34, &next);
		for (i = 0; next && i < 32; i++) {
			pci_read_config_byte(pdev, next, &cap);
			if (cap == 0x10) {
				pos = next;
				break;
			}
			pci_read_config_byte(pdev, next + 1, &next);
		}
	}
	if (!pos)
		return -ENODEV;
	pci_read_config_dword(pdev, pos + 4, &devcap);
	if (!(devcap & (1 << 28))) {
		nvkm_error(subdev, "GSP-FMC recovery: the device has no function level reset\n");
		return -ENODEV;
	}
	for (i = 0; i < 64; i++)
		pci_read_config_dword(pdev, i * 4, &cfg[i]);
	pci_read_config_word(pdev, pos + 8, &devctl);
	pci_write_config_word(pdev, pos + 8, devctl | 0x8000);
	msleep(100);

	/* the function answers again once its configuration space is back */
	for (t = 0; t < 1000; t += 10) {
		pci_read_config_dword(pdev, 0, &id);
		if (id == cfg[0])
			break;
		msleep(10);
	}
	if (id != cfg[0]) {
		nvkm_error(subdev, "GSP-FMC recovery: device gone after FLR (id %08x)\n", id);
		return -EIO;
	}
	for (i = 1; i < 64; i++)
		pci_write_config_dword(pdev, i * 4, cfg[i]);
	pci_write_config_dword(pdev, 4, cfg[1]);

	/* and the FSP has been through its boot (its chip's own boot-complete register) */
	ret = device->fsp->func->wait_secure_boot(device->fsp);
	nvkm_warn(subdev, "GSP-FMC recovery: FLR done, FSP %s\n",
		  ret ? "did not finish its boot" : "booted");
	return ret;
}
#endif

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
	compat_dma_sync_all_coherent();
	gh100_gsp_aros_snapshot(gsp, "before reset");
	/*
	 * GSP-FMC refuses a boot (mailbox 0xb) when the GSP-RM image it is
	 * given is damaged, as data read from storage can occasionally be.
	 * Then the image is put right from the firmware file
	 * (r535_gsp_verify_image), the GPU, which a refused boot
	 * leaves locked against the host, goes through a function level
	 * reset, and as that clears what the other subdevs set up, the whole
	 * device init is run again (-EAGAIN to nvkm_device_init). Not when
	 * WPR2 is already up: then a GSP-RM the previous driver instance never
	 * unloaded is still running behind it and every attempt is refused.
	 */
	{
		u32 wpr2_hi = nvkm_rd32(device, 0x1fa828);

		if (wpr2_hi) {
			nvkm_error(subdev, "WPR2 is up (0x%08x, RISC-V %s): a previous "
				   "instance's GSP-RM is still loaded and GSP-FMC will "
				   "refuse to boot another - the card needs a reset it "
				   "takes as power-on\n", wpr2_hi,
				   nvkm_falcon_riscv_active(&gsp->falcon) ? "active" : "halted");
			return -EBUSY;
		}
		if (gsp->boot_attempts)
			nvkm_warn(subdev, "GSP-FMC boot attempt %d\n", gsp->boot_attempts + 1);
		gh100_gsp_reset_hw(gsp);
		/* the firmware page table's level 2 is not a coherent block: clean it too */
		dma_sync_sgtable_for_device(device->dev, &gsp->radix3.lvl2, DMA_TO_DEVICE);
		gh100_gsp_aros_snapshot(gsp, gsp->boot_attempts ? "before retry" : "before boot");
		ret = nvkm_fsp_boot_gsp_fmc(device->fsp, gsp->fmc.args.addr, rsvd_size, resume,
					    gsp->fmc.fw.addr, gsp->fmc.hash, gsp->fmc.pkey, gsp->fmc.sig);
		if (ret) {
			nvkm_error(subdev, "GSP-FMC boot request failed: %d\n", ret);
			return ret;
		}

		time = 4000;
		mbox0 = 0;
		do {
			if (gh100_gsp_lockdown_released(gsp, &mbox0))
				break;
			usleep_range(1000, 2000);
		} while (time--);

		if (time < 0 || mbox0) {
			if (time < 0)
				nvkm_error(subdev, "GSP-FMC boot timed out\n");
			else
				nvkm_error(subdev, "GSP-FMC boot failed (mbox: 0x%08x, mbox1 0x%08x, "
					   "hwcfg2 0x%08x, reset plm 0x%08x, GPU up %llu ms)\n", mbox0,
					   nvkm_falcon_rd32(&gsp->falcon, NV_PFALCON_FALCON_MAILBOX1),
					   nvkm_falcon_rd32(&gsp->falcon, NV_PFALCON_FALCON_HWCFG2),
					   nvkm_falcon_rd32(&gsp->falcon, 0x3c4),
					   device->timer ? (unsigned long long)(nvkm_timer_read(device->timer) / 1000000) : 0ULL);
			if (!resume && gsp->boot_attempts + 1 < 4) {
				int r535_gsp_verify_image(struct nvkm_gsp *);

				r535_gsp_verify_image(gsp);
				if (!gh100_gsp_fmc_recover(gsp)) {
					gsp->boot_attempts++;
					return -EAGAIN;
				}
			}
			return (time < 0) ? -ETIMEDOUT : -EIO;
		}
		gsp->boot_attempts = 0;
		ret = 0;
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
