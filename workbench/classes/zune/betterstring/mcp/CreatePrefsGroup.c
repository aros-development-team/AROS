/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id: CreatePrefsGroup.c,v 1.1 2005/04/21 20:52:04 damato Exp $

***************************************************************************/

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <exec/libraries.h>
#include <libraries/asl.h>
#include <libraries/mui.h>

#include "BetterString_mcc.h"

#include "locale.h"
#include "private.h"
#ifndef __AROS__
#include "muiextra.h"
#else
#endif
#include "rev.h"

STRPTR Key1[]  = { "Amiga + c", "Copy all or marked text" };
STRPTR Key2[]  = { "Amiga + x", "Cut all or marked text" };
STRPTR Key3[]  = { "Amiga + v", "Paste" };
STRPTR Key4[]  = { "Amiga + z", "Undo last deletion" };
STRPTR Key5[]  = { "Amiga + Z", "Redo last deletion" };
STRPTR Key6[]  = { "Amiga + q", "Toggle between original and modified buffer" };
STRPTR Key7[]  = { "Amiga + g", "Toggle case on char" };
STRPTR Key8[]  = { "Amiga + G", "Toggle case on word" };
STRPTR Key9[]  = { "Amiga + i", "Increase number" };
STRPTR Key10[] = { "Amiga + d", "Decrease number" };
STRPTR Key11[] = { "Amiga + #", "Hex to decimal" };
STRPTR Key12[] = { "Amiga + $", "Decimal to hex" };
STRPTR Key13[] = { "Amiga + Tab", "Filenamecompletion (use shift to cycle back)" };

STRPTR Key14[] = { "Ctrl + crsr",    "Mark" };
STRPTR Key15[] = { "Ctrl + bs/del",  "Delete to start/end of line" };
STRPTR Key16[] = { "Shift + crsr",   "Go to start/end of line" };
STRPTR Key17[] = { "Shift + bs/del", "Delete to start/end of line" };
STRPTR Key18[] = { "Alt + crsr",     "Go to prev/next word" };
STRPTR Key19[] = { "Alt + bs/del",   "Delete prev/next word" };

STRPTR *Keyinfo[] = { Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9, Key10, Key11, Key12, Key13, Key14, Key15, Key16, Key17, Key18, Key19, NULL };

Object *TxtLabel (STRPTR text)
{
	return TextObject, ImageButtonFrame,
		MUIA_FramePhantomHoriz,		TRUE,
		MUIA_Text_PreParse,			"\33r",
		MUIA_Text_SetVMax,			FALSE,
		MUIA_Text_Contents,			text,
		MUIA_Weight,					0,
		End;
}

#ifdef __AROS__
AROS_HOOKPROTONH(DisplayCode, VOID, STRPTR*, place, STRPTR *, item)
#else
HOOKPROTONH(DisplayCode, VOID, STRPTR* place, STRPTR *item)
#endif
{
    	HOOK_INIT
	
	*place++ = *item++;
	*place++ = "=";
	*place   = *item;
	
	HOOK_EXIT
}
MakeStaticHook(DisplayHook, DisplayCode);

