/*
    Copyright (C) 2016-2026, The AROS Development Team. All rights reserved.

    Desc: Private definitions for the graphics Display class.
*/

#ifndef GFX_DISPLAY_H
#define GFX_DISPLAY_H

#define DISPLAY_COMPOSITORCURSOR

struct HIDDDisplayData
{
	OOP_Object  *gfxhidd;       /* The gfx driver object this display belongs to */
	OOP_Object  *dmenum;        /* The display mode enumerator object            */
	OOP_Object  *compositor;    /* The display's compositor object (if any)      */

	/* Framebuffer control stuff */
	OOP_Object  *framebuffer;
	OOP_Object  *shownbm;
	BYTE         fbmode;        /* cached copy of the driver framebuffer type     */
	struct SignalSemaphore fbsem;

	/* gc used for stuff like rendering cursor */
	OOP_Object  *gc;

	/* cached compositor cursor method ids */
	OOP_MethodID mid_CompositorSetCursorShape;
	OOP_MethodID mid_CompositorSetCursorVisible;
	OOP_MethodID mid_CompositorSetCursorPos;

	/*
	 * Software cursor state.
	 *
	 * The base Display class renders the mouse pointer in software for drivers
	 * that have no hardware sprite. The cursor is alpha-composited onto the
	 * bitmap currently scanned out (cursor_bm), preserving the pixels beneath
	 * it in a save-under bitmap (cursor_backup) so they can be restored when the
	 * cursor moves or is hidden.
	 */
	OOP_Object  *cursor_bm;        /* bitmap the cursor is composited onto       */
	OOP_Object  *cursor_backup;    /* save-under bitmap (cursor sized)           */
	UBYTE       *cursor_argb;      /* extracted ARGB32 cursor image              */
	UWORD        cursor_w;         /* cursor image width                         */
	UWORD        cursor_h;         /* cursor image height                        */
	WORD         cursor_mouseX;    /* last requested mouse X (pre hotspot)       */
	WORD         cursor_mouseY;    /* last requested mouse Y (pre hotspot)       */
	WORD         cursor_xoffset;   /* hotspot X offset                           */
	WORD         cursor_yoffset;   /* hotspot Y offset                           */
	WORD         cursor_drawX;     /* current composited top-left X              */
	WORD         cursor_drawY;     /* current composited top-left Y              */
	WORD         cursor_backupX;   /* where the save-under was taken             */
	WORD         cursor_backupY;
	UWORD        cursor_backupW;
	UWORD        cursor_backupH;
	BOOL         cursor_visible;   /* requested visible                          */
	BOOL         cursor_drawn;     /* currently composited (save-under valid)    */
};

/* This has to be a #define, otherwise the HIDD_Gfx_CopyBox()
 * macro won't expand the HiddGfxBase macro to the csd structure.
 */
#define Display__Hidd_Display__UpdateFB(cl, o, bm, srcX, srcY, destX, destY, xSize, ySize) \
do { \
    struct HIDDDisplayData *__data = OOP_INST_DATA(cl, o); \
    HIDD_Gfx_CopyBox(__data->gfxhidd, bm, srcX, srcY, \
                     __data->framebuffer, destX, destY, \
                     xSize, ySize, __data->gc); \
} while (0)

static inline BOOL Display__Hidd_Display__SetFBColors(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    return OOP_DoMethod(data->framebuffer, &msg->mID);
}

static inline UBYTE Display__Hidd_Display__GetFBModeQuick(OOP_Class *cl, OOP_Object *o)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    return data->fbmode;
}

#endif /* GFX_DISPLAY_H */
