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
#include <zunepriv.h>
#include <builtin.h>
#include <prefs.h>
#include <pen.h>
#include <font.h>

#include <Configdata.h>
#include <configdatadata.h>
#include <stdio.h>
#include <Dataspace.h>
#include <dataspacedata.h>

#define ZG_PENS          _U("pens")

#define ZG_WINDOWS          "windows"
#define ZG_WINDOWFONTS      _U(ZG_WINDOWS ".fonts")
#define ZG_WINDOWINNERSPACE _U(ZG_WINDOWS ".innerSpace")
#define ZG_WINDOWBACKS      _U(ZG_WINDOWS ".background")

#define ZG_GROUPS        "groups"
#define ZG_GROUPTITLE    _U(ZG_GROUPS ".title")
#define ZG_GROUPBACKS    _U(ZG_GROUPS ".background")
#define ZG_GROUPFRAMES   _U(ZG_GROUPS ".frame")
#define ZG_GROUPSPACING  _U(ZG_GROUPS ".spacing")

#define ZG_BUTTONS       "buttons"
#define ZG_TEXTBUTTONS   _U(ZG_BUTTONS ".textButtons")
#define ZG_IMAGEBUTTONS  _U(ZG_BUTTONS ".imageButtons")
#define ZG_CHECKMARKS    _U(ZG_BUTTONS ".checkMarks")
#define ZG_RADIOBUTTONS  _U(ZG_BUTTONS ".radioButtons")

#define ZG_CYCLE         "cycles"
#define ZG_CYCLECONTROL  _U(ZG_CYCLE ".popupControl")
#define ZG_CYCLELOOK     _U(ZG_CYCLE ".popupLook")

#define ZG_SLIDER          "sliders"
#define ZG_SLIDERCONTAINER _U(ZG_SLIDER ".container")
#define ZG_SLIDERKNOB      _U(ZG_SLIDER ".knob")

#define ZG_SCROLLBARS      "scrollbars"
#define ZG_SBARROW         _U(ZG_SCROLLBARS ".arrow")
#define ZG_SBKNOB          _U(ZG_SCROLLBARS ".knob")

#define ZG_LISTS           "lists"
#define ZG_LISTFONTS       _U(ZG_LISTS ".fonts")
#define ZG_LISTINPUT       _U(ZG_LISTS ".inputList")
#define ZG_LISTREAD        _U(ZG_LISTS ".readList")
#define ZG_LISTCURSOR      _U(ZG_LISTS ".cursor")

#define ZG_STRINGS         "strings"
#define ZG_STRINGPOPUP     _U(ZG_STRINGS ".popupButtons")
#define ZG_STRINGINACTIVE  _U(ZG_STRINGS ".inactiveColors")
#define ZG_STRINGACTIVE    _U(ZG_STRINGS ".activeColors")

#define ZG_DND             _U("dragndrop")

#define ZG_BALANCING       _U("balancing")

#define ZG_KEYBOARD        _U("keyboard")

#define ZG_TEXTFIELD       _U("textField")
#define ZG_GAUGE           _U("gauge")

#define ZG_IMAGES          "images"
#define ZG_DEVICESIMAGES   _U(ZG_IMAGES ".devices")
#define ZG_TAPESIMAGES     _U(ZG_IMAGES ".tapes")

/*  #define ZG_INTERNAL        "internal" */
/*  #define ZG_PREFSNOTIFYDELAY "internal" ".prefsNotifyDelay" */

#define ZK_SHINE       _U("0_shine")
#define ZK_HALFSHINE   _U("1_halfShine")
#define ZK_BACKGROUND  _U("2_background")
#define ZK_HALFSHADOW  _U("3_halfShadow")
#define ZK_SHADOW      _U("4_shadow")
#define ZK_TEXT        _U("5_text")
#define ZK_FILL        _U("6_fill")
#define ZK_MARK        _U("7_mark")

static STRPTR ZK_KEYS[] = {
    "press",
    "toggle",
    "up",
    "down",
    "pageUp",
    "pageDown",
    "top",
    "bottom",
    "left",
    "right",
    "wordLeft",
    "wordRight",
    "lineStart",
    "lineEnd",
    "gadgetNext",
    "gadgetPrev",
    "gadgetOff",
    "windowClose",
    "windowNext",
    "windowPrev",
    "help",
    "popup"
};

static ULONG
mNew (struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ConfigdataData *data;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);
    return (ULONG)obj;
}


