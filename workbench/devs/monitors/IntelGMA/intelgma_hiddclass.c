/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <utility/tagitem.h>
#include <hidd/gfx.h>
#include <hidd/i2c.h>
#include <graphics/displayinfo.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <stdio.h>
#include <stdint.h>

#include "intelgma_hidd.h"
#include "intelG45_regs.h"
#include "intelgma_gallium.h"
#include "compositing.h"

#define sd ((struct g45staticdata*)SD(cl))

#define MAX_MODE_NAME_LEN 30

#define SYNC_TAG_COUNT 16
#define SYNC_LIST_COUNT (15 + 8 + 4)

#define DEBUG_POINTER 1

#ifdef DEBUG_POINTER

#define PRINT_POINTER(image, xsize, xmax, ymax)		\
bug("[GMA] Pointer data:\n");				\
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
          v_total, v_front, v_sync, v_back, duty_cycle, pixel_freq;

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
	char *description = AllocVecPooled(sd->MemPool, MAX_MODE_NAME_LEN + 1);
	snprintf(description, MAX_MODE_NAME_LEN, "GMA: %dx%d@%d", x, y, refresh);
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

	PUSH_TAG(poolptr, aHidd_Sync_VMin, sync.height);
	PUSH_TAG(poolptr, aHidd_Sync_VMax, 4096);
	PUSH_TAG(poolptr, aHidd_Sync_HMin, sync.width);
	PUSH_TAG(poolptr, aHidd_Sync_HMax,  4096);
		
	PUSH_TAG(poolptr, aHidd_Sync_Flags, vHidd_Sync_VSyncPlus);
	PUSH_TAG(poolptr, TAG_DONE, 0);
}

static VOID G45_parse_ddc(OOP_Class *cl, struct TagItem **tagsptr,
    struct TagItem *poolptr, OOP_Object *obj)
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
				vsync_o |= 0x30 & ((edid[i+11] >> 2) << 4);
				vsync_w |= 0x30 & ((edid[i+11]) << 4);

				pixel = (edid[i] | (edid[i+1] << 8));

				D(bug("[GMA] Modeline: "));
				D(bug("%dx%d Pixel: %d0 kHz %d %d %d %d   %d %d %d %d\n", ha, va, pixel,
						ha, hb, hsync_o, hsync_w,
						va, vb, vsync_o, vsync_w));

				description = AllocVecPooled(sd->MemPool, MAX_MODE_NAME_LEN + 1);
				snprintf(description, MAX_MODE_NAME_LEN, "GMA: %dx%d@%d N",
					ha, va, (int)(((pixel * 10 / (uint32_t)(ha + hb)) * 1000)
					/ ((uint32_t)(va + vb))));

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

				PUSH_TAG(&poolptr, aHidd_Sync_VMin, va);
				PUSH_TAG(&poolptr, aHidd_Sync_VMax, 4096);
				PUSH_TAG(&poolptr, aHidd_Sync_HMin, ha);
				PUSH_TAG(&poolptr, aHidd_Sync_HMax,  4096);

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
    struct TagItem *poolptr;

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

	OOP_Object *i2cObj = NULL;

    modetags = tags = AllocVecPooled(sd->MemPool,
        sizeof (struct TagItem) * (3 + SYNC_LIST_COUNT + 1));
    poolptr = AllocVecPooled(sd->MemPool,
        sizeof(struct TagItem) * SYNC_TAG_COUNT * SYNC_LIST_COUNT);

	struct TagItem i2c_attrs[] = {
//			{ aHidd_I2C_HoldTime,   	40 },
//			{ aHidd_I2C_RiseFallTime,   40 },
			{ TAG_DONE, 0UL }
	};

	tags->ti_Tag = aHidd_Gfx_PixFmtTags;
	tags->ti_Data = (IPTR)pftags_24bpp;
	tags++;

	tags->ti_Tag = aHidd_Gfx_PixFmtTags;
	tags->ti_Data = (IPTR)pftags_16bpp;
	tags++;

