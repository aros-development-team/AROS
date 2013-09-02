/*
 * Copyright 2005 Stephane Marchesin.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __NOUVEAU_DRV_H__
#define __NOUVEAU_DRV_H__

#define DRIVER_AUTHOR		"Stephane Marchesin"
#define DRIVER_EMAIL		"dri-devel@lists.sourceforge.net"

#define DRIVER_NAME		"nouveau"
#define DRIVER_DESC		"nVidia Riva/TNT/GeForce"
#define DRIVER_DATE		"20090420"

#define DRIVER_MAJOR		0
#define DRIVER_MINOR		0
#define DRIVER_PATCHLEVEL	16

#define NOUVEAU_FAMILY   0x0000FFFF
#define NOUVEAU_FLAGS    0xFFFF0000

#if defined(__AROS__)
struct drm_encoder;
struct drm_minor;
struct drm_connector;
#include "drm_compat_types.h"
#include "drm_compat_funcs.h"
#endif

#include "ttm/ttm_bo_api.h"
#include "ttm/ttm_bo_driver.h"
#include "ttm/ttm_placement.h"
#if !defined(__AROS__)
#include "ttm/ttm_memory.h"
#endif
#include "ttm/ttm_module.h"

struct nouveau_fpriv {
	spinlock_t lock;
	struct list_head channels;
	struct nouveau_vm *vm;
};

static inline struct nouveau_fpriv *
nouveau_fpriv(struct drm_file *file_priv)
{
	return file_priv ? file_priv->driver_priv : NULL;
}

#define DRM_FILE_PAGE_OFFSET (0x100000000ULL >> PAGE_SHIFT)

#include "nouveau_drm.h"
#include "nouveau_reg.h"
#include "nouveau_bios.h"
#include "nouveau_util.h"

struct nouveau_grctx;
struct nouveau_mem;
#include "nouveau_vm.h"

#define MAX_NUM_DCB_ENTRIES 16

#define NOUVEAU_MAX_CHANNEL_NR 128
#define NOUVEAU_MAX_TILE_NR 15

struct nouveau_mem {
	struct drm_device *dev;

	struct nouveau_vma bar_vma;
	struct nouveau_vma vma[2];
	u8  page_shift;

	struct drm_mm_node *tag;
	struct list_head regions;
	dma_addr_t *pages;
	u32 memtype;
	u64 offset;
	u64 size;
};

struct nouveau_tile_reg {
	bool used;
	uint32_t addr;
	uint32_t limit;
	uint32_t pitch;
	uint32_t zcomp;
	struct drm_mm_node *tag_mem;
	struct nouveau_fence *fence;
};

struct nouveau_bo {
	struct ttm_buffer_object bo;
	struct ttm_placement placement;
	u32 valid_domains;
	u32 placements[3];
	u32 busy_placements[3];
	struct ttm_bo_kmap_obj kmap;
	struct list_head head;

	/* protected by ttm_bo_reserve() */
	struct drm_file *reserved_by;
	struct list_head entry;
	int pbbo_index;
	bool validate_mapped;

	struct nouveau_channel *channel;

	struct list_head vma_list;
	unsigned page_shift;

	uint32_t tile_mode;
	uint32_t tile_flags;
	struct nouveau_tile_reg *tile;

	struct drm_gem_object *gem;
	int pin_refcnt;
};

#define nouveau_bo_tile_layout(nvbo)				\
	((nvbo)->tile_flags & NOUVEAU_GEM_TILE_LAYOUT_MASK)

static inline struct nouveau_bo *
nouveau_bo(struct ttm_buffer_object *bo)
{
	return container_of(bo, struct nouveau_bo, bo);
}

static inline struct nouveau_bo *
nouveau_gem_object(struct drm_gem_object *gem)
{
	return gem ? gem->driver_private : NULL;
}

/* TODO: submit equivalent to TTM generic API upstream? */
static inline void __iomem *
nvbo_kmap_obj_iovirtual(struct nouveau_bo *nvbo)
{
	bool is_iomem;
	void __iomem *ioptr = (void __force __iomem *)ttm_kmap_obj_virtual(
						&nvbo->kmap, &is_iomem);
//FIXME:	WARN_ON_ONCE(ioptr && !is_iomem);
	return ioptr;
}

enum nouveau_flags {
	NV_NFORCE   = 0x10000000,
	NV_NFORCE2  = 0x20000000
};

#define NVOBJ_ENGINE_SW		0
#define NVOBJ_ENGINE_GR		1
#define NVOBJ_ENGINE_CRYPT	2
#define NVOBJ_ENGINE_COPY0	3
#define NVOBJ_ENGINE_COPY1	4
#define NVOBJ_ENGINE_MPEG	5
#define NVOBJ_ENGINE_PPP	NVOBJ_ENGINE_MPEG
#define NVOBJ_ENGINE_BSP	6
#define NVOBJ_ENGINE_VP		7
#define NVOBJ_ENGINE_DISPLAY	15
#define NVOBJ_ENGINE_NR		16

#define NVOBJ_FLAG_DONT_MAP             (1 << 0)
#define NVOBJ_FLAG_ZERO_ALLOC		(1 << 1)
#define NVOBJ_FLAG_ZERO_FREE		(1 << 2)
#define NVOBJ_FLAG_VM			(1 << 3)
#define NVOBJ_FLAG_VM_USER		(1 << 4)

#define NVOBJ_CINST_GLOBAL	0xdeadbeef

struct nouveau_gpuobj {
	struct drm_device *dev;
	struct kref refcount;
	struct list_head list;

	void *node;
	u32 *suspend;

	uint32_t flags;

	u32 size;
	u32 pinst;	/* PRAMIN BAR offset */
	u32 cinst;	/* Channel offset */
	u64 vinst;	/* VRAM address */
	u64 linst;	/* VM address */

	uint32_t engine;
	uint32_t class;

	void (*dtor)(struct drm_device *, struct nouveau_gpuobj *);
	void *priv;
};

struct nouveau_page_flip_state {
	struct list_head head;
	struct drm_pending_vblank_event *event;
	int crtc, bpp, pitch, x, y;
	uint64_t offset;
};

enum nouveau_channel_mutex_class {
	NOUVEAU_UCHANNEL_MUTEX,
	NOUVEAU_KCHANNEL_MUTEX
};

struct nouveau_channel {
	struct drm_device *dev;
	struct list_head list;
	int id;

	/* references to the channel data structure */
	struct kref ref;
	/* users of the hardware channel resources, the hardware
	 * context will be kicked off when it reaches zero. */
	atomic_t users;
	struct mutex mutex;

	/* owner of this fifo */
	struct drm_file *file_priv;
	/* mapping of the fifo itself */
	struct drm_local_map *map;

	/* mapping of the regs controlling the fifo */
	void __iomem *user;
	uint32_t user_get;
	uint32_t user_put;

	/* Fencing */
	struct {
		/* lock protects the pending list only */
		spinlock_t lock;
		struct list_head pending;
		uint32_t sequence;
		uint32_t sequence_ack;
		atomic_t last_sequence_irq;
		struct nouveau_vma vma;
	} fence;

	/* DMA push buffer */
	struct nouveau_gpuobj *pushbuf;
	struct nouveau_bo     *pushbuf_bo;
	struct nouveau_vma     pushbuf_vma;
	uint32_t               pushbuf_base;

	/* Notifier memory */
	struct nouveau_bo *notifier_bo;
	struct nouveau_vma notifier_vma;
	struct drm_mm notifier_heap;

	/* PFIFO context */
	struct nouveau_gpuobj *ramfc;
	struct nouveau_gpuobj *cache;
	void *fifo_priv;

	/* Execution engine contexts */
	void *engctx[NVOBJ_ENGINE_NR];

	/* NV50 VM */
	struct nouveau_vm     *vm;
	struct nouveau_gpuobj *vm_pd;

	/* Objects */
	struct nouveau_gpuobj *ramin; /* Private instmem */
	struct drm_mm          ramin_heap; /* Private PRAMIN heap */
	struct nouveau_ramht  *ramht; /* Hash table */

	/* GPU object info for stuff used in-kernel (mm_enabled) */
	uint32_t m2mf_ntfy;
	uint32_t vram_handle;
	uint32_t gart_handle;
	bool accel_done;