static ULONG
mAddPens (struct IClass *cl, Object *obj, struct MUIP_Configdata_AddPens *msg)
{
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_PENS, ZK_SHINE,
	     _U(zune_gdkcolor_to_string(&msg->prefs->muipens[MPEN_SHINE])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_PENS, ZK_HALFSHINE,
	     _U(zune_gdkcolor_to_string(&msg->prefs->muipens[MPEN_HALFSHINE])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_PENS, ZK_BACKGROUND,
	     _U(zune_gdkcolor_to_string(&msg->prefs->muipens[MPEN_BACKGROUND])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_PENS, ZK_HALFSHADOW,
	     _U(zune_gdkcolor_to_string(&msg->prefs->muipens[MPEN_HALFSHADOW])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_PENS, ZK_SHADOW,
	     _U(zune_gdkcolor_to_string(&msg->prefs->muipens[MPEN_SHADOW])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_PENS, ZK_TEXT,
	     _U(zune_gdkcolor_to_string(&msg->prefs->muipens[MPEN_TEXT])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_PENS, ZK_FILL,
	     _U(zune_gdkcolor_to_string(&msg->prefs->muipens[MPEN_FILL])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_PENS, ZK_MARK,
	     _U(zune_gdkcolor_to_string(&msg->prefs->muipens[MPEN_MARK])));
    return TRUE;
}

static ULONG
mAddWindows (struct IClass *cl, Object *obj, struct MUIP_Configdata_AddWindows *msg)
{
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_WINDOWFONTS, _U("normal"),
	     _U(msg->prefs->window_font_normal));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_WINDOWFONTS, _U("small"),
	     _U(msg->prefs->window_font_small));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_WINDOWFONTS, _U("big"),
	     _U(msg->prefs->window_font_big));

    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_WINDOWINNERSPACE, _U("left"),
	     _U(msg->prefs->window_inner_left));
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_WINDOWINNERSPACE, _U("right"),
	     _U(msg->prefs->window_inner_right));
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_WINDOWINNERSPACE, _U("top"),
	     _U(msg->prefs->window_inner_top));
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_WINDOWINNERSPACE, _U("bottom"),
	     _U(msg->prefs->window_inner_bottom));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_WINDOWBACKS, _U("window"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_WindowBack])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_WINDOWBACKS, _U("request"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_RequesterBack])));
    return TRUE;
}

static ULONG
mAddGroups (struct IClass *cl, Object *obj, struct MUIP_Configdata_AddGroups *msg)
{
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_GROUPBACKS, _U("framed"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_GroupBack])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_GROUPBACKS, _U("page"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_PageBack])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_GROUPBACKS, _U("register"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_RegisterBack])));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_GROUPFRAMES, _U("normal"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_Group])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_GROUPFRAMES, _U("virtual"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_Virtual])));

    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_GROUPSPACING, _U("horizontal"),
	     msg->prefs->group_hspacing);
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_GROUPSPACING, _U("vertical"),
	     msg->prefs->group_vspacing);

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_GROUPTITLE, _U("font"),
	     _U(msg->prefs->group_title_font));
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_GROUPTITLE, _U("position"),
	     msg->prefs->group_title_position);
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_GROUPTITLE, _U("color"),
	     msg->prefs->group_title_color);
    return TRUE;
}

static ULONG
mAddButtons (struct IClass *cl, Object *obj, struct MUIP_Configdata_AddButtons *msg)
{
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_TEXTBUTTONS, _U("frame"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_Button])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_TEXTBUTTONS, _U("back"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_ButtonBack])));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_TEXTBUTTONS, _U("selectedBack"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_SelectedBack])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_TEXTBUTTONS, _U("font"),
	     _U(msg->prefs->textbutton_font));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_IMAGEBUTTONS, _U("frame"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_ImageButton])));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_CHECKMARKS, _U("image"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_CheckMark])));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_RADIOBUTTONS, _U("image"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_RadioButton])));
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_RADIOBUTTONS, _U("horizSpacing"),
	     msg->prefs->radiobutton_hspacing);
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_RADIOBUTTONS, _U("vertSpacing"),
	     msg->prefs->radiobutton_vspacing);

    return TRUE;
}

static ULONG
mAddCycles (struct IClass *cl, Object *obj, struct MUIP_Configdata_AddCycles *msg)
{
    DoMethod(obj, MUIM_Dataspace_AddString, _U(ZG_CYCLE), _U("image"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_Cycle])));

    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_CYCLECONTROL, _U("position"),
	     msg->prefs->cycle_menu_position);
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_CYCLECONTROL, _U("minEntries"),
	     msg->prefs->cycle_menu_min_entries);
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_CYCLECONTROL, _U("speed"),
	     msg->prefs->cycle_menu_speed);

    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_CYCLELOOK, _U("recessedEntries"),
	     msg->prefs->cycle_menu_recessed_entries);
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_CYCLELOOK, _U("frame"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_PopUp])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_CYCLELOOK, _U("image"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_PopupBack])));
    return TRUE;
}

