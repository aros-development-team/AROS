/*
 * intelG45_class.c
 *
 *  Created on: Apr 15, 2010
 *      Author: misc
 */

#define DEBUG 0
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <utility/tagitem.h>

#include <hidd/graphics.h>
#include <hidd/i2c.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <stdio.h>
#include <stdint.h>



#include LC_LIBDEFS_FILE

#include "intelG45_intern.h"
#include "intelG45_regs.h"

#define sd ((struct g45staticdata*)SD(cl))

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#define HiddPCIDeviceAttrBase   (sd->pciAttrBase)
#define HiddGMABitMapAttrBase   (sd->gmaBitMapAttrBase)
#define HiddBitMapAttrBase  (sd->bitMapAttrBase)
#define HiddPixFmtAttrBase  (sd->pixFmtAttrBase)
#define HiddGfxAttrBase     (sd->gfxAttrBase)
#define HiddSyncAttrBase    (sd->syncAttrBase)
#define HiddI2CAttrBase         (sd->i2cAttrBase)
#define HiddI2CDeviceAttrBase   (sd->i2cDeviceAttrBase)

#define DEBUG_POINTER 1

#ifdef DEBUG_POINTER

#define PRINT_POINTER(image, xsize, xmax, ymax)		\
bug("[ATI] Pointer data:\n");			\
{							\
    ULONG *pix = (ULONG *)image;			\
    ULONG x, y;						\
							\
    for (y = 0; y < ymax; y++) {			\
        for (x = 0; x < xmax; x++)			\
	    bug("0x%08X ", pix[x]);			\
	bug("\n");					\
	pix += xsize;					\
    }							\
}

#else
#define PRINT_POINTER(image, xsize, xmax, ymax)
#endif

/* Definitions used in CVT formula */
#define M 600
#define C 40
#define K 128
#define J 20
#define DUTY_CYCLE(period) \
    (((C - J) / 2 + J) * 1000 - (M / 2 * (period) / 1000))
#define MIN_DUTY_CYCLE 20 /* % */
#define MIN_V_PORCH 3 /* lines */
#define MIN_V_PORCH_TIME 550 /* us */
#define CLOCK_STEP 250000 /* Hz */

typedef struct {
	uint16_t width;
	uint16_t height;

	uint16_t hstart;
	uint16_t hend;
	uint16_t htotal;
	uint16_t vstart;
	uint16_t vend;
	uint16_t vtotal;

	uint32_t pixel;
} sync_t;

/* Partial implementation of CVT formula */
void calcTimings(int x, int y, int vfreq, sync_t *sync)
{
    ULONG h_period, h_freq, h_total, h_blank, h_front, h_sync, h_back,
        v_freq, v_total, v_front, v_sync, v_back, duty_cycle, pixel_freq;

    sync->width = x;
    sync->height = y;

    /* Get horizontal period in microseconds */
    h_period = (1000000000 / vfreq - MIN_V_PORCH_TIME * 1000)
        / (y + MIN_V_PORCH);

    /* Vertical front porch is fixed */
    v_front = MIN_V_PORCH;

    /* Use aspect ratio to determine V-sync lines */
    if (x == y * 4 / 3)
        v_sync = 4;
    else if (x == y * 16 / 9)
        v_sync = 5;
    else if (x == y * 16 / 10)
        v_sync = 6;
    else if (x == y * 5 / 4)
        v_sync = 7;
    else if (x == y * 15 / 9)
        v_sync = 7;
    else
        v_sync = 10;

    /* Get vertical back porch */
    v_back = MIN_V_PORCH_TIME * 1000 / h_period + 1;
    if (v_back < MIN_V_PORCH)
        v_back = MIN_V_PORCH;
    v_back -= v_sync;

    /* Get total lines per frame */
    v_total = y + v_front + v_sync + v_back;

    /* Get horizontal blanking pixels */
    duty_cycle = DUTY_CYCLE(h_period);
    if (duty_cycle < MIN_DUTY_CYCLE)
        duty_cycle = MIN_DUTY_CYCLE;

    h_blank = 10 * x * duty_cycle / (100000 - duty_cycle);
    h_blank /= 2 * 8 * 10;
    h_blank = h_blank * (2 * 8);

    /* Get total pixels in a line */
    h_total = x + h_blank;

    /* Calculate frequencies for each pixel, line and field */
    h_freq = 1000000000 / h_period;
    pixel_freq = h_freq * h_total / CLOCK_STEP * CLOCK_STEP;
    h_freq = pixel_freq / h_total;
    v_freq = 100 * h_freq / v_total;

    /* Back porch is half of H-blank */
    h_back = h_blank / 2;

    /* H-sync is a fixed percentage of H-total */
    h_sync = h_total / 100 * 8;

    /* Front porch is whatever's left */
    h_front = h_blank - h_sync - h_back;

    /* Fill in VBE timings structure */
    sync->htotal = h_total;
    sync->hstart = x + h_front;
    sync->hend = h_total - h_back;
    sync->vtotal = v_total;
    sync->vstart = y + v_front;
    sync->vend = v_total - v_back;
    sync->pixel = pixel_freq;
}

