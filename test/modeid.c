/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <cybergraphx/cybergraphics.h>
#include <graphics/displayinfo.h>
#include <hidd/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    ULONG modeid;
    BOOL iscyber;
    struct NameInfo name;
    struct DimensionInfo info;
    struct Library *OOPBase;
    OOP_AttrBase HiddPixFmtAttrBase;

    if (argc < 2) {
        printf("Usage: %s <modeid>\n", argv[0]);
	return 0;
    }

    modeid = strtoul(argv[1], NULL, 16);
    
    OOPBase = OpenLibrary("oop.library", 0);
    if (!OOPBase) {
        printf("Failed to open oop.library\n");
	return 0;
    }
    
    HiddPixFmtAttrBase = OOP_ObtainAttrBase(IID_Hidd_PixFmt);
    if (!HiddPixFmtAttrBase) {
        printf("Failed to obtain PixFmt attribute base\n");
	CloseLibrary(OOPBase);
	return 0;
    }

    if (GetDisplayInfoData(NULL, (UBYTE *)&name, sizeof(name), DTAG_NAME, modeid) == sizeof(name)) {
        printf("Obtained NameInfo: %s\n", name.Name);
    } else
        printf("Failed to obtain NameInfo\n");

    iscyber = IsCyberModeID(modeid);
    printf("IsCyberModeID() returns %d\n", iscyber);

    if (GetDisplayInfoData(NULL, (UBYTE *)&info, sizeof(info), DTAG_DIMS, modeid) == sizeof(info)) {
	OOP_Object *pixfmt = (OOP_Object *)info.reserved[1];
	IPTR val;

        printf("Obtained DimensionInfo\n");

	printf("Pixelformat data:\n");
	OOP_GetAttr(pixfmt, aHidd_PixFmt_ColorModel, &val);
	printf("Color model: %lu\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_RedShift, &val);
	printf("Red shift: %lu\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_GreenShift, &val);
	printf("Green shift: %lu\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_BlueShift, &val);
	printf("Blue shift: %lu\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_AlphaShift, &val);
	printf("Alpha shift: %lu\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_RedMask, &val);
	printf("Red mask: 0x%08lX\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_GreenMask, &val);
	printf("Green mask: 0x%08lX\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_BlueMask, &val);
	printf("Blue mask: 0x%08lX\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_AlphaMask, &val);
	printf("Alpha mask: 0x%08lX\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_Depth, &val);
	printf("Depth: %lu\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_BitsPerPixel, &val);
	printf("Bits per pixel: %lu\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_BytesPerPixel, &val);
	printf("Bytes per pixel: %lu\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_StdPixFmt, &val);
	printf("Pixelformat code: %lu\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_CLUTMask, &val);
	printf("Palette mask: 0x%08lX\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_CLUTShift, &val);
	printf("Palette shift: %lu\n", val);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_BitMapType, &val);
	printf("Bitmap type: %lu\n", val);
    } else
        printf("Failed to obtain DimensionInfo\n");

    OOP_ReleaseAttrBase(IID_Hidd_PixFmt);
    CloseLibrary(OOPBase);
    return 0;
}