static ULONG
mAddSliders (struct IClass *cl, Object *obj, struct MUIP_Configdata_AddSliders *msg)
{
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_SLIDERCONTAINER, _U("back"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_SliderBack])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_SLIDERCONTAINER, _U("frame"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_Slider])));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_SLIDERKNOB, _U("back"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_SliderKnob])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_SLIDERKNOB, _U("frame"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_Knob])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_SLIDERKNOB, _U("font"),
	     _U(msg->prefs->slider_knob_font));
    return TRUE;
}

static ULONG
mAddScrollbars (struct IClass *cl, Object *obj, struct MUIP_Configdata_AddScrollbars *msg)
{
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_SBARROW, _U("up"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_ArrowUp])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_SBARROW, _U("down"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_ArrowDown])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_SBARROW, _U("left"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_ArrowLeft])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_SBARROW, _U("right"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_ArrowRight])));

    DoMethod(obj, MUIM_Dataspace_AddString, _U(ZG_SCROLLBARS), _U("frame"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_Prop])));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_SBKNOB, _U("image"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_PropKnob])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_SBKNOB, _U("back"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_PropBack])));

    DoMethod(obj, MUIM_Dataspace_AddInt, _U(ZG_SCROLLBARS), _U("look"),
	     msg->prefs->sb_look);
    return TRUE;
}

static ULONG
mAddListviews (struct IClass *cl, Object *obj, struct MUIP_Configdata_AddListviews *msg)
{
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_LISTFONTS, _U("normal"),
	     _U(msg->prefs->list_font_normal));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_LISTFONTS, _U("fixed"),
	     _U(msg->prefs->list_font_fixed));
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_LISTFONTS, _U("lineSpacing"),
	     msg->prefs->list_linespacing);

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_LISTREAD, _U("frame"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_ReadList])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_LISTINPUT, _U("frame"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_InputList])));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_LISTREAD, _U("back"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_ReadListBack])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_LISTINPUT, _U("back"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_ListBack])));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_LISTCURSOR, _U("active"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_ListCursor])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_LISTCURSOR, _U("selected"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_ListSelect])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_LISTCURSOR, _U("activeSelected"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_ListSelCur])));
    return TRUE;
}

static ULONG
mAddStrings (struct IClass *cl, Object *obj, struct MUIP_Configdata_AddStrings *msg)
{
    DoMethod(obj, MUIM_Dataspace_AddString, _U(ZG_STRINGS), _U("frame"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_String])));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_STRINGPOPUP, _U("default"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_PopUp])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_STRINGPOPUP, _U("file"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_PopFile])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_STRINGPOPUP, _U("drawer"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_PopDrawer])));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_STRINGINACTIVE, _U("back"),
	     _U(zune_penspec_to_string(&msg->prefs->string_bg_inactive)));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_STRINGINACTIVE, _U("text"),
	     _U(zune_penspec_to_string(&msg->prefs->string_text_inactive)));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_STRINGACTIVE, _U("back"),
	     _U(zune_penspec_to_string(&msg->prefs->string_bg_active)));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_STRINGACTIVE, _U("text"),
	     _U(zune_penspec_to_string(&msg->prefs->string_text_active)));
    return TRUE;
}

static ULONG
mAddNavigation (struct IClass *cl, Object *obj, struct MUIP_Configdata_AddNavigation *msg)
{
    int i;

    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_DND, _U("leftButton"),
	     msg->prefs->dragndrop_left_button);
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_DND, _U("leftButtonModifier"),
	     _U(msg->prefs->dragndrop_left_modifier));
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_DND, _U("middleButton"),
	     _U(msg->prefs->dragndrop_middle_button));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_DND, _U("middleButtonModifier"),
	     _U(msg->prefs->dragndrop_middle_modifier));
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_DND, _U("autoStartLength"),
	     msg->prefs->dragndrop_autostart);
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_DND, _U("frame"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_Drag])));
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_DND, _U("look"),
	     msg->prefs->dragndrop_look);
    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_BALANCING, _U("look"),
	     msg->prefs->balancing_look);

    DoMethod(obj, MUIM_Dataspace_AddInt, ZG_KEYBOARD, _U("activeObjectLook"),
	     msg->prefs->active_object_look);
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_KEYBOARD, _U("activeObjectColor"),
	     _U(zune_penspec_to_string(&msg->prefs->active_object_color)));

    for (i = 0; i < MUIKEY_COUNT; i++)
    {
	DoMethod(obj, MUIM_Dataspace_AddString, _U(ZG_KEYBOARD), _U(ZK_KEYS[i]),
		 _U(msg->prefs->muikeys[i].readable_hotkey));
    }
    return TRUE;
}

