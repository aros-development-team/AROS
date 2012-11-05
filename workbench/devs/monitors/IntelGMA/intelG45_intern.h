/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef INTELG45_INTERN_H_
#define INTELG45_INTERN_H_

#include <exec/nodes.h>
#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <devices/timer.h>
#include <oop/oop.h>
#include <hidd/graphics.h>
#include <hidd/pci.h>
#include <hidd/i2c.h>

#include <stdint.h>

#define CLID_Hidd_Gfx_IntelG45		"IntelGMA"
#define IID_Hidd_Gfx_IntelG45		"IIntelGMA"
#define IID_Hidd_IntelG45BitMap		"IIntelBitMap"

extern OOP_AttrBase HiddGMABitMapAttrBase;

struct Sync{
	uint16_t width;
	uint16_t height;
	uint8_t depth;
	uint32_t pixelclock;
	intptr_t framebuffer;
	uint16_t hdisp;
	uint16_t vdisp;
	uint16_t hstart;
	uint16_t hend;
	uint16_t htotal;
	uint16_t vstart;
	uint16_t vend;
	uint16_t vtotal;
	uint32_t flags;
};

typedef struct {
	uint32_t	fp;				// G45_FPA0
	uint32_t	dpll;			// G45_DPLL_A
	uint32_t	dpll_md_reg;	// G45_DPLL_A_MD
	uint32_t	pipeconf;		// G45_PIPEACONF
	uint32_t	pipesrc;		// G45_PIPEASRC
	uint32_t	dspcntr;		// G45_DSPACNTR
	uint32_t	dspsurf;		// G45_DSPASURF
	uint32_t	dsplinoff;		// G45_DSPALINOFF
	uint32_t	dspstride;		// G45_DSPASTRIDE
	uint32_t	htotal;			// G45_HTOTAL_A
	uint32_t	hblank;			// G45_HBLANK_A
	uint32_t	hsync;			// G45_HSYNC_A
	uint32_t	vtotal;			// G45_VTOTAL_A
	uint32_t	vblank;			// G45_VBLANK_A
	uint32_t	vsync;			// G45_VSYNC_A
	uint32_t	adpa;			// G45_ADPA
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

    BOOL    displayable;    /* Can bitmap be displayed on screen */

    /* Information connected with display */
    OOP_Object  *compositing;   /* Compositing object used by bitmap */
    LONG        xoffset;        /* Offset to bitmap point that is displayed as (0,0) on screen */
    LONG        yoffset;        /* Offset to bitmap point that is displayed as (0,0) on screen */
    ULONG       fbid;           /* Contains ID under which bitmap 
                                              is registered as framebuffer or 
                                              0 otherwise */
} GMABitMap_t;

typedef struct {
	uint8_t		h_min;		/* Minimal horizontal frequency in kHz */
	uint8_t		h_max;		/* Maximal horizontal frequency in kHz */
	uint8_t		v_min;		/* Minimal vertical frequency in Hz */
	uint8_t		v_max;		/* Maximal vertical frequency in Hz */
	uint8_t		pixel_max;	/* Maximal pixelclock/10 in MHz (multiply by 10 to get MHz value) */
} GMA_MonitorSpec_t;

struct g45chip {
	char *			Framebuffer;
	uint32_t		Framebuffer_size;
	char *			MMIO;
	uint32_t *		GATT;
	uint32_t		GATT_size;
	uint32_t		Stolen_size;
};

struct g45data
{
    /* TODO: Move object data here from staticdata */
    ULONG empty;
};

struct i2cdata
{
    ULONG empty;
};

struct g45staticdata
{
    void *MemPool;
    BOOL forced;
    BOOL force_gallium;
    ULONG memsize;

    /* The rest should be moved to object data */
    struct SignalSemaphore	HWLock;
    struct SignalSemaphore	MultiBMLock;
    struct MsgPort		MsgPort;
    struct timerequest		*tr;

    struct MemHeader	    CardMem;


	struct g45chip			Card;
	uint16_t				ProductID;

	HIDDT_DPMSLevel			dpms;

	GMABitMap_t *			Engine2DOwner;

	GMABitMap_t *			VisibleBitmap;

	GMAState_t *			initialState;
	intptr_t				initialBitMap;

	intptr_t				RingBuffer;
	uint32_t				RingBufferSize;
	uint32_t				RingBufferTail;
	char *					RingBufferPhys;
	char					RingActive;

	uint32_t				DDCPort;

	OOP_Class *				IntelG45Class;
	OOP_Class *				IntelI2C;
	OOP_Class *				BMClass;
    OOP_Class *				compositingclass;
    OOP_Class *				galliumclass;
    
	OOP_Object          *compositing;
    
	OOP_Object *			PCIObject;
	OOP_Object *			PCIDevice;
	OOP_Object * 			GMAObject;

	intptr_t				ScratchArea;
	intptr_t				AttachedMemory;
	intptr_t				AttachedSize;

	volatile uint32_t *	HardwareStatusPage;

