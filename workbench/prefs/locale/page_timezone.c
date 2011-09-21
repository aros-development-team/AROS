/*
   Copyright � 1995-2011, The AROS Development Team. All rights reserved.
   $Id$

Desc:
Lang: English
 */

/*************************************************************************/

// #define MUIMASTER_YES_INLINE_STDARG

#include <cybergraphx/cybergraphics.h>

//#define DEBUG 1
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <stdio.h>

#include <aros/debug.h>

#include "page_timezone.h"
#include "registertab.h"
#include "locale.h"
#include "misc.h"
#include "cities.h"

#include "earthmap_small_image.c"
#include "timezones_small_image.c"

/*************************************************************************/

#define H 16

#define DOUBLE_ARROWS           1

#define CONTINENT_RED           18
#define CONTINENT_GREEN         114
#define CONTINENT_BLUE          58

#define OCEAN_RED               21
#define OCEAN_GREEN             18
#define OCEAN_BLUE              114

#define SELECTED_INTENSITY_INC  80

#define GAD_DEC                 1000
#define GAD_INC                 1001
#define GAD_TEXT                1002

/*************************************************************************/

#define NUM_TIMEZONES 36

static struct timezone
{
    LONG id;
    LONG minoffset;
    LONG pen;
}

timezone_table[NUM_TIMEZONES] =
{
    {  0,   0           , -1 }, /* Z :    0:00 */
    {  1,   1 * 60      , -1 }, /* A : +  1:00 */
    {  2,   2 * 60      , -1 }, /* B : +  2:00 */
    {  3,   3 * 60      , -1 }, /* C : +  3:00 */
    {  4,   3 * 60 + 30 , -1 }, /* C*: +  3:30 */
    {  5,   4 * 60      , -1 }, /* D : +  4:00 */
    {  6,   4 * 60 + 30 , -1 }, /* D*: +  4:30 */
    {  7,   5 * 60      , -1 }, /* E : +  5:00 */
    {  8,   5 * 60 + 30 , -1 }, /* E*: +  5:30 */
    {  9,   6 * 60      , -1 }, /* F : +  6:00 */
    { 10,   6 * 60 + 30 , -1 }, /* F*: +  6:30 */
    { 11,   7 * 60      , -1 }, /* G : +  7:00 */
    { 12,   8 * 60      , -1 }, /* H : +  8:00 */
    { 13,   9 * 60      , -1 }, /* I : +  9:00 */
    { 14,   9 * 60 + 30 , -1 }, /* I*: +  9:30 */
    { 15,  10 * 60      , -1 }, /* K : + 10:00 */
    { 16,  10 * 60 + 30 , -1 }, /* K*: + 10:30 */
    { 17,  11 * 60      , -1 }, /* L : + 11:00 */
    { 18,  11 * 60 + 30 , -1 }, /* L*: + 11:30 */
    { 19,  12 * 60      , -1 }, /* M : + 12:00 */
    { 20,  13 * 60      , -1 }, /* M*: + 13:00 */
    { 35, -12 * 60      , -1 }, /* Y : - 12:00 */
    { 34, -11 * 60      , -1 }, /* X : - 11:00 */
    { 33, -10 * 60      , -1 }, /* W : - 10:00 */
    { 32,  -9 * 60 - 30 , -1 }, /* V*: -  9:30 */
    { 31,  -9 * 60      , -1 }, /* V : -  9:00 */
    { 30,  -8 * 60 - 30 , -1 }, /* U*: -  8:30 */
    { 29,  -8 * 60      , -1 }, /* U : -  8:00 */
    { 28,  -7 * 60      , -1 }, /* T : -  7:00 */
    { 27,  -6 * 60      , -1 }, /* S : -  6:00 */
    { 26,  -5 * 60      , -1 }, /* R : -  5:00 */
    { 25,  -4 * 60      , -1 }, /* Q : -  4:00 */
    { 24,  -3 * 60 - 30 , -1 }, /* P*: -  3:30 */
    { 23,  -3 * 60      , -1 }, /* P : -  3:00 */
    { 22,  -2 * 60      , -1 }, /* O : -  2:00 */
    { 21,  -1 * 60      , -1 }, /* N : -  1:00 */
};

