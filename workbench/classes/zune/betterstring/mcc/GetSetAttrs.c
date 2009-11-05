/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2009 by BetterString.mcc Open Source Team

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
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include "BetterString_mcc.h"

#include "private.h"
#include "version.h"

IPTR Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  IPTR ti_Data;

  switch(msg->opg_AttrID)
  {
    case MUIA_Font:
      ti_Data = (IPTR)(data->Font ? data->Font : muiAreaData(obj)->mad_Font);
    break;

    case MUIA_ControlChar:
      ti_Data = (IPTR)data->CtrlChar;
    break;

    case MUIA_String_AttachedList:
      ti_Data = (IPTR)data->ForwardObject;
    break;

    case MUIA_String_BufferPos:
      ti_Data = (IPTR)data->BufferPos;
    break;

    case MUIA_String_Acknowledge:
    case MUIA_String_Contents:
      ti_Data = (IPTR)data->Contents;
    break;

    case MUIA_String_DisplayPos:
      ti_Data = (IPTR)data->DisplayPos;
    break;

    case MUIA_String_Format:
      ti_Data = (IPTR)data->Alignment;
    break;

    case MUIA_String_Integer:
      StrToLong(data->Contents, (LONG *)&ti_Data);
    break;

    case MUIA_String_MaxLen:
      ti_Data = (IPTR)data->MaxLength;
    break;

    case MUIA_String_Reject:
      ti_Data = (IPTR)data->Reject;
    break;

    case MUIA_String_Secret:
      ti_Data = isFlagSet(data->Flags, FLG_Secret) ? TRUE : FALSE;
    break;

    case MUIA_String_EditHook:
      ti_Data = (IPTR)data->EditHook;
    break;

    case MUIA_String_AdvanceOnCR:
      ti_Data = isFlagSet(data->Flags, FLG_AdvanceOnCr) ? TRUE : FALSE;
    break;

    case MUIA_BetterString_KeyUpFocus:
      ti_Data = (IPTR)data->KeyUpFocus;
    break;

    case MUIA_BetterString_KeyDownFocus:
      ti_Data = (IPTR)data->KeyDownFocus;
    break;

    case MUIA_BetterString_SelectSize:
      ti_Data = isFlagSet(data->Flags, FLG_BlockEnabled) ? data->BlockStop-data->BlockStart : 0;
    break;

    case MUIA_BetterString_StayActive:
      ti_Data = isFlagSet(data->Flags, FLG_StayActive) ? TRUE : FALSE;
    break;

    case MUIA_BetterString_NoInput:
      ti_Data = isFlagSet(data->Flags, FLG_NoInput) ? TRUE : FALSE;
    break;

    case MUIA_BetterString_InactiveContents:
      ti_Data = (IPTR)data->InactiveContents;
    break;

    case MUIA_BetterString_NoShortcuts:
      ti_Data = isFlagSet(data->Flags, FLG_NoShortcuts) ? TRUE : FALSE;
    break;

    case MUIA_BetterString_SelectOnActive:
    {
      if((data->SelectOnActive == TRUE && isFlagClear(data->Flags, FLG_ForceSelectOff)) ||
         isFlagSet(data->Flags, FLG_ForceSelectOn))
      {
         ti_Data = TRUE;
      }
      else
         ti_Data = FALSE;
    }
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

IPTR Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  struct TagItem *tags, *tag;
  char IntegerString[12];
  IPTR ti_Data;

  struct TagItem boolMap[] =
  {
    { MUIA_Disabled,                  FLG_Ghosted     },
    { MUIA_String_AdvanceOnCR,        FLG_AdvanceOnCr },
    { MUIA_String_Secret,             FLG_Secret      },
    { MUIA_BetterString_StayActive,   FLG_StayActive  },
    { MUIA_BetterString_NoInput,      FLG_NoInput     },
    { MUIA_BetterString_NoShortcuts,  FLG_NoShortcuts },
    { TAG_DONE,                       0               }
  };

  tags = msg->ops_AttrList;
  data->Flags = PackBoolTags(data->Flags, tags, boolMap);

  while((tag = NextTagItem((APTR)&tags)))
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
        clearFlag(data->Flags, FLG_BlockEnabled);
      break;

      case MUIA_BetterString_Columns:
        data->Width = (UWORD)ti_Data;
      break;

      case MUIA_String_Integer:
        tag->ti_Tag = TAG_IGNORE;

        // we are using snprintf() here not only to be on the safe
        // side, but also because modern C runtime libraries should definitly
        // support it!
        snprintf(IntegerString, sizeof(IntegerString), "%d", (int)ti_Data);
        ti_Data = (ULONG)IntegerString;

        // The missing break is intended!

      case MUIA_String_Contents:
      {
        STRPTR new_str = (STRPTR)ti_Data;
        BOOL circular = FALSE;

        if(new_str != NULL)
        {
          circular = !strcmp(data->Contents, new_str);
          if(circular == FALSE && data->MaxLength && strlen(new_str) > data->MaxLength)
            circular = !strncmp(data->Contents, new_str, data->MaxLength);
        }

        if(circular == FALSE)
        {
          // we don't have a valid block anymore
          clearFlag(data->Flags, FLG_BlockEnabled);

          if(new_str != NULL)
          {
            WORD extra = strlen(new_str)-strlen(data->Contents);

            if(extra > 0)
              data->Contents = (STRPTR)SharedPoolExpand(data->Contents, extra);

            strcpy(data->Contents, new_str);
            data->BufferPos = strlen(data->Contents);
            data->DisplayPos = 0;
            if(data->MaxLength != 0 && data->BufferPos >= data->MaxLength)
            {
              data->Contents[data->MaxLength-1] = '\0';
              data->BufferPos = data->MaxLength-1;
            }

            // the start of a block cannot be behind the last character
            if(data->BlockStart > strlen(data->Contents))
              data->BlockStart = strlen(data->Contents);
          }
          else
          {
            data->Contents[0] = '\0';
            data->BlockStart = 0;
            data->BlockStop = 0;
            data->BufferPos = 0;
            data->DisplayPos = 0;
          }
        }
        else
        {
//        if(data->Contents != (STRPTR)ti_Data)
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
      break;

      case MUIA_String_Reject:
        data->Reject = (STRPTR)ti_Data;
      break;

      case MUIA_String_EditHook:
        data->EditHook = (struct Hook *)ti_Data;
      break;

      case MUIA_BetterString_KeyUpFocus:
        data->KeyUpFocus = (Object *)ti_Data;
      break;

      case MUIA_BetterString_KeyDownFocus:
        data->KeyDownFocus = (Object *)ti_Data;
      break;

      case MUIA_BetterString_InactiveContents:
        data->InactiveContents = (STRPTR)ti_Data;
      break;

      case MUIA_BetterString_SelectSize:
      {
        data->BlockStart = data->BufferPos;
        setFlag(data->Flags, FLG_BlockEnabled);

        data->BlockStop = data->BufferPos+ti_Data;

        if(data->BlockStop < 0)
          data->BlockStop = 0;

        if((ULONG)data->BlockStop > strlen(data->Contents))
          data->BlockStop = strlen(data->Contents);
      }
      break;

      case MUIA_BetterString_SelectOnActive:
      {
         if(ti_Data == FALSE)
         {
            setFlag(data->Flags, FLG_ForceSelectOff);
            clearFlag(data->Flags, FLG_ForceSelectOn);
         }
         else
         {
            setFlag(data->Flags, FLG_ForceSelectOn);
            clearFlag(data->Flags, FLG_ForceSelectOff);
         }
      }
      break;

      case 0x80420d71: /* MUIA_String_Popup */
        data->Popup = (Object *)ti_Data;
      break;

    }
  }

  if(data->BufferPos > strlen(data->Contents))
    data->BufferPos = strlen(data->Contents);

  MUI_Redraw(obj, MADF_DRAWUPDATE);
  return(0);
}