static ULONG
mAddSpecial (struct IClass *cl, Object *obj, struct MUIP_Configdata_AddSpecial *msg)
{
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_TEXTFIELD, _U("frame"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_Text])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_TEXTFIELD, _U("back"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_TextBack])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_GAUGE, _U("frame"),
	     _U(zune_framespec_to_string(&msg->prefs->frames[MUIV_Frame_Gauge])));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_DEVICESIMAGES, _U("drawer"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_Drawer])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_DEVICESIMAGES, _U("hardDisk"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_HardDisk])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_DEVICESIMAGES, _U("disk"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_Disk])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_DEVICESIMAGES, _U("chip"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_Chip])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_DEVICESIMAGES, _U("volume"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_Volume])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_DEVICESIMAGES, _U("network"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_Network])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_DEVICESIMAGES, _U("assign"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_Assign])));

    DoMethod(obj, MUIM_Dataspace_AddString, ZG_TAPESIMAGES, _U("play"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_TapePlay])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_TAPESIMAGES, _U("playBack"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_TapePlayBack])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_TAPESIMAGES, _U("pause"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_TapePause])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_TAPESIMAGES, _U("stop"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_TapeStop])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_TAPESIMAGES, _U("record"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_TapeRecord])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_TAPESIMAGES, _U("up"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_TapeUp])));
    DoMethod(obj, MUIM_Dataspace_AddString, ZG_TAPESIMAGES, _U("down"),
	     _U(zune_imspec_to_string(msg->prefs->images[MUII_TapeDown])));

    return TRUE;
}


/*
 * Reading prefs
 */

static ULONG
mFindPens (struct IClass *cl, Object *obj, struct MUIP_Configdata_FindPens *msg)
{
    zune_string_to_gdkcolor(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, ZG_PENS, ZK_SHINE),
	&msg->prefs->muipens[MPEN_SHINE]);
    zune_string_to_gdkcolor(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, ZG_PENS, ZK_HALFSHINE),
	&msg->prefs->muipens[MPEN_HALFSHINE]);
    zune_string_to_gdkcolor(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, ZG_PENS, ZK_BACKGROUND),
	&msg->prefs->muipens[MPEN_BACKGROUND]);
    zune_string_to_gdkcolor(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, ZG_PENS, ZK_HALFSHADOW),
	&msg->prefs->muipens[MPEN_HALFSHADOW]);
    zune_string_to_gdkcolor(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, ZG_PENS, ZK_SHADOW),
	&msg->prefs->muipens[MPEN_SHADOW]);
    zune_string_to_gdkcolor(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, ZG_PENS, ZK_TEXT),
	&msg->prefs->muipens[MPEN_TEXT]);
    zune_string_to_gdkcolor(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, ZG_PENS, ZK_FILL),
	&msg->prefs->muipens[MPEN_FILL]);
    zune_string_to_gdkcolor(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, ZG_PENS, ZK_MARK),
	&msg->prefs->muipens[MPEN_MARK]);
    return TRUE;
}

static ULONG
mFindWindows (struct IClass *cl, Object *obj, struct MUIP_Configdata_FindWindows *msg)
{
    STRPTR s;

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_WINDOWFONTS, _U("normal"))))
    {
	g_free(msg->prefs->window_font_normal);
	msg->prefs->window_font_normal = g_strdup(s);
    }
    zune_font_replace(&msg->prefs->fonts[-MUIV_Font_Normal],
		      msg->prefs->window_font_normal);
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_WINDOWFONTS, _U("small"))))
    {
	g_free(msg->prefs->window_font_small);
	msg->prefs->window_font_small = g_strdup(s);
    }
    zune_font_replace(&msg->prefs->fonts[-MUIV_Font_Tiny],
		      msg->prefs->window_font_small);
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_WINDOWFONTS, _U("big"))))
    {
	g_free(msg->prefs->window_font_big);
	msg->prefs->window_font_big = g_strdup(s);
    }
    zune_font_replace(&msg->prefs->fonts[-MUIV_Font_Big],
		      msg->prefs->window_font_big);

    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_WINDOWINNERSPACE, _U("left")))
	msg->prefs->window_inner_left =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_WINDOWINNERSPACE, _U("left"));
    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_WINDOWINNERSPACE, _U("right")))
	msg->prefs->window_inner_right =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_WINDOWINNERSPACE, _U("right"));
    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_WINDOWINNERSPACE, _U("top")))
	msg->prefs->window_inner_top =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_WINDOWINNERSPACE, _U("top"));
    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_WINDOWINNERSPACE, _U("bottom")))
	msg->prefs->window_inner_bottom =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_WINDOWINNERSPACE, _U(_U("bottom")));

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_WINDOWBACKS, _U("window"))))
	zune_link_rebind(msg->prefs->images[MUII_WindowBack],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_WINDOWBACKS, _U("request"))))
	zune_link_rebind(msg->prefs->images[MUII_RequesterBack],
			 zune_image_spec_to_structure((ULONG)s));
    return TRUE;
}