Object *CreatePrefsGroup(struct InstData_MCP *data)
{
	Object **objs = data->Objects;
	Object *group = VGroup,

		Child, PopobjectObject,
			MUIA_Popstring_String,  BetterStringObject, StringFrame,
				MUIA_String_Contents,	GetStr(MSG_String_TestString),
				MUIA_String_Format,		MUIV_String_Format_Center,
				MUIA_CycleChain,			TRUE,
				End,
			MUIA_Popstring_Button, PopButton(MUII_PopUp),
			MUIA_Popobject_Object, ListviewObject,
				MUIA_Listview_Input,		FALSE,
				MUIA_Listview_List,		ListObject, ReadListFrame,
					MUIA_List_DisplayHook,		&DisplayHook,
					MUIA_List_Format,				",,",
					MUIA_List_SourceArray,		Keyinfo,
					End,
				End,
			End,

		Child, RectangleObject,
			MUIA_VertWeight, 10,
			End,

		Child, HGroup,

			Child, ColGroup(2),
				Child, RectangleObject, End,
				Child, HGroup,
					MUIA_Group_SameWidth, TRUE,
					Child, TextObject,
						MUIA_Font,				  MUIV_Font_Tiny,
						MUIA_Text_Contents,	GetStr(MSG_Label_Background),
            MUIA_Text_PreParse, "\33c",
						End,
					Child, TextObject,
						MUIA_Font,				  MUIV_Font_Tiny,
						MUIA_Text_Contents,	GetStr(MSG_Label_Text),
            MUIA_Text_PreParse, "\33c",
						End,
					End,

				Child, TxtLabel(GetStr(MSG_Label_Inactive)),
				Child, HGroup,
					Child, objs[InactiveBack] = PopimageObject,
						MUIA_Imageadjust_Type,	2,
						MUIA_CycleChain,			TRUE,
						End,
					Child, objs[InactiveText] = PoppenObject,
						MUIA_CycleChain,			TRUE,
						End,
					End,

				Child, TxtLabel(GetStr(MSG_Label_Active)),
				Child, HGroup,
					Child, objs[ActiveBack] = PopimageObject,
						MUIA_Imageadjust_Type,	2,
						MUIA_CycleChain,			TRUE,
						End,
					Child, objs[ActiveText] = PoppenObject,
						MUIA_CycleChain,			TRUE,
						End,
					End,

				Child, RectangleObject, End,
				Child, RectangleObject,
					MUIA_Rectangle_HBar,		TRUE,
					MUIA_VertWeight,			10,
					End,

				Child, TxtLabel(GetStr(MSG_Label_Cursor)),
				Child, objs[Cursor] = PoppenObject,
					MUIA_CycleChain, TRUE,
					End,

				Child, TxtLabel(GetStr(MSG_Label_Marked)),
				Child, HGroup,
					Child, objs[MarkedBack] = PoppenObject,
						MUIA_CycleChain, TRUE,
						End,
					Child, objs[MarkedText] = PoppenObject,
						MUIA_CycleChain, TRUE,
						End,
					End,

				End,

			Child, RectangleObject,
				MUIA_Rectangle_VBar,	TRUE,
				MUIA_HorizWeight,		10,
				End,

			Child, ColGroup(2),
				Child, TxtLabel(GetStr(MSG_Label_Fonts)),
				Child, PopaslObject,
					MUIA_Popstring_String, objs[Font] = BetterStringObject, StringFrame,
						MUIA_CycleChain, TRUE,
						End,
					MUIA_Popstring_Button,	MUI_MakeObject(MUIO_PopButton, MUII_PopUp),
					MUIA_Popasl_Type,			ASL_FontRequest,
					End,

				Child, TxtLabel(GetStr(MSG_Label_Frame)),
				Child, objs[Frame] = PopframeObject,
					MUIA_CycleChain, TRUE,
					End,
				End,

			End,

		Child, RectangleObject,
			MUIA_VertWeight, 10,
			End,

		Child, CrawlingObject, TextFrame,
			MUIA_Background,			MUII_TextBack,
			MUIA_Virtgroup_Input,	FALSE,
			MUIA_FixHeightTxt,		"\n\n",
			Child, TextObject,
				MUIA_Text_Contents, "BetterString.mcp V" LIB_REV_STRING " (" LIB_DATE ")\n"
                            "Copyright 1997-2000 Allan Odgaard\n"
                            LIB_COPYRIGHT "\n\n"
                            "For the latest version, check out:\n"
                            "http://www.sf.net/projects/bstring-mcc/\n\n\n\n",
        MUIA_Text_PreParse, "\33c",
				End,
			End,

		End;

	if(MUIMasterBase->lib_Version <= 19 && objs[Frame])
		set(objs[Frame], MUIA_Disabled, TRUE);

	return group;
}
