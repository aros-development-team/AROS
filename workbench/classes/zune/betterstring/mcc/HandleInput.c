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

#include <string.h>
#include <stdlib.h>

#include <clib/alib_protos.h>
#include <clib/macros.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/keymap.h>
#include <proto/layers.h>
#include <proto/locale.h>
#include <proto/dos.h>

#include "private.h"

#include <SDI/SDI_stdarg.h>

#define BlockEnabled(data)  (isFlagSet((data)->Flags, FLG_BlockEnabled) && (data)->BlockStart != (data)->BlockStop)

#if defined(__amigaos4__) || defined(__MORPHOS__)
static int VARARGS68K MySPrintf(char *buf, const char *fmt, ...)
{
  VA_LIST args;

  VA_START(args, fmt);
  RawDoFmt(fmt, VA_ARG(args, void *), NULL, (STRPTR)buf);
  VA_END(args);

  return(strlen(buf));
}
#elif defined(__AROS__)
#define MySPrintf __sprintf /* from amiga lib */
#else
static int STDARGS MySPrintf(char *buf, const char *fmt, ...)
{
  static const UWORD PutCharProc[2] = {0x16C0,0x4E75};
  /* dirty hack to avoid assembler part :-)
     16C0: move.b d0,(a3)+
     4E75: rts */
  va_list args;

  va_start(args, fmt);
  RawDoFmt(fmt, args, (void (*)(void))PutCharProc, (STRPTR)buf);
  va_end(args);

  return(strlen(buf));
}
#endif

static void AddToUndo(struct InstData *data)
{
  if(data->Undo)
    SharedPoolFree(data->Undo);

  if((data->Undo = (STRPTR)SharedPoolAlloc(strlen(data->Contents)+1)))
  {
    strlcpy(data->Undo, data->Contents, strlen(data->Contents)+1);
    data->UndoPos = data->BufferPos;
    clearFlag(data->Flags, FLG_RedoAvailable);
  }
}

static WORD AlignOffset(Object *obj, struct InstData *data)
{
  struct MUI_AreaData  *ad  = muiAreaData(obj);
  struct TextFont  *font  = data->Font ? data->Font : ad->mad_Font;
  WORD   width = ad->mad_Box.Width - ad->mad_subwidth;
  WORD   offset = 0;

  if(data->Alignment != MUIV_String_Format_Left)
  {
    STRPTR text = data->Contents+data->DisplayPos;
    UWORD  StrLength = strlen(text);
    UWORD  length, textlength, crsr_width;
    struct TextExtent tExtend;

    SetFont(&data->rport, font);
    length = TextFit(&data->rport, text, StrLength, &tExtend, NULL, 1, width, font->tf_YSize);
    textlength = TextLength(&data->rport, text, length);

    crsr_width = isFlagSet(data->Flags, FLG_Active) ? TextLength(&data->rport, (*(data->Contents+data->BufferPos) == '\0') ? (char *)"n" : (char *)(data->Contents+data->BufferPos), 1) : 0;
    if(crsr_width && BlockEnabled(data) == FALSE && data->BufferPos == data->DisplayPos+StrLength)
    {
      textlength += crsr_width;
    }

    switch(data->Alignment)
    {
      case MUIV_String_Format_Center:
        offset = (width - textlength)/2;
        break;
      case MUIV_String_Format_Right:
        offset = (width - textlength);
        break;
    }
  }
  return(offset);
}

static BOOL Reject(UBYTE code, STRPTR reject)
{
  if(reject)
  {
    while(*reject)
    {
      if(code == *reject++)
        return(FALSE);
    }
  }
  return(TRUE);
}

static BOOL Accept(UBYTE code, STRPTR accept)
{
  return(accept ? !Reject(code, accept) : TRUE);
}

static UWORD DecimalValue(UBYTE code)
{
  if(code >= '0' && code <= '9')
    return(code - '0');
  if(code >= 'a' && code <= 'f')
    return(code - 'a' + 10);
  if(code >= 'A' && code <= 'F')
    return(code - 'A' + 10);

  return(0);
}

static BOOL IsHex(UBYTE code)
{
  return(
    (code >= '0' && code <= '9') ||
    (code >= 'a' && code <= 'f') ||
    (code >= 'A' && code <= 'F') ? TRUE : FALSE);
}

static LONG FindDigit(struct InstData *data)
{
    WORD  pos = data->BufferPos;

  if(IsDigit(data->locale, *(data->Contents+pos)))
  {
    while(pos > 0 && IsDigit(data->locale, *(data->Contents+pos-1)))
      pos--;
  }
  else
  {
    while(*(data->Contents+pos) != '\0' && !IsDigit(data->locale, *(data->Contents+pos)))
      pos++;
  }

  if(*(data->Contents+pos) == '\0')
  {
    pos = data->BufferPos;
    while(pos && !IsDigit(data->locale, *(data->Contents+pos)))
      pos--;

    while(pos > 0 && IsDigit(data->locale, *(data->Contents+pos-1)))
      pos--;

    if(!pos && !IsDigit(data->locale, *data->Contents))
    {
      pos = -1;
    }
  }
  return(pos);
}

static UWORD NextWord(STRPTR text, UWORD x, struct Locale *locale)
{
  while(IsAlNum(locale, (UBYTE)text[x]))
    x++;

  while(text[x] != '\0' && !IsAlNum(locale, (UBYTE)text[x]))
    x++;

  return(x);
}