	/* Push buffer state (only for drm's channel on !mm_enabled) */
	struct {
		int max;
		int free;
		int cur;
		int put;
		/* access via pushbuf_bo */

		int ib_base;
		int ib_max;
		int ib_free;
		int ib_put;
	} dma;

	uint32_t sw_subchannel[8];

	struct nouveau_vma dispc_vma[2];
	struct {
		struct nouveau_gpuobj *vblsem;
		uint32_t vblsem_head;
		uint32_t vblsem_offset;
		uint32_t vblsem_rval;
		struct list_head vbl_wait;
		struct list_head flip;
	} nvsw;

/*FIXME:COMMENT	struct {
		bool active;
		char name[32];
		struct drm_info_list info;
	} debugfs;*/
};

struct nouveau_exec_engine {
	void (*destroy)(struct drm_device *, int engine);
	int  (*init)(struct drm_device *, int engine);
	int  (*fini)(struct drm_device *, int engine, bool suspend);
	int  (*context_new)(struct nouveau_channel *, int engine);
	void (*context_del)(struct nouveau_channel *, int engine);
	int  (*object_new)(struct nouveau_channel *, int engine,
			   u32 handle, u16 class);
	void (*set_tile_region)(struct drm_device *dev, int i);
	void (*tlb_flush)(struct drm_device *, int engine);
};

struct nouveau_instmem_engine {
	void	*priv;

	int	(*init)(struct drm_device *dev);
	void	(*takedown)(struct drm_device *dev);
	int	(*suspend)(struct drm_device *dev);
	void	(*resume)(struct drm_device *dev);

	int	(*get)(struct nouveau_gpuobj *, struct nouveau_channel *,
		       u32 size, u32 align);
	void	(*put)(struct nouveau_gpuobj *);
	int	(*map)(struct nouveau_gpuobj *);
	void	(*unmap)(struct nouveau_gpuobj *);

	void	(*flush)(struct drm_device *);
};

struct nouveau_mc_engine {
	int  (*init)(struct drm_device *dev);
	void (*takedown)(struct drm_device *dev);
};

struct nouveau_timer_engine {
	int      (*init)(struct drm_device *dev);
	void     (*takedown)(struct drm_device *dev);
	uint64_t (*read)(struct drm_device *dev);
};

struct nouveau_fb_engine {
	int num_tiles;
	struct drm_mm tag_heap;
	void *priv;

	int  (*init)(struct drm_device *dev);
	void (*takedown)(struct drm_device *dev);

	void (*init_tile_region)(struct drm_device *dev, int i,
				 uint32_t addr, uint32_t size,
				 uint32_t pitch, uint32_t flags);
	void (*set_tile_region)(struct drm_device *dev, int i);
	void (*free_tile_region)(struct drm_device *dev, int i);
};

struct nouveau_fifo_engine {
	void *priv;
	int  channels;

	struct nouveau_gpuobj *playlist[2];
	int cur_playlist;

	int  (*init)(struct drm_device *);
	void (*takedown)(struct drm_device *);

	void (*disable)(struct drm_device *);
	void (*enable)(struct drm_device *);
	bool (*reassign)(struct drm_device *, bool enable);
	bool (*cache_pull)(struct drm_device *dev, bool enable);

	int  (*channel_id)(struct drm_device *);

	int  (*create_context)(struct nouveau_channel *);
	void (*destroy_context)(struct nouveau_channel *);
	int  (*load_context)(struct nouveau_channel *);
	int  (*unload_context)(struct drm_device *);
	void (*tlb_flush)(struct drm_device *dev);
};

struct nouveau_display_engine {
	void *priv;
	int (*early_init)(struct drm_device *);
	void (*late_takedown)(struct drm_device *);
	int (*create)(struct drm_device *);
	int (*init)(struct drm_device *);
	void (*destroy)(struct drm_device *);
};

struct nouveau_gpio_engine {
	void *priv;

	int  (*init)(struct drm_device *);
	void (*takedown)(struct drm_device *);

	int  (*get)(struct drm_device *, enum dcb_gpio_tag);
	int  (*set)(struct drm_device *, enum dcb_gpio_tag, int state);

	int  (*irq_register)(struct drm_device *, enum dcb_gpio_tag,
			     void (*)(void *, int), void *);
	void (*irq_unregister)(struct drm_device *, enum dcb_gpio_tag,
			       void (*)(void *, int), void *);
	bool (*irq_enable)(struct drm_device *, enum dcb_gpio_tag, bool on);
};

struct nouveau_pm_voltage_level {
	u32 voltage; /* microvolts */
	u8  vid;
};

struct nouveau_pm_voltage {
	bool supported;
	u8 version;
	u8 vid_mask;

	struct nouveau_pm_voltage_level *level;
	int nr_level;
};

struct nouveau_pm_memtiming {
	int id;
	u32 reg_0; /* 0x10f290 on Fermi, 0x100220 for older */
	u32 reg_1;
	u32 reg_2;
	u32 reg_3;
	u32 reg_4;
	u32 reg_5;
	u32 reg_6;
	u32 reg_7;
	u32 reg_8;
	/* To be written to 0x1002c0 */
	u8 CL;
	u8 WR;
};

struct nouveau_pm_tbl_header{
	u8 version;
	u8 header_len;
	u8 entry_cnt;
	u8 entry_len;
};

struct nouveau_pm_tbl_entry{
	u8 tWR;
	u8 tUNK_1;
	u8 tCL;
	u8 tRP;		/* Byte 3 */
	u8 empty_4;
	u8 tRAS;	/* Byte 5 */
	u8 empty_6;
	u8 tRFC;	/* Byte 7 */
	u8 empty_8;
	u8 tRC;		/* Byte 9 */
	u8 tUNK_10, tUNK_11, tUNK_12, tUNK_13, tUNK_14;
	u8 empty_15,empty_16,empty_17;
	u8 tUNK_18, tUNK_19, tUNK_20, tUNK_21;
};

/* nouveau_mem.c */
void nv30_mem_timing_entry(struct drm_device *dev, struct nouveau_pm_tbl_header *hdr,
							struct nouveau_pm_tbl_entry *e, uint8_t magic_number,
							struct nouveau_pm_memtiming *timing);

#define NOUVEAU_PM_MAX_LEVEL 8
struct nouveau_pm_level {
//FIXME	struct device_attribute dev_attr;
	char name[32];
	int id;

	u32 core;
	u32 memory;
	u32 shader;
	u32 rop;
	u32 copy;
	u32 daemon;
	u32 vdec;
	u32 unk05;	/* nv50:nva3, roughly.. */
	u32 unka0;	/* nva3:nvc0 */
	u32 hub01;	/* nvc0- */
	u32 hub06;	/* nvc0- */
	u32 hub07;	/* nvc0- */

	u32 volt_min; /* microvolts */
	u32 volt_max;
	u8  fanspeed;

	u16 memscript;
	struct nouveau_pm_memtiming *timing;
};

struct nouveau_pm_temp_sensor_constants {
	u16 offset_constant;
	s16 offset_mult;
	s16 offset_div;
	s16 slope_mult;
	s16 slope_div;
};

struct nouveau_pm_threshold_temp {
	s16 critical;
	s16 down_clock;
	s16 fan_boost;
};

struct nouveau_pm_memtimings {
	bool supported;
	struct nouveau_pm_memtiming *timing;
	int nr_timing;
};

struct nouveau_pm_fan {
	u32 min_duty;
	u32 max_duty;
	u32 pwm_freq;
};

struct nouveau_pm_engine {
	struct nouveau_pm_voltage voltage;
	struct nouveau_pm_level perflvl[NOUVEAU_PM_MAX_LEVEL];
	int nr_perflvl;
	struct nouveau_pm_memtimings memtimings;
	struct nouveau_pm_temp_sensor_constants sensor_constants;
	struct nouveau_pm_threshold_temp threshold_temp;
	struct nouveau_pm_fan fan;
	u32 pwm_divisor;

	struct nouveau_pm_level boot;
	struct nouveau_pm_level *cur;

	struct device *hwmon;
//FIXME	struct notifier_block acpi_nb;

	int (*clock_get)(struct drm_device *, u32 id);
	void *(*clock_pre)(struct drm_device *, struct nouveau_pm_level *,
			   u32 id, int khz);
	void (*clock_set)(struct drm_device *, void *);