/*************************************************************************/

struct Timezone_DATA
{
    Object          *me;
    Object          *zone_name;
    Object          *city_name;
    Object	    *gmt_switch;
    Object	    *gmt_label;
    Object          *prefs;
    struct Hook      h;
    struct Hook      Timezone_list_hook;
    LONG             remaptable[256];
    ULONG            earthmap_coltab[256];
    LONG             lp_GMTOffset;
    char           **city_select_names;
    BOOL             truecolor;

    struct MUI_EventHandlerNode ehn;
};

struct MUI_CustomClass     *Timezone_CLASS;

/*************************************************************************/

static WORD   active_timezone;

static UBYTE  timezones_chunky[TIMEZONES_SMALL_WIDTH * TIMEZONES_SMALL_HEIGHT];
static UBYTE  earthmap_chunky[EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT];
static UBYTE *earthmap_chunky_remapped;

static char   timezone_text[256];
static LONG   pen2index[NUM_TIMEZONES];

static BOOL   pens_alloced;

/*************************************************************************/

#if EARTHMAP_SMALL_PACKED || TIMEZONES_SMALL_PACKED

static const UBYTE *unpack_byterun1(const UBYTE *source, UBYTE *dest, LONG unpackedsize)
{
    UBYTE r;
    BYTE c;

    for(;;)
    {
        c = (BYTE)(*source++);
        if (c >= 0)
        {
            while(c-- >= 0)
            {
                *dest++ = *source++;
                if (--unpackedsize <= 0) return source;
            }
        }
        else if (c != -128)
        {
            c = -c;
            r = *source++;

            while(c-- >= 0)
            {
                *dest++ = r;
                if (--unpackedsize <= 0) return source;
            }
        }
    }

}

#endif

/*************************************************************************/

static LONG timezone_init(Object *obj,struct Timezone_DATA *data)
{
    LONG i;
    LONG id;
    LONG i2;
    ULONG a;
    ULONG r;
    ULONG g;
    ULONG b;
    ULONG rgb;

    D(bug("[timezone class] timezone_init\n"));

    data->truecolor = GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH) >= 15;

    if (!data->truecolor)
    {
        earthmap_chunky_remapped = AllocVec(EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT, MEMF_PUBLIC);
        if (!earthmap_chunky_remapped)
            return FALSE;
    }

#if EARTHMAP_SMALL_PACKED
    unpack_byterun1(earthmap_small_data, earthmap_chunky, EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT);
#endif
#if TIMEZONES_SMALL_PACKED
    unpack_byterun1(timezones_small_data, timezones_chunky, TIMEZONES_SMALL_WIDTH * TIMEZONES_SMALL_HEIGHT);
#endif

    for(i = 0; i < EARTHMAP_SMALL_COLORS; i++)
    {
        rgb = earthmap_small_pal[i];
        r = (rgb & 0xFF0000) >> 16;
        g = (rgb & 0x00FF00) >> 8;
        b = (rgb & 0x0000FF);
        a = (r + g + b) / 3;

        r = (a * OCEAN_RED   + (255 - a) * CONTINENT_RED  ) / 255;
        g = (a * OCEAN_GREEN + (255 - a) * CONTINENT_GREEN) / 255;
        b = (a * OCEAN_BLUE  + (255 - a) * CONTINENT_BLUE ) / 255;

        rgb = (r << 16) + (g << 8) + b;

        data->earthmap_coltab[i] = rgb;

        if (!data->truecolor)
        {
            data->remaptable[i] = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
                    r * 0x01010101,
                    g * 0x01010101,
                    b * 0x01010101,
                    OBP_Precision, PRECISION_IMAGE,
                    OBP_FailIfBad, FALSE,
                    TAG_DONE);
        }

        r += SELECTED_INTENSITY_INC; if (r > 255) r = 255;
        g += SELECTED_INTENSITY_INC; if (g > 255) g = 255;
        b += SELECTED_INTENSITY_INC; if (b > 255) b = 255;

        rgb = (r << 16) + (g << 8) + b;

        data->earthmap_coltab[128 + i] = rgb;

        if (!data->truecolor)
        {
            data->remaptable[128 + i] = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
                    r * 0x01010101,
                    g * 0x01010101,
                    b * 0x01010101,
                    OBP_Precision, PRECISION_IMAGE,
                    OBP_FailIfBad, FALSE,
                    TAG_DONE);
        }

    }

    if (!data->truecolor) pens_alloced = TRUE;

    /* For each timezone find out, with which pen (index) it is
       represented in the timezones image */

    for(i = 0; i < NUM_TIMEZONES; i++)
    {
        id  = timezone_table[i].id;
        r   = ((id & 0x30) >> 4) * 64;
        g   = ((id & 0x0C) >> 2) * 64;
        b   = ((id & 0x03)     ) * 64;
        rgb = (r << 16) + (g << 8) + b;

        for(i2 = 0; i2 < TIMEZONES_SMALL_COLORS; i2++)
        {
            if (timezones_small_pal[i2] == rgb)
            {
                timezone_table[i].pen = i2;
                break;
            }
        }

        pen2index[id] = i;
    }

    return TRUE;
}

