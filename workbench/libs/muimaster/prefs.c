/* Zune -- a free Magic User Interface implementation
 * Copyright (C) 1999 David Le Corfec
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <exec/types.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

extern struct Library *MUIMasterBase;

#include "muimaster_intern.h"
#include "mui.h"
#include "imspec.h"

#define g_strdup strdup
#define g_free free

struct ZunePrefs __zprefs;

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
    frame->type = FST_THIN_BORDER;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 0;

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
    frame->type = FST_THIN_BORDER;
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
    prefs->images[MUII_WindowBack] = zune_image_spec_to_structure((IPTR)"0:128"); /* MUII_BACKGROUND */
    prefs->images[MUII_RequesterBack] =  zune_image_spec_to_structure((IPTR)"0:132"); /* MUII_SHINEBACK */
    prefs->images[MUII_ButtonBack] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_ListBack] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_TextBack] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_PropBack] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_PopupBack] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_SelectedBack] = zune_image_spec_to_structure((IPTR)"0:131");
    prefs->images[MUII_ListCursor] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_ListSelect] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_ListSelCur] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_ArrowUp] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_ArrowDown] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_ArrowLeft] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_ArrowRight] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_CheckMark] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_RadioButton] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_Cycle] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_PopUp] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_PopFile] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_PopDrawer] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_PropKnob] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_Drawer] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_HardDisk] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_Disk] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_Chip] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_Volume] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_RegisterBack] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_Network] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_Assign] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_TapePlay] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_TapePlayBack] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_TapePause] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_TapeStop] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_TapeRecord] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_GroupBack] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_SliderBack] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_SliderKnob] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_TapeUp] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_TapeDown] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_PageBack] = zune_image_spec_to_structure((IPTR)"0:128");
    prefs->images[MUII_ReadListBack] = zune_image_spec_to_structure((IPTR)"0:128");
} /* prefs_init_images */


static void prefs_init_keys (struct ZunePrefs *prefs)
{
    int i;

#warning FIXME: muikeys
#if 0
    prefs->muikeys[MUIKEY_PRESS].readable_hotkey = g_strdup("Return");
    prefs->muikeys[MUIKEY_TOGGLE].readable_hotkey = g_strdup("space");
    prefs->muikeys[MUIKEY_UP].readable_hotkey = g_strdup("Up");
    prefs->muikeys[MUIKEY_DOWN].readable_hotkey = g_strdup("Down");
    prefs->muikeys[MUIKEY_PAGEUP].readable_hotkey = g_strdup("Prior");
    prefs->muikeys[MUIKEY_PAGEDOWN].readable_hotkey = g_strdup("Next");
    prefs->muikeys[MUIKEY_TOP].readable_hotkey = g_strdup("Home");
    prefs->muikeys[MUIKEY_BOTTOM].readable_hotkey = g_strdup("End");
    prefs->muikeys[MUIKEY_LEFT].readable_hotkey = g_strdup("Left");
    prefs->muikeys[MUIKEY_RIGHT].readable_hotkey = g_strdup("Right");
    prefs->muikeys[MUIKEY_WORDLEFT].readable_hotkey = g_strdup("Control Left");
    prefs->muikeys[MUIKEY_WORDRIGHT].readable_hotkey = g_strdup("Control Right");
    prefs->muikeys[MUIKEY_LINESTART].readable_hotkey = g_strdup("Shift Left");
    prefs->muikeys[MUIKEY_LINEEND].readable_hotkey = g_strdup("Shift Right");
    prefs->muikeys[MUIKEY_GADGET_NEXT].readable_hotkey = g_strdup("Tab");
    prefs->muikeys[MUIKEY_GADGET_PREV].readable_hotkey = g_strdup("Shift Tab");
    prefs->muikeys[MUIKEY_GADGET_OFF].readable_hotkey = g_strdup("Control Tab");
    prefs->muikeys[MUIKEY_WINDOW_CLOSE].readable_hotkey = g_strdup("Escape");
    prefs->muikeys[MUIKEY_WINDOW_NEXT].readable_hotkey = g_strdup("Alt Next");
    prefs->muikeys[MUIKEY_WINDOW_PREV].readable_hotkey = g_strdup("Alt Prior");
    prefs->muikeys[MUIKEY_HELP].readable_hotkey = g_strdup("Pause");
    prefs->muikeys[MUIKEY_POPUP].readable_hotkey = g_strdup("Control p");

    for (i = 0; i < MUIKEY_COUNT; i++)
    {
	zune_keyspec_parse(&prefs->muikeys[i]);
    }
#endif
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
#warning FIXME: muipens
//    prefs->muipens = __mpens;

/* fonts */
    prefs->textbutton_font = g_strdup(""); /* default to window_font_normal */
    prefs->group_title_font = g_strdup("fixed");
    prefs->list_font_normal = g_strdup("fixed");
    prefs->list_font_fixed = g_strdup("fixed");
    prefs->slider_knob_font = g_strdup("fixed");
    prefs->window_font_normal = g_strdup("fixed");
    prefs->window_font_small = g_strdup("fixed");
    prefs->window_font_big = g_strdup("fixed");

    for (i = 0; i < -MUIV_Font_NegCount; i++)
    {
	prefs->fonts[i] = OpenFont(&defaultFont);
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

#warning FIXME: keys
#if 0
  for (i = 0; i < MUIKEY_COUNT; i++)
	g_free(prefs->muikeys[i].readable_hotkey);
#endif
 
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
