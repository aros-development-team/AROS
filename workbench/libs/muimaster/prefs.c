/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <stdlib.h>
#include <string.h>
#include <exec/types.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/commodities.h>
#include <proto/intuition.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

extern struct Library *MUIMasterBase;

#include "muimaster_intern.h"
#include "mui.h"
#include "imspec.h"

#ifdef _AROS
#define g_strdup(x) 	    	    	    	    \
    ({	    	    	    	    	    	    \
    	UBYTE *dup; 	    	    	    	    \
	    	    	    	    	    	    \
	dup = AllocVec(strlen(x) + 1, MEMF_PUBLIC); \
	if (dup) CopyMem((x), dup, strlen(x) + 1);  \
	dup; 	    	    	    	    	    \
    })
#define g_free FreeVec
#else

static char *g_strdup(char *x)
{
    char *dup;
    dup = AllocVec(strlen(x) + 1, MEMF_PUBLIC);
    if (dup) CopyMem((x), dup, strlen(x) + 1);
    return dup;
}

#define g_free(x) FreeVec(x);
#endif

static struct TextAttr defaultFont =
    { "topaz.font", 8, FS_NORMAL, 0 };


static void prefs_init_frames (struct ZunePrefs *prefs)
{
    struct MUI_FrameSpec *frame;

/* invisible frame */
    frame = &prefs->frames[MUIV_Frame_None];
    frame->type = FST_NONE;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 0;

/* text button */
    frame = &prefs->frames[MUIV_Frame_Button];
    frame->type = FST_BEVEL;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;

/* image button */
    frame = &prefs->frames[MUIV_Frame_ImageButton];
    frame->type = FST_BEVEL;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 0;
/* textfield without input */
    frame = &prefs->frames[MUIV_Frame_Text];
    frame->type = FST_BEVEL;
    frame->state = 1;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;
/* string gadget */
    frame = &prefs->frames[MUIV_Frame_String];
    frame->type = FST_THIN_BORDER;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;

/* list without input */
    frame = &prefs->frames[MUIV_Frame_ReadList];
    frame->type = FST_BEVEL;
    frame->state = 1;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;

/* list with input */
    frame = &prefs->frames[MUIV_Frame_InputList];
    frame->type = FST_BEVEL;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;
/* scrollbar container */
    frame = &prefs->frames[MUIV_Frame_Prop];
    frame->type = FST_BEVEL;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;

/* gauge */
    frame = &prefs->frames[MUIV_Frame_Gauge];
    frame->type = FST_BEVEL;
    frame->state = 1;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 0;
/* normal group */
    frame = &prefs->frames[MUIV_Frame_Group];
    frame->type = FST_THIN_BORDER;
    frame->state = 1;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;
/* cycle menu, popup window */
    frame = &prefs->frames[MUIV_Frame_PopUp];
    frame->type = FST_THIN_BORDER;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;
/* virt group */
    frame = &prefs->frames[MUIV_Frame_Virtual];
    frame->type = FST_BEVEL;
    frame->state = 1;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;
/* slider container */
    frame = &prefs->frames[MUIV_Frame_Slider];
    frame->type = FST_THICK_BORDER;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 0;
/* slider knob - perhaps one day added to the array ? */
    frame = &prefs->frames[MUIV_Frame_Knob];
    frame->type = FST_BEVEL;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 1;
/* dnd frame */
    frame = &prefs->frames[MUIV_Frame_Drag];
    frame->type = FST_THIN_BORDER;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 0;
} /* prefs_init_frames */