/*************************************************************************/

static void ClearEarthmapSelection(void)
{
    LONG l;

    for(l = 0; l < EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT; l++)
    {
        earthmap_chunky[l] &= 127;
    }
}

/*************************************************************************/

static void SetEarthmapSelection(UBYTE timezonespen)
{
    LONG l;

    for(l = 0; l < EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT; l++)
    {
        if (timezones_chunky[l] == timezonespen) earthmap_chunky[l] |= 128;
    }
}

/*************************************************************************/

static void UpdateZoneName(Object *zone_name, char *newname, BOOL noNotify)
{
    char *old = NULL;

    GET(zone_name, MUIA_Text_Contents, &old);

    if(old && strcmp(old, newname))
    {
    	SetAttrs(zone_name, MUIA_Text_Contents, newname, MUIA_NoNotify, noNotify, TAG_DONE);
    }
}

/*************************************************************************/

static void RepaintEarthmap(Object *obj, struct Timezone_DATA *data, BOOL noNotify)
{
    char  *fmt;
    WORD   minoffset; /* is only used for text gadget */

    minoffset = timezone_table[active_timezone].minoffset;

    if (data->truecolor)
    {
        WriteLUTPixelArray(earthmap_chunky,
                0,
                0,
                EARTHMAP_SMALL_WIDTH,
                _rp(obj),
                data->earthmap_coltab,
                _mleft(obj) +1,
                _mtop(obj) +1,
                EARTHMAP_SMALL_WIDTH,
                EARTHMAP_SMALL_HEIGHT,
                CTABFMT_XRGB8);
    }
    else
    {
        LONG l;

        for(l = 0; l < EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT; l++)
        {
            earthmap_chunky_remapped[l] = data->remaptable[earthmap_chunky[l]];
        }

        WriteChunkyPixels(_rp(obj),
                _mleft(obj) + 1,
                _mtop(obj) + 1,
                _mleft(obj) + EARTHMAP_SMALL_WIDTH,
                _mtop(obj) + EARTHMAP_SMALL_HEIGHT,
                earthmap_chunky_remapped,
                EARTHMAP_SMALL_WIDTH);
    }

    if (minoffset < 0)
        minoffset = -minoffset;

    if (minoffset == 60)
    {
        fmt = (char *) _(MSG_TIMEZONE_1HOUR);
    }
    else if (minoffset % 60)
    {
        fmt = (char *) _(MSG_TIMEZONE_HOURSMINS);
    }
    else
    {
        fmt = (char *) _(MSG_TIMEZONE_HOURS);
    }

    sprintf(timezone_text, fmt, minoffset / 60, minoffset % 60);

    UpdateZoneName(data->zone_name, timezone_text, noNotify);
}

/*************************************************************************/

static void timezone_cleanup(Object *obj, struct Timezone_DATA *data)
{
    WORD i;

    if (earthmap_chunky_remapped)
    {
        FreeVec(earthmap_chunky_remapped);
        earthmap_chunky_remapped = NULL;
    }

    if (pens_alloced)
    {
        for(i = 0; i < EARTHMAP_SMALL_COLORS; i++)
        {
            if (data->remaptable[i]       != -1)
            {
                ReleasePen(_screen(obj)->ViewPort.ColorMap, data->remaptable[i]);
                data->remaptable[i] = -1;
            }
            if (data->remaptable[128 + i] != -1)
            {
                ReleasePen(_screen(obj)->ViewPort.ColorMap, data->remaptable[128 + i]);
                data->remaptable[128 + i] = -1;
            }
        }
        pens_alloced = FALSE;
    }
}