#define MAKE_SYNC(name,clock,hdisp,hstart,hend,htotal,vdisp,vstart,vend,vtotal,descr)	\
    struct TagItem sync_ ## name[]={ \
        { aHidd_Sync_PixelClock,  clock*1000 }, \
        { aHidd_Sync_HDisp,       hdisp }, \
        { aHidd_Sync_HSyncStart,  hstart }, \
        { aHidd_Sync_HSyncEnd,    hend }, \
        { aHidd_Sync_HTotal,      htotal }, \
        { aHidd_Sync_VDisp,       vdisp }, \
        { aHidd_Sync_VSyncStart,  vstart }, \
        { aHidd_Sync_VSyncEnd,    vend }, \
        { aHidd_Sync_VTotal,      vtotal }, \
        { aHidd_Sync_Description, (IPTR)descr }, \
        { TAG_DONE, 0UL }}

#define PUSH_TAG(ptr, tag, data) do { (*(ptr))->ti_Tag = (tag); (*(ptr))->ti_Data= (IPTR)(data); (*ptr)++; } while(0)

void createSync(OOP_Class *cl, int x, int y, int refresh, struct TagItem **tagsptr, struct TagItem **poolptr)
{
	sync_t sync;
	char *description = AllocVecPooled(sd->MemPool, 30);
	snprintf(description, 29, "GMA: %dx%d@%d", x, y, refresh);
	calcTimings(x, y, refresh, &sync);

	D(bug("[GMA]  %s %d  %d %d %d %d  %d %d %d %d  -HSync +VSync\n", description+5,
			sync.pixel / 1000, sync.width, sync.hstart, sync.hend, sync.htotal,
			sync.height, sync.vstart, sync.vend, sync.vtotal));

	PUSH_TAG(tagsptr, aHidd_Gfx_SyncTags, *poolptr);

	PUSH_TAG(poolptr, aHidd_Sync_Description, description);
	PUSH_TAG(poolptr, aHidd_Sync_PixelClock, sync.pixel);

	PUSH_TAG(poolptr, aHidd_Sync_HDisp, sync.width);
	PUSH_TAG(poolptr, aHidd_Sync_HSyncStart, sync.hstart);
	PUSH_TAG(poolptr, aHidd_Sync_HSyncEnd, sync.hend);
	PUSH_TAG(poolptr, aHidd_Sync_HTotal, sync.htotal);

	PUSH_TAG(poolptr, aHidd_Sync_VDisp, sync.height);
	PUSH_TAG(poolptr, aHidd_Sync_VSyncStart, sync.vstart);
	PUSH_TAG(poolptr, aHidd_Sync_VSyncEnd, sync.vend);
	PUSH_TAG(poolptr, aHidd_Sync_VTotal, sync.vtotal);

	PUSH_TAG(poolptr, aHidd_Sync_Flags, vHidd_Sync_VSyncPlus);
	PUSH_TAG(poolptr, TAG_DONE, 0);
}