	intptr_t				CursorImage;
	intptr_t				CursorBase;
    BOOL					CursorVisible;

    OOP_MethodID    mid_ReadLong;
    OOP_MethodID    mid_CopyMemBox8;
    OOP_MethodID    mid_CopyMemBox16;
    OOP_MethodID    mid_CopyMemBox32;
    OOP_MethodID    mid_PutMem32Image8;
    OOP_MethodID    mid_PutMem32Image16;
    OOP_MethodID    mid_GetMem32Image8;
    OOP_MethodID    mid_GetMem32Image16;
    OOP_MethodID    mid_GetImage;
    OOP_MethodID    mid_Clear;
    OOP_MethodID    mid_PutMemTemplate8;
    OOP_MethodID    mid_PutMemTemplate16;
    OOP_MethodID    mid_PutMemTemplate32;
    OOP_MethodID    mid_PutMemPattern8;
    OOP_MethodID    mid_PutMemPattern16;
    OOP_MethodID    mid_PutMemPattern32;
    OOP_MethodID    mid_CopyLUTMemBox16;
    OOP_MethodID    mid_CopyLUTMemBox32;

    OOP_MethodID    mid_BitMapPositionChanged;
    OOP_MethodID    mid_BitMapRectChanged;
    OOP_MethodID    mid_ValidateBitMapPositionChange;

	ULONG pipe;
	struct Sync lvds_fixed;
	LONG pointerx;
	LONG pointery;
};

enum {
    aoHidd_GMABitMap_Drawable,
    aoHidd_BitMap_IntelG45_CompositingHidd,
    num_Hidd_GMABitMap_Attrs
};

#define aHidd_GMABitMap_Drawable (HiddGMABitMapAttrBase + aoHidd_GMABitMap_Drawable)
#define aHidd_BitMap_IntelG45_CompositingHidd    (HiddGMABitMapAttrBase + aoHidd_BitMap_IntelG45_CompositingHidd)

#define IS_BM_ATTR(attr, idx) (((idx)=(attr)-HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_GMABM_ATTR(attr, idx) (((idx)=(attr)-HiddGMABitMapAttrBase) < num_Hidd_GMABitMap_Attrs)
#define HIDD_BM_OBJ(bitmap)     (*(OOP_Object **)&((bitmap)->Planes[0]))

#define SD(cl) ((struct g45staticdata *)cl->UserData)

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

extern const struct OOP_InterfaceDescr INTELG45_ifdescr[];
extern const struct OOP_InterfaceDescr GMABM_ifdescr[];
extern const struct OOP_InterfaceDescr INTELI2C_ifdescr[];

int G45_Init(struct g45staticdata *sd);

intptr_t G45_VirtualToPhysical(struct g45staticdata *sd, intptr_t virtual);
void G45_AttachMemory(struct g45staticdata *sd, intptr_t physical, intptr_t virtual, intptr_t length);
void G45_AttachCacheableMemory(struct g45staticdata *sd, intptr_t physical, intptr_t virtual, intptr_t length);
void G45_DetachMemory(struct g45staticdata *sd, intptr_t virtual, intptr_t length);

void G45_InitMode(struct g45staticdata *sd, GMAState_t *state,
		uint16_t width, uint16_t height, uint8_t depth, uint32_t pixelclock, intptr_t framebuffer,
        uint16_t hdisp, uint16_t vdisp, uint16_t hstart, uint16_t hend, uint16_t htotal,
        uint16_t vstart, uint16_t vend, uint16_t vtotal, uint32_t flags);
void G45_LoadState(struct g45staticdata *sd, GMAState_t *state);
IPTR AllocBitmapArea(struct g45staticdata *sd, ULONG width, ULONG height, ULONG bpp);
VOID FreeBitmapArea(struct g45staticdata *sd, IPTR bmp, ULONG width, ULONG height, ULONG bpp);
VOID delay_ms(struct g45staticdata *sd, uint32_t msec);
VOID delay_us(struct g45staticdata *sd, uint32_t usec);

BOOL adpa_Enabled(struct g45staticdata *sd);
BOOL lvds_Enabled(struct g45staticdata *sd);
void GetSync(struct g45staticdata *sd,struct Sync *sync,ULONG pipe);
void UpdateCursor(struct g45staticdata *sd);
void SetCursorPosition(struct g45staticdata *sd,LONG x,LONG y);

BOOL HIDD_INTELG45_SetFramebuffer(OOP_Object * bm);
BOOL HIDD_INTELG45_SwitchToVideoMode(OOP_Object * bm);
BOOL copybox3d_supported();
BOOL copybox3d( GMABitMap_t *bm_dst, GMABitMap_t *bm_src,
               ULONG dst_x,ULONG dst_y,ULONG dst_width, ULONG dst_height,
               ULONG src_x,ULONG src_y,ULONG src_width, ULONG src_height );

//#define GALLIUM_SIMULATION             
#endif /* INTELG45_INTERN_H_ */