/*************************************************************************/

STATIC IPTR Timezone__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Timezone_DATA *data = INST_DATA(cl, obj);

    struct DrawInfo             *dri=_dri(obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    if ( !(msg->flags & MADF_DRAWOBJECT) &&
            !(msg->flags & MADF_DRAWUPDATE) )
    {
        D(bug("[timezone class]   nothing to draw (!MADF_DRAWOBJECT and !MADF_DRAWUPDATE)\n"));
        return 0;
    }

    SetDrMd(_rp(obj), JAM1);

    SetAPen(_rp(obj), dri->dri_Pens[SHADOWPEN]);

    RectFill(_rp(obj), _mleft(obj),
            _mtop(obj),
            _mleft(obj) + _mwidth(obj) - 2,
            _mtop(obj));

    RectFill(_rp(obj), _mleft(obj),
            _mtop(obj) + 1,
            _mleft(obj),
            _mtop(obj) + EARTHMAP_SMALL_HEIGHT + 2 - 1);

    SetAPen(_rp(obj), dri->dri_Pens[SHINEPEN]);

    RectFill(_rp(obj), _mleft(obj) + _mwidth(obj) - 1,
            _mtop(obj),
            _mleft(obj) + _mwidth(obj) - 1,
            _mtop(obj) + EARTHMAP_SMALL_HEIGHT + 2 - 1);

    RectFill(_rp(obj), _mleft(obj) + 1,
            _mtop(obj) + EARTHMAP_SMALL_HEIGHT + 2 - 1,
            _mleft(obj) + _mwidth(obj) - 2,
            _mtop(obj) + EARTHMAP_SMALL_HEIGHT + 2 - 1);

    RepaintEarthmap(obj, data, TRUE);

    return TRUE;
}

/*************************************************************************/

AROS_UFH3(
        static ULONG, LayoutHook,
        AROS_UFHA(struct Hook *, hook, A0),
        AROS_UFHA(APTR, obj, A2),
        AROS_UFHA(struct MUI_LayoutMsg *, lm, A1))
{
    AROS_USERFUNC_INIT
        struct Timezone_DATA *data = hook->h_Data;

    D(bug("[timezone class] LayoutHook\n"));

    switch (lm->lm_Type)
    {
        case MUILM_MINMAX:
            {

                lm->lm_MinMax.MinWidth  = EARTHMAP_SMALL_WIDTH +
                    _minwidth(data->city_name);
                lm->lm_MinMax.MinHeight = EARTHMAP_SMALL_HEIGHT +
                    _minheight(data->zone_name) + _minheight(data->gmt_switch) + H + (H/2);

                lm->lm_MinMax.DefWidth  = EARTHMAP_SMALL_WIDTH +
                    _defwidth(data->city_name);
                lm->lm_MinMax.DefHeight = EARTHMAP_SMALL_HEIGHT +
                    _defheight(data->zone_name) + _defheight(data->gmt_switch) + H + (H/2);

                lm->lm_MinMax.MaxWidth  = EARTHMAP_SMALL_WIDTH +
                    _maxwidth(data->city_name);
                lm->lm_MinMax.MaxHeight = EARTHMAP_SMALL_HEIGHT +
                    _maxheight(data->zone_name) + _maxheight(data->gmt_switch);
                return 0;
            }
        case MUILM_LAYOUT:
            {
                /* we only have a few childs ;) */

                if(!MUI_Layout(data->city_name,
                            /* x,y */
                            EARTHMAP_SMALL_WIDTH , 1,
                            /* width, height: */
                            _defwidth(data->city_name), EARTHMAP_SMALL_HEIGHT,
                            0))
                {
                    return FALSE;
                }

                if(!MUI_Layout(data->zone_name,
                            1,  EARTHMAP_SMALL_HEIGHT + (H/2),
                            /* full width */
                            EARTHMAP_SMALL_WIDTH + _defwidth(data->city_name),
                            _minheight(data->zone_name),0))
                {
                    return FALSE;
                }

                if(!MUI_Layout(data->gmt_switch,
                            1,  EARTHMAP_SMALL_HEIGHT + H + _minheight(data->zone_name),
                            /* full width */
                            _minwidth(data->gmt_switch),
                            _minheight(data->gmt_switch),0))
                {
                    return FALSE;
                }

                if(!MUI_Layout(data->gmt_label,
                            1 + _minwidth(data->gmt_switch) + (H/2),  EARTHMAP_SMALL_HEIGHT + H + _minheight(data->zone_name),
                            /* full width */
                            _minwidth(data->gmt_label),
                            _minheight(data->gmt_label),0))
                {
                    return FALSE;
                }

                return TRUE;
            }

    }
    return MUILM_UNKNOWN;

    AROS_USERFUNC_EXIT
}