	int  (*clocks_get)(struct drm_device *, struct nouveau_pm_level *);
	void *(*clocks_pre)(struct drm_device *, struct nouveau_pm_level *);
	void (*clocks_set)(struct drm_device *, void *);

	int (*voltage_get)(struct drm_device *);
	int (*voltage_set)(struct drm_device *, int voltage);
	int (*fanspeed_get)(struct drm_device *);
	int (*fanspeed_set)(struct drm_device *, int fanspeed);
	int (*temp_get)(struct drm_device *);
};

struct nouveau_vram_engine {
	struct nouveau_mm mm;

	int  (*init)(struct drm_device *);
	void (*takedown)(struct drm_device *dev);
	int  (*get)(struct drm_device *, u64, u32 align, u32 size_nc,
		    u32 type, struct nouveau_mem **);
	void (*put)(struct drm_device *, struct nouveau_mem **);

	bool (*flags_valid)(struct drm_device *, u32 tile_flags);
};

struct nouveau_engine {
	struct nouveau_instmem_engine instmem;
	struct nouveau_mc_engine      mc;
	struct nouveau_timer_engine   timer;
	struct nouveau_fb_engine      fb;
	struct nouveau_fifo_engine    fifo;
	struct nouveau_display_engine display;
	struct nouveau_gpio_engine    gpio;
	struct nouveau_pm_engine      pm;
	struct nouveau_vram_engine    vram;
};

struct nouveau_pll_vals {
	union {
		struct {
#ifdef __BIG_ENDIAN
			uint8_t N1, M1, N2, M2;
#else
			uint8_t M1, N1, M2, N2;
#endif
		};
		struct {
			uint16_t NM1, NM2;
		} __attribute__((packed));
	};
	int log2P;

	int refclk;
};

enum nv04_fp_display_regs {
	FP_DISPLAY_END,
	FP_TOTAL,
	FP_CRTC,
	FP_SYNC_START,
	FP_SYNC_END,
	FP_VALID_START,
	FP_VALID_END
};

struct nv04_crtc_reg {
	unsigned char MiscOutReg;
	uint8_t CRTC[0xa0];
	uint8_t CR58[0x10];
	uint8_t Sequencer[5];
	uint8_t Graphics[9];
	uint8_t Attribute[21];
	unsigned char DAC[768];

	/* PCRTC regs */
	uint32_t fb_start;
	uint32_t crtc_cfg;
	uint32_t cursor_cfg;
	uint32_t gpio_ext;
	uint32_t crtc_830;
	uint32_t crtc_834;
	uint32_t crtc_850;
	uint32_t crtc_eng_ctrl;

	/* PRAMDAC regs */
	uint32_t nv10_cursync;
	struct nouveau_pll_vals pllvals;
	uint32_t ramdac_gen_ctrl;
	uint32_t ramdac_630;
	uint32_t ramdac_634;
	uint32_t tv_setup;
	uint32_t tv_vtotal;
	uint32_t tv_vskew;
	uint32_t tv_vsync_delay;
	uint32_t tv_htotal;
	uint32_t tv_hskew;
	uint32_t tv_hsync_delay;
	uint32_t tv_hsync_delay2;
	uint32_t fp_horiz_regs[7];
	uint32_t fp_vert_regs[7];
	uint32_t dither;
	uint32_t fp_control;
	uint32_t dither_regs[6];
	uint32_t fp_debug_0;
	uint32_t fp_debug_1;
	uint32_t fp_debug_2;
	uint32_t fp_margin_color;
	uint32_t ramdac_8c0;
	uint32_t ramdac_a20;
	uint32_t ramdac_a24;
	uint32_t ramdac_a34;
	uint32_t ctv_regs[38];
};

struct nv04_output_reg {
	uint32_t output;
	int head;
};

struct nv04_mode_state {
	struct nv04_crtc_reg crtc_reg[2];
	uint32_t pllsel;
	uint32_t sel_clk;
};

enum nouveau_card_type {
	NV_04      = 0x00,
	NV_10      = 0x10,
	NV_20      = 0x20,
	NV_30      = 0x30,
	NV_40      = 0x40,
	NV_50      = 0x50,
	NV_C0      = 0xc0,
	NV_D0      = 0xd0
};

struct drm_nouveau_private {
	struct drm_device *dev;
	bool noaccel;

	/* the card type, takes NV_* as values */
	enum nouveau_card_type card_type;
	/* exact chipset, derived from NV_PMC_BOOT_0 */
	int chipset;
	int flags;
	u32 crystal;

	void __iomem *mmio;

	spinlock_t ramin_lock;
	void __iomem *ramin;
	u32 ramin_size;
	u32 ramin_base;
	bool ramin_available;
	struct drm_mm ramin_heap;
	struct nouveau_exec_engine *eng[NVOBJ_ENGINE_NR];
	struct list_head gpuobj_list;
	struct list_head classes;

	struct nouveau_bo *vga_ram;

	/* interrupt handling */
	void (*irq_handler[32])(struct drm_device *);
	bool msi_enabled;

	struct list_head vbl_waiting;

	struct {
		struct drm_global_reference mem_global_ref;
		struct ttm_bo_global_ref bo_global_ref;
		struct ttm_bo_device bdev;
		atomic_t validate_sequence;
	} ttm;

	struct {
		spinlock_t lock;
		struct drm_mm heap;
		struct nouveau_bo *bo;
	} fence;

	struct {
		spinlock_t lock;
		struct nouveau_channel *ptr[NOUVEAU_MAX_CHANNEL_NR];
	} channels;

	struct nouveau_engine engine;
	struct nouveau_channel *channel;

	/* For PFIFO and PGRAPH. */
	spinlock_t context_switch_lock;

	/* VM/PRAMIN flush, legacy PRAMIN aperture */
	spinlock_t vm_lock;

	/* RAMIN configuration, RAMFC, RAMHT and RAMRO offsets */
	struct nouveau_ramht  *ramht;
	struct nouveau_gpuobj *ramfc;
	struct nouveau_gpuobj *ramro;

	uint32_t ramin_rsvd_vram;

	struct {
		enum {
			NOUVEAU_GART_NONE = 0,
			NOUVEAU_GART_AGP,	/* AGP */
			NOUVEAU_GART_PDMA,	/* paged dma object */
			NOUVEAU_GART_HW		/* on-chip gart/vm */
		} type;
		uint64_t aper_base;
		uint64_t aper_size;
		uint64_t aper_free;

		struct ttm_backend_func *func;

		struct {
			struct page *page;
			dma_addr_t   addr;
		} dummy;

		struct nouveau_gpuobj *sg_ctxdma;
	} gart_info;

	/* nv10-nv40 tiling regions */
	struct {
		struct nouveau_tile_reg reg[NOUVEAU_MAX_TILE_NR];
		spinlock_t lock;
	} tile;

	/* VRAM/fb configuration */
	uint64_t vram_size;
	uint64_t vram_sys_base;

	uint64_t fb_available_size;
	uint64_t fb_mappable_pages;
	uint64_t fb_aper_free;
	int fb_mtrr;

	/* BAR control (NV50-) */
	struct nouveau_vm *bar1_vm;
	struct nouveau_vm *bar3_vm;

	/* G8x/G9x virtual address space */
	struct nouveau_vm *chan_vm;

	struct nvbios vbios;

	struct nv04_mode_state mode_reg;
	struct nv04_mode_state saved_reg;
	uint32_t saved_vga_font[4][16384];
	uint32_t crtc_owner;
	uint32_t dac_users[4];

	struct backlight_device *backlight;

	struct {
		struct dentry *channel_root;
	} debugfs;

	struct nouveau_fbdev *nfbdev;
	struct apertures_struct *apertures;
};

static inline struct drm_nouveau_private *
nouveau_private(struct drm_device *dev)
{
	return dev->dev_private;
}

static inline struct drm_nouveau_private *
nouveau_bdev(struct ttm_bo_device *bd)
{
	return container_of(bd, struct drm_nouveau_private, ttm.bdev);
}