static ULONG
mFindGroups (struct IClass *cl, Object *obj, struct MUIP_Configdata_FindGroups *msg)
{
    STRPTR s;

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_GROUPBACKS, _U("framed"))))
	zune_link_rebind(msg->prefs->images[MUII_GroupBack],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_GROUPBACKS, _U("page"))))
	zune_link_rebind(msg->prefs->images[MUII_PageBack],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_GROUPBACKS, _U("register"))))
	zune_link_rebind(msg->prefs->images[MUII_RegisterBack],
			 zune_image_spec_to_structure((ULONG)s));

    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_GROUPFRAMES, _U("normal")),
	&msg->prefs->frames[MUIV_Frame_Group]);
    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_GROUPFRAMES, _U("virtual")),
	&msg->prefs->frames[MUIV_Frame_Virtual]);

    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_GROUPSPACING, _U("horizontal")))
	msg->prefs->group_hspacing =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_GROUPSPACING, _U("horizontal"));
    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_GROUPSPACING, _U("vertical")))
	msg->prefs->group_vspacing =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_GROUPSPACING, _U("vertical"));

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_GROUPTITLE, _U("font"))))
    {
	g_free(msg->prefs->group_title_font);
	msg->prefs->group_title_font = g_strdup(s);
    }
    zune_font_replace(&msg->prefs->fonts[-MUIV_Font_Title],
		      msg->prefs->group_title_font);

    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_GROUPTITLE, _U("position")))
	msg->prefs->group_title_position =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_GROUPTITLE, _U("position"));
    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_GROUPTITLE, _U("color")))
	msg->prefs->group_title_color =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_GROUPTITLE, _U("color"));

    return TRUE;
}

static ULONG
mFindButtons (struct IClass *cl, Object *obj, struct MUIP_Configdata_FindButtons *msg)
{
    STRPTR s;

/*  g_print(_U("mFindButtons : reading textButtons.back\n")); */
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_TEXTBUTTONS, _U("back"))))
	zune_link_rebind(msg->prefs->images[MUII_ButtonBack],
			 zune_image_spec_to_structure((ULONG)s));

/*  g_print(_U("mFindButtons : reading textButtons.selectedBack\n")); */
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_TEXTBUTTONS, _U("selectedBack"))))
	zune_link_rebind(msg->prefs->images[MUII_SelectedBack],
			 zune_image_spec_to_structure((ULONG)s));

    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_TEXTBUTTONS, _U("frame")),
	&msg->prefs->frames[MUIV_Frame_Button]);

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_TEXTBUTTONS, _U("font"))))
    {
	g_free(msg->prefs->textbutton_font);
	msg->prefs->textbutton_font = g_strdup(s);
    }
    zune_font_replace(&msg->prefs->fonts[-MUIV_Font_Button],
		      msg->prefs->textbutton_font);

    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_IMAGEBUTTONS, _U("frame")),
	&msg->prefs->frames[MUIV_Frame_ImageButton]);

/*  g_print(_U("mFindButtons : reading checkMarks.image\n")); */
    s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_CHECKMARKS, _U("image"));
    if (s)
	zune_link_rebind(msg->prefs->images[MUII_CheckMark],
			 zune_image_spec_to_structure((ULONG)s));

/*  g_print(_U("mFindButtons : reading radioButtons.image\n")); */
    s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_RADIOBUTTONS, _U("image"));
    if (s)
	zune_link_rebind(msg->prefs->images[MUII_RadioButton],
			 zune_image_spec_to_structure((ULONG)s));

    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_RADIOBUTTONS, _U("horizSpacing")))
	msg->prefs->radiobutton_hspacing =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_RADIOBUTTONS, _U("horizSpacing"));

    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_RADIOBUTTONS, _U("vertSpacing")))
	msg->prefs->radiobutton_vspacing =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_RADIOBUTTONS, _U("vertSpacing"));

    return TRUE;
}