/*************************************************************************/

AROS_UFH2(
        void, Timezone_list_hook_func,
        AROS_UFHA(struct Hook *,    hook,   A0),
        AROS_UFHA(APTR,             obj,    A2)
        )
{
    AROS_USERFUNC_INIT
        struct Timezone_DATA *data= hook->h_Data;
    ULONG sel = 0;
    ULONG i;
    LONG tz;

    GET(obj, MUIA_List_Active, &sel);
    D(bug("[timezone class] Timezone_list_hook_func: %d\n",sel));

    tz=-1;
    i = 0;
    while ((tz < 0) && (i < NUM_TIMEZONES))
    {
        if(CityArray[sel].timediff / 100*60 == timezone_table[i].minoffset)
        {
            D(bug("[timezone class] Found timezone: %s\n", CityArray[sel].city_name));
            tz=i;
        }
        i++;
    }

    active_timezone = tz;

    data->lp_GMTOffset = -timezone_table[active_timezone].minoffset;

    ClearEarthmapSelection();
    SetEarthmapSelection(timezone_table[tz].pen);
    RepaintEarthmap(data->me, data, FALSE);

    AROS_USERFUNC_EXIT
}
/*************************************************************************/

static Object *handle_New_error(Object *obj, struct IClass *cl, char *error)
{
    struct Timezone_DATA *data;

    D(bug("[Timezone class] %s\n",error));
    ShowMessage(error);

    if(!obj)
        return NULL;

    data = INST_DATA(cl, obj);

    if (data->gmt_label)
    	DisposeObject(data->gmt_label);

    if (data->gmt_switch)
    	DisposeObject(data->gmt_switch);

    if(data->zone_name)
    {
        DisposeObject(data->zone_name);
        data->zone_name = NULL;
    }

    if(data->city_select_names)
    {
        FreeVec(data->city_select_names);
        data->city_select_names = NULL;
    }

    CoerceMethod(cl, obj, OM_DISPOSE);
    return NULL;
}