static inline int
nouveau_bo_ref(struct nouveau_bo *ref, struct nouveau_bo **pnvbo)
{
	struct nouveau_bo *prev;

	if (!pnvbo)
		return -EINVAL;
	prev = *pnvbo;

	*pnvbo = ref ? nouveau_bo(ttm_bo_reference(&ref->bo)) : NULL;
	if (prev) {
		struct ttm_buffer_object *bo = &prev->bo;

		ttm_bo_unref(&bo);
	}

	return 0;
}

/* nouveau_drv.c */
extern int nouveau_modeset;
extern int nouveau_agpmode;
extern int nouveau_duallink;
extern int nouveau_uscript_lvds;
extern int nouveau_uscript_tmds;
extern int nouveau_vram_pushbuf;
extern int nouveau_vram_notify;
extern int nouveau_fbpercrtc;
extern int nouveau_tv_disable;
extern char *nouveau_tv_norm;
extern int nouveau_reg_debug;
extern char *nouveau_vbios;
extern int nouveau_ignorelid;
extern int nouveau_nofbaccel;
extern int nouveau_noaccel;
extern int nouveau_force_post;
extern int nouveau_override_conntype;
extern char *nouveau_perflvl;
extern int nouveau_perflvl_wr;
extern int nouveau_msi;
extern int nouveau_ctxfw;

#if !defined(__AROS__)
extern int nouveau_pci_suspend(struct pci_dev *pdev, pm_message_t pm_state);
extern int nouveau_pci_resume(struct pci_dev *pdev);
#endif

/* nouveau_state.c */
extern int  nouveau_open(struct drm_device *, struct drm_file *);
extern void nouveau_preclose(struct drm_device *dev, struct drm_file *);
extern void nouveau_postclose(struct drm_device *, struct drm_file *);
extern int  nouveau_load(struct drm_device *, unsigned long flags);
extern int  nouveau_firstopen(struct drm_device *);
extern void nouveau_lastclose(struct drm_device *);
extern int  nouveau_unload(struct drm_device *);
extern int  nouveau_ioctl_getparam(struct drm_device *, void *data,
				   struct drm_file *);
extern int  nouveau_ioctl_setparam(struct drm_device *, void *data,
				   struct drm_file *);
extern bool nouveau_wait_eq(struct drm_device *, uint64_t timeout,
			    uint32_t reg, uint32_t mask, uint32_t val);
extern bool nouveau_wait_ne(struct drm_device *, uint64_t timeout,
			    uint32_t reg, uint32_t mask, uint32_t val);
extern bool nouveau_wait_cb(struct drm_device *, u64 timeout,
			    bool (*cond)(void *), void *);
extern bool nouveau_wait_for_idle(struct drm_device *);
extern int  nouveau_card_init(struct drm_device *);

/* nouveau_mem.c */
extern int  nouveau_mem_vram_init(struct drm_device *);
extern void nouveau_mem_vram_fini(struct drm_device *);
extern int  nouveau_mem_gart_init(struct drm_device *);
extern void nouveau_mem_gart_fini(struct drm_device *);
extern int  nouveau_mem_init_agp(struct drm_device *);
extern int  nouveau_mem_reset_agp(struct drm_device *);
extern void nouveau_mem_close(struct drm_device *);
extern int  nouveau_mem_detect(struct drm_device *);
extern bool nouveau_mem_flags_valid(struct drm_device *, u32 tile_flags);
extern struct nouveau_tile_reg *nv10_mem_set_tiling(
	struct drm_device *dev, uint32_t addr, uint32_t size,
	uint32_t pitch, uint32_t flags);
extern void nv10_mem_put_tile_region(struct drm_device *dev,
				     struct nouveau_tile_reg *tile,
				     struct nouveau_fence *fence);
extern const struct ttm_mem_type_manager_func nouveau_vram_manager;
extern const struct ttm_mem_type_manager_func nouveau_gart_manager;

/* nouveau_notifier.c */
extern int  nouveau_notifier_init_channel(struct nouveau_channel *);
extern void nouveau_notifier_takedown_channel(struct nouveau_channel *);
extern int  nouveau_notifier_alloc(struct nouveau_channel *, uint32_t handle,
				   int cout, uint32_t start, uint32_t end,
				   uint32_t *offset);
extern int  nouveau_notifier_offset(struct nouveau_gpuobj *, uint32_t *);
extern int  nouveau_ioctl_notifier_alloc(struct drm_device *, void *data,
					 struct drm_file *);
extern int  nouveau_ioctl_notifier_free(struct drm_device *, void *data,
					struct drm_file *);

/* nouveau_channel.c */
extern struct drm_ioctl_desc nouveau_ioctls[];
extern int nouveau_max_ioctl;
extern void nouveau_channel_cleanup(struct drm_device *, struct drm_file *);
extern int  nouveau_channel_alloc(struct drm_device *dev,
				  struct nouveau_channel **chan,
				  struct drm_file *file_priv,
				  uint32_t fb_ctxdma, uint32_t tt_ctxdma);
extern struct nouveau_channel *
nouveau_channel_get_unlocked(struct nouveau_channel *);
extern struct nouveau_channel *
nouveau_channel_get(struct drm_file *, int id);
extern void nouveau_channel_put_unlocked(struct nouveau_channel **);
extern void nouveau_channel_put(struct nouveau_channel **);
extern void nouveau_channel_ref(struct nouveau_channel *chan,
				struct nouveau_channel **pchan);
extern void nouveau_channel_idle(struct nouveau_channel *chan);

/* nouveau_object.c */
#define NVOBJ_ENGINE_ADD(d, e, p) do {                                         \
	struct drm_nouveau_private *dev_priv = (d)->dev_private;               \
	dev_priv->eng[NVOBJ_ENGINE_##e] = (p);                                 \
} while (0)

#define NVOBJ_ENGINE_DEL(d, e) do {                                            \
	struct drm_nouveau_private *dev_priv = (d)->dev_private;               \
	dev_priv->eng[NVOBJ_ENGINE_##e] = NULL;                                \
} while (0)

#define NVOBJ_CLASS(d, c, e) do {                                              \
	int ret = nouveau_gpuobj_class_new((d), (c), NVOBJ_ENGINE_##e);        \
	if (ret)                                                               \
		return ret;                                                    \
} while (0)

#define NVOBJ_MTHD(d, c, m, e) do {                                            \
	int ret = nouveau_gpuobj_mthd_new((d), (c), (m), (e));                 \
	if (ret)                                                               \
		return ret;                                                    \
} while (0)

extern int  nouveau_gpuobj_early_init(struct drm_device *);
extern int  nouveau_gpuobj_init(struct drm_device *);
extern void nouveau_gpuobj_takedown(struct drm_device *);
extern int  nouveau_gpuobj_suspend(struct drm_device *dev);
extern void nouveau_gpuobj_resume(struct drm_device *dev);
extern int  nouveau_gpuobj_class_new(struct drm_device *, u32 class, u32 eng);
extern int  nouveau_gpuobj_mthd_new(struct drm_device *, u32 class, u32 mthd,
				    int (*exec)(struct nouveau_channel *,
						u32 class, u32 mthd, u32 data));
extern int  nouveau_gpuobj_mthd_call(struct nouveau_channel *, u32, u32, u32);
extern int  nouveau_gpuobj_mthd_call2(struct drm_device *, int, u32, u32, u32);
extern int nouveau_gpuobj_channel_init(struct nouveau_channel *,
				       uint32_t vram_h, uint32_t tt_h);
extern void nouveau_gpuobj_channel_takedown(struct nouveau_channel *);
extern int nouveau_gpuobj_new(struct drm_device *, struct nouveau_channel *,
			      uint32_t size, int align, uint32_t flags,
			      struct nouveau_gpuobj **);
extern void nouveau_gpuobj_ref(struct nouveau_gpuobj *,
			       struct nouveau_gpuobj **);
extern int nouveau_gpuobj_new_fake(struct drm_device *, u32 pinst, u64 vinst,
				   u32 size, u32 flags,
				   struct nouveau_gpuobj **);
extern int nouveau_gpuobj_dma_new(struct nouveau_channel *, int class,
				  uint64_t offset, uint64_t size, int access,
				  int target, struct nouveau_gpuobj **);
