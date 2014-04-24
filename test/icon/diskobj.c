/*
 * Copyright (C) 2011-2014, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * Tests for icon.library that do not involve rendering to the screen.
 *
 * All 'draw' tests will be directed to a fake 3-bit planar bitmap
 */
#include <proto/icon.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "testframe.h"

#include "icon.inc"

int main(int argc, char **argv)
{
    struct DiskObject *icon;

    TESTING_BEGINS();

    TEST_START("GetDiskObject(NULL)");
        icon = GetDiskObject(NULL);
        VERIFY_NEQ(icon, NULL);
        VERIFY_EQ(icon->do_Magic, 0);
        VERIFY_EQ(icon->do_Version, 0);
        VERIFY_EQ(icon->do_Gadget.LeftEdge, 0);
        VERIFY_EQ(icon->do_Gadget.TopEdge, 0);
        VERIFY_EQ(icon->do_Gadget.Width, 0);
        VERIFY_EQ(icon->do_Gadget.Height, 0);
        VERIFY_EQ(icon->do_Gadget.Flags, 0);
        VERIFY_EQ(icon->do_Gadget.Activation, 0);
        VERIFY_EQ(icon->do_Gadget.GadgetType, 0);
        VERIFY_EQ(icon->do_Gadget.GadgetRender, NULL);
        VERIFY_EQ(icon->do_Gadget.SelectRender, NULL);
        VERIFY_EQ(icon->do_Gadget.GadgetText, NULL);
        VERIFY_EQ(icon->do_Gadget.SpecialInfo, NULL);
        VERIFY_EQ(icon->do_Gadget.GadgetID, 0);
        VERIFY_EQ(icon->do_Gadget.UserData, NULL);
        VERIFY_EQ(icon->do_Type, 0);
        VERIFY_EQ(icon->do_DefaultTool, NULL);
        VERIFY_EQ(icon->do_ToolTypes, NULL);
        VERIFY_EQ(icon->do_CurrentX, 0);
        VERIFY_EQ(icon->do_CurrentY, 0);
        VERIFY_EQ(icon->do_ToolWindow, 0);
        VERIFY_EQ(icon->do_StackSize, 0);
        icon->do_Magic = WB_DISKMAGIC;
        icon->do_Type = WBPROJECT;
        icon->do_Version = (WB_DISKVERSION << 8) | WB_DISKREVISION;
        icon->do_ToolTypes = (STRPTR *)test_tooltypes;
        icon->do_DefaultTool = (STRPTR)test_defaulttool;
        icon->do_CurrentX = NO_ICON_POSITION;
        icon->do_CurrentY = NO_ICON_POSITION;
    TEST_END();

    /* Global properties */

    TEST_START("IconControl(NULL, ICONCTRLA_{Set,Get}GlobalScreen, ...)");
        struct Screen *screen = NULL;
        
        /* Verify that GlobalScreen can be freed */
        VERIFY_RET(IconControl(NULL, ICONCTRLA_SetGlobalScreen, NULL, TAG_END), 0, -1);
        VERIFY_RET(IconControl(NULL, ICONCTRLA_GetGlobalScreen, &screen, TAG_END), 1, -1);
        VERIFY_EQ(screen, NULL);

        /* Verify that GlobalScreen can be changed if freed */
        VERIFY_RET(IconControl(NULL, ICONCTRLA_SetGlobalScreen, 0xcafebabe, TAG_END), 1, -1);
        VERIFY_RET(IconControl(NULL, ICONCTRLA_GetGlobalScreen, &screen, TAG_END), 1, -1);
        VERIFY_EQ((APTR)screen, (APTR)0xcafebabe);

        /* Verify that GlobalScreen can't be changed once set
         */
        VERIFY_RET(IconControl(NULL, ICONCTRLA_SetGlobalScreen, 0xdeadbeef, TAG_END), 0, -1);
        VERIFY_RET(IconControl(NULL, ICONCTRLA_GetGlobalScreen, &screen, TAG_END), 1, -1);
        VERIFY_EQ((APTR)screen, (APTR)0xcafebabe);

        VERIFY_RET(IconControl(NULL, ICONCTRLA_SetGlobalScreen, NULL, TAG_END), 1, -1);
        VERIFY_RET(IconControl(NULL, ICONCTRLA_GetGlobalScreen, &screen, TAG_END), 1, -1);
        VERIFY_EQ((APTR)screen, (APTR)NULL);
    TEST_END();

    TEST_START("IconControl(NULL, ICONCTRLA_{Set,Get}GlobalPrecision, ...)");
        LONG prec = 0x8eadbeef;
        VERIFY_RET(IconControl(NULL, ICONCTRLA_SetGlobalPrecision, PRECISION_IMAGE, TAG_END), 1, -1);
        VERIFY_RET(IconControl(NULL, ICONCTRLA_GetGlobalPrecision, &prec, TAG_END), 1, -1);
        VERIFY_EQ(prec, PRECISION_IMAGE);
        VERIFY_RET(IconControl(NULL, ICONCTRLA_SetGlobalPrecision, PRECISION_ICON, TAG_END), 1, -1);
        VERIFY_RET(IconControl(NULL, ICONCTRLA_GetGlobalPrecision, &prec, TAG_END), 1, -1);
        VERIFY_EQ(prec, PRECISION_ICON);
    TEST_END();

    TEST_START("IconControl(NULL, ICONCTRLA_{Set,Get}GlobalFrameless, ...)");
        LONG frameless = 0xdeadbeef;
        VERIFY_RET(IconControl(NULL, ICONCTRLA_SetGlobalFrameless, TRUE, TAG_END), 1, 0);
        VERIFY_RET(IconControl(NULL, ICONCTRLA_GetGlobalFrameless, &frameless, TAG_END), 1, -1);
        VERIFY_EQ(frameless, TRUE);
        VERIFY_RET(IconControl(NULL, ICONCTRLA_SetGlobalFrameless, FALSE, TAG_END), 1, 0);
        VERIFY_RET(IconControl(NULL, ICONCTRLA_GetGlobalFrameless, &frameless, TAG_END), 1, -1);
        VERIFY_EQ(frameless, FALSE);
    TEST_END();

    /* Icon configuration */
    TEST_START("IconControl(icon, ICONCTRLA_IsPaletteMapped (FALSE)");
        LONG ispal = 0xdeadcafe;
        VERIFY_RET(IconControl(icon, ICONCTRLA_IsPaletteMapped, &ispal, TAG_END), 1, -1);
        VERIFY_EQ(ispal, 0);
    TEST_END();

    TEST_START("IconControl(icon, ICONCTRLA_{Set,Get}Width, ...)");
        ULONG width = 0xdeadcafe;
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetWidth, &width, TAG_END), 0, -1);
        VERIFY_EQ(width, 0xdeadcafe);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetWidth, test_width, TAG_END), 1, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetWidth, &width, TAG_END), 1, -1);
        VERIFY_EQ(width, test_width);

        /* Verify that width <= 0 and width > 256 is invalid */
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetWidth, 0, TAG_END), 0, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetWidth, &width, TAG_END), 1, -1);
        VERIFY_EQ(width, test_width);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetWidth, 257, TAG_END), 0, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetWidth, &width, TAG_END), 1, -1);
        VERIFY_EQ(width, test_width);
    TEST_END();

    TEST_START("IconControl(icon, ICONCTRLA_{Set,Get}Height, ...)");
        ULONG height = 0xdeadcafe;
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetHeight, &height, TAG_END), 1, -1);
        VERIFY_EQ(height, 0);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetHeight, test_height, TAG_END), 1, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetHeight, &height, TAG_END), 1, -1);
        VERIFY_EQ(height, test_height);
        /* Verify that height <= 0 and height > 256 is invalid */
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetHeight, 0, TAG_END), 0, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetHeight, &height, TAG_END), 1, -1);
        VERIFY_EQ(height, test_height);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetHeight, 257, TAG_END), 0, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetHeight, &height, TAG_END), 1, -1);
        VERIFY_EQ(height, test_height);
    TEST_END();

    TEST_START("IconControl(icon, ICONCTRLA_{Set,Get}PaletteSize1, ...)");
        ULONG palsize = 0xdeadbeef;
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetPaletteSize1, &palsize, TAG_END), 1, -1);
        VERIFY_EQ(palsize, 0);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetPaletteSize1, test_palsize, TAG_END), 1, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetPaletteSize1, &palsize, TAG_END), 1, -1);
        VERIFY_EQ(palsize, test_palsize);
        /* Verify that palsize <= 0 and palsize > 256 is invalid */
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetPaletteSize1, 0, TAG_END), 0, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetPaletteSize1, &palsize, TAG_END), 1, -1);
        VERIFY_EQ(palsize, 8);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetPaletteSize1, 257, TAG_END), 0, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetPaletteSize1, &palsize, TAG_END), 1, -1);
        VERIFY_EQ(palsize, 8);
    TEST_END();

    TEST_START("IconControl(icon, ICONCTRLA_{Set,Get}TransparentColor1, ...)");
        LONG trans = 0xdeadcafe;
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetTransparentColor1, &trans, TAG_END), 1, -1);
        VERIFY_EQ(trans, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetTransparentColor1, 3, TAG_END), 1, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetTransparentColor1, &trans, TAG_END), 1, -1);
        VERIFY_EQ(trans, 3);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetTransparentColor1, test_transcolor, TAG_END), 1, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetTransparentColor1, &trans, TAG_END), 1, -1);
        VERIFY_EQ(trans, test_transcolor);
    TEST_END();
        
    TEST_START("IconControl(icon, ICONCTRLA_{Set,Get}Palette1, ...)");
        struct ColorRegister *pal;
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetPalette1, &pal, TAG_END), 1, -1);
        VERIFY_EQ(pal, NULL);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetPalette1, &test_pal[0], TAG_END), 1, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetPalette1, &pal, TAG_END), 1, -1);
        VERIFY_EQ(pal, &test_pal[0]);
    TEST_END();

    TEST_START("IconControl(icon, ICONCTRLA_{Set,Get}ImageData1, ...)");
        struct ColorRegister *img;
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetImageData1, &img, TAG_END), 1, -1);
        VERIFY_EQ(img, NULL);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetImageData1, &test_img[0], TAG_END), 1, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_GetImageData1, &img, TAG_END), 1, -1);
        VERIFY_EQ(img, (struct ColorRegister *)&test_img[0]);
    TEST_END();

    TEST_START("IconControl(icon, ICONCTRLA_IsPaletteMapped (TRUE)");
        LONG ispal = 0xdeadcafe;
        VERIFY_RET(IconControl(icon, ICONCTRLA_IsPaletteMapped, &ispal, TAG_END), 1, -1);
        VERIFY_NEQ(ispal, 0);
    TEST_END();

    /* Test getting the default object for a non-existent icon */
    TEST_START("Get Default Object");
        BPTR file;
        struct DiskObject *itmp;

        DeleteDiskObject("RAM:diskobj.tmp");
        DeleteFile("RAM:diskobj.tmp");
        file = Open("RAM:diskobj.tmp", MODE_NEWFILE);
        Write(file, "Hello World\n", 12);
        Close(file);
        itmp = GetDiskObjectNew("RAM:diskobj.tmp");
        VERIFY_NEQ(itmp, NULL);
        VERIFY_EQ(itmp->do_Type, WBPROJECT);
        FreeDiskObject(itmp);
    TEST_END();

    /* Negative test saving to disk (no imagery data) */
    TEST_START("PutDiskObject(..., icon)");
        VERIFY_RET(PutDiskObject("RAM:diskobj.tmp", icon), 0, ERROR_OBJECT_WRONG_TYPE);
    TEST_END();

    /* Test saving to disk */
    TEST_START("PutDiskObject(..., icon)");
        struct DiskObject *itmp, *isrc;
        isrc = GetDiskObjectNew("SYS:");
        VERIFY_NEQ(isrc, NULL);
        icon->do_Gadget = isrc->do_Gadget;
        VERIFY_NEQ(icon->do_Gadget.GadgetRender, NULL);
        itmp = DupDiskObject(icon, ICONDUPA_DuplicateImages, TRUE,
                                   ICONDUPA_DuplicateImageData, TRUE,
                                   TAG_END);
        FreeDiskObject(isrc);
        FreeDiskObject(icon);
        icon = itmp;
        VERIFY_RET(PutDiskObject("RAM:diskobj.tmp", icon), 1, 0);
    TEST_END();

    TEST_START("FreeDiskObject()");
        FreeDiskObject(icon);
    TEST_END();

    TESTING_ENDS();

    return 0;
}