static int G45_parse_ddc(OOP_Class *cl, struct TagItem **tagsptr, struct TagItem *poolptr, OOP_Object *obj)
{
	struct pHidd_I2CDevice_WriteRead msg;
	uint8_t edid[128];
	char wb[2] = {0, 0};
	int i;
	uint8_t chksum = 0;
	char *description;

	D(bug("[GMA] Trying to parse the DDC data\n"));

	msg.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_WriteRead);
	msg.readBuffer = &edid[0];
	msg.readLength = 128;
	msg.writeBuffer = &wb[0];
	msg.writeLength = 1;

	OOP_DoMethod(obj, &msg.mID);

	for (i=0; i < 128; i++)
		chksum += edid[i];

	if (chksum == 0 &&
			edid[0] == 0 && edid[1] == 0xff && edid[2] == 0xff && edid[3] == 0xff &&
			edid[4] == 0xff && edid[5] == 0xff && edid[6] == 0xff && edid[7] == 0)
	{
		D(bug("[GMA] Valid EDID%d.%d header\n", edid[18], edid[19]));

		D(bug("[GMA] Established timing: %02x %02x %02x\n", edid[35], edid[36], edid[37]));
		if (edid[35] & 0x80)
			createSync(cl, 720, 400, 70, tagsptr, &poolptr);
		if (edid[35] & 0x40)
			createSync(cl, 720, 400, 88, tagsptr, &poolptr);
		if (edid[35] & 0x20)
			createSync(cl, 640, 480, 60, tagsptr, &poolptr);
		if (edid[35] & 0x10)
			createSync(cl, 640, 480, 67, tagsptr, &poolptr);
		if (edid[35] & 0x08)
			createSync(cl, 640, 480, 72, tagsptr, &poolptr);
		if (edid[35] & 0x04)
			createSync(cl, 640, 480, 75, tagsptr, &poolptr);
		if (edid[35] & 0x02)
			createSync(cl, 800, 600, 56, tagsptr, &poolptr);
		if (edid[35] & 0x01)
			createSync(cl, 800, 600, 60, tagsptr, &poolptr);
		if (edid[36] & 0x80)
			createSync(cl, 800, 600, 72, tagsptr, &poolptr);
		if (edid[36] & 0x40)
			createSync(cl, 800, 600, 75, tagsptr, &poolptr);
		if (edid[36] & 0x20)
			createSync(cl, 832, 624, 75, tagsptr, &poolptr);
		if (edid[36] & 0x08)
			createSync(cl, 1024, 768, 60, tagsptr, &poolptr);
		if (edid[36] & 0x04)
			createSync(cl, 1024, 768, 70, tagsptr, &poolptr);
		if (edid[36] & 0x02)
			createSync(cl, 1024, 768, 75, tagsptr, &poolptr);
		if (edid[36] & 0x01)
			createSync(cl, 1280, 1024, 75, tagsptr, &poolptr);

		//createSync(cl, 736, 566, 60, tagsptr, &poolptr);

		D(bug("[GMA] Standard timing identification:\n"));

		for (i=38; i < 54; i+=2)
		{
			int w, h = 0, freq;
			w = edid[i] * 8 + 248;
			if (w > 400)
			{
				freq = (edid[i+1] & 0x3f) + 60;
				switch (edid[i+1] >> 6)
				{
				case 0: /* 16:10 */
					h = (w * 10) / 16;
					break;
				case 1: /* 4:3 */
					h = (w * 3) / 4;
					break;
				case 2: /* 5:4 */
					h = (w * 4) / 5;
					break;
				case 3: /* 16:9 */
					h = (w * 9) / 16;
					break;
				}
				createSync(cl, w, h, freq, tagsptr, &poolptr);
			}
		}

		for (i=54; i < 126; i+= 18)
		{
			if (edid[i] || edid[i+1])
			{
				int ha, hb, va, vb, hsync_o, hsync_w, vsync_o, vsync_w, pixel;
				ha = edid[i+2];
				hb = edid[i+3];
				ha |= (edid[i+4] >> 4) << 8;
				hb |= (edid[i+4] & 0x0f) << 8;
				va = edid[i+5];
				vb = edid[i+6];
				va |= (edid[i+7] >> 4) << 8;
				vb |= (edid[i+7] & 0x0f) << 8;
				hsync_o = edid[i+8];
				hsync_w = edid[i+9];
				vsync_o = edid[i+10] >> 4;
				vsync_w = edid[i+10] & 0x0f;
				hsync_o |= 0x300 & ((edid[i+11] >> 6) << 8);
				hsync_w |= 0x300 & ((edid[i+11] >> 4) << 8);
				vsync_o |= 0x300 & ((edid[i+11] >> 2) << 8);
				vsync_w |= 0x300 & ((edid[i+11]) << 8);

				pixel = (edid[i] | (edid[i+1] << 8));

				D(bug("[GMA] Modeline: "));
				D(bug("%dx%d Pixel: %d0 kHz %d %d %d %d   %d %d %d %d\n", ha, va, pixel,
						ha, hb, hsync_o, hsync_w,
						va, vb, vsync_o, vsync_w));

				AllocVecPooled(sd->MemPool, 30);
				snprintf(description, 29, "GMA: %dx%d@%d N", ha, va, ((pixel*10 / (uint32_t)(ha+hb)) * 1000) / ((uint32_t)(va+vb)));

				PUSH_TAG(tagsptr, aHidd_Gfx_SyncTags, poolptr);

				PUSH_TAG(&poolptr, aHidd_Sync_Description, description);
				PUSH_TAG(&poolptr, aHidd_Sync_PixelClock, pixel*10000);

				PUSH_TAG(&poolptr, aHidd_Sync_HDisp, ha);
				PUSH_TAG(&poolptr, aHidd_Sync_HSyncStart, ha+hsync_o);
				PUSH_TAG(&poolptr, aHidd_Sync_HSyncEnd, ha+hsync_o+hsync_w);
				PUSH_TAG(&poolptr, aHidd_Sync_HTotal, ha+hb);

				PUSH_TAG(&poolptr, aHidd_Sync_VDisp, va);
				PUSH_TAG(&poolptr, aHidd_Sync_VSyncStart, va+vsync_o);
				PUSH_TAG(&poolptr, aHidd_Sync_VSyncEnd, va+vsync_o+vsync_w);
				PUSH_TAG(&poolptr, aHidd_Sync_VTotal, va+vb);

				PUSH_TAG(&poolptr, aHidd_Sync_Flags, vHidd_Sync_VSyncPlus);
				PUSH_TAG(&poolptr, TAG_DONE, 0);

			}
			else
			{
				switch (edid[i+3])
				{
				case 0xff:
					D(bug("[GMA] Monitor Serial: %s\n", &edid[i+5]));
					break;
				case 0xfe:
					D(bug("[GMA] ASCII String: %s\n", &edid[i+5]));
					break;
				case 0xfc:
					D(bug("[GMA] Monitor Name: %s\n", &edid[i+5]));
					break;
				case 0xfd:
					if (edid[i+10] == 0 && edid[i+11] == 0x0a)
					{
						D(bug("[GMA] Monitor limits: H: %d - %d kHz, V: %d - %d Hz, PixelClock: %dMHz\n",
								edid[i+7], edid[i+8], edid[i+5], edid[i+6], edid[i+9]*10));
					}
					break;
				default:
					D(bug("[GMA] Entry %02x\n", edid[i+3]));

				}
			}
		}

		D(bug("[GMA] %d additional pages available\n", edid[126]));
	}
	else
		D(bug("[GMA] Not a valid EDID data\n"));
}