extern int nouveau_gpuobj_gr_new(struct nouveau_channel *, u32 handle, int class);
extern int nv50_gpuobj_dma_new(struct nouveau_channel *, int class, u64 base,
			       u64 size, int target, int access, u32 type,
			       u32 comp, struct nouveau_gpuobj **pobj);
extern void nv50_gpuobj_dma_init(struct nouveau_gpuobj *, u32 offset,
				 int class, u64 base, u64 size, int target,
				 int access, u32 type, u32 comp);
extern int nouveau_ioctl_grobj_alloc(struct drm_device *, void *data,
				     struct drm_file *);
extern int nouveau_ioctl_gpuobj_free(struct drm_device *, void *data,
				     struct drm_file *);

/* nouveau_irq.c */
extern int         nouveau_irq_init(struct drm_device *);
extern void        nouveau_irq_fini(struct drm_device *);
extern irqreturn_t nouveau_irq_handler(DRM_IRQ_ARGS);
extern void        nouveau_irq_register(struct drm_device *, int status_bit,
					void (*)(struct drm_device *));
extern void        nouveau_irq_unregister(struct drm_device *, int status_bit);
extern void        nouveau_irq_preinstall(struct drm_device *);
extern int         nouveau_irq_postinstall(struct drm_device *);
extern void        nouveau_irq_uninstall(struct drm_device *);

/* nouveau_sgdma.c */
extern int nouveau_sgdma_init(struct drm_device *);
extern void nouveau_sgdma_takedown(struct drm_device *);
extern uint32_t nouveau_sgdma_get_physical(struct drm_device *,
					   uint32_t offset);
extern struct ttm_backend *nouveau_sgdma_init_ttm(struct drm_device *);

/* nouveau_debugfs.c */
#if defined(CONFIG_DRM_NOUVEAU_DEBUG)
extern int  nouveau_debugfs_init(struct drm_minor *);
extern void nouveau_debugfs_takedown(struct drm_minor *);
extern int  nouveau_debugfs_channel_init(struct nouveau_channel *);
extern void nouveau_debugfs_channel_fini(struct nouveau_channel *);
#else
static inline int
nouveau_debugfs_init(struct drm_minor *minor)
{
	return 0;
}

static inline void nouveau_debugfs_takedown(struct drm_minor *minor)
{
}

static inline int
nouveau_debugfs_channel_init(struct nouveau_channel *chan)
{
	return 0;
}

static inline void
nouveau_debugfs_channel_fini(struct nouveau_channel *chan)
{
}
#endif

/* nouveau_dma.c */
extern void nouveau_dma_pre_init(struct nouveau_channel *);
extern int  nouveau_dma_init(struct nouveau_channel *);
extern int  nouveau_dma_wait(struct nouveau_channel *, int slots, int size);

/* nouveau_acpi.c */
#define ROM_BIOS_PAGE 4096
#if defined(CONFIG_ACPI)
void nouveau_register_dsm_handler(void);
void nouveau_unregister_dsm_handler(void);
int nouveau_acpi_get_bios_chunk(uint8_t *bios, int offset, int len);
bool nouveau_acpi_rom_supported(struct pci_dev *pdev);
int nouveau_acpi_edid(struct drm_device *, struct drm_connector *);
#else
static inline void nouveau_register_dsm_handler(void) {}
static inline void nouveau_unregister_dsm_handler(void) {}
//FIXME static inline bool nouveau_acpi_rom_supported(struct pci_dev *pdev) { return false; }
static inline int nouveau_acpi_get_bios_chunk(uint8_t *bios, int offset, int len) { return -EINVAL; }
static inline int nouveau_acpi_edid(struct drm_device *dev, struct drm_connector *connector) { return -EINVAL; }
#endif

/* nouveau_backlight.c */
#ifdef CONFIG_DRM_NOUVEAU_BACKLIGHT
extern int nouveau_backlight_init(struct drm_device *);
extern void nouveau_backlight_exit(struct drm_device *);
#else
static inline int nouveau_backlight_init(struct drm_device *dev)
{
	return 0;
}

static inline void nouveau_backlight_exit(struct drm_device *dev) { }
#endif

/* nouveau_bios.c */
extern int nouveau_bios_init(struct drm_device *);
extern void nouveau_bios_takedown(struct drm_device *dev);
extern int nouveau_run_vbios_init(struct drm_device *);
extern void nouveau_bios_run_init_table(struct drm_device *, uint16_t table,
					struct dcb_entry *, int crtc);
extern void nouveau_bios_init_exec(struct drm_device *, uint16_t table);
extern struct dcb_gpio_entry *nouveau_bios_gpio_entry(struct drm_device *,
						      enum dcb_gpio_tag);
extern struct dcb_connector_table_entry *
nouveau_bios_connector_entry(struct drm_device *, int index);
extern u32 get_pll_register(struct drm_device *, enum pll_types);
extern int get_pll_limits(struct drm_device *, uint32_t limit_match,
			  struct pll_lims *);
extern int nouveau_bios_run_display_table(struct drm_device *, u16 id, int clk,
					  struct dcb_entry *, int crtc);
extern bool nouveau_bios_fp_mode(struct drm_device *, struct drm_display_mode *);
extern uint8_t *nouveau_bios_embedded_edid(struct drm_device *);
extern int nouveau_bios_parse_lvds_table(struct drm_device *, int pxclk,
					 bool *dl, bool *if_is_24bit);
extern int run_tmds_table(struct drm_device *, struct dcb_entry *,
			  int head, int pxclk);
extern int call_lvds_script(struct drm_device *, struct dcb_entry *, int head,
			    enum LVDS_script, int pxclk);
bool bios_encoder_match(struct dcb_entry *, u32 hash);

/* nouveau_ttm.c */
int nouveau_ttm_global_init(struct drm_nouveau_private *);
void nouveau_ttm_global_release(struct drm_nouveau_private *);
//FIXME:TTM int nouveau_ttm_mmap(struct file *, struct vm_area_struct *);

/* nouveau_dp.c */
int nouveau_dp_auxch(struct nouveau_i2c_chan *auxch, int cmd, int addr,
		     uint8_t *data, int data_nr);
bool nouveau_dp_detect(struct drm_encoder *);
bool nouveau_dp_link_train(struct drm_encoder *, u32 datarate);
void nouveau_dp_tu_update(struct drm_device *, int, int, u32, u32);
u8 *nouveau_dp_bios_data(struct drm_device *, struct dcb_entry *, u8 **);

/* nv04_fb.c */
extern int  nv04_fb_init(struct drm_device *);
extern void nv04_fb_takedown(struct drm_device *);

/* nv10_fb.c */
extern int  nv10_fb_init(struct drm_device *);
extern void nv10_fb_takedown(struct drm_device *);
extern void nv10_fb_init_tile_region(struct drm_device *dev, int i,
				     uint32_t addr, uint32_t size,
				     uint32_t pitch, uint32_t flags);
extern void nv10_fb_set_tile_region(struct drm_device *dev, int i);
extern void nv10_fb_free_tile_region(struct drm_device *dev, int i);

/* nv30_fb.c */
extern int  nv30_fb_init(struct drm_device *);
extern void nv30_fb_takedown(struct drm_device *);
extern void nv30_fb_init_tile_region(struct drm_device *dev, int i,
				     uint32_t addr, uint32_t size,
				     uint32_t pitch, uint32_t flags);
extern void nv30_fb_free_tile_region(struct drm_device *dev, int i);

/* nv40_fb.c */
extern int  nv40_fb_init(struct drm_device *);
extern void nv40_fb_takedown(struct drm_device *);
extern void nv40_fb_set_tile_region(struct drm_device *dev, int i);

/* nv50_fb.c */
extern int  nv50_fb_init(struct drm_device *);
extern void nv50_fb_takedown(struct drm_device *);
extern void nv50_fb_vm_trap(struct drm_device *, int display);

/* nvc0_fb.c */
extern int  nvc0_fb_init(struct drm_device *);
extern void nvc0_fb_takedown(struct drm_device *);