static UWORD PrevWord(STRPTR text, UWORD x, struct Locale *locale)
{
  if(x)
    x--;

  while(x && !IsAlNum(locale, (UBYTE)text[x]))
    x--;

  while(x > 0 && IsAlNum(locale, (UBYTE)text[x-1]))
    x--;

  return(x);
}

void strcpyback(STRPTR dest, STRPTR src)
{
  UWORD  length;

  length = strlen(src)+1;
  dest = dest + length;
  src = src + length;

  length++;
  while(--length)
  {
    *--dest = *--src;
  }
}

void DeleteBlock(struct InstData *data)
{
  AddToUndo(data);

  if(BlockEnabled(data) == TRUE)
  {
    UWORD Blk_Start, Blk_Width;
    Blk_Start = (data->BlockStart < data->BlockStop) ? data->BlockStart : data->BlockStop;
    Blk_Width = abs(data->BlockStop-data->BlockStart);
    strcpy(data->Contents+Blk_Start, data->Contents+Blk_Start+Blk_Width);
    data->BufferPos = Blk_Start;
  }
}

static void CopyBlock(struct InstData *data)
{
  ENTER();

  if(isFlagClear(data->Flags, FLG_Secret))
  {
    UWORD Blk_Start, Blk_Width;
    //struct IFFHandle *iff;

    if(BlockEnabled(data) == TRUE)
    {
      Blk_Start = (data->BlockStart < data->BlockStop) ? data->BlockStart : data->BlockStop;
      Blk_Width = abs(data->BlockStop-data->BlockStart);
    }
    else
    {
      Blk_Start = 0;
      Blk_Width = strlen(data->Contents);
    }

    StringToClipboard(&data->Contents[Blk_Start], Blk_Width);
  }

  LEAVE();
}

static void CutBlock(struct InstData *data)
{
  ENTER();

  AddToUndo(data);

  if(BlockEnabled(data) == TRUE)
  {
    CopyBlock(data);
    DeleteBlock(data);
    clearFlag(data->Flags, FLG_BlockEnabled);
  }
  else
  {
    *data->Contents = '\0';
    data->BufferPos = 0;
  }

  LEAVE();
}

static void Paste(struct InstData *data)
{
  STRPTR str;
  LONG length;

  ENTER();

  // clear the selection
  DeleteBlock(data);

  ClipboardToString(&str, &length);
  if(str != NULL && length > 0)
  {
    if(data->MaxLength != 0 && strlen(data->Contents) + length > (ULONG)data->MaxLength - 1)
    {
      DisplayBeep(NULL);
      length = data->MaxLength - 1 - strlen(data->Contents);
    }

    data->Contents = (STRPTR)SharedPoolExpand(data->Contents, length);
    strcpyback(data->Contents + data->BufferPos + length, data->Contents + data->BufferPos);
    memcpy(data->Contents + data->BufferPos, str, length);
    data->BufferPos += length;

    SharedPoolFree(str);
  }

  LEAVE();
}

static void UndoRedo(struct InstData *data)
{
  STRPTR oldcontents = data->Contents;
  UWORD oldpos = data->BufferPos;

  ENTER();

  data->Contents = data->Undo;
  data->Undo = oldcontents;

  if(isFlagSet(data->Flags, FLG_RedoAvailable))
    clearFlag(data->Flags, FLG_RedoAvailable);
  else
    setFlag(data->Flags, FLG_RedoAvailable);

  clearFlag(data->Flags, FLG_BlockEnabled);
  data->BufferPos = data->UndoPos;
  data->UndoPos = oldpos;

  LEAVE();
}

static void RevertToOriginal(struct InstData *data)
{
  STRPTR oldcontents = data->Contents;

  ENTER();

  data->Contents = data->Original;
  data->Original = oldcontents;
  setFlag(data->Flags, FLG_Original);
  clearFlag(data->Flags, FLG_BlockEnabled);
  data->BufferPos = strlen(data->Contents);

  LEAVE();
}

static BOOL ToggleCaseChar(struct InstData *data)
{
  UBYTE key = *(data->Contents+data->BufferPos);
  BOOL result = FALSE;

  ENTER();

  if(data->BufferPos < strlen(data->Contents))
  {
    *(data->Contents+data->BufferPos) = IsLower(data->locale, key) ? ConvToUpper(data->locale, key) : ConvToLower(data->locale, key);
    data->BufferPos++;
    result = TRUE;
  }

  RETURN(result);
  return result;
}

static BOOL ToggleCaseWord(struct InstData *data)
{
  UWORD Stop = NextWord(data->Contents, data->BufferPos, data->locale);
  BOOL result = FALSE;

  ENTER();

  while(data->BufferPos < Stop)
  {
    UBYTE key = *(data->Contents+data->BufferPos);

    *(data->Contents+data->BufferPos) = IsLower(data->locale, key) ? ConvToUpper(data->locale, key) : ConvToLower(data->locale, key);
    data->BufferPos++;
    result = TRUE;
  }

  RETURN(result);
  return result;
}