OOP_Object *METHOD(INTELG45, Root, New)
{
    D(bug("[GMA] Root New\n"));

    struct TagItem *modetags, *tags;
    struct TagItem *pool, *poolptr;

    struct TagItem pftags_24bpp[] = {
        { aHidd_PixFmt_RedShift,    8   }, /* 0 */
        { aHidd_PixFmt_GreenShift,  16  }, /* 1 */
        { aHidd_PixFmt_BlueShift,   24  }, /* 2 */
        { aHidd_PixFmt_AlphaShift,  0   }, /* 3 */
        { aHidd_PixFmt_RedMask,     0x00ff0000 }, /* 4 */
        { aHidd_PixFmt_GreenMask,   0x0000ff00 }, /* 5 */
        { aHidd_PixFmt_BlueMask,    0x000000ff }, /* 6 */
        { aHidd_PixFmt_AlphaMask,   0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,  vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,       24  }, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,   4   }, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,    24  }, /* 11 */
        { aHidd_PixFmt_StdPixFmt,   vHidd_StdPixFmt_BGR032 }, /* 12 Native */
        { aHidd_PixFmt_BitMapType,  vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
    };

    struct TagItem pftags_16bpp[] = {
        { aHidd_PixFmt_RedShift,    16  }, /* 0 */
        { aHidd_PixFmt_GreenShift,  21  }, /* 1 */
        { aHidd_PixFmt_BlueShift,   27  }, /* 2 */
        { aHidd_PixFmt_AlphaShift,  0   }, /* 3 */
        { aHidd_PixFmt_RedMask,     0x0000f800 }, /* 4 */
        { aHidd_PixFmt_GreenMask,   0x000007e0 }, /* 5 */
        { aHidd_PixFmt_BlueMask,    0x0000001f }, /* 6 */
        { aHidd_PixFmt_AlphaMask,   0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,  vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,       16  }, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,   2   }, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,    16  }, /* 11 */
        { aHidd_PixFmt_StdPixFmt,   vHidd_StdPixFmt_RGB16_LE }, /* 12 */
        { aHidd_PixFmt_BitMapType,  vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
    };