/* nv04_fifo.c */
extern int  nv04_fifo_init(struct drm_device *);
extern void nv04_fifo_fini(struct drm_device *);
extern void nv04_fifo_disable(struct drm_device *);
extern void nv04_fifo_enable(struct drm_device *);
extern bool nv04_fifo_reassign(struct drm_device *, bool);
extern bool nv04_fifo_cache_pull(struct drm_device *, bool);
extern int  nv04_fifo_channel_id(struct drm_device *);
extern int  nv04_fifo_create_context(struct nouveau_channel *);
extern void nv04_fifo_destroy_context(struct nouveau_channel *);
extern int  nv04_fifo_load_context(struct nouveau_channel *);
extern int  nv04_fifo_unload_context(struct drm_device *);
extern void nv04_fifo_isr(struct drm_device *);

/* nv10_fifo.c */
extern int  nv10_fifo_init(struct drm_device *);
extern int  nv10_fifo_channel_id(struct drm_device *);
extern int  nv10_fifo_create_context(struct nouveau_channel *);
extern int  nv10_fifo_load_context(struct nouveau_channel *);
extern int  nv10_fifo_unload_context(struct drm_device *);

/* nv40_fifo.c */
extern int  nv40_fifo_init(struct drm_device *);
extern int  nv40_fifo_create_context(struct nouveau_channel *);
extern int  nv40_fifo_load_context(struct nouveau_channel *);
extern int  nv40_fifo_unload_context(struct drm_device *);

/* nv50_fifo.c */
extern int  nv50_fifo_init(struct drm_device *);
extern void nv50_fifo_takedown(struct drm_device *);
extern int  nv50_fifo_channel_id(struct drm_device *);
extern int  nv50_fifo_create_context(struct nouveau_channel *);
extern void nv50_fifo_destroy_context(struct nouveau_channel *);
extern int  nv50_fifo_load_context(struct nouveau_channel *);
extern int  nv50_fifo_unload_context(struct drm_device *);
extern void nv50_fifo_tlb_flush(struct drm_device *dev);

/* nvc0_fifo.c */
extern int  nvc0_fifo_init(struct drm_device *);
extern void nvc0_fifo_takedown(struct drm_device *);
extern void nvc0_fifo_disable(struct drm_device *);
extern void nvc0_fifo_enable(struct drm_device *);
extern bool nvc0_fifo_reassign(struct drm_device *, bool);
extern bool nvc0_fifo_cache_pull(struct drm_device *, bool);
extern int  nvc0_fifo_channel_id(struct drm_device *);
extern int  nvc0_fifo_create_context(struct nouveau_channel *);
extern void nvc0_fifo_destroy_context(struct nouveau_channel *);
extern int  nvc0_fifo_load_context(struct nouveau_channel *);
extern int  nvc0_fifo_unload_context(struct drm_device *);

/* nv04_graph.c */
extern int  nv04_graph_create(struct drm_device *);
extern int  nv04_graph_object_new(struct nouveau_channel *, int, u32, u16);
extern int  nv04_graph_mthd_page_flip(struct nouveau_channel *chan,
				      u32 class, u32 mthd, u32 data);
extern struct nouveau_bitfield nv04_graph_nsource[];

/* nv10_graph.c */
extern int  nv10_graph_create(struct drm_device *);
extern struct nouveau_channel *nv10_graph_channel(struct drm_device *);
extern struct nouveau_bitfield nv10_graph_intr[];
extern struct nouveau_bitfield nv10_graph_nstatus[];

/* nv20_graph.c */
extern int  nv20_graph_create(struct drm_device *);

/* nv40_graph.c */
extern int  nv40_graph_create(struct drm_device *);
extern void nv40_grctx_init(struct nouveau_grctx *);

/* nv50_graph.c */
extern int  nv50_graph_create(struct drm_device *);
extern int  nv50_grctx_init(struct nouveau_grctx *);
extern struct nouveau_enum nv50_data_error_names[];
extern int  nv50_graph_isr_chid(struct drm_device *dev, u64 inst);

/* nvc0_graph.c */
extern int  nvc0_graph_create(struct drm_device *);
extern int  nvc0_graph_isr_chid(struct drm_device *dev, u64 inst);

/* nv84_crypt.c */
extern int  nv84_crypt_create(struct drm_device *);

/* nv98_crypt.c */
extern int  nv98_crypt_create(struct drm_device *dev);

/* nva3_copy.c */
extern int  nva3_copy_create(struct drm_device *dev);

/* nvc0_copy.c */
extern int  nvc0_copy_create(struct drm_device *dev, int engine);

/* nv31_mpeg.c */
extern int  nv31_mpeg_create(struct drm_device *dev);

/* nv50_mpeg.c */
extern int  nv50_mpeg_create(struct drm_device *dev);

/* nv84_bsp.c */
/* nv98_bsp.c */
extern int  nv84_bsp_create(struct drm_device *dev);

/* nv84_vp.c */
/* nv98_vp.c */
extern int  nv84_vp_create(struct drm_device *dev);

/* nv98_ppp.c */
extern int  nv98_ppp_create(struct drm_device *dev);

/* nv04_instmem.c */
extern int  nv04_instmem_init(struct drm_device *);
extern void nv04_instmem_takedown(struct drm_device *);
extern int  nv04_instmem_suspend(struct drm_device *);
extern void nv04_instmem_resume(struct drm_device *);
extern int  nv04_instmem_get(struct nouveau_gpuobj *, struct nouveau_channel *,
			     u32 size, u32 align);
extern void nv04_instmem_put(struct nouveau_gpuobj *);
extern int  nv04_instmem_map(struct nouveau_gpuobj *);
extern void nv04_instmem_unmap(struct nouveau_gpuobj *);
extern void nv04_instmem_flush(struct drm_device *);

/* nv50_instmem.c */
extern int  nv50_instmem_init(struct drm_device *);
extern void nv50_instmem_takedown(struct drm_device *);
extern int  nv50_instmem_suspend(struct drm_device *);
extern void nv50_instmem_resume(struct drm_device *);
extern int  nv50_instmem_get(struct nouveau_gpuobj *, struct nouveau_channel *,
			     u32 size, u32 align);
extern void nv50_instmem_put(struct nouveau_gpuobj *);
extern int  nv50_instmem_map(struct nouveau_gpuobj *);
extern void nv50_instmem_unmap(struct nouveau_gpuobj *);
extern void nv50_instmem_flush(struct drm_device *);
extern void nv84_instmem_flush(struct drm_device *);

/* nvc0_instmem.c */
extern int  nvc0_instmem_init(struct drm_device *);
extern void nvc0_instmem_takedown(struct drm_device *);
extern int  nvc0_instmem_suspend(struct drm_device *);
extern void nvc0_instmem_resume(struct drm_device *);

/* nv04_mc.c */
extern int  nv04_mc_init(struct drm_device *);
extern void nv04_mc_takedown(struct drm_device *);

/* nv40_mc.c */
extern int  nv40_mc_init(struct drm_device *);
extern void nv40_mc_takedown(struct drm_device *);

/* nv50_mc.c */
extern int  nv50_mc_init(struct drm_device *);
extern void nv50_mc_takedown(struct drm_device *);

/* nv04_timer.c */
extern int  nv04_timer_init(struct drm_device *);
extern uint64_t nv04_timer_read(struct drm_device *);
extern void nv04_timer_takedown(struct drm_device *);

extern long nouveau_compat_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg);

/* nv04_dac.c */
extern int nv04_dac_create(struct drm_connector *, struct dcb_entry *);
extern uint32_t nv17_dac_sample_load(struct drm_encoder *encoder);
extern int nv04_dac_output_offset(struct drm_encoder *encoder);
extern void nv04_dac_update_dacclk(struct drm_encoder *encoder, bool enable);
extern bool nv04_dac_in_use(struct drm_encoder *encoder);

/* nv04_dfp.c */
extern int nv04_dfp_create(struct drm_connector *, struct dcb_entry *);
extern int nv04_dfp_get_bound_head(struct drm_device *dev, struct dcb_entry *dcbent);
extern void nv04_dfp_bind_head(struct drm_device *dev, struct dcb_entry *dcbent,
			       int head, bool dl);
extern void nv04_dfp_disable(struct drm_device *dev, int head);
extern void nv04_dfp_update_fp_control(struct drm_encoder *encoder, int mode);

/* nv04_tv.c */
extern int nv04_tv_identify(struct drm_device *dev, int i2c_index);
extern int nv04_tv_create(struct drm_connector *, struct dcb_entry *);

/* nv17_tv.c */
extern int nv17_tv_create(struct drm_connector *, struct dcb_entry *);