Object *Timezone__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Timezone_DATA *data;
    struct TagItem *tstate, *tag;
    ULONG                    i;

    D(bug("[timezone class] Timezone new\n"));

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
        MUIA_Frame,      MUIV_Frame_Text,
        TAG_DONE
    );

    if (obj == NULL)
    {
        return handle_New_error(obj, cl, "ERROR: Unable to create object!\n");
    }

    data = INST_DATA(cl, obj);
    data->me = obj;

    tstate = ((struct opSet *)msg)->ops_AttrList;
    while ((tag = (struct TagItem *) NextTagItem((APTR) &tstate)))
    {
        switch (tag->ti_Tag)
        {
            case MUIA_UserData:
                data->prefs = (Object *) tag->ti_Data;
                break;
        }
    }

    if (!data->prefs)
    {
        return handle_New_error(obj, cl, "ERROR: MA_PrefsObject not supplied!\n");
    }

    /* Layout Hook */
    data->h.h_Entry = (HOOKFUNC) LayoutHook;
    data->h.h_Data = data;
    SET(obj, MUIA_Group_LayoutHook, &data->h);

    /* xx hours away from GMT */
    data->zone_name = TextObject,
        MUIA_Frame,         MUIV_Frame_Text,
        MUIA_Text_PreParse, "\33c",
        MUIA_Text_Contents, "-", /* will be replaced anyway */
    End;

    if(!data->zone_name)
    {
        return handle_New_error(obj,cl,"ERROR: Unable to create TextObject!\n");
    }

    /* count cities */
    i = 0;
    while(CityArray[i].city_name != NULL)
    {
        i++;
    }

    /* build SourceArray */
    data->city_select_names = AllocVec((i + 1) * sizeof(IPTR), MEMF_CLEAR);
    if(!data->city_select_names)
    {
        /* we are dead anyway, but .. */
        return handle_New_error(obj, cl, "ERROR: Out of memory!\n");
    }

    i = 0;
    while (CityArray[i].city_name != NULL)
    {
        data->city_select_names[i] = CityArray[i].city_name;
        i++;
    }
    data->city_select_names[i] = NULL;

    /* city selection list */
    data->city_name = ListviewObject,
        MUIA_Listview_List,
        /* This crashes the List in FamilyAddTail..?:
         * MUIA_Listview_MultiSelect, MUIV_Listview_ScrollerPos_Default
         */
        ListObject, InputListFrame,
            MUIA_List_AdjustWidth, TRUE,
            MUIA_List_SourceArray, data->city_select_names,
        End,
    End;

    if(!data->city_name)
    {
        return handle_New_error(obj, cl, "ERROR: unable to create city_name ListviewObject!\n");
    }

    data->gmt_switch = MUI_MakeObject(MUIO_Checkmark, NULL);
    if (!data->gmt_switch)
    	return handle_New_error(obj, cl, "ERROR: unable to create checkmark bject!\n");

    data->gmt_label = Label1(_(MSG_GMT_CLOCK));
    if (!data->gmt_label)
    	return handle_New_error(obj, cl, "ERROR: unable to create label object!\n");

    DoMethod(obj, OM_ADDMEMBER, data->zone_name);
    DoMethod(obj, OM_ADDMEMBER, data->city_name);
    DoMethod(obj, OM_ADDMEMBER, data->gmt_switch);
    DoMethod(obj, OM_ADDMEMBER, data->gmt_label);

    data->Timezone_list_hook.h_Entry = (HOOKFUNC) &Timezone_list_hook_func;
    data->Timezone_list_hook.h_Data  = data ;
    DoMethod(data->city_name,
            MUIM_Notify, MUIA_List_Active,
            MUIV_EveryTime, data->city_name,
            2, MUIM_CallHook, &data->Timezone_list_hook);

    /* handle mouse clicks to map image */
    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS |
        IDCMP_ACTIVEWINDOW |
        IDCMP_INACTIVEWINDOW;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    /* changed hooks */
    DoMethod(data->zone_name, MUIM_Notify, MUIA_Text_Contents, MUIV_EveryTime, (IPTR) data->prefs, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
    DoMethod(data->gmt_switch, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, (IPTR) data->prefs, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);

    return obj;
}

/*************************************************************************/

static IPTR Timezone__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Timezone_DATA *data = INST_DATA(cl, obj);

    if(data->city_select_names)
    {
        FreeVec(data->city_select_names);
        data->city_select_names = NULL;
    }

    return DoSuperMethodA(cl, obj, msg);
}

/*** Setup ******************************************************************/
static IPTR Timezone__MUIM_Setup(struct IClass *cl, Object *obj, Msg msg)
{
    struct Timezone_DATA *data = INST_DATA(cl, obj);
    D(bug("[timezone class] Timezone_Setup\n"));

    if (!DoSuperMethodA(cl, obj, msg))
        return FALSE;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

    return timezone_init(obj, data);
}