//    struct TagItem pftags_15bpp[] = {
//        { aHidd_PixFmt_RedShift,    17  }, /* 0 */
//        { aHidd_PixFmt_GreenShift,  22  }, /* 1 */
//        { aHidd_PixFmt_BlueShift,   27  }, /* 2 */
//        { aHidd_PixFmt_AlphaShift,  0   }, /* 3 */
//        { aHidd_PixFmt_RedMask,     0x00007c00 }, /* 4 */
//        { aHidd_PixFmt_GreenMask,   0x000003e0 }, /* 5 */
//        { aHidd_PixFmt_BlueMask,    0x0000001f }, /* 6 */
//        { aHidd_PixFmt_AlphaMask,   0x00000000 }, /* 7 */
//        { aHidd_PixFmt_ColorModel,  vHidd_ColorModel_TrueColor }, /* 8 */
//        { aHidd_PixFmt_Depth,       15  }, /* 9 */
//        { aHidd_PixFmt_BytesPerPixel,   2   }, /* 10 */
//        { aHidd_PixFmt_BitsPerPixel,    15  }, /* 11 */
//        { aHidd_PixFmt_StdPixFmt,   vHidd_StdPixFmt_RGB15_LE }, /* 12 */
//        { aHidd_PixFmt_BitMapType,  vHidd_BitMapType_Chunky }, /* 15 */
//        { TAG_DONE, 0UL }
//    };

	OOP_Object *i2c;

	modetags = tags = AllocVecPooled(sd->MemPool, sizeof (struct TagItem) * 60);
	pool = poolptr = AllocVecPooled(sd->MemPool, sizeof(struct TagItem) * 11 * 60);

	struct TagItem i2c_attrs[] = {
//			{ aHidd_I2C_HoldTime,   	40 },
//			{ aHidd_I2C_RiseFallTime,   40 },
			{ TAG_DONE, 0UL }
	};

	int automode_count = 0;

	tags->ti_Tag = aHidd_Gfx_PixFmtTags;
	tags->ti_Data = (IPTR)pftags_24bpp;
	tags++;

	tags->ti_Tag = aHidd_Gfx_PixFmtTags;
	tags->ti_Data = (IPTR)pftags_16bpp;
	tags++;

//	tags->ti_Tag = aHidd_Gfx_PixFmtTags;
//	tags->ti_Data = (IPTR)pftags_15bpp;
//	tags++;

	i2c = OOP_NewObject(sd->IntelI2C, NULL, i2c_attrs);

	if (i2c)
	{
		if (HIDD_I2C_ProbeAddress(i2c, 0xa0))
		{
			struct TagItem attrs[] = {
					{ aHidd_I2CDevice_Driver,   (IPTR)i2c       },
					{ aHidd_I2CDevice_Address,  0xa0            },
					{ aHidd_I2CDevice_Name,     (IPTR)"Display" },
					{ TAG_DONE, 0UL }
			};

			D(bug("[GMA] I2C device found\n"));

			OOP_Object *obj = OOP_NewObject(NULL, CLID_Hidd_I2CDevice, attrs);

			if (obj)
			{
				G45_parse_ddc(cl, &tags, poolptr, obj);

				OOP_DisposeObject(obj);
			}
		}
		OOP_DisposeObject(i2c);
	}

	tags->ti_Tag = TAG_DONE;
	tags->ti_Data = 0;

    struct TagItem mytags[] = {
        { aHidd_Gfx_ModeTags,   (IPTR)modetags  },
        { TAG_MORE, (IPTR)msg->attrList }
    };

    struct pRoot_New mymsg;

    mymsg.mID = msg->mID;
    mymsg.attrList = mytags;

    msg = &mymsg;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        sd->GMAObject = o;
    }

    FreeVecPooled(sd->MemPool, modetags);
    FreeVecPooled(sd->MemPool, poolptr);

    D(bug("[GMA] INTELG45::New() = %p\n", o));

    return o;
}

