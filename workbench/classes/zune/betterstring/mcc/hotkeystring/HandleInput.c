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

#include <string.h>

#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <devices/inputevent.h>
#include <libraries/mui.h>

#ifndef __AROS__
#include <newmouse.h>
#else
#include <devices/rawkeycodes.h>
#define NM_WHEEL_UP 	RAWKEY_NM_WHEEL_UP
#define NM_WHEEL_DOWN 	RAWKEY_NM_WHEEL_DOWN
#define NM_WHEEL_LEFT 	RAWKEY_NM_WHEEL_LEFT
#define NM_WHEEL_RIGHT	RAWKEY_NM_WHEEL_RIGHT
#define NM_BUTTON_FOURTH 0x7E
#endif

#include "HotkeyString_mcc.h"

#include "private.h"

ULONG ConvertKey (struct IntuiMessage *imsg)
{
	struct InputEvent event;
	event.ie_NextEvent      = NULL;
	event.ie_Class          = IECLASS_RAWKEY;
	event.ie_SubClass       = 0;
	event.ie_Code           = imsg->Code;
	event.ie_Qualifier      = 0; /* imsg->Qualifier; */
	event.ie_EventAddress   = 0; /* (APTR *) *((ULONG *)imsg->IAddress); */

	UBYTE code = 0;
	return MapRawKey(&event, &code, 1, NULL), code;
}

#define BETWEEN(a, min, max) (a >= min && a <= max)

/* wheel mouse support */
#undef	IECODE_KEY_CODE_LAST
#define	IECODE_KEY_CODE_LAST 0x7e

ULONG HandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
	struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
	ULONG result = 0;
	BOOL nokey = FALSE, backspace = FALSE, qual_only;

	qual_only = BETWEEN(msg->imsg->Code, 0x60, 0x67);
	if(qual_only || (data->Flags & FLG_Snoop && data->Flags & FLG_Active && msg->imsg->Class == IDCMP_RAWKEY && BETWEEN(msg->imsg->Code, IECODE_KEY_CODE_FIRST, IECODE_KEY_CODE_LAST) && msg->muikey != MUIKEY_GADGET_NEXT && msg->muikey != MUIKEY_GADGET_PREV))
	{
		const STRPTR qualifier_name[] =
		{
			"lshift", "rshift", "capslock", "control", "lalt",
			"ralt", "lcommand", "rcommand", "numericpad", "repeat",
			NULL
		};

		UBYTE buffer[256] = { '\0' };
		ULONG qualifier = msg->imsg->Qualifier;
		ULONG i;

    for(i=0; qualifier_name[i]; i++)
		{
			if(qualifier & (1 << i))
			{
				strcat(buffer, qualifier_name[i]);
				strcat(buffer, " ");
			}
		}

		if(qual_only)
		{
			if(*buffer)
				buffer[strlen(buffer)-1] = '\0';
		}
		else
		{
			UWORD code = msg->imsg->Code;
			if(code >= 76 && code <= 89)
			{
				const STRPTR key_name[] =
				{
					"up", "down", "right", "left",
					"f1", "f2", "f3", "f4", "f5",
					"f6", "f7", "f8", "f9", "f10"
				};
				strcat(buffer, key_name[code-76]);
			}
			else switch(code)
			{
				case 95:
					strcat(buffer, "help");
				break;

				case NM_WHEEL_UP:
					strcat(buffer, "wheel_up");
				break;

				case NM_WHEEL_DOWN:
					strcat(buffer, "wheel_down");
				break;

				case NM_WHEEL_LEFT:
					strcat(buffer, "wheel_left");
				break;

				case NM_WHEEL_RIGHT:
					strcat(buffer, "wheel_right");
				break;

				case NM_BUTTON_FOURTH:
					strcat(buffer, "wheel_button");
				break;

				default:
				{
					STRPTR append = NULL;
					UBYTE key;
					switch(key = ConvertKey(msg->imsg))
					{
						case 0:
							nokey = TRUE;
						break;

						case 8:
							backspace = TRUE;
							append = "backspace";
						break;

						case 9:
							append = "tab";
						break;

						case 13:
							append = ((qualifier & IEQUALIFIER_NUMERICPAD) ? "enter" : "return");
						break;

						case 27:
							append = "esc";
						break;

						case 32:
							append = "space";
						break;

						case 0x7f:
							append = "del";
						break;

						default:
							strncat(buffer, &key, 1);
						break;
					}

					if(append)
						strcat(buffer, append);
				}
				break;
			}
		}

		if(!nokey)
		{
			if(backspace)
			{
				if(data->Flags & FLG_Backspace)
				{
					*buffer = '\0';
					data->Flags &= ~FLG_Backspace;
				}
				else
				{
					data->Flags |= FLG_Backspace;
				}
			}
			else
			{
				data->Flags &= ~FLG_Backspace;
			}
			SetAttrs(obj, MUIA_String_Contents, buffer, TAG_DONE);
		}
		result = MUI_EventHandlerRC_Eat;
	}
	return result;
}