static IPTR Timezone__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Timezone_DATA *data = INST_DATA(cl, obj);

    D(bug("[timezone class] Timezone_Cleanup\n"));

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    timezone_cleanup(obj, data);

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/* handle clicks on the map */
static IPTR Timezone__MUIM_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct Timezone_DATA *data = INST_DATA(cl, obj);

    if (msg->imsg)
    {
        LONG mx = msg->imsg->MouseX - _left(obj);
        LONG my = msg->imsg->MouseY - _top(obj);
        switch (msg->imsg->Class)
        {
            case    IDCMP_MOUSEBUTTONS:
                if (msg->imsg->Code == SELECTDOWN)
                {
                    if(my > 0 && my < EARTHMAP_SMALL_HEIGHT &&
                            mx > 0 && mx < EARTHMAP_SMALL_WIDTH)
                    {
                        ULONG timezonergb;
                        LONG timezoneid;
                        UBYTE timezonepen;

                        timezonepen = timezones_chunky[my * TIMEZONES_SMALL_WIDTH + mx];
                        timezonergb = timezones_small_pal[timezonepen];
                        timezoneid =  (timezonergb & 0xC00000) >> (16 + 2);
                        timezoneid += (timezonergb & 0x00C000) >> (8 + 4);
                        timezoneid += (timezonergb & 0x0000C0) >> (0 + 6);

                        if ((timezoneid >= 0) && (timezoneid < NUM_TIMEZONES))
                        {
                            active_timezone = pen2index[timezoneid];

                            /* AmigaOS seems to have the sign the other way round, therefore the "-" */

                            data->lp_GMTOffset = -timezone_table[active_timezone].minoffset;
                            D(bug("[timezone class] data->lp_GMTOffset: %d\n",data->lp_GMTOffset));

                            ClearEarthmapSelection();
                            SetEarthmapSelection(timezonepen);
                            RepaintEarthmap(obj, data, FALSE);
                            NNSET(data->city_name, MUIA_List_Active, MUIV_List_Active_Off);
                        }
                    }
                }
        }
    }
    return 0;
}

/*** Get ******************************************************************/
static IPTR Timezone__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Timezone_DATA *data = INST_DATA(cl, obj);
    IPTR rc;

    switch (msg->opg_AttrID)
    {
        case MUIA_Timezone_Timeoffset:
            rc = data->lp_GMTOffset;
            D(bug("[timezone class] Timezone_Get: MA_TimeOffset: %d\n",rc));
            break;

	case MUIA_Timezone_GMTClock:
	    GetAttr(MUIA_Selected, data->gmt_switch, &rc);
	    break;

        default:
            return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    *msg->opg_Storage = rc;
    return TRUE;
}

/*** Set ******************************************************************/
static IPTR Timezone__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Timezone_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tstate, *tag;
    ULONG update;
    ULONG i;
    LONG tz;
    LONG t;

    tstate = msg->ops_AttrList;
    update = FALSE;

    while ((tag = (struct TagItem *) NextTagItem((APTR) &tstate)))
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Timezone_Timeoffset:
                tz=-1;
                i = 0;
                t = (LONG)tag->ti_Data;
                D(bug("[timezone class] Timezone_Set: t: %d\n",t));
                while ((tz < 0) && (i < NUM_TIMEZONES))
                {
                    if(t == timezone_table[i].minoffset)
                    {
                        D(bug("[timezone class] Found timezone: %d\n",i));
                        tz = i;
                    }
                    i++;
                }

                if(tz < 0)
                {
                    D(bug("[timezone class] ERROR: Could not find timezone !?\n"));
                    break;
                }

                active_timezone = tz;
                /* removed - */
                data->lp_GMTOffset = timezone_table[active_timezone].minoffset;
                D(bug("[timezone class] Timezone_Set; data->lp_GMTOffset: %d\n",data->lp_GMTOffset));

                ClearEarthmapSelection();
                SetEarthmapSelection(timezone_table[tz].pen);

                update=TRUE;
                break;
        
            case MUIA_Timezone_GMTClock:
            	SetAttrs(data->gmt_switch, MUIA_Selected, tag->ti_Data, MUIA_NoNotify, TRUE, TAG_DONE);
            	break;

            default:
                return DoSuperMethodA(cl, obj, (Msg)msg);
        }
    }

    if(update)
    {
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    }

    return TRUE;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_8
(
    Timezone, NULL, MUIC_Group, NULL,
    OM_NEW, struct opSet *,
    OM_SET, struct opSet *,
    OM_GET, struct opGet *,
    OM_DISPOSE, Msg,
    MUIM_Setup, Msg,
    MUIM_Draw, struct MUIP_Draw *,
    MUIM_Cleanup, struct MUIP_Cleanup *,
    MUIM_HandleEvent, struct MUIP_HandleEvent *
);