void METHOD(INTELG45, Root, Get)
{
    D(bug("[GMA] Root Get\n"));

    uint32_t idx;
    BOOL found = FALSE;
    if (IS_GFX_ATTR(msg->attrID, idx))
    {
    	switch (idx)
    	{
    	case aoHidd_Gfx_SupportsHWCursor:
    		*msg->storage = (IPTR)TRUE;
    		found = TRUE;
    		break;

    	case aoHidd_Gfx_NoFrameBuffer:
    		*msg->storage = (IPTR)TRUE;
    		found = TRUE;
    		break;

        case aoHidd_Gfx_HWSpriteTypes:
            *msg->storage = vHidd_SpriteType_DirectColor;
            found = TRUE;
            return;

    	case aoHidd_Gfx_DPMSLevel:
    		*msg->storage = SD(cl)->dpms;
    		found = TRUE;
    		break;
    	}
    }

    if (!found)
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return;
}

void METHOD(INTELG45, Root, Set)
{
	D(bug("[GMA] Root Set\n"));

	ULONG idx;
	struct TagItem *tag;
	const struct TagItem *tags = msg->attrList;

	while ((tag = NextTagItem(&tags)))
	{
		if (IS_GFX_ATTR(tag->ti_Tag, idx))
		{
			switch(idx)
			{
			case aoHidd_Gfx_DPMSLevel:
				LOCK_HW
				uint32_t adpa = readl(sd->Card.MMIO + G45_ADPA) & ~G45_ADPA_DPMS_MASK;
				switch (tag->ti_Data)
				{
				case vHidd_Gfx_DPMSLevel_On:
					adpa |= G45_ADPA_DPMS_ON;
					break;
				case vHidd_Gfx_DPMSLevel_Off:
					adpa |= G45_ADPA_DPMS_OFF;
					break;
				case vHidd_Gfx_DPMSLevel_Standby:
					adpa |= G45_ADPA_DPMS_STANDBY;
					break;
				case vHidd_Gfx_DPMSLevel_Suspend:
					adpa |= G45_ADPA_DPMS_SUSPEND;
					break;
				}
				writel(adpa, sd->Card.MMIO + G45_ADPA);
				sd->dpms = tag->ti_Data;

				UNLOCK_HW
				break;
			}
		}
	}

	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}


OOP_Object *METHOD(INTELG45, Hidd_Gfx, Show)
{
    if (msg->bitMap)
    {
    	GMABitMap_t *bm = OOP_INST_DATA(OOP_OCLASS(msg->bitMap), msg->bitMap);

    	D(bug("[GMA] Show(%p)\n", msg->bitMap));

        if (bm->state && sd->VisibleBitmap != bm)
        {
            /* Suppose bm has properly allocated state structure */
            if (bm->fbgfx)
            {
                bm->usecount++;

                LOCK_HW
                G45_LoadState(sd, bm->state);

                sd->VisibleBitmap = bm;

                UNLOCK_HW
            }
        }
    }
    else
    {
    	/* Blank screen */
    }

    /* Specification for NoFrameBuffer drivers says to return the received 
       bitmap and not to call the base Show method */
    return msg->bitMap;
}

OOP_Object *METHOD(INTELG45, Hidd_Gfx, NewBitMap)
{
    BOOL displayable, framebuffer;
    OOP_Class *classptr = NULL;
    struct TagItem mytags[2];
    struct pHidd_Gfx_NewBitMap mymsg;

    /* Displayable bitmap ? */
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);

    if (displayable)
    {
        classptr = sd->BMClass;   //offbmclass;
    }
    else
    {
        HIDDT_ModeID modeid;
        /*
            For the non-displayable case we can either supply a class ourselves
            if we can optimize a certain type of non-displayable bitmaps. Or we
            can let the superclass create on for us.

            The attributes that might come from the user deciding the bitmap
            pixel format are:
            - aHidd_BitMap_ModeID:  a modeid. create a nondisplayable
                bitmap with the size  and pixelformat of a gfxmode.
            - aHidd_BitMap_StdPixFmt: a standard pixelformat as described in
                hidd/graphics.h
            - aHidd_BitMap_Friend: if this is supplied and none of the two above
                are supplied, then the pixel format of the created bitmap
                will be the same as the one of the friend bitmap.

            These tags are listed in prioritized order, so if
            the user supplied a ModeID tag, then you should not care about StdPixFmt
            or Friend. If there is no ModeID, but a StdPixFmt tag supplied,
            then you should not care about Friend because you have to
            create the correct pixelformat. And as said above, if only Friend
            is supplied, you can create a bitmap with same pixelformat as Frien
        */

        modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);

        if (vHidd_ModeID_Invalid != modeid)
        {
            /* User supplied a valid modeid. We can use our offscreen class */
            classptr = sd->BMClass;
        }
        else
        {
            /*
               We may create an offscreen bitmap if the user supplied a friend
               bitmap. But we need to check that he did not supplied a StdPixFmt
            */
            HIDDT_StdPixFmt stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);