//	tags->ti_Tag = aHidd_Gfx_PixFmtTags;
//	tags->ti_Data = (IPTR)pftags_15bpp;
//	tags++;

	if( sd->pipe == PIPE_B )
	{
		char *description = AllocVecPooled(sd->MemPool, MAX_MODE_NAME_LEN + 1);
		snprintf(description, MAX_MODE_NAME_LEN, "GMA_LVDS:%dx%d",
			sd->lvds_fixed.hdisp, sd->lvds_fixed.vdisp);

		//native lcd mode
		struct TagItem sync_native[]={
		{ aHidd_Sync_PixelClock,sd->lvds_fixed.pixelclock*1000000 },
		{ aHidd_Sync_HDisp,     sd->lvds_fixed.hdisp },
		{ aHidd_Sync_HSyncStart,sd->lvds_fixed.hstart },
		{ aHidd_Sync_HSyncEnd,  sd->lvds_fixed.hend },
		{ aHidd_Sync_HTotal,    sd->lvds_fixed.htotal },
		{ aHidd_Sync_VDisp,     sd->lvds_fixed.vdisp },
		{ aHidd_Sync_VSyncStart,sd->lvds_fixed.vstart },
		{ aHidd_Sync_VSyncEnd,  sd->lvds_fixed.vend },
		{ aHidd_Sync_VTotal,    sd->lvds_fixed.vtotal },
		{ aHidd_Sync_VMin,     sd->lvds_fixed.vdisp},
		{ aHidd_Sync_VMax,     4096},
		{ aHidd_Sync_HMin,     sd->lvds_fixed.hdisp},
		{ aHidd_Sync_HMax,     4096},
		{ aHidd_Sync_Description, (IPTR)description },
		{ TAG_DONE, 0UL }};

		MAKE_SYNC(640x480_60,   25174,
         640,  656,  752,  800,
         480,  490,  492,  525,
         "GMA_LVDS:640x480");
		
		tags->ti_Tag =  aHidd_Gfx_SyncTags;
		tags->ti_Data = (IPTR)sync_640x480_60;
		tags++;
		
		tags->ti_Tag =  aHidd_Gfx_SyncTags;
		tags->ti_Data = (IPTR)sync_native;
		tags++;
		
	}
	else
	{
		OOP_Object *i2cDev = OOP_NewObject(sd->IntelI2C, NULL, i2c_attrs);

		if (i2cDev)
		{
			if (HIDD_I2C_ProbeAddress(i2cDev, 0xa0))
			{
				struct TagItem attrs[] = {
						{ aHidd_I2CDevice_Driver,   (IPTR)i2cDev       },
						{ aHidd_I2CDevice_Address,  0xa0            },
						{ aHidd_I2CDevice_Name,     (IPTR)"Display" },
						{ TAG_DONE, 0UL }
				};

				D(bug("[GMA] I2C device found\n"));

				i2cObj = OOP_NewObject(NULL, CLID_Hidd_I2CDevice, attrs);

				if (i2cObj)
				{
					G45_parse_ddc(cl, &tags, poolptr, i2cObj);
				}
			}
		}

	}

	tags->ti_Tag = TAG_DONE;
	tags->ti_Data = 0;

    struct TagItem mytags[] = {
        { aHidd_Gfx_ModeTags,   (IPTR)modetags  },
        { aHidd_Name            , (IPTR)"IntelGMA"     },
        { aHidd_HardwareName    , (IPTR)"Intel GMA Display Adaptor"   },
        { aHidd_ProducerName    , (IPTR)"Intel Corporation"  },
        { TAG_MORE, (IPTR)msg->attrList }
    };

    struct pRoot_New mymsg;

    mymsg.mID = msg->mID;
    mymsg.attrList = mytags;

    msg = &mymsg;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
		struct g45data * gfxdata = OOP_INST_DATA(cl, o);
		gfxdata->i2cobj = i2cObj;
        sd->GMAObject = o;

		/* Create compositor object */
		{
			struct TagItem comptags [] =
			{
				{ aHidd_Compositor_GfxHidd, (IPTR)o },
				{ TAG_DONE, TAG_DONE }
			};
			sd->compositor = OOP_NewObject(sd->compositorclass, NULL, comptags);
			/* TODO: Check if object was created, how to handle ? */
		}
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
	struct TagItem *tags = msg->attrList;

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