static void prefs_init_images (struct ZunePrefs *prefs)
{
    prefs->images[MUII_WindowBack] = zune_image_spec_to_structure((IPTR)"0:128",NULL); /* MUII_BACKGROUND */
    prefs->images[MUII_RequesterBack] =  zune_image_spec_to_structure((IPTR)"0:137",NULL); /* MUII_SHINEBACK */
    prefs->images[MUII_ButtonBack] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_ListBack] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_TextBack] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_PropBack] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_PopupBack] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_SelectedBack] = zune_image_spec_to_structure((IPTR)"0:131",NULL);
    prefs->images[MUII_ListCursor] = zune_image_spec_to_structure((IPTR)"0:131",NULL);
    prefs->images[MUII_ListSelect] = zune_image_spec_to_structure((IPTR)"0:135",NULL);
    prefs->images[MUII_ListSelCur] = zune_image_spec_to_structure((IPTR)"0:138",NULL);
    prefs->images[MUII_ArrowUp] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_ArrowDown] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_ArrowLeft] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_ArrowRight] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_CheckMark] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_RadioButton] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_Cycle] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_PopUp] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_PopFile] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_PopDrawer] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_PropKnob] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_Drawer] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_HardDisk] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_Disk] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_Chip] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_Volume] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_RegisterBack] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_Network] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_Assign] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_TapePlay] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_TapePlayBack] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_TapePause] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_TapeStop] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_TapeRecord] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_GroupBack] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_SliderBack] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_SliderKnob] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_TapeUp] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_TapeDown] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_PageBack] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
    prefs->images[MUII_ReadListBack] = zune_image_spec_to_structure((IPTR)"0:128",NULL);
} /* prefs_init_images */


static void prefs_init_keys (struct ZunePrefs *prefs)
{
    int i;

    prefs->muikeys[MUIKEY_PRESS].readable_hotkey = g_strdup("-upstroke return");
    prefs->muikeys[MUIKEY_TOGGLE].readable_hotkey = g_strdup("-repeat space");
    prefs->muikeys[MUIKEY_UP].readable_hotkey = g_strdup("-repeat up");
    prefs->muikeys[MUIKEY_DOWN].readable_hotkey = g_strdup("-repeat down");
    prefs->muikeys[MUIKEY_PAGEUP].readable_hotkey = g_strdup("-repeat shift up");
    prefs->muikeys[MUIKEY_PAGEDOWN].readable_hotkey = g_strdup("-repeat shift down");
    prefs->muikeys[MUIKEY_TOP].readable_hotkey = g_strdup("control up");
    prefs->muikeys[MUIKEY_BOTTOM].readable_hotkey = g_strdup("control down");
    prefs->muikeys[MUIKEY_LEFT].readable_hotkey = g_strdup("-repeat left");
    prefs->muikeys[MUIKEY_RIGHT].readable_hotkey = g_strdup("-repeat right");
    prefs->muikeys[MUIKEY_WORDLEFT].readable_hotkey = g_strdup("-repeat control left");
    prefs->muikeys[MUIKEY_WORDRIGHT].readable_hotkey = g_strdup("-repeat control right");
    prefs->muikeys[MUIKEY_LINESTART].readable_hotkey = g_strdup("shift left");
    prefs->muikeys[MUIKEY_LINEEND].readable_hotkey = g_strdup("shift right");
    prefs->muikeys[MUIKEY_GADGET_NEXT].readable_hotkey = g_strdup("-repeat tab");
    prefs->muikeys[MUIKEY_GADGET_PREV].readable_hotkey = g_strdup("-repeat shift tab");
    prefs->muikeys[MUIKEY_GADGET_OFF].readable_hotkey = g_strdup("control tab");
    prefs->muikeys[MUIKEY_WINDOW_CLOSE].readable_hotkey = g_strdup("esc");
    prefs->muikeys[MUIKEY_WINDOW_NEXT].readable_hotkey = g_strdup("-repeat alt tab");
    prefs->muikeys[MUIKEY_WINDOW_PREV].readable_hotkey = g_strdup("-repeat alt shift tab");
    prefs->muikeys[MUIKEY_HELP].readable_hotkey = g_strdup("help");
    prefs->muikeys[MUIKEY_POPUP].readable_hotkey = g_strdup("control p");

    for (i = 0; i < MUIKEY_COUNT; i++)
    {
    	if (prefs->muikeys[i].readable_hotkey)
	    prefs->muikeys[i].ix_well = !ParseIX(prefs->muikeys[i].readable_hotkey, &prefs->muikeys[i].ix);
	else prefs->muikeys[i].ix_well = 0;
    }
} /* prefs_init_keys */


/*
 * default prefs
 */
