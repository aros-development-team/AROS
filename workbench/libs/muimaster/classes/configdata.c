/*
    Copyright � 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/dos.h>
#include <proto/commodities.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"
#include "prefs.h"

extern struct Library *MUIMasterBase;

struct MUI_ConfigdataData
{
    char *appname;
    struct ZunePrefsNew prefs;
    int test;
};

static char *StrDup(char *x)
{
    char *dup;
    if (!x) return NULL;
    dup = AllocVec(strlen(x) + 1, MEMF_PUBLIC);
    if (dup) CopyMem((x), dup, strlen(x) + 1);
    return dup;
}

static void *GetConfigData(Object *obj, ULONG id, void *def)
{
    void *f = (void*)DoMethod(obj,MUIM_Dataspace_Find,id);
    if (!f) return def;
    return f;
}

static void LoadPrefs(STRPTR filename, Object *obj)
{
    struct IFFHandle *iff;
    if ((iff = AllocIFF()))
    {
	if ((iff->iff_Stream = Open(filename,MODE_OLDFILE)))
	{
	    InitIFFasDOS(iff);

	    if (!OpenIFF(iff, IFFF_READ))
	    {
		StopChunk( iff, MAKE_ID('P','R','E','F'), MAKE_ID('M','U','I','C'));

		while (!ParseIFF(iff, IFFPARSE_SCAN))
		{
		    struct ContextNode *cn;
		    if (!(cn = CurrentChunk(iff))) continue;
		    if (cn->cn_ID == MAKE_ID('M','U','I','C')) DoMethod(obj,MUIM_Dataspace_ReadIFF,iff);
		}

		CloseIFF(iff);
	    }
	    Close(iff->iff_Stream);
	}
	FreeIFF(iff);
    }
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Configdata_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ConfigdataData *data;
    struct MUI_FrameSpec *frame;
    struct TagItem *tags,*tag;
    APTR cdata;
    int i;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return NULL;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Configdata_Application:
		    data->appname = (char*)tag->ti_Data;
		    break;
	}
    }

    LoadPrefs("env:zune/global.prefs",obj);

    data->prefs.fonts[-MUIV_Font_Normal] = (char*)DoMethod(obj,MUIM_Dataspace_Find,MUICFG_Font_Normal);
    data->prefs.fonts[-MUIV_Font_Big] = (char*)DoMethod(obj,MUIM_Dataspace_Find,MUICFG_Font_Big);
    data->prefs.fonts[-MUIV_Font_Tiny] = (char*)DoMethod(obj,MUIM_Dataspace_Find,MUICFG_Font_Tiny);
    data->prefs.fonts[-MUIV_Font_Button] = (char*)DoMethod(obj,MUIM_Dataspace_Find,MUICFG_Font_Button);

    data->prefs.imagespecs[MUII_WindowBack] = (char*)GetConfigData(obj,MUICFG_Background_Window,"0:128"); /* MUII_BACKGROUND */
    data->prefs.imagespecs[MUII_RequesterBack] = (char*)GetConfigData(obj,MUICFG_Background_Requester,"0:137"); /* MUII_SHINEBACK */
    data->prefs.imagespecs[MUII_ButtonBack] = (char*)GetConfigData(obj,MUICFG_Buttons_Background,"0:128");
    data->prefs.imagespecs[MUII_ListBack] = "0:128";
    data->prefs.imagespecs[MUII_TextBack] = "0:128";
    data->prefs.imagespecs[MUII_PropBack] = "0:128";
    data->prefs.imagespecs[MUII_PopupBack] = "0:128";
    data->prefs.imagespecs[MUII_SelectedBack] = (char*)GetConfigData(obj,MUICFG_Buttons_SelBackground,"0:131");
    data->prefs.imagespecs[MUII_ListCursor] = "0:131";
    data->prefs.imagespecs[MUII_ListSelect] = "0:135";
    data->prefs.imagespecs[MUII_ListSelCur] = "0:138";
    data->prefs.imagespecs[MUII_ArrowUp] = "1:0";
    data->prefs.imagespecs[MUII_ArrowDown] = "1:1";
    data->prefs.imagespecs[MUII_ArrowLeft] = "1:2";
    data->prefs.imagespecs[MUII_ArrowRight] = "1:3";
    data->prefs.imagespecs[MUII_CheckMark] = "1:4";
    data->prefs.imagespecs[MUII_RadioButton] = "1:5";
    data->prefs.imagespecs[MUII_Cycle] = "1:6";
    data->prefs.imagespecs[MUII_PopUp] = "1:7";
    data->prefs.imagespecs[MUII_PopFile] = "1:8";
    data->prefs.imagespecs[MUII_PopDrawer] = "0:128";
    data->prefs.imagespecs[MUII_PropKnob] = "0:128";
    data->prefs.imagespecs[MUII_Drawer] = "0:128";
    data->prefs.imagespecs[MUII_HardDisk] = "0:128";
    data->prefs.imagespecs[MUII_Disk] = "0:128";
    data->prefs.imagespecs[MUII_Chip] = "0:128";
    data->prefs.imagespecs[MUII_Volume] = "0:128";
    data->prefs.imagespecs[MUII_RegisterBack] = (char*)GetConfigData(obj,MUICFG_Background_Register,"0:128");
    data->prefs.imagespecs[MUII_Network] = "0:128";
    data->prefs.imagespecs[MUII_Assign] = "0:128";
    data->prefs.imagespecs[MUII_TapePlay] = "0:128";
    data->prefs.imagespecs[MUII_TapePlayBack] = "0:128";
    data->prefs.imagespecs[MUII_TapePause] = "0:128";
    data->prefs.imagespecs[MUII_TapeStop] = "0:128";
    data->prefs.imagespecs[MUII_TapeRecord] = "0:128";
    data->prefs.imagespecs[MUII_GroupBack] = (char*)GetConfigData(obj,MUICFG_Background_Framed,"0:128");
    data->prefs.imagespecs[MUII_SliderBack] = "0:128";
    data->prefs.imagespecs[MUII_SliderKnob] = "0:128";
    data->prefs.imagespecs[MUII_TapeUp] = "0:128";
    data->prefs.imagespecs[MUII_TapeDown] = "0:128";
    data->prefs.imagespecs[MUII_PageBack] = (char*)GetConfigData(obj,MUICFG_Background_Page,"0:128");
    data->prefs.imagespecs[MUII_ReadListBack] = "0:128";

    /* frame stuff */

    /* invisible frame */
    frame = &data->prefs.frames[MUIV_Frame_None];
    frame->type = FST_NONE;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 0;

    /* text button */
    frame = &data->prefs.frames[MUIV_Frame_Button];
    frame->type = FST_BEVEL;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;

    /* image button */
    frame = &data->prefs.frames[MUIV_Frame_ImageButton];
    frame->type = FST_BEVEL;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 0;

    /* textfield without input */
    frame = &data->prefs.frames[MUIV_Frame_Text];
    frame->type = FST_BEVEL;
    frame->state = 1;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;

    /* string gadget */
    frame = &data->prefs.frames[MUIV_Frame_String];
    frame->type = FST_THIN_BORDER;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;

    /* list without input */
    frame = &data->prefs.frames[MUIV_Frame_ReadList];
    frame->type = FST_BEVEL;
    frame->state = 1;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;

    /* list with input */
    frame = &data->prefs.frames[MUIV_Frame_InputList];
    frame->type = FST_BEVEL;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;

    /* scrollbar container */
    frame = &data->prefs.frames[MUIV_Frame_Prop];
    frame->type = FST_BEVEL;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;

    /* gauge */
    frame = &data->prefs.frames[MUIV_Frame_Gauge];
    frame->type = FST_BEVEL;
    frame->state = 1;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 0;

    /* normal group */
    frame = &data->prefs.frames[MUIV_Frame_Group];
    frame->type = FST_THIN_BORDER;
    frame->state = 1;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;

    /* cycle menu, popup window */
    frame = &data->prefs.frames[MUIV_Frame_PopUp];
    frame->type = FST_THIN_BORDER;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;

    /* virt group */
    frame = &data->prefs.frames[MUIV_Frame_Virtual];
    frame->type = FST_BEVEL;
    frame->state = 1;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 2;

    /* slider container */
    frame = &data->prefs.frames[MUIV_Frame_Slider];
    frame->type = FST_THICK_BORDER;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 0;

    /* slider knob - perhaps one day added to the array ? */
    frame = &data->prefs.frames[MUIV_Frame_Knob];
    frame->type = FST_BEVEL;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 1;

    /* dnd frame */
    frame = &data->prefs.frames[MUIV_Frame_Drag];
    frame->type = FST_THIN_BORDER;
    frame->state = 0;
    frame->innerLeft = frame->innerRight =
	frame->innerTop = frame->innerBottom = 0;

    data->prefs.group_title_position = GROUP_TITLE_POSITION_CENTERED;
    data->prefs.group_title_color = GROUP_TITLE_COLOR_BLACK;
    data->prefs.group_hspacing = 2;
    data->prefs.group_vspacing = 2;

    data->prefs.window_inner_left = 2;
    data->prefs.window_inner_right = 2;
    data->prefs.window_inner_top = 2;
    data->prefs.window_inner_bottom = 2;
    data->prefs.window_position = WINDOW_POSITION_FORGET_ON_EXIT;

    /* pens */
    data->prefs.muipens[MPEN_SHINE].red = 0xffffffff;
    data->prefs.muipens[MPEN_SHINE].green = 0xffffffff;
    data->prefs.muipens[MPEN_SHINE].blue = 0xffffffff;

    data->prefs.muipens[MPEN_HALFSHINE].red = 0xd0000000;
    data->prefs.muipens[MPEN_HALFSHINE].green = 0xd0000000;
    data->prefs.muipens[MPEN_HALFSHINE].blue = 0xd0000000;

    data->prefs.muipens[MPEN_BACKGROUND].red = 0xa0000000;
    data->prefs.muipens[MPEN_BACKGROUND].green = 0xa0000000;
    data->prefs.muipens[MPEN_BACKGROUND].blue = 0xa0000000;

    data->prefs.muipens[MPEN_HALFSHADOW].red = 0x50000000;
    data->prefs.muipens[MPEN_HALFSHADOW].green = 0x50000000;
    data->prefs.muipens[MPEN_HALFSHADOW].blue = 0x50000000;

    data->prefs.muipens[MPEN_SHADOW].red = 0x00000000;
    data->prefs.muipens[MPEN_SHADOW].green = 0x00000000;
    data->prefs.muipens[MPEN_SHADOW].blue = 0x00000000;

    data->prefs.muipens[MPEN_TEXT].red = 0x00000000;
    data->prefs.muipens[MPEN_TEXT].green = 0x00000000;
    data->prefs.muipens[MPEN_TEXT].blue = 0x00000000;

    data->prefs.muipens[MPEN_FILL].red = 0x05000000;
    data->prefs.muipens[MPEN_FILL].green = 0x84000000;
    data->prefs.muipens[MPEN_FILL].blue = 0xc4000000;

    data->prefs.muipens[MPEN_MARK].red = 0xf4000000;
    data->prefs.muipens[MPEN_MARK].green = 0xb5000000;
    data->prefs.muipens[MPEN_MARK].blue = 0x8b000000;

    /* mui keys */
    data->prefs.muikeys[MUIKEY_PRESS].readable_hotkey = StrDup("-upstroke return");
    data->prefs.muikeys[MUIKEY_TOGGLE].readable_hotkey = StrDup("-repeat space");
    data->prefs.muikeys[MUIKEY_UP].readable_hotkey = StrDup("-repeat up");
    data->prefs.muikeys[MUIKEY_DOWN].readable_hotkey = StrDup("-repeat down");
    data->prefs.muikeys[MUIKEY_PAGEUP].readable_hotkey = StrDup("-repeat shift up");
    data->prefs.muikeys[MUIKEY_PAGEDOWN].readable_hotkey = StrDup("-repeat shift down");
    data->prefs.muikeys[MUIKEY_TOP].readable_hotkey = StrDup("control up");
    data->prefs.muikeys[MUIKEY_BOTTOM].readable_hotkey = StrDup("control down");
    data->prefs.muikeys[MUIKEY_LEFT].readable_hotkey = StrDup("-repeat left");
    data->prefs.muikeys[MUIKEY_RIGHT].readable_hotkey = StrDup("-repeat right");
    data->prefs.muikeys[MUIKEY_WORDLEFT].readable_hotkey = StrDup("-repeat control left");
    data->prefs.muikeys[MUIKEY_WORDRIGHT].readable_hotkey = StrDup("-repeat control right");
    data->prefs.muikeys[MUIKEY_LINESTART].readable_hotkey = StrDup("shift left");
    data->prefs.muikeys[MUIKEY_LINEEND].readable_hotkey = StrDup("shift right");
    data->prefs.muikeys[MUIKEY_GADGET_NEXT].readable_hotkey = StrDup("-repeat tab");
    data->prefs.muikeys[MUIKEY_GADGET_PREV].readable_hotkey = StrDup("-repeat shift tab");
    data->prefs.muikeys[MUIKEY_GADGET_OFF].readable_hotkey = StrDup("control tab");
    data->prefs.muikeys[MUIKEY_WINDOW_CLOSE].readable_hotkey = StrDup("esc");
    data->prefs.muikeys[MUIKEY_WINDOW_NEXT].readable_hotkey = StrDup("-repeat alt tab");
    data->prefs.muikeys[MUIKEY_WINDOW_PREV].readable_hotkey = StrDup("-repeat alt shift tab");
    data->prefs.muikeys[MUIKEY_HELP].readable_hotkey = StrDup("help");
    data->prefs.muikeys[MUIKEY_POPUP].readable_hotkey = StrDup("control p");

    for (i = 0; i < MUIKEY_COUNT; i++)
    {
    	if (data->prefs.muikeys[i].readable_hotkey)
	    data->prefs.muikeys[i].ix_well = !ParseIX(data->prefs.muikeys[i].readable_hotkey, &data->prefs.muikeys[i].ix);
	else data->prefs.muikeys[i].ix_well = 0;
    }


    return (ULONG)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static ULONG Configdata_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ConfigdataData *data = INST_DATA(cl, obj);
    DoSuperMethodA(cl,obj,msg);
    return NULL;
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG  Configdata_Get(struct IClass *cl, Object * obj, struct opGet *msg)
{
    struct MUI_ConfigdataData *data = INST_DATA(cl, obj);
    ULONG *store = msg->opg_Storage;
    ULONG    tag = msg->opg_AttrID;

    switch (tag)
    {
	case 	MUIA_Configdata_ZunePrefs:
		*store = (ULONG)&data->prefs;
		return 1;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/*
 * The class dispatcher
 */
#ifndef __AROS
static __asm IPTR Configdata_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, Configdata_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	/* Whenever an object shall be created using NewObject(), it will be
	** sent a OM_NEW method.
	*/
	case OM_NEW: return Configdata_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Configdata_Dispose(cl, obj, (APTR)msg);
	case OM_GET: return Configdata_Get(cl, obj, (APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Configdata_desc = {
    MUIC_Configdata,                        /* Class name */
    MUIC_Dataspace,                         /* super class name */
    sizeof(struct MUI_ConfigdataData),      /* size of class own datas */
    (void*)Configdata_Dispatcher            /* class dispatcher */
};