OOP_Object * METHOD(INTELG45, Hidd_Gfx, CreateObject)
{
    OOP_Object      *object = NULL;

    if (msg->cl == SD(cl)->basebm)
    {
        struct pHidd_Gfx_CreateObject mymsg;
        HIDDT_ModeID modeid;
        HIDDT_StdPixFmt stdpf;

        struct TagItem mytags [] =
        {
            { TAG_IGNORE, TAG_IGNORE }, /* Placeholder for aHidd_BitMap_ClassPtr */
            { TAG_IGNORE, TAG_IGNORE }, /* Placeholder for aHidd_BitMap_Align */
            { aHidd_BitMap_IntelG45_CompositorHidd, (IPTR)sd->compositor },
            { TAG_MORE, (IPTR)msg->attrList }
        };

        /* Check if user provided valid ModeID */
        /* Check for framebuffer - not needed as IntelG45 is a NoFramebuffer driver */
        /* Check for displayable - not needed - displayable has ModeID and we don't
           distinguish between on-screen and off-screen bitmaps */
        modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
        if (vHidd_ModeID_Invalid != modeid) 
        {
            /* User supplied a valid modeid. We can use our bitmap class */
            mytags[0].ti_Tag	= aHidd_BitMap_ClassPtr;
            mytags[0].ti_Data	= (IPTR)SD(cl)->BMClass;
        } 

        /* Check if bitmap is a planar bitmap */
        stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
        if (vHidd_StdPixFmt_Plane == stdpf)
        {
            mytags[1].ti_Tag    = aHidd_BitMap_Align;
            mytags[1].ti_Data   = 32;
        }
        
        /* We init a new message struct */
        mymsg.mID	= msg->mID;
        mymsg.cl	= msg->cl;
        mymsg.attrList	= mytags;

        /* Pass the new message to the superclass */
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
    }
    else if (SD(cl)->basegallium && (msg->cl == SD(cl)->basegallium))
    {
		/* Create the gallium 3d driver object .. */
        object = OOP_NewObject(NULL, CLID_Hidd_Gallium_IntelGMA, msg->attrList);
    }
    else if (SD(cl)->basei2c && (msg->cl == SD(cl)->basei2c))
    {
		struct g45data * gfxdata = OOP_INST_DATA(cl, o);
        /* Expose the i2c bus object .. */
		object = gfxdata->i2cobj;
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return object;
}


void METHOD(INTELG45, Hidd_Gfx, SetCursorVisible)
{
    sd->CursorVisible = msg->visible;
    if (msg->visible)
    {
		writel( (sd->pipe == PIPE_A ? G45_CURCNTR_PIPE_A : G45_CURCNTR_PIPE_B ) | G45_CURCNTR_TYPE_ARGB ,
				sd->Card.MMIO +  (sd->pipe == PIPE_A ? G45_CURACNTR:G45_CURBCNTR));
    }
    else
    {
		writel( (sd->pipe == PIPE_A ? G45_CURCNTR_PIPE_A : G45_CURCNTR_PIPE_B ) | G45_CURCNTR_TYPE_OFF ,
				sd->Card.MMIO +  (sd->pipe == PIPE_A ? G45_CURACNTR:G45_CURBCNTR));
    }
    UpdateCursor(sd);
}


void METHOD(INTELG45, Hidd_Gfx, SetCursorPos)
{
	SetCursorPosition(sd,msg->x,msg->y);
	sd->pointerx = msg->x;
	sd->pointery = msg->y;
}

BOOL METHOD(INTELG45, Hidd_Gfx, SetCursorShape)
{
    if (msg->shape == NULL)
    {
        sd->CursorVisible = 0;
		writel( (sd->pipe == PIPE_A ? G45_CURCNTR_PIPE_A : G45_CURCNTR_PIPE_B ) | G45_CURCNTR_TYPE_OFF ,
				sd->Card.MMIO +  (sd->pipe == PIPE_A ? G45_CURACNTR:G45_CURBCNTR));
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
		writel( (sd->pipe == PIPE_A ? G45_CURCNTR_PIPE_A : G45_CURCNTR_PIPE_B ) | G45_CURCNTR_TYPE_ARGB ,
				sd->Card.MMIO +  (sd->pipe == PIPE_A ? G45_CURACNTR:G45_CURBCNTR));
    }
    UpdateCursor(sd);

    return TRUE;
}

void METHOD(INTELG45, Hidd_Gfx, CopyBox)
{
    ULONG mode = GC_DRMD(msg->gc);
    IPTR src=0, dst=0;

    /* Check whether we can get Drawable attribute of our GMA class */
    OOP_GetAttr(msg->src,   aHidd_GMABitMap_Drawable,   &src);
    OOP_GetAttr(msg->dest,  aHidd_GMABitMap_Drawable,   &dst);

    if (!dst || !src)
    {
        /* No. One of the bitmaps is not a GMA bitmap */
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

            DO_FLUSH();
            UNLOCK_HW

            UNLOCK_BITMAP_BM(bm_src)
            UNLOCK_BITMAP_BM(bm_dst)
        }
        else /* Case 2: different bitmaps.Use 3d hardware. */
        {
        	D(bug("[GMA] Depth mismatch! CopyBox(src(%p,%d:%d@%d),dst(%p,%d:%d@%d),%d:%d\n",
        			bm_src->framebuffer,msg->srcX,msg->srcY,bm_src->depth,
        			bm_dst->framebuffer,msg->destX,msg->destY,bm_dst->depth,
        			msg->width, msg->height));

            LOCK_MULTI_BITMAP
            LOCK_BITMAP_BM(bm_src)
            LOCK_BITMAP_BM(bm_dst)
            UNLOCK_MULTI_BITMAP

            BOOL done = copybox3d( bm_dst, bm_src,
                           msg->destX, msg->destY, msg->width, msg->height,
                           msg->srcX, msg->srcY, msg->width, msg->height );

            UNLOCK_BITMAP_BM(bm_src)
            UNLOCK_BITMAP_BM(bm_dst)

            if( ! done ) OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
    }
}

ULONG METHOD(INTELG45, Hidd_Gfx, ShowViewPorts)
{
    struct pHidd_Compositor_BitMapStackChanged bscmsg =
    {
        mID : OOP_GetMethodID(IID_Hidd_Compositor, moHidd_Compositor_BitMapStackChanged),
        data : msg->Data
    };
    D(bug("[IntelG45] ShowViewPorts enter TopLevelBM %x\n", msg->Data->Bitmap));
    OOP_DoMethod(sd->compositor, (OOP_Msg)&bscmsg);
    return TRUE; /* Indicate driver supports this method */
}

BOOL HIDD_INTELG45_SwitchToVideoMode(OOP_Object * bm)
{
    OOP_Class * cl = OOP_OCLASS(bm);
    GMABitMap_t * bmdata = OOP_INST_DATA(cl, bm);
    OOP_Object * gfx = NULL;
    HIDDT_ModeID modeid;
    OOP_Object * sync;
    OOP_Object * pf;
    IPTR pixel, e;
    IPTR hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal;

    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);
    gfx = (OOP_Object *)e;

    bug("[IntelG45] HIDD_INTELG45_SwitchToVideoMode bitmap:%x\n",bmdata);
    
    /* We should be able to get modeID from the bitmap */
    OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);

    if (modeid == vHidd_ModeID_Invalid)
    {
        D(bug("[IntelG45] Invalid ModeID\n"));
        return FALSE;
    }

    /* Get Sync and PixelFormat properties */
    struct pHidd_Gfx_GetMode __getmodemsg = 
    {
        modeID:	modeid,
        syncPtr:	&sync,
        pixFmtPtr:	&pf,
    }, *getmodemsg = &__getmodemsg;

    getmodemsg->mID = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_GetMode);
    OOP_DoMethod(gfx, (OOP_Msg)getmodemsg);

    OOP_GetAttr(sync, aHidd_Sync_PixelClock,    &pixel);
    OOP_GetAttr(sync, aHidd_Sync_HDisp,         &hdisp);
    OOP_GetAttr(sync, aHidd_Sync_VDisp,         &vdisp);
    OOP_GetAttr(sync, aHidd_Sync_HSyncStart,    &hstart);
    OOP_GetAttr(sync, aHidd_Sync_VSyncStart,    &vstart);
    OOP_GetAttr(sync, aHidd_Sync_HSyncEnd,      &hend);
    OOP_GetAttr(sync, aHidd_Sync_VSyncEnd,      &vend);
    OOP_GetAttr(sync, aHidd_Sync_HTotal,        &htotal);
    OOP_GetAttr(sync, aHidd_Sync_VTotal,        &vtotal);    

    D(bug("[IntelG45] Sync: %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
    pixel, hdisp, hstart, hend, htotal, vdisp, vstart, vend, vtotal));

	if (bmdata->state && sd->VisibleBitmap != bmdata)
	{
		/* Suppose bm has properly allocated state structure */
		if (bmdata->fbgfx)
		{
			bmdata->usecount++;
			SetCursorPosition(sd,0,0);
			LOCK_HW
			sd->VisibleBitmap = bmdata;
			G45_LoadState(sd, bmdata->state);
			UNLOCK_HW
			SetCursorPosition(sd,sd->pointerx,sd->pointery);
			return TRUE;
		}
	}
	
	return FALSE;     
}


