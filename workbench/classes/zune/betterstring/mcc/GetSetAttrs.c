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

 $Id$

***************************************************************************/

#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>

#include "BetterString_mcc.h"

#include "private.h"
#include "rev.h"

ULONG Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
		struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

	ULONG ti_Data;
	switch(msg->opg_AttrID)
	{
		case MUIA_Font:
			ti_Data = (ULONG)(data->Font ? data->Font : muiAreaData(obj)->mad_Font);
		break;

		case MUIA_ControlChar:
			ti_Data = (ULONG)data->CtrlChar;
		break;

		case MUIA_String_AttachedList:
			ti_Data = (ULONG)data->ForwardObject;
		break;

		case MUIA_String_BufferPos:
			ti_Data = (ULONG)data->BufferPos;
		break;

		case MUIA_String_Acknowledge:
		case MUIA_String_Contents:
			ti_Data = (ULONG)data->Contents;
		break;

		case MUIA_String_DisplayPos:
			ti_Data = (ULONG)data->DisplayPos;
		break;

		case MUIA_String_Format:
			ti_Data = (ULONG)data->Alignment;
		break;

		case MUIA_String_Integer:
			StrToLong(data->Contents, (LONG *)&ti_Data);
		break;

		case MUIA_String_MaxLen:
			ti_Data = (ULONG)data->MaxLength;
		break;

		case MUIA_String_Reject:
			ti_Data = (ULONG)data->Reject;
		break;

		case MUIA_String_Secret:
			ti_Data = (data->Flags & FLG_Secret) ? TRUE : FALSE;
		break;

		case MUIA_BetterString_KeyUpFocus:
			ti_Data = (ULONG)data->KeyUpFocus;
		break;

		case MUIA_BetterString_KeyDownFocus:
			ti_Data = (ULONG)data->KeyDownFocus;
		break;

		case MUIA_BetterString_SelectSize:
			ti_Data = (data->Flags & FLG_BlockEnabled) ? data->BlockStop-data->BlockStart : 0;
		break;

		case MUIA_BetterString_StayActive:
			ti_Data = (data->Flags & FLG_StayActive) ? TRUE : FALSE;
		break;

		case MUIA_BetterString_NoInput:
			ti_Data = (data->Flags & FLG_NoInput) ? TRUE : FALSE;
		break;

		case MUIA_Version:
			ti_Data = LIB_VERSION;
		break;

		case MUIA_Revision:
			ti_Data = LIB_REVISION;
		break;

		default:
			return DoSuperMethodA(cl, obj, (Msg)msg);
		break;
	}
	*msg->opg_Storage = ti_Data;
	return TRUE;
}

ULONG Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
		struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
		struct TagItem *tags, *tag;
		UBYTE IntegerString[12];
		ULONG ti_Data;

	struct TagItem boolMap[] =
	{
		{ MUIA_Disabled,							    FLG_Ghosted },
		{ MUIA_String_AdvanceOnCR,			  FLG_AdvanceOnCr },
		{ MUIA_String_Secret,					    FLG_Secret },
		{ MUIA_BetterString_StayActive,		FLG_StayActive },
		{ MUIA_BetterString_NoInput,			FLG_NoInput },
		{ TAG_DONE,                       NULL }
	};

	tags = msg->ops_AttrList;
	data->Flags = PackBoolTags(data->Flags, tags, boolMap);

	while((tag = NextTagItem(&tags)))
	{
		ti_Data = tag->ti_Data;
		switch(tag->ti_Tag)
		{
			case MUIA_Disabled:
				MUI_Redraw(obj, MADF_DRAWOBJECT);
			break;

			case MUIA_String_AttachedList:
				data->ForwardObject = (Object *)ti_Data;
			break;

			case MUIA_String_Accept:
				data->Accept = (STRPTR)ti_Data;
			break;

			case MUIA_String_BufferPos:
				data->BufferPos = (UWORD)ti_Data;
				data->Flags &= ~FLG_BlockEnabled;
			break;

			case MUIA_BetterString_Columns:
				data->Width = (UWORD)ti_Data;
			break;

			case MUIA_String_Integer:
				tag->ti_Tag = TAG_IGNORE;
				snprintf(IntegerString, 11, "%ld", ti_Data);
				ti_Data = (ULONG)IntegerString;
				/* The missing break is intended! */

			case MUIA_String_Contents:
			{
				STRPTR new_str = (STRPTR)ti_Data;
				BOOL circular = FALSE;
				if(new_str)
				{
					circular = !strcmp(data->Contents, new_str);
					if(!circular && data->MaxLength && strlen(new_str) > data->MaxLength)
						circular = !strncmp(data->Contents, new_str, data->MaxLength);
				}

				if(!circular)
				{
					if(new_str)
					{
						WORD extra = strlen(new_str)-strlen(data->Contents);

						if(extra > 0)
							data->Contents = (STRPTR)ExpandPool(data->Pool, data->Contents, extra);

						strcpy(data->Contents, new_str);
						data->Flags &= ~FLG_BlockEnabled;
						data->BufferPos = strlen(data->Contents);
						data->DisplayPos = 0;
						if(data->MaxLength && data->BufferPos >= data->MaxLength)
						{
							data->Contents[data->MaxLength-1] = '\0';
							data->BufferPos = data->MaxLength-1;
						}
					}
					else
					{
						*data->Contents = '\0';
						data->Flags &= ~FLG_BlockEnabled;
//						data->BlockStart = data->BlockStop = 0;
					}
				}
				else
				{
//					if(data->Contents != (STRPTR)ti_Data)
						tag->ti_Tag = TAG_IGNORE;
				}
			}
			break;

			case MUIA_ControlChar:
				data->CtrlChar = (UBYTE)ti_Data;
			break;

			case MUIA_String_DisplayPos:
				data->DisplayPos = (UWORD)ti_Data;
			break;

			case MUIA_String_Format:
				data->Alignment = (WORD)ti_Data;
			break;

			case MUIA_String_MaxLen:
				data->MaxLength = (UWORD)ti_Data;
//				kprintf("MaxLen: %ld\n", ti_Data);
			break;

			case MUIA_String_Reject:
				data->Reject = (STRPTR)ti_Data;
			break;

			case MUIA_BetterString_KeyUpFocus:
				data->KeyUpFocus = (Object *)ti_Data;
			break;

			case MUIA_BetterString_KeyDownFocus:
				data->KeyDownFocus = (Object *)ti_Data;
			break;

			case MUIA_BetterString_SelectSize:
				data->BlockStart = data->BufferPos;
				data->Flags |= FLG_BlockEnabled;

				data->BlockStop = data->BufferPos+ti_Data;
				if(data->BlockStop < 0)
					data->BlockStop = 0;
				if(data->BlockStop > strlen(data->Contents))
					data->BlockStop = strlen(data->Contents);
			break;

			case 0x80420d71: /* MUIA_String_Popup */
				data->Popup = (Object *)ti_Data;
			break;

/*			default:
				printf("Object: 0x%lx, tag: 0x%lx, Unknown attr: 0x%lx\n", obj, tag->ti_Tag, ti_Data);
			break;
*/		}
	}
	if(data->BufferPos > strlen(data->Contents))
		data->BufferPos = strlen(data->Contents);

	MUI_Redraw(obj, MADF_DRAWUPDATE);
	return(0);
}