//            if (vHidd_StdPixFmt_Plane == stdpf)
//            {
//                classptr = sd->PlanarBMClass;
//            }
//            else
            if (vHidd_StdPixFmt_Unknown == stdpf)
            {
                /* No std pixfmt supplied */
                OOP_Object *friend;

                /* Did the user supply a friend bitmap ? */
                friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
                if (NULL != friend)
                {
                    OOP_Class *friend_class = NULL;
                    /* User supplied friend bitmap. Is the friend bitmap a Ati Gfx hidd bitmap ? */
                    OOP_GetAttr(friend, aHidd_BitMap_ClassPtr, (APTR)&friend_class);
                    if (friend_class == sd->BMClass)
                    {
                        /* Friend was ATI hidd bitmap. Now we can supply our own class */
                        classptr = sd->BMClass;
                    }
                }
            }
        }
    }

    D(bug("[GMA] classptr = %p\n", classptr));

    /* Do we supply our own class ? */
    if (NULL != classptr)
    {
        /* Yes. We must let the superclass not that we do this. This is
           done through adding a tag in the frot of the taglist */
        mytags[0].ti_Tag    = aHidd_BitMap_ClassPtr;
        mytags[0].ti_Data   = (IPTR)classptr;
        mytags[1].ti_Tag    = TAG_MORE;
        mytags[1].ti_Data   = (IPTR)msg->attrList;

        /* Like in Gfx::New() we init a new message struct */
        mymsg.mID       = msg->mID;
        mymsg.attrList  = mytags;

        /* Pass the new message to the superclass */
        msg = &mymsg;
    }

    return (OOP_Object*)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

void METHOD(INTELG45, Hidd_Gfx, SetCursorVisible)
{
    sd->CursorVisible = msg->visible;
    if (msg->visible)
    {
    	writel(G45_CURCNTR_PIPE_A | G45_CURCNTR_TYPE_ARGB, sd->Card.MMIO + G45_CURACNTR);
    	writel(sd->CursorBase, sd->Card.MMIO + G45_CURABASE);
    }
    else
    {
    	writel(G45_CURCNTR_PIPE_A | G45_CURCNTR_TYPE_OFF, sd->Card.MMIO + G45_CURACNTR);
    	writel(sd->CursorBase, sd->Card.MMIO + G45_CURABASE);
    }
}

void METHOD(INTELG45, Hidd_Gfx, SetCursorPos)
{
    ULONG x,y;
    WORD mx=msg->x, my=msg->y;

    if (mx < 0)
    {
    	x = G45_CURPOS_SIGN | (-mx);
    }
    else
    	x = mx;

    if (my < 0)
    {
    	y = G45_CURPOS_SIGN | (-my);
    }
    else
    	y = my;

    writel(((ULONG)x << G45_CURPOS_XSHIFT) | ((ULONG)y << G45_CURPOS_YSHIFT), sd->Card.MMIO + G45_CURAPOS);
    writel(sd->CursorBase, sd->Card.MMIO + G45_CURABASE);
}

BOOL METHOD(INTELG45, Hidd_Gfx, SetCursorShape)
{
    if (msg->shape == NULL)
    {
        sd->CursorVisible = 0;
        writel(G45_CURCNTR_PIPE_A | G45_CURCNTR_TYPE_OFF, sd->Card.MMIO + G45_CURACNTR);
        writel(sd->CursorBase, sd->Card.MMIO + G45_CURABASE);
    }
    else
    {
        IPTR       width, height, x;

        ULONG       *curimg = (ULONG*)((IPTR)sd->CursorImage + (IPTR)sd->Card.Framebuffer);

        OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
        OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);

        if (width > 64) width = 64;
        if (height > 64) height = 64;

        for (x = 0; x < 64*64; x++)
           curimg[x] = 0;

        HIDD_BM_GetImage(msg->shape, (UBYTE *)curimg, 64*4, 0, 0, width, height, vHidd_StdPixFmt_BGRA32);

        writel(G45_CURCNTR_PIPE_A | G45_CURCNTR_TYPE_ARGB, sd->Card.MMIO + G45_CURACNTR);
        writel(sd->CursorBase, sd->Card.MMIO + G45_CURABASE);
    }

    return TRUE;
}