void __zune_prefs_init (struct ZunePrefs *prefs)
{
    struct MUI_PenSpec   *pen;
    int i;

    memset(prefs, 0, sizeof(struct ZunePrefs));
    prefs_init_frames(prefs);
    prefs_init_images(prefs);

    /* pens */
    prefs->muipens[MPEN_SHINE].red = 0xffffffff;
    prefs->muipens[MPEN_SHINE].green = 0xffffffff;
    prefs->muipens[MPEN_SHINE].blue = 0xffffffff;

    prefs->muipens[MPEN_HALFSHINE].red = 0xd0000000;
    prefs->muipens[MPEN_HALFSHINE].green = 0xd0000000;
    prefs->muipens[MPEN_HALFSHINE].blue = 0xd0000000;

    prefs->muipens[MPEN_BACKGROUND].red = 0xa0000000;
    prefs->muipens[MPEN_BACKGROUND].green = 0xa0000000;
    prefs->muipens[MPEN_BACKGROUND].blue = 0xa0000000;

    prefs->muipens[MPEN_HALFSHADOW].red = 0x50000000;
    prefs->muipens[MPEN_HALFSHADOW].green = 0x50000000;
    prefs->muipens[MPEN_HALFSHADOW].blue = 0x50000000;

    prefs->muipens[MPEN_SHADOW].red = 0x00000000;
    prefs->muipens[MPEN_SHADOW].green = 0x00000000;
    prefs->muipens[MPEN_SHADOW].blue = 0x00000000;

    prefs->muipens[MPEN_TEXT].red = 0x00000000;
    prefs->muipens[MPEN_TEXT].green = 0x00000000;
    prefs->muipens[MPEN_TEXT].blue = 0x00000000;

    prefs->muipens[MPEN_FILL].red = 0x05000000;
    prefs->muipens[MPEN_FILL].green = 0x84000000;
    prefs->muipens[MPEN_FILL].blue = 0xc4000000;

    prefs->muipens[MPEN_MARK].red = 0xf4000000;
    prefs->muipens[MPEN_MARK].green = 0xb5000000;
    prefs->muipens[MPEN_MARK].blue = 0x8b000000;

    /* fonts */
    prefs->textbutton_font = g_strdup(""); /* default to window_font_normal */
    prefs->group_title_font = g_strdup("fixed");
    prefs->list_font_normal = g_strdup("fixed");
    prefs->list_font_fixed = g_strdup("fixed");
    prefs->slider_knob_font = g_strdup("fixed");
    prefs->window_font_normal = g_strdup("fixed");
    prefs->window_font_small = g_strdup("fixed");
    prefs->window_font_big = g_strdup("fixed");

    {
	struct Screen *scr = LockPubScreen(NULL);
	if (scr)
	{
	    struct TextAttr scr_attr;
	    scr_attr = *scr->Font;

	    prefs->fonts[0] = NULL;
	    prefs->fonts[-MUIV_Font_Normal] = OpenFont(&scr_attr);
	    prefs->fonts[-MUIV_Font_List] = OpenFont(&scr_attr);
	    prefs->fonts[-MUIV_Font_Tiny] = OpenFont(&scr_attr);
	    prefs->fonts[-MUIV_Font_Fixed] = OpenFont(&defaultFont);
	    prefs->fonts[-MUIV_Font_Title] = OpenFont(&scr_attr);
	    prefs->fonts[-MUIV_Font_Big] = OpenFont(&scr_attr);
	    prefs->fonts[-MUIV_Font_Button] = OpenFont(&scr_attr);
	    prefs->fonts[-MUIV_Font_Knob] = OpenFont(&scr_attr);
	    UnlockPubScreen(NULL,scr);
	} else
	{
	    for (i = 0; i < -MUIV_Font_NegCount; i++)
	    {
		prefs->fonts[i] = OpenFont(&defaultFont);
	    }
	}
    }

/* radio */
    prefs->radiobutton_hspacing = 2;
    prefs->radiobutton_vspacing = 2;

/* cycle */
    prefs->cycle_menu_position = CYCLE_MENU_POSITION_CENTERED;
    prefs->cycle_menu_min_entries = 2;
    prefs->cycle_menu_speed = 0;
    prefs->cycle_menu_recessed_entries = TRUE;

/* group */
    prefs->group_title_position = GROUP_TITLE_POSITION_CENTERED;
    prefs->group_title_color = GROUP_TITLE_COLOR_BLACK;
    prefs->group_hspacing = 2;
    prefs->group_vspacing = 2;

/* string */
    pen = &prefs->string_bg_active;
    pen->ps_penType = PST_MUI;
    pen->ps_mui = MPEN_HALFSHINE;
  
    pen = &prefs->string_bg_inactive;
    pen->ps_penType = PST_MUI;
    pen->ps_mui = MPEN_BACKGROUND;

    pen = &prefs->string_text_active;
    pen->ps_penType = PST_MUI;
    pen->ps_mui = MPEN_TEXT;

    pen = &prefs->string_text_inactive;
    pen->ps_penType = PST_MUI;
    pen->ps_mui = MPEN_TEXT;

/* list */
    prefs->list_linespacing = 2;

/* scrollbars */
    prefs->sb_look = SB_LOOK_TOP;

/* navigation */
    prefs->dragndrop_left_button = FALSE;
    prefs->dragndrop_left_modifier = g_strdup("");
    prefs->dragndrop_middle_button = FALSE;
    prefs->dragndrop_middle_modifier = g_strdup("");
    prefs->dragndrop_autostart = -1;
    prefs->dragndrop_look = DND_LOOK_GHOSTED_ON_BOX;

    prefs->balancing_look = BALANCING_SHOW_FRAMES;

    /* keys */
    prefs->active_object_look = ACTIVE_OBJECT_LOOK_CORNER;
    pen = &prefs->active_object_color;
    pen->ps_penType = PST_MUI;
    pen->ps_mui = MPEN_SHINE;

    prefs_init_keys(prefs);

/* window */
    prefs->window_inner_left = 2;
    prefs->window_inner_right = 2;
    prefs->window_inner_top = 2;
    prefs->window_inner_bottom = 2;
    prefs->window_position = WINDOW_POSITION_FORGET_ON_EXIT;

/* internal */
    prefs->app_cfg_spy_delay = 1000;

#if 0
    prefs->comments = z_string_set_new();
#endif
}