static BOOL IncreaseNearNumber(struct InstData *data)
{
  LONG pos;
  BOOL result = FALSE;

  ENTER();

  if((pos = FindDigit(data)) >= 0)
  {
    LONG  cut;
    ULONG  res;

    if((cut = StrToLong(data->Contents+pos, (LONG *)&res)))
    {
      char string[12];
      char format[12];

      res++;
      MySPrintf(format, "%%0%ldlu", cut);
      MySPrintf(string, format, res);
      Overwrite(string, pos, cut, data);
      result = TRUE;
    }
  }

  RETURN(result);
  return result;
}

static BOOL DecreaseNearNumber(struct InstData *data)
{
  LONG pos;
  BOOL result = FALSE;

  ENTER();

  if((pos = FindDigit(data)) >= 0)
  {
    LONG cut;
    ULONG  res;

    if((cut = StrToLong(data->Contents+pos, (LONG *)&res)))
    {
      if(res)
      {
        char *format;
        char string[12];
        char format2[12];

        format = &format2[0];
        MySPrintf(format, "%%0%ldlu", cut);
        res--;
        MySPrintf(string, format, res);
        Overwrite(string, pos, cut, data);
        result = TRUE;
      }
    }
  }

  RETURN(result);
  return result;
}

static BOOL HexToDec(struct InstData *data)
{
  LONG  cut = 0;
  UWORD pos = data->BufferPos;
  ULONG  res = 0;
  BOOL result = FALSE;

  ENTER();

  while(pos && IsHex(*(data->Contents+pos-1)))
    pos--;

  while(IsHex(*(data->Contents+pos+cut)))
  {
    res = (result << 4) + DecimalValue(*(data->Contents+pos+cut));
    cut++;
  }

  if(cut)
  {
    char string[12];

    MySPrintf(string, "%lu", res);
    Overwrite(string, pos, cut, data);
    result = TRUE;
  }

  RETURN(result);
  return result;
}

static BOOL DecToHex(struct InstData *data)
{
  LONG pos;
  BOOL result = FALSE;

  ENTER();

  if((pos = FindDigit(data)) >= 0)
  {
    LONG cut;
    ULONG  res;

    if((cut = StrToLong(data->Contents+pos, (LONG *)&res)))
    {
      const char *format = "%lx";
      char string[12];

      MySPrintf(string, format, res);
      Overwrite(string, pos, cut, data);
      result  = TRUE;
    }
  }

  RETURN(result);
  return result;
}

ULONG ConvertKey(struct IntuiMessage *imsg)
{
  struct InputEvent  event;
  unsigned char code = 0;

  event.ie_NextEvent      = NULL;
  event.ie_Class          = IECLASS_RAWKEY;
  event.ie_SubClass       = 0;
  event.ie_Code           = imsg->Code;
  event.ie_Qualifier      = imsg->Qualifier;
  event.ie_EventAddress   = (APTR *) *((IPTR *)imsg->IAddress);

  MapRawKey(&event, (STRPTR)&code, 1, NULL);
  return(code);
}

IPTR mDoAction(struct IClass *cl, Object *obj, struct MUIP_BetterString_DoAction *msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  IPTR result = FALSE;
  BOOL edited = FALSE;

  ENTER();

  D(DBF_INPUT, "DoAction(%ld)", msg->action);

  switch(msg->action)
  {
    case MUIV_BetterString_DoAction_Cut:
    {
      CutBlock(data);
      edited = TRUE;
      result = TRUE;
    }
    break;

    case MUIV_BetterString_DoAction_Copy:
    {
      CopyBlock(data);
      clearFlag(data->Flags, FLG_BlockEnabled);
      result = TRUE;
    }
    break;

    case MUIV_BetterString_DoAction_Paste:
    {
      Paste(data);
      clearFlag(data->Flags, FLG_BlockEnabled);
      edited = TRUE;
      result = TRUE;
    }
    break;

    case MUIV_BetterString_DoAction_Delete:
    {
      DeleteBlock(data);
      clearFlag(data->Flags, FLG_BlockEnabled);
      edited = TRUE;
      result = TRUE;
    }
    break;

    case MUIV_BetterString_DoAction_SelectAll:
    {
      data->BlockStart = 0;
      data->BlockStop = strlen(data->Contents);
      setFlag(data->Flags, FLG_BlockEnabled);
      result = TRUE;
    }
    break;

    case MUIV_BetterString_DoAction_SelectNone:
    {
      clearFlag(data->Flags, FLG_BlockEnabled);
      result = TRUE;
    }
    break;

    case MUIV_BetterString_DoAction_Undo:
    case MUIV_BetterString_DoAction_Redo:
    {
      if(data->Undo &&
         (((msg->action == MUIV_BetterString_DoAction_Redo) && isFlagSet(data->Flags, FLG_RedoAvailable)) ||
          ((msg->action == MUIV_BetterString_DoAction_Undo) && isFlagClear(data->Flags, FLG_RedoAvailable))))
      {
        UndoRedo(data);
        clearFlag(data->Flags, FLG_BlockEnabled);
        edited = TRUE;
        result = TRUE;
      }
    }
    break;

    case MUIV_BetterString_DoAction_Revert:
    {
      RevertToOriginal(data);
      clearFlag(data->Flags, FLG_BlockEnabled);
      edited = TRUE;
      result = TRUE;
    }
    break;

    case MUIV_BetterString_DoAction_ToggleCase:
    {
      edited = result = ToggleCaseChar(data);
      clearFlag(data->Flags, FLG_BlockEnabled);
    }
    break;

    case MUIV_BetterString_DoAction_ToggleCaseWord:
    {
      edited = result = ToggleCaseWord(data);
      clearFlag(data->Flags, FLG_BlockEnabled);
    }
    break;

    case MUIV_BetterString_DoAction_IncreaseNum:
    {
      edited = result = IncreaseNearNumber(data);
      clearFlag(data->Flags, FLG_BlockEnabled);
    }
    break;

    case MUIV_BetterString_DoAction_DecreaseNum:
    {
      edited = result = DecreaseNearNumber(data);
      clearFlag(data->Flags, FLG_BlockEnabled);
    }
    break;

    case MUIV_BetterString_DoAction_HexToDec:
    {
      edited = result = HexToDec(data);
      clearFlag(data->Flags, FLG_BlockEnabled);
    }
    break;

    case MUIV_BetterString_DoAction_DecToHex:
    {
      edited = result = DecToHex(data);
      clearFlag(data->Flags, FLG_BlockEnabled);
    }
    break;

    case MUIV_BetterString_DoAction_NextFileComp:
    {
      edited = result = FileNameComplete(obj, FALSE, data);
      clearFlag(data->Flags, FLG_BlockEnabled);
    }
    break;

    case MUIV_BetterString_DoAction_PrevFileComp:
    {
      edited = result = FileNameComplete(obj, TRUE, data);
      clearFlag(data->Flags, FLG_BlockEnabled);
    }
    break;
  }

  if(edited == TRUE)
  {
    struct TagItem tags[] =
    {
      { MUIA_String_Contents, (IPTR)data->Contents  },
      { TAG_DONE,             0                     }
    };

    DoSuperMethod(cl, obj, OM_SET, tags, NULL);
  }

  MUI_Redraw(obj, MADF_DRAWUPDATE);

  RETURN(result);
  return result;
}