static ULONG
mFindCycles (struct IClass *cl, Object *obj, struct MUIP_Configdata_FindCycles *msg)
{
    STRPTR s;

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 _U(ZG_CYCLE), _U("image"))))
	zune_link_rebind(msg->prefs->images[MUII_Cycle],
			 zune_image_spec_to_structure((ULONG)s));

    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_CYCLECONTROL, _U("position")))
	msg->prefs->cycle_menu_position =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_CYCLECONTROL, _U("position"));
    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_CYCLECONTROL, _U("minEntries")))
	msg->prefs->cycle_menu_min_entries =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_CYCLECONTROL, _U("minEntries"));
    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_CYCLECONTROL, _U("speed")))
	msg->prefs->cycle_menu_speed =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_CYCLECONTROL, _U("speed"));

    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_CYCLELOOK, _U("recessedEntries")))
	msg->prefs->cycle_menu_recessed_entries =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_CYCLELOOK, _U("recessedEntries"));

    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_CYCLELOOK, _U("frame")),
	&msg->prefs->frames[MUIV_Frame_PopUp]);
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_CYCLELOOK, _U("image"))))
	zune_link_rebind(msg->prefs->images[MUII_PopupBack],
			 zune_image_spec_to_structure((ULONG)s));

    return TRUE;
}

static ULONG
mFindSliders (struct IClass *cl, Object *obj, struct MUIP_Configdata_FindSliders *msg)
{
    STRPTR s;

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_SLIDERCONTAINER, _U("back"))))
	zune_link_rebind(msg->prefs->images[MUII_SliderBack],
			 zune_image_spec_to_structure((ULONG)s));
    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_SLIDERCONTAINER, _U("frame")),
	&msg->prefs->frames[MUIV_Frame_Slider]);

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_SLIDERKNOB, _U("back"))))
	zune_link_rebind(msg->prefs->images[MUII_SliderKnob],
			 zune_image_spec_to_structure((ULONG)s));
    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_SLIDERKNOB, _U("frame")),
	&msg->prefs->frames[MUIV_Frame_Knob]);

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_SLIDERKNOB, _U("font"))))
    {
	g_free(msg->prefs->slider_knob_font);
	msg->prefs->slider_knob_font = g_strdup(s);
    }
    zune_font_replace(&msg->prefs->fonts[-MUIV_Font_Knob],
		      msg->prefs->slider_knob_font);

    return TRUE;
}

static ULONG
mFindScrollbars (struct IClass *cl, Object *obj, struct MUIP_Configdata_FindScrollbars *msg)
{
    STRPTR s;

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_SBARROW, _U("up"))))
	zune_link_rebind(msg->prefs->images[MUII_ArrowUp],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_SBARROW, _U("down"))))
	zune_link_rebind(msg->prefs->images[MUII_ArrowDown],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_SBARROW, _U("left"))))
	zune_link_rebind(msg->prefs->images[MUII_ArrowLeft],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_SBARROW, _U("right"))))
	zune_link_rebind(msg->prefs->images[MUII_ArrowRight],
			 zune_image_spec_to_structure((ULONG)s));

    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 _U(ZG_SCROLLBARS), _U("frame")),
	&msg->prefs->frames[MUIV_Frame_Prop]);

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_SBKNOB, _U("image"))))
	zune_link_rebind(msg->prefs->images[MUII_PropKnob],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_SBKNOB, _U("back"))))
	zune_link_rebind(msg->prefs->images[MUII_PropBack],
			 zune_image_spec_to_structure((ULONG)s));

    if (DoMethod(obj, MUIM_Dataspace_FindString, _U(ZG_SCROLLBARS), _U("look")))
	msg->prefs->sb_look =
	    DoMethod(obj, MUIM_Dataspace_FindInt, _U(ZG_SCROLLBARS), _U("look"));
    return TRUE;
}

static ULONG
mFindListviews (struct IClass *cl, Object *obj, struct MUIP_Configdata_FindListviews *msg)
{
    STRPTR s;

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_LISTFONTS, _U("normal"))))
    {
	g_free(msg->prefs->list_font_normal);
	msg->prefs->list_font_normal = g_strdup(s);
    }
    zune_font_replace(&msg->prefs->fonts[-MUIV_Font_List],
		      msg->prefs->list_font_normal);
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_LISTFONTS, _U("fixed"))))
    {
	g_free(msg->prefs->list_font_fixed);
	msg->prefs->list_font_fixed = g_strdup(s);
    }
    zune_font_replace(&msg->prefs->fonts[-MUIV_Font_Fixed],
		      msg->prefs->list_font_fixed);

    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_LISTFONTS, _U("lineSpacing")))
	msg->prefs->list_linespacing =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_LISTFONTS, _U("lineSpacing"));

    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, ZG_LISTREAD, _U("frame")),
	&msg->prefs->frames[MUIV_Frame_ReadList]);
    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, ZG_LISTINPUT, _U("frame")),
	&msg->prefs->frames[MUIV_Frame_InputList]);

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_LISTREAD, _U("back"))))
	zune_link_rebind(msg->prefs->images[MUII_ReadListBack],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_LISTINPUT, _U("back"))))
	zune_link_rebind(msg->prefs->images[MUII_ListBack],
			 zune_image_spec_to_structure((ULONG)s));

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_LISTCURSOR, _U("active"))))
	zune_link_rebind(msg->prefs->images[MUII_ListCursor],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_LISTCURSOR, _U("selected"))))
	zune_link_rebind(msg->prefs->images[MUII_ListSelect],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_LISTCURSOR, _U("activeSelected"))))
	zune_link_rebind(msg->prefs->images[MUII_ListSelCur],
			 zune_image_spec_to_structure((ULONG)s));
    return TRUE;
}