/*
 * release resources dynamically allocated by prefs
 */
void __zune_prefs_release(struct ZunePrefs *prefs)
{
    int i;

    for (i = MUII_WindowBack; i < MUII_Count; i++)
	zune_imspec_free(prefs->images[i]); /* NULL is ok */

    g_free(prefs->textbutton_font);
    g_free(prefs->group_title_font);
    g_free(prefs->list_font_normal);
    g_free(prefs->list_font_fixed);
    g_free(prefs->slider_knob_font);
    g_free(prefs->window_font_normal);
    g_free(prefs->window_font_small);
    g_free(prefs->window_font_big);

    for (i = 0; i < -MUIV_Font_NegCount; i++)
    {
	if (prefs->fonts[i])
	    CloseFont(prefs->fonts[i]);
    }

#if 0
    zune_penspec_destroy_content(&prefs->string_bg_active);
    zune_penspec_destroy_content(&prefs->string_bg_inactive);
    zune_penspec_destroy_content(&prefs->string_text_active);
    zune_penspec_destroy_content(&prefs->string_text_inactive);
    zune_penspec_destroy_content(&prefs->active_object_color);
#endif

    g_free(prefs->dragndrop_left_modifier);
    g_free(prefs->dragndrop_middle_modifier);

  for (i = 0; i < MUIKEY_COUNT; i++)
	g_free(prefs->muikeys[i].readable_hotkey);
 
#if 0
    if (prefs->comments)
	z_string_set_destroy(prefs->comments);
#endif
}


int __zune_prefs_sys_global_read(struct ZunePrefs *prefs)
{
//    return __zune_prefs_read(prefs, __zune_file_get_sys_prefs_global_file());
    return 0;
}


int __zune_prefs_sys_app_read(struct ZunePrefs *prefs, STRPTR app_title)
{
//    return __zune_prefs_read(prefs, __zune_file_get_sys_prefs_app_file(app_title));
    return 0;
}


int __zune_prefs_user_global_read(struct ZunePrefs *prefs)
{
//    return __zune_prefs_read(prefs, __zune_file_get_user_prefs_global_file());
    return 0;
}