BOOL HIDD_INTELG45_SetFramebuffer(OOP_Object * bm)
{
	OOP_Class * cl = OOP_OCLASS(bm);
	GMABitMap_t * bmdata = OOP_INST_DATA(cl, bm);
	//bug("[IntelG45] HIDD_INTELG45_SetFramebuffer %x %d,%d\n",bmdata,bmdata->xoffset,bmdata->yoffset);
	if (bmdata->fbgfx)
	{
		char *linoff_reg = sd->Card.MMIO + ((sd->pipe == PIPE_A) ? G45_DSPALINOFF : G45_DSPBLINOFF);
		char *stride_reg = sd->Card.MMIO + ((sd->pipe == PIPE_A) ? G45_DSPASTRIDE : G45_DSPBSTRIDE);
		
		// bitmap width in bytes
		writel( bmdata->state->dspstride , stride_reg );
		
		// framebuffer address + possible xy offset  
		writel(	bmdata->framebuffer - ( bmdata->yoffset * bmdata->pitch +
										bmdata->xoffset * bmdata->bpp ) ,linoff_reg );
		readl( linoff_reg );	
		return TRUE;
	}
	//bug("[IntelG45] HIDD_INTELG45_SetFramebuffer: not Framebuffer Bitmap!\n");
    return FALSE;
}