void METHOD(INTELG45, Hidd_Gfx, CopyBox)
{
    ULONG mode = GC_DRMD(msg->gc);
    IPTR src=0, dst=0;

    /* Check whether we can get Drawable attribute of our ATI class */
    OOP_GetAttr(msg->src,   aHidd_GMABitMap_Drawable,   &src);
    OOP_GetAttr(msg->dest,  aHidd_GMABitMap_Drawable,   &dst);

    if (!dst || !src)
    {
        /* No. One of the bitmaps is not an ATI bitmap */
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    {
        /* Yes. Get the instance data of both bitmaps */
        GMABitMap_t *bm_src = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        GMABitMap_t *bm_dst = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);

//        D(bug("[GMA] CopyBox(src(%p,%d:%d@%d),dst(%p,%d:%d@%d),%d:%d\n",
//                bm_src->framebuffer,msg->srcX,msg->srcY,bm_src->depth,
//                bm_dst->framebuffer,msg->destX,msg->destY,bm_dst->depth,
//                msg->width, msg->height));

        /* Case -1: (To be fixed) one of the bitmaps have chunky outside GFX mem */
        if (!bm_src->fbgfx || !bm_dst->fbgfx)
        {
        	D(bug("[GMA] one of bitmaps outside VRAM! CopyBox(src(%p,%d:%d@%d),dst(%p,%d:%d@%d),%d:%d\n",
        			bm_src->framebuffer,msg->srcX,msg->srcY,bm_src->depth,
        			bm_dst->framebuffer,msg->destX,msg->destY,bm_dst->depth,
        			msg->width, msg->height));

        	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
        /* Case 0: one of bitmaps is 8bpp, whereas the other is TrueColor one */
        else if ((bm_src->depth <= 8 || bm_dst->depth <= 8) &&
            (bm_src->depth != bm_dst->depth))
        {
            /* Unsupported case */
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            return;
        }
        /* Case 1: both bitmaps have the same depth - use Blit engine */
        else if (bm_src->depth == bm_dst->depth)
        {
        	LOCK_MULTI_BITMAP
            LOCK_BITMAP_BM(bm_src)
            LOCK_BITMAP_BM(bm_dst)
            UNLOCK_MULTI_BITMAP

            LOCK_HW

            uint32_t br00, br13, br22, br23, br09, br11, br26, br12;

            br00 = (2 << 29) | (0x53 << 22) | (6);
            if (bm_dst->bpp == 4)
            	br00 |= 3 << 20;

            br13 = bm_dst->pitch | ROP_table[mode].rop;
            if (bm_dst->bpp == 4)
            	br13 |= 3 << 24;
            else if (bm_dst->bpp == 2)
            	br13 |= 1 << 24;

            br22 = msg->destX | (msg->destY << 16);
            br23 = (msg->destX + msg->width) | (msg->destY + msg->height) << 16;
            br09 = bm_dst->framebuffer;
            br11 = bm_src->pitch;
            br26 = msg->srcX | (msg->srcY << 16);
            br12 = bm_src->framebuffer;

            START_RING(8);

            OUT_RING(br00);
            OUT_RING(br13);
            OUT_RING(br22);
            OUT_RING(br23);
            OUT_RING(br09);
            OUT_RING(br26);
            OUT_RING(br11);
            OUT_RING(br12);

            ADVANCE_RING();

            UNLOCK_HW

            UNLOCK_BITMAP_BM(bm_src)
            UNLOCK_BITMAP_BM(bm_dst)
        }
        else /* Case 2: different bitmaps. HELP? */
        {
        	D(bug("[GMA] Depth mismatch! CopyBox(src(%p,%d:%d@%d),dst(%p,%d:%d@%d),%d:%d\n",
        			bm_src->framebuffer,msg->srcX,msg->srcY,bm_src->depth,
        			bm_dst->framebuffer,msg->destX,msg->destY,bm_dst->depth,
        			msg->width, msg->height));

            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
    }
}