int __zune_prefs_user_app_read(struct ZunePrefs *prefs, STRPTR app_title)
{
//    return __zune_prefs_read(prefs, __zune_file_get_user_prefs_app_file(app_title));
    return 0;
}


/*
 * Should be forgotten as soon as there's a prefs editor
 */
int __zune_prefs_user_global_write(struct ZunePrefs *prefs)
{
//    return __zune_prefs_write(prefs, __zune_file_get_user_prefs_global_file());
    return 0;
}

int __zune_prefs_user_global_write_current(void)
{
//    return __zune_prefs_write(&__zprefs, __zune_file_get_user_prefs_global_file());
    return 0;
}


int __zune_prefs_user_app_write(struct ZunePrefs *prefs, STRPTR app_title)
{
//    return __zune_prefs_write(prefs, __zune_file_get_user_prefs_app_file(app_title));
    return 0;
}

int __zune_prefs_sys_global_write(struct ZunePrefs *prefs)
{
//    return __zune_prefs_write(prefs, __zune_file_get_sys_prefs_global_file());
    return 0;
}


int __zune_prefs_sys_app_write(struct ZunePrefs *prefs, STRPTR app_title)
{
    return 0;

//    return __zune_prefs_write(prefs, __zune_file_get_sys_prefs_app_file(app_title));
}

#if 0

static gboolean 
zune_open_windows (gpointer appdata)
{
    struct MUI_ApplicationData *data = (struct MUI_ApplicationData *)appdata;
    Object                *cstate;
    Object                *child;
    struct MinList        *ChildList;

/*      zune_gc_cache_cleanup(); */
    get(data->app_WindowFamily, MUIA_Family_List, (ULONG *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	set(child, MUIA_Window_Open, TRUE);
    }

    return FALSE;
}

static void
__zune_prefs_update_app (struct MUI_ApplicationData *data)
{
    Object                *cstate;
    Object                *child;
    struct MinList        *ChildList;
    guint v;

    get(data->app_WindowFamily, MUIA_Family_List, (ULONG *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	set(child, MUIA_Window_Open, FALSE);
    }
    __zune_prefs_user_global_read(&__zprefs);
    if (data->app_Title)
	__zune_prefs_user_app_read(&__zprefs, data->app_Title);
    v = g_idle_add(zune_open_windows, data);
}


BOOL
__zune_prefs_spy (struct MUI_ApplicationData *data)
{
    static time_t global_mtime = 0;
    static time_t app_mtime = 0;
    struct stat st;

/*  g_print("spy\n"); */
/* On first invokation, initalize the data stamps.
 */
    if (!global_mtime && !app_mtime)
    {
	if (stat(__zune_file_get_user_prefs_global_file(), &st) == 0)
	    global_mtime = st.st_mtime;
	if (stat(__zune_file_get_user_prefs_app_file(data->app_Title), &st) == 0)
	    app_mtime = st.st_mtime;
	return TRUE;
    }
/*  g_print("stating %s\n", __zune_file_get_user_prefs_global_file()); */
    if (stat(__zune_file_get_user_prefs_global_file(), &st) == 0)
    {
/*  g_print("success %s\n", __zune_file_get_user_prefs_global_file()); */

	if (st.st_mtime > global_mtime)
	{
	    global_mtime = st.st_mtime;
/*  g_print("global cfg update\n"); */
	    __zune_prefs_update_app(data);
	}
    }
/*      else */
/*  	g_print("fail %s\n", __zune_file_get_user_prefs_global_file()); */

/*  g_print("stating %s\n", __zune_file_get_user_prefs_app_file(data->app_Title)); */
    if (stat(__zune_file_get_user_prefs_app_file(data->app_Title), &st) == 0)
    {
/*  g_print("success %s\n", __zune_file_get_user_prefs_app_file(data->app_Title)); */
	if (st.st_mtime > app_mtime)
	{
	    app_mtime = st.st_mtime;
/*  g_print("app cfg update\n"); */
	    __zune_prefs_update_app(data);
	}
    }
/*      else */
/*  	g_print("fail %s\n", __zune_file_get_user_prefs_app_file(data->app_Title)); */

    return TRUE;
}

#endif