/* nv04_display.c */
extern int nv04_display_early_init(struct drm_device *);
extern void nv04_display_late_takedown(struct drm_device *);
extern int nv04_display_create(struct drm_device *);
extern int nv04_display_init(struct drm_device *);
extern void nv04_display_destroy(struct drm_device *);

/* nvd0_display.c */
extern int nvd0_display_create(struct drm_device *);
extern int nvd0_display_init(struct drm_device *);
extern void nvd0_display_destroy(struct drm_device *);

/* nv04_crtc.c */
extern int nv04_crtc_create(struct drm_device *, int index);

/* nouveau_bo.c */
extern struct ttm_bo_driver nouveau_bo_driver;
extern int nouveau_bo_new(struct drm_device *, int size, int align,
			  uint32_t flags, uint32_t tile_mode,
			  uint32_t tile_flags, struct nouveau_bo **);
extern int nouveau_bo_pin(struct nouveau_bo *, uint32_t flags);
extern int nouveau_bo_unpin(struct nouveau_bo *);
extern int nouveau_bo_map(struct nouveau_bo *);
extern void nouveau_bo_unmap(struct nouveau_bo *);
extern void nouveau_bo_placement_set(struct nouveau_bo *, uint32_t type,
				     uint32_t busy);
extern u16 nouveau_bo_rd16(struct nouveau_bo *nvbo, unsigned index);
extern void nouveau_bo_wr16(struct nouveau_bo *nvbo, unsigned index, u16 val);
extern u32 nouveau_bo_rd32(struct nouveau_bo *nvbo, unsigned index);
extern void nouveau_bo_wr32(struct nouveau_bo *nvbo, unsigned index, u32 val);
extern void nouveau_bo_fence(struct nouveau_bo *, struct nouveau_fence *);
extern int nouveau_bo_validate(struct nouveau_bo *, bool interruptible,
			       bool no_wait_reserve, bool no_wait_gpu);

extern struct nouveau_vma *
nouveau_bo_vma_find(struct nouveau_bo *, struct nouveau_vm *);
extern int  nouveau_bo_vma_add(struct nouveau_bo *, struct nouveau_vm *,
			       struct nouveau_vma *);
extern void nouveau_bo_vma_del(struct nouveau_bo *, struct nouveau_vma *);

/* nouveau_fence.c */
struct nouveau_fence;
extern int nouveau_fence_init(struct drm_device *);
extern void nouveau_fence_fini(struct drm_device *);
extern int nouveau_fence_channel_init(struct nouveau_channel *);
extern void nouveau_fence_channel_fini(struct nouveau_channel *);
extern void nouveau_fence_update(struct nouveau_channel *);
extern int nouveau_fence_new(struct nouveau_channel *, struct nouveau_fence **,
			     bool emit);
extern int nouveau_fence_emit(struct nouveau_fence *);
extern void nouveau_fence_work(struct nouveau_fence *fence,
			       void (*work)(void *priv, bool signalled),
			       void *priv);
struct nouveau_channel *nouveau_fence_channel(struct nouveau_fence *);

extern bool __nouveau_fence_signalled(void *obj, void *arg);
extern int __nouveau_fence_wait(void *obj, void *arg, bool lazy, bool intr);
extern int __nouveau_fence_flush(void *obj, void *arg);
extern void __nouveau_fence_unref(void **obj);
extern void *__nouveau_fence_ref(void *obj);

static inline bool nouveau_fence_signalled(struct nouveau_fence *obj)
{
	return __nouveau_fence_signalled(obj, NULL);
}
static inline int
nouveau_fence_wait(struct nouveau_fence *obj, bool lazy, bool intr)
{
	return __nouveau_fence_wait(obj, NULL, lazy, intr);
}
extern int nouveau_fence_sync(struct nouveau_fence *, struct nouveau_channel *);
static inline int nouveau_fence_flush(struct nouveau_fence *obj)
{
	return __nouveau_fence_flush(obj, NULL);
}
static inline void nouveau_fence_unref(struct nouveau_fence **obj)
{
	__nouveau_fence_unref((void **)obj);
}
static inline struct nouveau_fence *nouveau_fence_ref(struct nouveau_fence *obj)
{
	return __nouveau_fence_ref(obj);
}

/* nouveau_gem.c */
extern int nouveau_gem_new(struct drm_device *, int size, int align,
			   uint32_t domain, uint32_t tile_mode,
			   uint32_t tile_flags, struct nouveau_bo **);
extern int nouveau_gem_object_new(struct drm_gem_object *);
extern void nouveau_gem_object_del(struct drm_gem_object *);
extern int nouveau_gem_object_open(struct drm_gem_object *, struct drm_file *);
extern void nouveau_gem_object_close(struct drm_gem_object *,
				     struct drm_file *);
extern int nouveau_gem_ioctl_new(struct drm_device *, void *,
				 struct drm_file *);
extern int nouveau_gem_ioctl_pushbuf(struct drm_device *, void *,
				     struct drm_file *);
extern int nouveau_gem_ioctl_cpu_prep(struct drm_device *, void *,
				      struct drm_file *);
extern int nouveau_gem_ioctl_cpu_fini(struct drm_device *, void *,
				      struct drm_file *);
extern int nouveau_gem_ioctl_info(struct drm_device *, void *,
				  struct drm_file *);

/* nouveau_display.c */
int nouveau_vblank_enable(struct drm_device *dev, int crtc);
void nouveau_vblank_disable(struct drm_device *dev, int crtc);
int nouveau_crtc_page_flip(struct drm_crtc *crtc, struct drm_framebuffer *fb,
			   struct drm_pending_vblank_event *event);
int nouveau_finish_page_flip(struct nouveau_channel *,
			     struct nouveau_page_flip_state *);

/* nv10_gpio.c */
int nv10_gpio_get(struct drm_device *dev, enum dcb_gpio_tag tag);
int nv10_gpio_set(struct drm_device *dev, enum dcb_gpio_tag tag, int state);

/* nv50_gpio.c */
int nv50_gpio_init(struct drm_device *dev);
void nv50_gpio_fini(struct drm_device *dev);
int nv50_gpio_get(struct drm_device *dev, enum dcb_gpio_tag tag);
int nv50_gpio_set(struct drm_device *dev, enum dcb_gpio_tag tag, int state);
int nvd0_gpio_get(struct drm_device *dev, enum dcb_gpio_tag tag);
int nvd0_gpio_set(struct drm_device *dev, enum dcb_gpio_tag tag, int state);
int  nv50_gpio_irq_register(struct drm_device *, enum dcb_gpio_tag,
			    void (*)(void *, int), void *);
void nv50_gpio_irq_unregister(struct drm_device *, enum dcb_gpio_tag,
			      void (*)(void *, int), void *);
bool nv50_gpio_irq_enable(struct drm_device *, enum dcb_gpio_tag, bool on);

/* nv50_calc. */
int nv50_calc_pll(struct drm_device *, struct pll_lims *, int clk,
		  int *N1, int *M1, int *N2, int *M2, int *P);
int nva3_calc_pll(struct drm_device *, struct pll_lims *,
		  int clk, int *N, int *fN, int *M, int *P);

/* nv50_gpio.c */
int nv50_gpio_get(struct drm_device *dev, enum dcb_gpio_tag tag);
int nv50_gpio_set(struct drm_device *dev, enum dcb_gpio_tag tag, int state);

#ifndef ioread32_native
#ifdef __BIG_ENDIAN
#define ioread16_native ioread16be
#define iowrite16_native iowrite16be
#define ioread32_native  ioread32be
#define iowrite32_native iowrite32be
#else /* def __BIG_ENDIAN */
#define ioread16_native ioread16
#define iowrite16_native iowrite16
#define ioread32_native  ioread32
#define iowrite32_native iowrite32
#endif /* def __BIG_ENDIAN else */
#endif /* !ioread32_native */

/* channel control reg access */
static inline u32 nvchan_rd32(struct nouveau_channel *chan, unsigned reg)
{
	return ioread32_native(chan->user + reg);
}

static inline void nvchan_wr32(struct nouveau_channel *chan,
							unsigned reg, u32 val)
{
	iowrite32_native(val, chan->user + reg);
}