static struct HIDD_ModeProperties modeprops = 
{
    DIPF_IS_SPRITES,
    1,
    COMPF_ABOVE
};

ULONG METHOD(INTELG45, Hidd_Gfx, ModeProperties)
{
    ULONG len = msg->propsLen;
    if (len > sizeof(modeprops))
        len = sizeof(modeprops);
    CopyMem(&modeprops, msg->props, len);

    return len;
}

static const struct OOP_MethodDescr INTELG45_Root_descr[] =
{
    {(OOP_MethodFunc)INTELG45__Root__New, moRoot_New},
    {(OOP_MethodFunc)INTELG45__Root__Get, moRoot_Get},
    {(OOP_MethodFunc)INTELG45__Root__Set, moRoot_Set},
    {NULL, 0}
};
#define NUM_INTELG45_Root_METHODS 3

static const struct OOP_MethodDescr INTELG45_Hidd_Gfx_descr[] =
{
    {(OOP_MethodFunc)INTELG45__Hidd_Gfx__CopyBox         , moHidd_Gfx_CopyBox         },
    {(OOP_MethodFunc)INTELG45__Hidd_Gfx__CreateObject       , moHidd_Gfx_CreateObject       },
    {(OOP_MethodFunc)INTELG45__Hidd_Gfx__SetCursorVisible, moHidd_Gfx_SetCursorVisible},
    {(OOP_MethodFunc)INTELG45__Hidd_Gfx__SetCursorPos    , moHidd_Gfx_SetCursorPos    },
    {(OOP_MethodFunc)INTELG45__Hidd_Gfx__SetCursorShape  , moHidd_Gfx_SetCursorShape  },
    {(OOP_MethodFunc)INTELG45__Hidd_Gfx__ShowViewPorts   , moHidd_Gfx_ShowViewPorts   },
    {(OOP_MethodFunc)INTELG45__Hidd_Gfx__ModeProperties  , moHidd_Gfx_ModeProperties  },
    {NULL, 0}
};
#define NUM_INTELG45_Hidd_Gfx_METHODS 7

const struct OOP_InterfaceDescr INTELG45_ifdescr[] =
{
    {INTELG45_Root_descr    , IID_Root    , NUM_INTELG45_Root_METHODS    },
    {INTELG45_Hidd_Gfx_descr, IID_Hidd_Gfx, NUM_INTELG45_Hidd_Gfx_METHODS},
    {NULL		    , NULL	  , 0				 }
};