IPTR HandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  struct MUI_AreaData *ad = muiAreaData(obj);
  struct TextFont *Font = data->Font ? data->Font : ad->mad_Font;
  IPTR  result = 0;
  BOOL  movement = FALSE;
  BOOL  edited = FALSE;
  BOOL  FNC = FALSE;
  Object *focus = NULL;

  ENTER();

  if(msg->muikey == MUIKEY_UP)
    focus = data->KeyUpFocus;
  else if(msg->muikey == MUIKEY_DOWN)
    focus = data->KeyDownFocus;

  if(focus && _win(obj))
  {
    set(_win(obj), MUIA_Window_ActiveObject, focus);
    result = MUI_EventHandlerRC_Eat;
  }
  else if(msg->imsg)
  {
    ULONG StringLength = strlen(data->Contents);

    if(msg->imsg->Class == IDCMP_RAWKEY &&
//       msg->imsg->Code >= IECODE_KEY_CODE_FIRST &&
       msg->imsg->Code <= IECODE_KEY_CODE_LAST)
    {
      if(isFlagSet(data->Flags, FLG_Active))
      {
        BOOL input = TRUE;

        if(isFlagClear(data->Flags, FLG_BlockEnabled))
          data->BlockStart = data->BufferPos;

        if(isFlagSet(data->Flags, FLG_NoInput))
        {
          switch(msg->imsg->Code)
          {
            case RAWKEY_TAB:
            case RAWKEY_BACKSPACE:
            case RAWKEY_DEL:
              input = FALSE;
            break;
          }
        }

        if(input == TRUE)
        {
          switch(msg->imsg->Code)
          {
            // Tab
            case RAWKEY_TAB:
            {
              if(isFlagSet(data->Flags, FLG_NoShortcuts) || isFlagClear(msg->imsg->Qualifier, IEQUALIFIER_RCOMMAND))
              {
                RETURN(0);
                return 0;
              }

              if(!(edited = FileNameComplete(obj, isAnyFlagSet(msg->imsg->Qualifier, IEQUALIFIER_RSHIFT|IEQUALIFIER_LSHIFT), data)))
                DisplayBeep(NULL);

              FNC = TRUE;
            }
            break;

            // Right
            case RAWKEY_CRSRRIGHT:
            {
              if(data->BufferPos < StringLength)
              {
                if(isAnyFlagSet(msg->imsg->Qualifier, IEQUALIFIER_RSHIFT|IEQUALIFIER_LSHIFT))
                {
                  data->BufferPos = StringLength;
                }
                else if(isAnyFlagSet(msg->imsg->Qualifier, IEQUALIFIER_RALT|IEQUALIFIER_LALT))
                {
                  data->BufferPos = NextWord(data->Contents, data->BufferPos, data->locale);
                }
                else if(BlockEnabled(data) && isFlagClear(msg->imsg->Qualifier, IEQUALIFIER_CONTROL))
                {
                  data->BufferPos = MAX(data->BlockStart, data->BlockStop);
                }
                else
                {
                  data->BufferPos++;
                }
              }
              movement = TRUE;
            }
            break;

            // Left
            case RAWKEY_CRSRLEFT:
            {
              if(data->BufferPos)
              {
                if(isAnyFlagSet(msg->imsg->Qualifier, IEQUALIFIER_RSHIFT|IEQUALIFIER_LSHIFT))
                {
                  data->BufferPos = 0;
                }
                else if(isAnyFlagSet(msg->imsg->Qualifier, IEQUALIFIER_RALT|IEQUALIFIER_LALT))
                {
                  data->BufferPos = PrevWord(data->Contents, data->BufferPos, data->locale);
                }
                else
                {
                  if(BlockEnabled(data) && isFlagClear(msg->imsg->Qualifier, IEQUALIFIER_CONTROL))
                    data->BufferPos = MIN(data->BlockStart, data->BlockStop);

                  if(data->BufferPos)
                    data->BufferPos--;
                }
              }
              movement = TRUE;
            }
            break;

            // Backspace
            case RAWKEY_BACKSPACE:
            {
              if(BlockEnabled(data))
              {
                DeleteBlock(data);
              }
              else
              {
                if(data->BufferPos != 0)
                {
                  if(isAnyFlagSet(msg->imsg->Qualifier, IEQUALIFIER_RSHIFT|IEQUALIFIER_LSHIFT|IEQUALIFIER_CONTROL))
                  {
                    AddToUndo(data);
                    strcpy(data->Contents, data->Contents+data->BufferPos);
                    data->BufferPos = 0;
                  }
                  else if(isAnyFlagSet(msg->imsg->Qualifier, IEQUALIFIER_RALT|IEQUALIFIER_LALT))
                  {
                    UWORD NewPos = PrevWord(data->Contents, data->BufferPos, data->locale);

                    AddToUndo(data);
                    strcpy(data->Contents+NewPos, data->Contents+data->BufferPos);
                    data->BufferPos = NewPos;
                  }
                  else
                  {
                    strcpy(data->Contents+data->BufferPos-1, data->Contents+data->BufferPos);
                    data->BufferPos--;
                  }
                }
              }
              edited = TRUE;
            }
            break;

            // Delete
            case RAWKEY_DEL:
            {
              if(BlockEnabled(data))
              {
                DeleteBlock(data);
              }
              else
              {
                if(data->BufferPos < StringLength)
                {
                  if(isAnyFlagSet(msg->imsg->Qualifier, IEQUALIFIER_RSHIFT|IEQUALIFIER_LSHIFT) || isFlagSet(msg->imsg->Qualifier, IEQUALIFIER_CONTROL))
                  {
                    AddToUndo(data);
                    *(data->Contents+data->BufferPos) = '\0';
                  }
                  else if(isAnyFlagSet(msg->imsg->Qualifier, IEQUALIFIER_RALT|IEQUALIFIER_LALT))
                  {
                    AddToUndo(data);
                    strcpy(data->Contents+data->BufferPos, data->Contents+NextWord(data->Contents, data->BufferPos, data->locale));
                  }
                  else
                  {
                    strcpy(data->Contents+data->BufferPos, data->Contents+data->BufferPos+1);
                  }
                }
              }
              edited = TRUE;
            }
            break;

            // Home
            case RAWKEY_HOME:
            {
              if(data->BufferPos)
                data->BufferPos = 0;

              movement = TRUE;
            }
            break;

            // End
            case RAWKEY_END:
            {
              if(data->BufferPos < StringLength)
                data->BufferPos = StringLength;

              movement = TRUE;
            }
            break;

            default:
            {
              if(data->Popup && msg->muikey == MUIKEY_POPUP)
              {
                DoMethod(data->Popup, MUIM_Popstring_Open);
              }
              else
              {
                UBYTE code = ConvertKey(msg->imsg);

                if((((code >= 32 && code <= 126) || code >= 160) && isFlagClear(msg->imsg->Qualifier, IEQUALIFIER_RCOMMAND)) ||
                   (code && isFlagSet(msg->imsg->Qualifier, IEQUALIFIER_CONTROL)))
                {
                  if(isFlagClear(data->Flags, FLG_NoInput))
                  {
                    DeleteBlock(data);

                    if((data->MaxLength == 0 || (ULONG)data->MaxLength-1 > strlen(data->Contents)) &&
                       Accept(code, data->Accept) && Reject(code, data->Reject))
                    {
                      data->Contents = (STRPTR)SharedPoolExpand(data->Contents, 1);
                      strcpyback(data->Contents+data->BufferPos+1, data->Contents+data->BufferPos);
                      *(data->Contents+data->BufferPos) = code;
                      data->BufferPos++;
                      edited = TRUE;
                    }
                    else
                    {
                      DisplayBeep(NULL);
                    }
                  }
                }
                else
                {
                  // if this betterstring object is a read-only object
                  // we only accept a view characters or otherwise reject
                  // the rawkey operation
                  if(isFlagSet(data->Flags, FLG_NoInput))
                  {
                    switch(code)
                    {
                      case '\r':
                      case 'c':
                      break;

                      default:
                        RETURN(MUI_EventHandlerRC_Eat);
                        return MUI_EventHandlerRC_Eat;
                      break;
                    }
                  }

                  // check if the user pressed return and if he has activated AdvanceOnCr or not
                  if(code == '\r')
                  {
                    if(isFlagClear(data->Flags, FLG_StayActive))
                    {
                      ULONG active;

                      if(isFlagSet(data->Flags, FLG_AdvanceOnCr))
                      {
                        if(isAnyFlagSet(msg->imsg->Qualifier, IEQUALIFIER_RSHIFT|IEQUALIFIER_LSHIFT))
                          active = MUIV_Window_ActiveObject_Prev;
                        else
                          active = MUIV_Window_ActiveObject_Next;
                      }
                      else
                        active = MUIV_Window_ActiveObject_None;

                      set(_win(obj), MUIA_Window_ActiveObject, active);
                    }

                    set(obj, MUIA_String_Acknowledge, data->Contents);

                    RETURN(MUI_EventHandlerRC_Eat);
                    return MUI_EventHandlerRC_Eat;
                  }

                  // see if we should skip the default shorcuts or not
                  if(isFlagClear(data->Flags, FLG_NoShortcuts))
                  {
                    // depending on the pressed key code
                    // we perform different actions.
                    switch(code)
                    {
                      case 'g':
                      {
                        edited = ToggleCaseChar(data);
                      }
                      break;

                      case 'G':
                      {
                        edited = ToggleCaseWord(data);
                      }
                      break;

                      case 'c':
                        CopyBlock(data);
                      break;

                      case 'x':
                        CutBlock(data);
                        edited = TRUE;
                      break;

                      case 'v':
                        Paste(data);
                        edited = TRUE;
                      break;

                      case 'i':
                      {
                        if((edited = IncreaseNearNumber(data)) == FALSE)
                          DisplayBeep(NULL);
                      }
                      break;

                      case 'd':
                      {
                        if((edited = DecreaseNearNumber(data)) == FALSE)
                          DisplayBeep(NULL);
                      }
                      break;

                      case '#':
                      {
                        if((edited = HexToDec(data)) == FALSE)
                          DisplayBeep(NULL);
                      }
                      break;

                      case '$':
                      {
                        if((edited = DecToHex(data)) == FALSE)
                          DisplayBeep(NULL);
                      }
                      break;

                      case 'q':
                      {
                        RevertToOriginal(data);
                        edited = TRUE;
                      }
                      break;

                      case 'z':
                      case 'Z':
                      {
                        if(data->Undo && (((code == 'Z') && isFlagSet(data->Flags, FLG_RedoAvailable)) ||
                                          ((code == 'z') && isFlagClear(data->Flags, FLG_RedoAvailable))))
                        {
                          UndoRedo(data);
                          edited = TRUE;
                        }
                        else
                          DisplayBeep(NULL);
                      }
                      break;

                      default:
                        clearFlag(msg->imsg->Qualifier, IEQUALIFIER_RSHIFT);
                        RETURN(0);
                        return 0;
                    }
                  }
                  else
                  {
                    clearFlag(msg->imsg->Qualifier, IEQUALIFIER_RSHIFT);
                    RETURN(0);
                    return 0;
                  }
                }
              }
            }
            break;
          }
        }

        if(data->FNCBuffer && !FNC)
        {
          struct FNCData *fncbuffer = data->FNCBuffer, *fncframe;

          while(fncbuffer)
          {
            fncframe = fncbuffer;
            fncbuffer = fncbuffer->next;
            SharedPoolFree(fncframe);
          }
          data->FNCBuffer = NULL;
        }

        if(movement && isFlagSet(msg->imsg->Qualifier, IEQUALIFIER_CONTROL))
          setFlag(data->Flags, FLG_BlockEnabled);
        else
          clearFlag(data->Flags, FLG_BlockEnabled);

        if(isFlagSet(data->Flags, FLG_BlockEnabled))
          data->BlockStop = data->BufferPos;

        if(edited)
        {
          struct TagItem tags[] =
          {
            { MUIA_String_Contents, (IPTR)data->Contents },
            { TAG_DONE,             0                     }
          };

          DoSuperMethod(cl, obj, OM_SET, tags, NULL);
        }

        MUI_Redraw(obj, MADF_DRAWUPDATE);
        result = MUI_EventHandlerRC_Eat;
      }
      else
      {
        if(data->CtrlChar && ConvertKey(msg->imsg) == data->CtrlChar)
        {
          set(_win(obj), MUIA_Window_ActiveObject, obj);
          result = MUI_EventHandlerRC_Eat;
        }
      }
    }
    else
    {
      D(DBF_INPUT, "%08lx: keycode: %08lx (%ld) %08lx (%ld)", obj, msg->imsg->Code, isFlagSet(data->Flags, FLG_Active), RAWKEY_TAB, msg->imsg->Code <= IECODE_KEY_CODE_LAST);

      // check if the user pressed the TAB key for cycling to the next
      // mui object and if this object is part of the cyclechain
      if(msg->imsg->Class == IDCMP_RAWKEY && isFlagSet(data->Flags, FLG_Active) && isFlagSet(data->Flags, FLG_FreshActive))
      {
        // clear the FreshActive flag
        clearFlag(data->Flags, FLG_FreshActive);
        // no need to do an DoAction(SelectAll), because this effectively has been
        // done during MUIM_GoActive already
      }

      // we check if this is a mousemove input message and if
      // so we check whether the mouse is currently over our
      // texteditor object or not.
      if(msg->imsg->Class == IDCMP_MOUSEMOVE)
      {
        BOOL isOverObject = FALSE;

        D(DBF_INPUT, "IDCMP_MOUSEMOVE");
        if(_isinobject(obj, msg->imsg->MouseX, msg->imsg->MouseY))
        {
          #if defined(__MORPHOS__)
          if(IS_MORPHOS2)
            isOverObject = TRUE;
          #endif

          if(isOverObject == FALSE)
          {
            struct Layer_Info *li = &(_screen(obj)->LayerInfo);
            struct Layer *layer;

            // get the layer that belongs to the current mouse coordinates
            LockLayerInfo(li);
            layer = WhichLayer(li, _window(obj)->LeftEdge + msg->imsg->MouseX, _window(obj)->TopEdge + msg->imsg->MouseY);
            UnlockLayerInfo(li);

            // if the mouse is currently over the object and over the object's
            // window we go and change the pointer to show the selection pointer
            if(layer != NULL && layer->Window == _window(obj))
              isOverObject = TRUE;
          }
        }

        if(isOverObject == TRUE)
          ShowSelectPointer(obj, data);
        else
          HideSelectPointer(obj, data);
      }
      else if(msg->imsg->Class == IDCMP_MOUSEBUTTONS)
      {
        D(DBF_INPUT, "IDCMP_MOUSEBUTTONS");
        if(msg->imsg->Code == (IECODE_LBUTTON | IECODE_UP_PREFIX))
        {
          // forget the pressed mouse button
          clearFlag(data->Flags, FLG_MouseButtonDown);

          if(isAnyFlagSet(data->ehnode.ehn_Events, /*IDCMP_MOUSEMOVE|*/IDCMP_INTUITICKS))
          {
            DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);
//            clearFlag(data->ehnode.ehn_Events, IDCMP_MOUSEMOVE);
            clearFlag(data->ehnode.ehn_Events, IDCMP_INTUITICKS);
            DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);
          }

          // make sure to select the whole betterstring content in case
          // the objec was freshly active and the user pressed the mousebutton
          if(isFlagSet(data->Flags, FLG_FreshActive) && BlockEnabled(data) == FALSE &&
             ((data->SelectOnActive == TRUE && isFlagClear(data->Flags, FLG_ForceSelectOff)) || isFlagSet(data->Flags, FLG_ForceSelectOn)))
          {
            DoMethod(obj, MUIM_BetterString_DoAction, MUIV_BetterString_DoAction_SelectAll);
          }

#ifdef ALLOW_OUTSIDE_MARKING
          if(isFlagSet(data->Flags, FLG_Active) && isFlagSet(data->Flags, FLG_DragOutside))
          {
            Object *active;
            get(_win(obj), MUIA_Window_ActiveObject, &active);
            if(obj != active)
              E(DBF_STARTUP, "MUI error: %lx, %lx", active, obj);

            WORD x = ad->mad_Box.Left + ad->mad_addleft;
            WORD y = ad->mad_Box.Top  + ad->mad_addtop;
            WORD width = ad->mad_Box.Width - ad->mad_subwidth;
            WORD height = Font->tf_YSize;

            if(!(msg->imsg->MouseX >= x && msg->imsg->MouseX < x+width && msg->imsg->MouseY >= y && msg->imsg->MouseY < y+height))
            {
              D(DBF_STARTUP, "Detected LMB+up outside (drag: %ld)", isFlagSet(data->Flags, FLG_DragOutside));
              set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
            }
          }
#endif

          // clear the FreshActive flag
          clearFlag(data->Flags, FLG_FreshActive);
        }
        else if(msg->imsg->Code == IECODE_LBUTTON)
        {
          WORD x = ad->mad_Box.Left + ad->mad_addleft;
          WORD y = ad->mad_Box.Top  + ad->mad_addtop;
          WORD width = ad->mad_Box.Width - ad->mad_subwidth;
          WORD height = Font->tf_YSize;

          // remember the pressed mouse button
          setFlag(data->Flags, FLG_MouseButtonDown);

          if(msg->imsg->MouseX >= x && msg->imsg->MouseX < x+width && msg->imsg->MouseY >= y && msg->imsg->MouseY < y+height)
          {
            WORD offset = msg->imsg->MouseX - x;
            struct TextExtent tExtend;

            offset -= AlignOffset(obj, data);

            SetFont(&data->rport, Font);
            data->BufferPos = data->DisplayPos + TextFit(&data->rport, data->Contents+data->DisplayPos, StringLength-data->DisplayPos, &tExtend, NULL, 1, offset+1, Font->tf_YSize);

            if(data->BufferPos == data->BufferLastPos &&
               DoubleClick(data->StartSecs, data->StartMicros, msg->imsg->Seconds, msg->imsg->Micros))
            {
              // on a secret gadget we skip clickcount step 1 as
              // it might be misused to guess the words in the gadget.
              if(isFlagSet(data->Flags, FLG_Secret) && data->ClickCount == 0)
                data->ClickCount++;

              data->ClickCount++;
            }
            else
              data->ClickCount = 0;

            // lets save the current bufferpos to the lastpos variable
            data->BufferLastPos = data->BufferPos;

            data->StartSecs  = msg->imsg->Seconds;
            data->StartMicros  = msg->imsg->Micros;

            switch(data->ClickCount)
            {
              case 0:
              {
                if(isFlagClear(data->Flags, FLG_BlockEnabled) || isFlagClear(msg->imsg->Qualifier, IEQUALIFIER_CONTROL))
                  data->BlockStart = data->BufferPos;
              }
              break;

              case 1:
              {
                if(data->Contents[data->BufferPos] != '\0')
                {
                  UWORD start = data->BufferPos;
                  UWORD stop  = data->BufferPos;
                  ULONG alpha = IsAlNum(data->locale, (UBYTE)*(data->Contents+data->BufferPos));

                  while(start > 0 && alpha == (ULONG)IsAlNum(data->locale, (UBYTE)*(data->Contents+start-1)))
                    start--;

                  while(alpha == (ULONG)IsAlNum(data->locale, (UBYTE)*(data->Contents+stop)) && *(data->Contents+stop) != '\0')
                    stop++;

                  data->BlockStart = start;
                  data->BufferPos = stop;
                }
              }
              break;

              case 2:
              {
                data->BlockStart = 0;
                data->BufferPos = strlen(data->Contents);
              }
              break;

              case 3:
              {
                data->BlockStart = data->BufferPos;
                data->ClickCount = 0;
              }
              break;
            }
            data->BlockStop = data->BufferPos;
            setFlag(data->Flags, FLG_BlockEnabled);

            DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);
//            setFlag(data->ehnode.ehn_Events, IDCMP_MOUSEMOVE);
            setFlag(data->ehnode.ehn_Events, IDCMP_INTUITICKS);
            DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);

            if(isFlagSet(data->Flags, FLG_Active))
              MUI_Redraw(obj, MADF_DRAWUPDATE);
            else
              set(_win(obj), MUIA_Window_ActiveObject, obj);

            result = MUI_EventHandlerRC_Eat;
          }
          else
          {
            data->ClickCount = 0;
            if(isFlagSet(data->Flags, FLG_Active) && isFlagClear(data->Flags, FLG_StayActive))
            {
#ifdef ALLOW_OUTSIDE_MARKING
              D(DBF_STARTUP, "Clicked outside gadget");
              setFlag(data->Flags, FLG_DragOutside);

              DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);
//              data->ehnode.ehn_Events |= IDCMP_MOUSEMOVE;
              DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);
