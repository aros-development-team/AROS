/*
 * intelG45_class.c
 *
 *  Created on: Apr 15, 2010
 *      Author: misc
 */

#define DEBUG 1
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

#include <stdint.h>



#include LC_LIBDEFS_FILE

#include "intelG45_intern.h"

#define sd ((struct g45staticdata*)SD(cl))

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#define HiddPCIDeviceAttrBase   (sd->pciAttrBase)
#define HiddATIBitMapAttrBase   (sd->atiBitMapAttrBase)
#define HiddBitMapAttrBase  (sd->bitMapAttrBase)
#define HiddPixFmtAttrBase  (sd->pixFmtAttrBase)
#define HiddGfxAttrBase     (sd->gfxAttrBase)
#define HiddSyncAttrBase    (sd->syncAttrBase)
#define HiddI2CAttrBase         (sd->i2cAttrBase)
#define HiddI2CDeviceAttrBase   (sd->i2cDeviceAttrBase)

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

#if 0
# 800x600 @ 56 Hz (VESA) HSync: 35.1562 kHz
ModeLine "800x600" 36.00 800 824 896 1024 600 601 603 625 +Hsync +Vsync
# 800x600 @ 60 Hz (VESA) HSync: 37.8788 kHz
ModeLine "800x600" 40.00 800 840 968 1056 600 601 605 628 +Hsync +Vsync
# 800x600 @ 72 Hz (VESA) HSync: 48.0769 kHz
ModeLine "800x600" 50.00 800 856 976 1040 600 637 643 666 +Hsync +Vsync
# 800x600 @ 75 Hz (VESA) HSync: 46.875 kHz
ModeLine "800x600" 49.50 800 816 896 1056 600 601 604 625 +Hsync +Vsync
# 800x600 @ 85 Hz (VESA) HSync: 53.7214 kHz
ModeLine "800x600" 56.30 800 832 896 1048 600 601 604 631 +Hsync +Vsync
# 1024x768 @ 60 Hz (VESA) HSync: 48.3631 kHz
ModeLine "1024x768" 65.00 1024 1048 1184 1344 768 771 777 806 -Hsync -Vsync
# 1024x768 @ 70 Hz (VESA) HSync: 56.4759 kHz
ModeLine "1024x768" 75.00 1024 1048 1184 1328 768 771 777 806 -Hsync -Vsync
# 1024x768 @ 75 Hz (VESA) HSync: 60.0229 kHz
ModeLine "1024x768" 78.75 1024 1040 1136 1312 768 769 772 800 +Hsync +Vsync
# 1024x768 @ 85 Hz (VESA) HSync: 68.6773 kHz
ModeLine "1024x768" 94.50 1024 1072 1168 1376 768 769 772 808 +Hsync +Vsync
# 1024x768 @ 87 Hz (VESA) HSync: 35.5222 kHz
ModeLine "1024x768" 44.90 1024 1032 1208 1264 768 768 776 817 +Hsync +Vsync Interlace
# 1152x864 @ 75 Hz (VESA) HSync: 67.5 kHz
ModeLine "1152x864" 108.00 1152 1216 1344 1600 864 865 868 900 +Hsync +Vsync
# 1280x960 @ 60 Hz (VESA) HSync: 60 kHz
ModeLine "1280x960" 108.00 1280 1376 1488 1800 960 961 964 1000 +Hsync +Vsync
# 1280x960 @ 85 Hz (VESA) HSync: 85.9375 kHz
ModeLine "1280x960" 148.50 1280 1344 1504 1728 960 961 964 1011 +Hsync +Vsync
# 1280x1024 @ 60 Hz (VESA) HSync: 63.981 kHz
ModeLine "1280x1024" 108.00 1280 1328 1440 1688 1024 1025 1028 1066 +Hsync +Vsync
# 1280x1024 @ 75 Hz (VESA) HSync: 79.9763 kHz
ModeLine "1280x1024" 135.00 1280 1296 1440 1688 1024 1025 1028 1066 +Hsync +Vsync
# 1280x1024 @ 85 Hz (VESA) HSync: 91.1458 kHz
ModeLine "1280x1024" 157.50 1280 1344 1504 1728 1024 1025 1028 1072 +Hsync +Vsync
# 1600x1200 @ 60 Hz (VESA) HSync: 75 kHz
ModeLine "1600x1200" 162.00 1600 1664 1856 2160 1200 1201 1204 1250 +Hsync +Vsync
# 1600x1200 @ 65 Hz (VESA) HSync: 81.25 kHz
ModeLine "1600x1200" 175.50 1600 1664 1856 2160 1200 1201 1204 1250 +Hsync +Vsync
# 1600x1200 @ 70 Hz (VESA) HSync: 87.5 kHz
ModeLine "1600x1200" 189.00 1600 1664 1856 2160 1200 1201 1204 1250 +Hsync +Vsync
# 1600x1200 @ 75 Hz (VESA) HSync: 93.75 kHz
ModeLine "1600x1200" 202.50 1600 1664 1856 2160 1200 1201 1204 1250 +Hsync +Vsync
# 1600x1200 @ 85 Hz (VESA) HSync: 106.25 kHz
ModeLine "1600x1200" 229.50 1600 1664 1856 2160 1200 1201 1204 1250 +Hsync +Vsync
# 1792x1344 @ 60 Hz (VESA) HSync: 83.6601 kHz
ModeLine "1792x1344" 204.80 1792 1920 2120 2448 1344 1345 1348 1394 -Hsync +Vsync
# 1792x1344 @ 75 Hz (VESA) HSync: 106.27 kHz
ModeLine "1792x1344" 261.00 1792 1888 2104 2456 1344 1345 1348 1417 -Hsync +Vsync
# 1856x1392 @ 60 Hz (VESA) HSync: 86.3528 kHz
ModeLine "1856x1392" 218.30 1856 1952 2176 2528 1392 1393 1396 1439 -Hsync +Vsync
# 1856x1392 @ 75 Hz (VESA) HSync: 112.5 kHz
ModeLine "1856x1392" 288.00 1856 1984 2208 2560 1392 1393 1396 1500 -Hsync +Vsync
# 1920x1440 @ 60 Hz (VESA) HSync: 90 kHz
ModeLine "1920x1440" 234.00 1920 2048 2256 2600 1440 1441 1444 1500 -Hsync +Vsync
# 1920x1440 @ 75 Hz (VESA) HSync: 112.5 kHz
ModeLine "1920x1440" 297.00 1920 2064 2288 2640 1440 1441 1444 1500 -Hsync +Vsync
#endif

