/*
 * Copyright (C) 2011-2014, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * Tests for icon.library that do no involve rendering to the screen.
 *
 * All 'draw' tests will be directed to a fake 3-bit planar bitmap
 */
#include <proto/icon.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <string.h>     /* For memcmp() */

#include "testframe.h"

#include "icon.inc"

int main(int argc, char **argv)
{
    struct DiskObject *icon;
    struct Screen *screen;
    LONG frameless;
    struct Rectangle emboss = {};

    TESTING_BEGINS();

    TEST_START("Make Test Icon")
        LONG ispal = 0xdeadcafe;
        icon = GetDiskObject(NULL);
        VERIFY_NEQ(icon, NULL);
        VERIFY_RET(IconControl(NULL, ICONCTRLA_GetGlobalFrameless, &frameless, TAG_END), 1, -1);
        VERIFY_RET(IconControl(NULL, ICONCTRLA_GetGlobalEmbossRect, &emboss, TAG_END), 1, -1);
        if (frameless)
            emboss.MinX = emboss.MinY = emboss.MaxX = emboss.MaxY = 0;
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetWidth, test_width, TAG_END), 1, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetHeight, test_height, TAG_END), 1, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetPaletteSize1, test_palsize, TAG_END), 1, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetTransparentColor1, 0, TAG_END), 1, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetPalette1, &test_pal[0], TAG_END), 1, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_SetImageData1, &test_img[0], TAG_END), 1, -1);
        VERIFY_RET(IconControl(icon, ICONCTRLA_IsPaletteMapped, &ispal, TAG_END), 1, -1);
        VERIFY_NEQ(ispal, 0);
    TEST_END();

    TEST_START("LayoutIcon(icon, screen, TAG_END)");
        UWORD dummypens[] = { ~0 };
        int i;

        screen = OpenScreenTags(NULL,
                                     SA_Depth, test_depth,
                                     SA_Type, CUSTOMSCREEN,
                                     SA_ShowTitle, FALSE,
                                     SA_Title, "Icon",
                                     SA_Quiet, TRUE,
                                     SA_Left, 0,
                                     SA_Top, 0,
                                     SA_Colors, test_colorspec,
                                     SA_Width, test_width * 2,
                                     SA_Height, test_height * 2,
                                     SA_Pens, dummypens,
                                     TAG_END);
        VERIFY_NEQ(screen, NULL);
        /* Allocate *precisely* the pens we want */
        for (i = 0; i < test_palsize; i++) {
            ObtainPen(screen->ViewPort.ColorMap, i,
                        test_pal[i].red ? 0xffffffff : 0,
                        test_pal[i].green ? 0xffffffff : 0,
                        test_pal[i].blue ? 0xffffffff : 0,
                        0);
        }

        VERIFY_RET(LayoutIcon(icon, screen, OBP_Precision, PRECISION_EXACT, TAG_END), TRUE, 0);
    TEST_END();

    TEST_START("DrawIconState(&screen->RastPort, icon, NULL, 0, 0, 0, NULL)");
        UWORD x, y;
        int i;
        UBYTE *pixel, s_img[test_width * test_height], t_img[test_width * test_height];
        const UBYTE *pp, *i2p;
        struct TagItem tags[] = {
            { ICONDRAWA_Frameless, TRUE, },
            { ICONDRAWA_Borderless, TRUE, },
            { ICONDRAWA_EraseBackground, FALSE, },
            { TAG_DONE }
        };

        /* Draw a solid block of '7' behind the icon */
        SetAPen(&screen->RastPort, 7);
        RectFill(&screen->RastPort, 0, 0, test_width*2 - 1, test_height*2 - 1);

        DrawIconStateA(&screen->RastPort, icon, NULL, 0, 0, 0, tags);

        /* Verify that the icon was written correctly.
         * All the '4' colors should be '7'
         */
        pixel = t_img;
        pp = test_img;
        for (y = 0; y < test_height; y++)
            for (x = 0; x < test_width; x++, pixel++, pp++)
               *pixel = (*pp == 4) ? 7 : *pp;

        pixel = s_img;
        for (y = 0; y < test_height; y++)
            for (x = 0; x < test_width; x++, pixel++)
                *pixel = (UBYTE)ReadPixel(&screen->RastPort, x, y);

        pixel = t_img;
        pp = s_img;
        i2p = s_img; /* Icon to Pen color mapping */
        for (y = 0; y < test_height; y++) {
            for (x = 0; x < test_width; x++, pixel++, pp++) {
                *pixel = i2p[*pixel];
                if (*pp != *pixel) {
                    Printf("(%ld,%ld): != %ld, got %ld\n", x, y, *pixel, *pp);
                    VERIFY_EQ(*pp, *pixel);
                }
            }
        }

        for (i = 0; i < test_palsize; i++) {
            Printf("Icon %ld = Pen %ld\n", i, i2p[i]);
        }

        VERIFY_EQ(memcmp(s_img, t_img, sizeof(t_img)), 0);
    TEST_END();

    TEST_START("Cleanup");
        int i;
        for (i = 0; i < test_palsize; i++) {
            ReleasePen(screen->ViewPort.ColorMap, i);
        }

        FreeDiskObject(icon);
        CloseScreen(screen);
    TEST_END();

    TESTING_ENDS();

    return 0;
}