static ULONG
mFindStrings (struct IClass *cl, Object *obj, struct MUIP_Configdata_FindStrings *msg)
{
    STRPTR s;

    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, _U("strings"), _U("frame")),
	&msg->prefs->frames[MUIV_Frame_String]);

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_STRINGPOPUP, _U("default"))))
	zune_link_rebind(msg->prefs->images[MUII_PopUp],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_STRINGPOPUP, _U("file"))))
	zune_link_rebind(msg->prefs->images[MUII_PopFile],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_STRINGPOPUP, _U("drawer"))))
	zune_link_rebind(msg->prefs->images[MUII_PopDrawer],
			 zune_image_spec_to_structure((ULONG)s));

    zune_string_to_penspec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_STRINGINACTIVE, _U("back")),
	&msg->prefs->string_bg_inactive);
    zune_string_to_penspec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_STRINGINACTIVE, _U("text")),
	&msg->prefs->string_text_inactive);
    zune_string_to_penspec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_STRINGACTIVE, _U("back")),
	&msg->prefs->string_bg_active);
    zune_string_to_penspec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			ZG_STRINGACTIVE , _U("text")),
	&msg->prefs->string_text_active);
    return TRUE;
}

static ULONG
mFindNavigation (struct IClass *cl, Object *obj, struct MUIP_Configdata_FindNavigation *msg)
{
    STRPTR s;
    int i;

    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_DND, _U("leftButton")))
	msg->prefs->dragndrop_left_button =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_DND, _U("leftButton"));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_DND, _U("leftButtonModifier"))))
    {
	g_free(msg->prefs->dragndrop_left_modifier);
	msg->prefs->dragndrop_left_modifier = g_strdup(s);
    }

    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_DND, _U("middleButton")))
	msg->prefs->dragndrop_middle_button =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_DND, _U("middleButton"));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_DND, _U("middleButtonModifier"))))
    {
	g_free(msg->prefs->dragndrop_middle_modifier);
	msg->prefs->dragndrop_middle_modifier = g_strdup(s);
    }

    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_DND, _U("autoStartLength")))
	msg->prefs->dragndrop_autostart =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_DND, _U("autoStartLength"));
    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, ZG_DND, _U("frame")),
	&msg->prefs->frames[MUIV_Frame_Drag]);

    if (DoMethod(obj, MUIM_Dataspace_FindString, ZG_KEYBOARD, _U("activeObjectLook")))
	msg->prefs->active_object_look =
	    DoMethod(obj, MUIM_Dataspace_FindInt, ZG_KEYBOARD, _U("activeObjectLook"));

    zune_string_to_penspec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_KEYBOARD, _U("activeObjectColor")),
	&msg->prefs->active_object_color);


/* keys */
    for (i = 0; i < MUIKEY_COUNT; i++)
    {
	if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
				  ZG_KEYBOARD, _U(ZK_KEYS[i]))))
	{
	    g_free(msg->prefs->muikeys[i].readable_hotkey);
	    msg->prefs->muikeys[i].readable_hotkey = g_strdup(s);
	}
    }

    return TRUE;
}