static int G45_parse_ddc(OOP_Class *cl, struct TagItem **tagsptr, OOP_Object *obj)
{
	MAKE_SYNC(640x350_85, 31.50, 640, 672, 736, 832, 350, 382, 385, 445, "GMA: 640x350 @ 85 Hz (VESA)");
	MAKE_SYNC(640x400_85, 31.50, 640, 672, 736, 832, 400, 401, 404, 445, "GMA: 640x400 @ 85 Hz (VESA)");
	MAKE_SYNC(640x480_60, 25.18, 640, 656, 752, 800, 480, 490, 492, 525, "GMA: 640x480 @ 60 Hz (VESA)");
	MAKE_SYNC(640x480_73, 31.50, 640, 664, 704, 832, 480, 489, 492, 520, "GMA: 640x480 @ 73 Hz (VESA)");
	MAKE_SYNC(640x480_75, 31.50, 640, 656, 720, 840, 480, 481, 484, 500, "GMA: 640x480 @ 75 Hz (VESA)");
	MAKE_SYNC(640x480_85, 36.00, 640, 696, 752, 832, 480, 481, 484, 509, "GMA: 640x480 @ 85 Hz (VESA)");
	MAKE_SYNC(720x400_85, 35.50, 720, 756, 828, 936, 400, 401, 404, 446, "GMA: 720x400 @ 85 Hz (VESA)");

	struct TagItem *tags = *tagsptr;
	struct pHidd_I2CDevice_WriteRead msg;
	uint8_t edid[128];
	char wb[2] = {0, 0};
	int i;
	uint8_t chksum = 0;

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
			D(bug("[GMA]  720x400@70\n"));
		if (edid[35] & 0x40)
			D(bug("[GMA]  720x400@88\n"));
		if (edid[35] & 0x20)
			D(bug("[GMA]  640x480@60\n"));
		if (edid[35] & 0x10)
			D(bug("[GMA]  640x480@67\n"));
		if (edid[35] & 0x08)
			D(bug("[GMA]  640x480@72\n"));
		if (edid[35] & 0x04)
			D(bug("[GMA]  640x480@75\n"));
		if (edid[35] & 0x02)
			D(bug("[GMA]  800x600@56\n"));
		if (edid[35] & 0x01)
			D(bug("[GMA]  800x600@60\n"));
		if (edid[36] & 0x80)
			D(bug("[GMA]  800x600@72\n"));
		if (edid[36] & 0x40)
			D(bug("[GMA]  800x600@75\n"));
		if (edid[36] & 0x20)
			D(bug("[GMA]  832x624@75\n"));
		if (edid[36] & 0x10)
			D(bug("[GMA]  1024x768@87i\n"));
		if (edid[36] & 0x08)
			D(bug("[GMA]  1024x768@60\n"));
		if (edid[36] & 0x04)
			D(bug("[GMA]  1024x768@70\n"));
		if (edid[36] & 0x02)
			D(bug("[GMA]  1024x768@75\n"));
		if (edid[36] & 0x01)
			D(bug("[GMA]  1280x1024@75\n"));


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

				D(bug("[GMA]  %dx%d@%d\n", w, h, freq));
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

	*tagsptr = tags;
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
        { aHidd_PixFmt_StdPixFmt,   vHidd_StdPixFmt_Native }, /* 12 Native */
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
        { aHidd_PixFmt_StdPixFmt,   vHidd_StdPixFmt_Native }, /* 12 */
        { aHidd_PixFmt_BitMapType,  vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
    };

    struct TagItem pftags_15bpp[] = {
        { aHidd_PixFmt_RedShift,    17  }, /* 0 */
        { aHidd_PixFmt_GreenShift,  22  }, /* 1 */
        { aHidd_PixFmt_BlueShift,   27  }, /* 2 */
        { aHidd_PixFmt_AlphaShift,  0   }, /* 3 */
        { aHidd_PixFmt_RedMask,     0x00007c00 }, /* 4 */
        { aHidd_PixFmt_GreenMask,   0x000003e0 }, /* 5 */
        { aHidd_PixFmt_BlueMask,    0x0000001f }, /* 6 */
        { aHidd_PixFmt_AlphaMask,   0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,  vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,       15  }, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,   2   }, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,    15  }, /* 11 */
        { aHidd_PixFmt_StdPixFmt,   vHidd_StdPixFmt_Native }, /* 12 */
        { aHidd_PixFmt_BitMapType,  vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
    };

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

	tags->ti_Tag = aHidd_Gfx_PixFmtTags;
	tags->ti_Data = (IPTR)pftags_15bpp;
	tags++;

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
				G45_parse_ddc(cl, &tags, obj);

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
    		*msg->storage = (IPTR)FALSE;
    		found = TRUE;
    		break;

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
// TODO: Add DPMS function
				//DPMS(sd, tag->ti_Data);
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
    OOP_Object *fb = NULL;
    if (msg->bitMap)
    {
    	GMABitMap_t *bm = OOP_INST_DATA(OOP_OCLASS(msg->bitMap), msg->bitMap);
#if 0
        if (bm->state)
        {
            /* Suppose bm has properly allocated state structure */
            if (bm->fbgfx)
            {
                bm->usecount++;

                LOCK_HW
                LoadState(sd, bm->state);
                DPMS(sd, sd->dpms);

                fb = bm->BitMap;
                ShowHideCursor(sd, sd->Card.cursorVisible);

                RADEONEngineReset(sd);
                RADEONEngineRestore(sd);

                UNLOCK_HW
            }
        }
#endif
    }

    if (!fb)
        fb = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return fb;
}

OOP_Object *METHOD(INTELG45, Hidd_Gfx, NewBitMap)
{
    BOOL displayable, framebuffer;
    OOP_Class *classptr = NULL;
    struct TagItem mytags[2];
    struct pHidd_Gfx_NewBitMap mymsg;

    /* Displayable bitmap ? */
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);

    if (framebuffer)
    {
        /* If the user asks for a framebuffer map we must ALLWAYS supply a class */
        classptr = sd->OnBMClass;
    }
    else if (displayable)
    {
        classptr = sd->OnBMClass;   //offbmclass;
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
            classptr = sd->OffBMClass;
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
                    if (friend_class == sd->OnBMClass)
                    {
                        /* Friend was ATI hidd bitmap. Now we can supply our own class */
                        classptr = sd->OffBMClass;
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
