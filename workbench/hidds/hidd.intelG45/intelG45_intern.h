/*
 * intelG45_intern.h
 *
 *  Created on: Apr 14, 2010
 *      Author: misc
 */

#ifndef INTELG45_INTERN_H_
#define INTELG45_INTERN_H_

#include <exec/nodes.h>
#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <exec/ports.h>

#include <devices/timer.h>

#include <oop/oop.h>

#include <hidd/graphics.h>
#include <hidd/pci.h>
#include <hidd/i2c.h>

#include <stdint.h>

#define CLID_Hidd_Gfx_IntelG45		"IntelGMA"
#define IID_Hidd_Gfx_IntelG45		"IIntelGMA"
#define IID_Hidd_IntelG45BitMap		"IIntelBitMap"

extern OOP_AttrBase HiddPCIDeviceAttrBase;
extern OOP_AttrBase HiddBitMapAttrBase;
extern OOP_AttrBase HiddPixFmtAttrBase;
extern OOP_AttrBase HiddSyncAttrBase;
extern OOP_AttrBase HiddGfxAttrBase;
extern OOP_AttrBase HiddIntelG45BitMapAttrBase;
extern OOP_AttrBase HiddI2CAttrBase;
extern OOP_AttrBase HiddI2CDeviceAttrBase;
extern OOP_AttrBase __IHidd_PlanarBM;

typedef struct {
	uint8_t		h_min;		/* Minimal horizontal frequency in kHz */
	uint8_t		h_max;		/* Maximal horizontal frequency in kHz */
	uint8_t		v_min;		/* Minimal vertical frequency in Hz */
	uint8_t		v_max;		/* Maximal vertical frequency in Hz */
	uint8_t		pixel_max;	/* Maximal pixelclock/10 in MHz (multiply by 10 to get MHz value) */
} GMA_MonitorSpec_t;

struct g45chip {
	char *			Framebuffer;
	uint32_t		fb_size;
	char *			MMIO;
	uint32_t *		GATT;
};

struct g45staticdata {
	struct SignalSemaphore	HWLock;
	struct SignalSemaphore	MultiBMLock;
	struct MsgPort			MsgPort;
	struct timerequest		tr;

	void *					MemPool;

	struct g45chip			Card;

	HIDDT_DPMSLevel			dpms;

	OOP_Class *				IntelG45Class;
	OOP_Class *				IntelI2C;
	OOP_Class *				OnBMClass;
	OOP_Class *				OffBMClass;

	OOP_Object *			PCIObject;
	OOP_Object *			PCIDevice;
	OOP_Object * 			GMAObject;


	OOP_AttrBase			pciAttrBase;
	OOP_AttrBase			atiBitMapAttrBase;
	OOP_AttrBase			bitMapAttrBase;
	OOP_AttrBase			pixFmtAttrBase;
	OOP_AttrBase			gfxAttrBase;
	OOP_AttrBase			syncAttrBase;
	OOP_AttrBase			i2cAttrBase;
	OOP_AttrBase			i2cDeviceAttrBase;
	OOP_AttrBase			planarAttrBase;
};

struct intelg45base {
	struct Library			g45_LibNode;
	struct g45staticdata	g45_sd;
};

typedef struct {
	uint32_t	dpll_ctrl;
	uint32_t	pipeconf;
	uint32_t	dispconf;

	uint32_t	htotal;
	uint32_t	hblank;
	uint32_t	hsync;
	uint32_t	vtotal;
	uint32_t	vblank;
	uint32_t	vsync;
} GMAState_t;

typedef struct {
    struct SignalSemaphore bmLock;

    OOP_Object *bitmap;    // BitMap OOP Object
    intptr_t	framebuffer;    // Points to pixel data
    uint16_t	width;      // Bitmap width
    uint16_t	height;     // Bitmap height
    uint16_t	pitch;      // BytesPerRow aligned
    uint8_t		depth;      // Bitmap depth
    uint8_t		bpp;        // BytesPerPixel
    uint8_t		onbm;       // is onbitmap?
    uint8_t		fbgfx;      // is framebuffer in gfx memory
    uint64_t	usecount;   // counts BitMap accesses
    GMAState_t *state;
} GMABitMap_t;

#define BASE(lib) ((struct intelg45base*)(lib))

#define SD(cl) (&BASE(cl->UserData)->g45_sd)

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define METHOD_NAME(base, id, name) \
  base ## __ ## id ## __ ## name

#define METHOD_NAME_S(base, id, name) \
  # base "__" # id "__" # name

#define LOCK_HW          { ObtainSemaphore(&sd->HWLock); }
#define UNLOCK_HW        { ReleaseSemaphore(&sd->HWLock); }

#define LOCK_BITMAP      { ObtainSemaphore(&bm->bmLock); }
#define UNLOCK_BITMAP        { ReleaseSemaphore(&bm->bmLock); }

#define LOCK_BITMAP_BM(bm)   { ObtainSemaphore(&(bm)->bmLock); }
#define UNLOCK_BITMAP_BM(bm) { ReleaseSemaphore(&(bm)->bmLock); }

#define LOCK_MULTI_BITMAP    { ObtainSemaphore(&sd->MultiBMLock); }
#define UNLOCK_MULTI_BITMAP  { ReleaseSemaphore(&sd->MultiBMLock); }


void G45_InitMode(struct g45staticdata *sd, GMAState_t *state,
		uint16_t width, uint16_t height, uint8_t depth, uint32_t pixelclock, intptr_t framebuffer,
        uint16_t hdisp, uint16_t vdisp, uint16_t hstart, uint16_t hend, uint16_t htotal,
        uint16_t vstart, uint16_t vend, uint16_t vtotal, uint32_t flags);

#endif /* INTELG45_INTERN_H_ */