#else
              set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
#endif
            }
          }
        }
      }
      else
      {
#ifdef ALLOW_OUTSIDE_MARKING
        if(msg->imsg->Class == IDCMP_MOUSEMOVE)
        {
          clearFlag(data->Flags, FLG_DragOutside);
          D(DBF_STARTUP, "Detected drag");
        }
#endif
        if((/*msg->imsg->Class == IDCMP_MOUSEMOVE ||*/ msg->imsg->Class == IDCMP_INTUITICKS) && isFlagSet(data->Flags, FLG_Active))
        {
          WORD x, width, mousex;
          struct TextExtent tExtend;

          x = ad->mad_Box.Left + ad->mad_addleft;
          mousex = msg->imsg->MouseX - AlignOffset(obj, data);
          width = ad->mad_Box.Width - ad->mad_subwidth;

          SetFont(&data->rport, Font);

          switch(data->ClickCount)
          {
            case 0:
            {
              if(mousex < x)
              {
                if(data->DisplayPos)
                  data->DisplayPos--;
                data->BufferPos = data->DisplayPos;
              }
              else
              {
                if(mousex >= x+width)
                {
                  if(data->DisplayPos < StringLength)
                  {
                    data->DisplayPos++;
                    data->BufferPos = data->DisplayPos + TextFit(&data->rport, data->Contents+data->DisplayPos, StringLength-data->DisplayPos, &tExtend, NULL, 1, ad->mad_Box.Width - ad->mad_subwidth, Font->tf_YSize);
                  }
                  else
                  {
                    data->BufferPos = StringLength;
                  }
                }
                else
                {
                    WORD offset = mousex - x;

/*                  if(offset < 0)
                    data->BufferPos = 0;
                  else
*/                  data->BufferPos = data->DisplayPos + TextFit(&data->rport, data->Contents+data->DisplayPos, StringLength-data->DisplayPos, &tExtend, NULL, 1, offset+1, Font->tf_YSize);
                }
              }
              data->BlockStop = data->BufferPos;
              MUI_Redraw(obj, MADF_DRAWUPDATE);
            }
            break;

            case 1:
            {
              WORD offset = mousex - x;
              WORD newpos = 0;

              if(mousex < x && data->DisplayPos)
              {
                data->DisplayPos--;
                newpos = data->DisplayPos;
              }
              else
              {
//                offset -= AlignOffset(obj, data);
                if(offset > 0)
                  newpos = data->DisplayPos + TextFit(&data->rport, data->Contents+data->DisplayPos, StringLength-data->DisplayPos, &tExtend, NULL, 1, offset+1, Font->tf_YSize);
              }

              if(newpos >= data->BlockStart)
              {
                while(IsAlNum(data->locale, (UBYTE)*(data->Contents+newpos)))
                  newpos++;
              }
              else
              {
                while(newpos > 0 && IsAlNum(data->locale, (UBYTE)*(data->Contents+newpos-1)))
                  newpos--;
              }

              if(data->BufferPos != newpos)
              {
                data->BlockStop = data->BufferPos = newpos;
                MUI_Redraw(obj, MADF_DRAWUPDATE);
              }
            }
            break;
          }
        }
      }
    }
  }

  RETURN(result);
  return result;
}