/* register access */
static inline u32 nv_rd32(struct drm_device *dev, unsigned reg)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	return ioread32_native(dev_priv->mmio + reg);
}

static inline void nv_wr32(struct drm_device *dev, unsigned reg, u32 val)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	iowrite32_native(val, dev_priv->mmio + reg);
}

static inline u32 nv_mask(struct drm_device *dev, u32 reg, u32 mask, u32 val)
{
	u32 tmp = nv_rd32(dev, reg);
	nv_wr32(dev, reg, (tmp & ~mask) | val);
	return tmp;
}

static inline u8 nv_rd08(struct drm_device *dev, unsigned reg)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	return ioread8(dev_priv->mmio + reg);
}

static inline void nv_wr08(struct drm_device *dev, unsigned reg, u8 val)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	iowrite8(val, dev_priv->mmio + reg);
}

#define nv_wait(dev, reg, mask, val) \
	nouveau_wait_eq(dev, 2000000000ULL, (reg), (mask), (val))
#define nv_wait_ne(dev, reg, mask, val) \
	nouveau_wait_ne(dev, 2000000000ULL, (reg), (mask), (val))
#define nv_wait_cb(dev, func, data) \
	nouveau_wait_cb(dev, 2000000000ULL, (func), (data))

/* PRAMIN access */
static inline u32 nv_ri32(struct drm_device *dev, unsigned offset)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	return ioread32_native(dev_priv->ramin + offset);
}

static inline void nv_wi32(struct drm_device *dev, unsigned offset, u32 val)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	iowrite32_native(val, dev_priv->ramin + offset);
}

/* object access */
extern u32 nv_ro32(struct nouveau_gpuobj *, u32 offset);
extern void nv_wo32(struct nouveau_gpuobj *, u32 offset, u32 val);

/*
 * Logging
 * Argument d is (struct drm_device *).
 */

#define NV_PRINTK(level, d, fmt, arg...) \
	printk(level "[" DRM_NAME "] " DRIVER_NAME " %s: " fmt, \
					pci_name(d->pdev), ##arg)
#ifndef NV_DEBUG_NOTRACE
#define NV_DEBUG(d, fmt, arg...) do {                                          \
	if (drm_debug & DRM_UT_DRIVER) {                                       \
		NV_PRINTK(KERN_DEBUG, d, "%s:%d - " fmt, __func__,             \
			  __LINE__, ##arg);                                    \
	}                                                                      \
} while (0)
#define NV_DEBUG_KMS(d, fmt, arg...) do {                                      \
	if (drm_debug & DRM_UT_KMS) {                                          \
		NV_PRINTK(KERN_DEBUG, d, "%s:%d - " fmt, __func__,             \
			  __LINE__, ##arg);                                    \
	}                                                                      \
} while (0)
#else
#define NV_DEBUG(d, fmt, arg...) do {                                          \
	if (drm_debug & DRM_UT_DRIVER)                                         \
		NV_PRINTK(KERN_DEBUG, d, fmt, ##arg);                          \
} while (0)
#define NV_DEBUG_KMS(d, fmt, arg...) do {                                      \
	if (drm_debug & DRM_UT_KMS)                                            \
		NV_PRINTK(KERN_DEBUG, d, fmt, ##arg);                          \
} while (0)
#endif
#define NV_ERROR(d, fmt, arg...) NV_PRINTK(KERN_ERR, d, fmt, ##arg)
#define NV_INFO(d, fmt, arg...) NV_PRINTK(KERN_INFO, d, fmt, ##arg)
#define NV_TRACEWARN(d, fmt, arg...) NV_PRINTK(KERN_NOTICE, d, fmt, ##arg)
#define NV_TRACE(d, fmt, arg...) NV_PRINTK(KERN_INFO, d, fmt, ##arg)
#define NV_WARN(d, fmt, arg...) NV_PRINTK(KERN_WARNING, d, fmt, ##arg)

/* nouveau_reg_debug bitmask */
enum {
	NOUVEAU_REG_DEBUG_MC             = 0x1,
	NOUVEAU_REG_DEBUG_VIDEO          = 0x2,
	NOUVEAU_REG_DEBUG_FB             = 0x4,
	NOUVEAU_REG_DEBUG_EXTDEV         = 0x8,
	NOUVEAU_REG_DEBUG_CRTC           = 0x10,
	NOUVEAU_REG_DEBUG_RAMDAC         = 0x20,
	NOUVEAU_REG_DEBUG_VGACRTC        = 0x40,
	NOUVEAU_REG_DEBUG_RMVIO          = 0x80,
	NOUVEAU_REG_DEBUG_VGAATTR        = 0x100,
	NOUVEAU_REG_DEBUG_EVO            = 0x200,
	NOUVEAU_REG_DEBUG_AUXCH          = 0x400
};

#define NV_REG_DEBUG(type, dev, fmt, arg...) do { \
	if (nouveau_reg_debug & NOUVEAU_REG_DEBUG_##type) \
		NV_PRINTK(KERN_DEBUG, dev, "%s: " fmt, __func__, ##arg); \
} while (0)

static inline bool
nv_two_heads(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	const int impl = dev->pci_device & 0x0ff0;

	if (dev_priv->card_type >= NV_10 && impl != 0x0100 &&
	    impl != 0x0150 && impl != 0x01a0 && impl != 0x0200)
		return true;

	return false;
}

static inline bool
nv_gf4_disp_arch(struct drm_device *dev)
{
	return nv_two_heads(dev) && (dev->pci_device & 0x0ff0) != 0x0110;
}

static inline bool
nv_two_reg_pll(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	const int impl = dev->pci_device & 0x0ff0;

	if (impl == 0x0310 || impl == 0x0340 || dev_priv->card_type >= NV_40)
		return true;
	return false;
}

static inline bool
nv_match_device(struct drm_device *dev, unsigned device,
		unsigned sub_vendor, unsigned sub_device)
{
#if !defined(__AROS__)
	return dev->pdev->device == device &&
		dev->pdev->subsystem_vendor == sub_vendor &&
		dev->pdev->subsystem_device == sub_device;
#else
    return dev->driver->ProductID == device &&
        dev->driver->SubSystemVendorID == sub_vendor &&
        dev->driver->SubSystemProductID == sub_device;
#endif
}

static inline void *
nv_engine(struct drm_device *dev, int engine)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	return (void *)dev_priv->eng[engine];
}

/* returns 1 if device is one of the nv4x using the 0x4497 object class,
 * helpful to determine a number of other hardware features
 */
static inline int
nv44_graph_class(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	if ((dev_priv->chipset & 0xf0) == 0x60)
		return 1;

	return !(0x0baf & (1 << (dev_priv->chipset & 0x0f)));
}

/* memory type/access flags, do not match hardware values */
#define NV_MEM_ACCESS_RO  1
#define NV_MEM_ACCESS_WO  2
#define NV_MEM_ACCESS_RW (NV_MEM_ACCESS_RO | NV_MEM_ACCESS_WO)
#define NV_MEM_ACCESS_SYS 4
#define NV_MEM_ACCESS_VM  8

#define NV_MEM_TARGET_VRAM        0
#define NV_MEM_TARGET_PCI         1
#define NV_MEM_TARGET_PCI_NOSNOOP 2
#define NV_MEM_TARGET_VM          3
#define NV_MEM_TARGET_GART        4

#define NV_MEM_TYPE_VM 0x7f
#define NV_MEM_COMP_VM 0x03

/* NV_SW object class */
#define NV_SW                                                        0x0000506e
#define NV_SW_DMA_SEMAPHORE                                          0x00000060
#define NV_SW_SEMAPHORE_OFFSET                                       0x00000064
#define NV_SW_SEMAPHORE_ACQUIRE                                      0x00000068
#define NV_SW_SEMAPHORE_RELEASE                                      0x0000006c
#define NV_SW_YIELD                                                  0x00000080
#define NV_SW_DMA_VBLSEM                                             0x0000018c
#define NV_SW_VBLSEM_OFFSET                                          0x00000400
#define NV_SW_VBLSEM_RELEASE_VALUE                                   0x00000404
#define NV_SW_VBLSEM_RELEASE                                         0x00000408
#define NV_SW_PAGE_FLIP                                              0x00000500

#endif /* __NOUVEAU_DRV_H__ */