static ULONG
mFindSpecial (struct IClass *cl, Object *obj, struct MUIP_Configdata_FindSpecial *msg)
{
    STRPTR s;

    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, ZG_TEXTFIELD, _U("frame")),
	&msg->prefs->frames[MUIV_Frame_Text]);
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			      ZG_TEXTFIELD, _U("back"))))
	zune_link_rebind(msg->prefs->images[MUII_TextBack],
			 zune_image_spec_to_structure((ULONG)s));
    zune_string_to_framespec(
	(STRPTR)DoMethod(obj, MUIM_Dataspace_FindString, ZG_GAUGE, _U("frame")),
	&msg->prefs->frames[MUIV_Frame_Gauge]);

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_DEVICESIMAGES, _U("drawer"))))
	zune_link_rebind(msg->prefs->images[MUII_Drawer],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_DEVICESIMAGES, _U("hardDisk"))))
	zune_link_rebind(msg->prefs->images[MUII_HardDisk],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_DEVICESIMAGES, _U("disk"))))
	zune_link_rebind(msg->prefs->images[MUII_Disk],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_DEVICESIMAGES, _U("chip"))))
	zune_link_rebind(msg->prefs->images[MUII_Chip],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_DEVICESIMAGES, _U("volume"))))
	zune_link_rebind(msg->prefs->images[MUII_Volume],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_DEVICESIMAGES, _U("network"))))
	zune_link_rebind(msg->prefs->images[MUII_Network],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_DEVICESIMAGES, _U("assign"))))
	zune_link_rebind(msg->prefs->images[MUII_Assign],
			 zune_image_spec_to_structure((ULONG)s));

    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_TAPESIMAGES, _U("play"))))
	zune_link_rebind(msg->prefs->images[MUII_TapePlay],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_TAPESIMAGES, _U("playBack"))))
	zune_link_rebind(msg->prefs->images[MUII_TapePlayBack],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_TAPESIMAGES, _U("pause"))))
	zune_link_rebind(msg->prefs->images[MUII_TapePause],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_TAPESIMAGES, _U("stop"))))
	zune_link_rebind(msg->prefs->images[MUII_TapeStop],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_TAPESIMAGES, _U("record"))))
	zune_link_rebind(msg->prefs->images[MUII_TapeRecord],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_TAPESIMAGES, _U("up"))))
	zune_link_rebind(msg->prefs->images[MUII_TapeUp],
			 zune_image_spec_to_structure((ULONG)s));
    if ((s = (STRPTR)DoMethod(obj, MUIM_Dataspace_FindString,
			 ZG_TAPESIMAGES, _U("down"))))
	zune_link_rebind(msg->prefs->images[MUII_TapeDown],
			 zune_image_spec_to_structure((ULONG)s));

    return TRUE;
}



/*
 * The class dispatcher
 */
static ULONG 
MyDispatcher (struct IClass *cl, Object *obj, Msg msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
	return(mNew(cl, obj, (struct opSet *) msg));
    case MUIM_Configdata_AddPens:
	return(mAddPens(cl, obj, (APTR)msg));
    case MUIM_Configdata_AddWindows:
	return(mAddWindows(cl, obj, (APTR)msg));
    case MUIM_Configdata_AddGroups:
	return(mAddGroups(cl, obj, (APTR)msg));
    case MUIM_Configdata_AddButtons:
	return(mAddButtons(cl, obj, (APTR)msg));
    case MUIM_Configdata_AddCycles:
	return(mAddCycles(cl, obj, (APTR)msg));
    case MUIM_Configdata_AddSliders:
	return(mAddSliders(cl, obj, (APTR)msg));
    case MUIM_Configdata_AddScrollbars:
	return(mAddScrollbars(cl, obj, (APTR)msg));
    case MUIM_Configdata_AddListviews:
	return(mAddListviews(cl, obj, (APTR)msg));
    case MUIM_Configdata_AddStrings:
	return(mAddStrings(cl, obj, (APTR)msg));
    case MUIM_Configdata_AddNavigation:
	return(mAddNavigation(cl, obj, (APTR)msg));
    case MUIM_Configdata_AddSpecial:
	return(mAddSpecial(cl, obj, (APTR)msg));
    case MUIM_Configdata_FindPens:
	return(mFindPens(cl, obj, (APTR)msg));
    case MUIM_Configdata_FindWindows:
	return(mFindWindows(cl, obj, (APTR)msg));
    case MUIM_Configdata_FindGroups:
	return(mFindGroups(cl, obj, (APTR)msg));
    case MUIM_Configdata_FindButtons:
	return(mFindButtons(cl, obj, (APTR)msg));
    case MUIM_Configdata_FindCycles:
	return(mFindCycles(cl, obj, (APTR)msg));
    case MUIM_Configdata_FindSliders:
	return(mFindSliders(cl, obj, (APTR)msg));
    case MUIM_Configdata_FindScrollbars:
	return(mFindScrollbars(cl, obj, (APTR)msg));
    case MUIM_Configdata_FindListviews:
	return(mFindListviews(cl, obj, (APTR)msg));
    case MUIM_Configdata_FindStrings:
	return(mFindStrings(cl, obj, (APTR)msg));
    case MUIM_Configdata_FindNavigation:
	return(mFindNavigation(cl, obj, (APTR)msg));
    case MUIM_Configdata_FindSpecial:
	return(mFindSpecial(cl, obj, (APTR)msg));
    }

    return(DoSuperMethodA(cl, obj, msg));
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Configdata_desc = {
    MUIC_Configdata,                    /* Class name */
    MUIC_Dataspace,                     /* super class name */
    sizeof(struct MUI_ConfigdataData),  /* size of class own datas */
    MyDispatcher                        /* class dispatcher */
};

