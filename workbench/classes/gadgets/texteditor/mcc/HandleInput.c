/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id: HandleInput.c,v 1.27 2005/12/06 23:41:22 damato Exp $

***************************************************************************/

#include <libraries/mui.h>
#include <intuition/classes.h>
#include <clib/alib_protos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/locale.h>
#include <proto/utility.h>

#include "TextEditor_mcc.h"
#include "private.h"

#ifdef __AROS__
#include "../includes/newmouse.h"
#elif !defined(__amigaos4__)
#include "newmouse.h"
#endif

extern struct keybindings keys[];

static LONG ReactOnRawKey(UBYTE key, ULONG qualifier, struct IntuiMessage *imsg, struct InstData *data);

static ULONG RAWToANSI(struct IntuiMessage *imsg)
{
  struct InputEvent event;
  UBYTE code = 0;

  ENTER();

  event.ie_NextEvent    = NULL;
  event.ie_Class        = IECLASS_RAWKEY;
  event.ie_SubClass     = 0;
  event.ie_Code         = imsg->Code;
  event.ie_Qualifier    = imsg->Qualifier;
  event.ie_EventAddress = (APTR *) *((ULONG *)imsg->IAddress);

  MapRawKey(&event, (STRPTR)&code, 1, NULL);

  SHOWVALUE(DBF_INPUT, code);

  RETURN(code);
  return(code);
}

ULONG HandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  BOOL wasActivated;

  ENTER();

  // check if the gadget was activate recently (via TAB key?)
  wasActivated = (data->flags & FLG_Activated) == FLG_Activated;
  data->flags &= ~FLG_Activated; // clear the activated flag immediately

  if((msg->muikey == MUIKEY_UP) && data->KeyUpFocus && _win(obj) && (data->firstline == data->actualline))
  {
    set(_win(obj), MUIA_Window_ActiveObject, data->KeyUpFocus);

    RETURN(MUI_EventHandlerRC_Eat);
    return MUI_EventHandlerRC_Eat;
  }
  else if(!(data->flags & FLG_Ghosted) && data->shown && msg->imsg &&
          (msg->muikey != MUIKEY_GADGET_PREV) && (msg->muikey != MUIKEY_GADGET_NEXT) && // if the user moves to another obj with TAB
          (msg->muikey != MUIKEY_GADGET_OFF)) // user deselected gadget with CTRL+TAB
  {
    Object *activeobj;
    Object *defaultobj;
    struct IntuiMessage *imsg = msg->imsg;

    get(_win(obj), MUIA_Window_ActiveObject, (IPTR *)&activeobj);
    get(_win(obj), MUIA_Window_DefaultObject, (IPTR *)&defaultobj);

    if(data->CtrlChar && activeobj != obj && defaultobj != obj && RAWToANSI(imsg) == data->CtrlChar)
    {
      set(_win(obj), MUIA_Window_ActiveObject, obj);

      RETURN(MUI_EventHandlerRC_Eat);
      return(MUI_EventHandlerRC_Eat);
    }

    #if defined(__amigaos4__)
    if((imsg->Class == IDCMP_MOUSEBUTTONS) || (activeobj == obj) ||
       (data->flags & FLG_ReadOnly && defaultobj == obj && !activeobj) ||
       (imsg->Class == IDCMP_EXTENDEDMOUSE && imsg->Code & IMSGCODE_INTUIWHEELDATA))
    #else
    if((imsg->Class == IDCMP_MOUSEBUTTONS) || (activeobj == obj) ||
       (data->flags & FLG_ReadOnly && defaultobj == obj && !activeobj) ||
       (imsg->Class == IDCMP_RAWKEY &&
        (imsg->Code == NM_WHEEL_UP || imsg->Code == NM_WHEEL_DOWN ||
         imsg->Code == NM_WHEEL_LEFT || imsg->Code == NM_WHEEL_RIGHT)))
    #endif
    {
      if(!(data->flags & FLG_Draw))
      {
        data->UpdateInfo = msg;
        MUI_Redraw(obj, MADF_DRAWUPDATE);

        RETURN((ULONG)data->UpdateInfo);
        return((ULONG)data->UpdateInfo);
      }

      switch(imsg->Class)
      {
        case IDCMP_RAWKEY:
        {
          if(data->ypos != data->realypos ||
             (wasActivated && imsg->Code == 66)) // ignore TAB key if the gadget was activated recently
          {
            RETURN(MUI_EventHandlerRC_Eat);
            return(MUI_EventHandlerRC_Eat);
          }

          #if !defined(__amigaos4__)
          // we check wheter the mouse is currently within our object borders
          // and if so we check wheter the newmouse wheel stuff is used or not
          if(data->slider &&
             _isinwholeobject(obj, imsg->MouseX, imsg->MouseY))
          {
            // MouseWheel events are only possible if the mouse is above the
            // object
            if(imsg->Code == NM_WHEEL_UP   || imsg->Code == NM_WHEEL_LEFT ||
               imsg->Code == NM_WHEEL_DOWN || imsg->Code == NM_WHEEL_RIGHT)
            {
              LONG visible;

              get(obj, MUIA_TextEditor_Prop_Visible, &visible);
              if(visible > 0)
              {
                // we scroll about 1/6 of the displayed text by default
                LONG delta = (visible + 3) / 6;

                // make sure that we scroll at least 1 line
                if(delta < 1) delta = 1;

                if(imsg->Code == NM_WHEEL_UP || imsg->Code == NM_WHEEL_LEFT)
                {
                  DoMethod(data->slider, MUIM_Prop_Decrease, delta);
                }
                else
                  DoMethod(data->slider, MUIM_Prop_Increase, delta);
              }

              RETURN(MUI_EventHandlerRC_Eat);
              return MUI_EventHandlerRC_Eat;
            }
          }
          #endif

          // if not we check wheter we have to react on that particular RAWKEY event
          if(ReactOnRawKey(imsg->Code, imsg->Qualifier, imsg, data) == 0)
          {
            RETURN(0);
            return(0);
          }
        }
        break;

        #if defined(__amigaos4__)
        case IDCMP_EXTENDEDMOUSE:
        {
          if(data->slider &&
             _isinwholeobject(obj, imsg->MouseX, imsg->MouseY))
          {
            LONG visible;

            get(obj, MUIA_TextEditor_Prop_Visible, &visible);
            if(visible > 0)
            {
          		struct IntuiWheelData *iwd = (struct IntuiWheelData *)imsg->IAddress;

              // we scroll about 1/6 of the displayed text by default
              LONG delta = (visible + 3) / 6;

              // make sure that we scroll at least 1 line
              if(delta < 1) delta = 1;

          		if(iwd->WheelY < 0 || iwd->WheelX < 0)
                DoMethod(data->slider, MUIM_Prop_Decrease, delta);
	            else if(iwd->WheelY > 0 || iwd->WheelX > 0)
                DoMethod(data->slider, MUIM_Prop_Increase, delta);
            }

            RETURN(MUI_EventHandlerRC_Eat);
            return MUI_EventHandlerRC_Eat;
          }

          RETURN(0);
          return(0);
        }
        break;
        #endif

/*
        case IDCMP_INACTIVEWINDOW:
        {
          if(data->mousemove)
          {
            data->mousemove = FALSE;
            RejectInput(data);
          }
        }
        break;
*/
/*
        case IDCMP_MOUSEMOVE:
          if(data->mousemove && !data->smooth_wait)
            InputTrigger(cl, data);
          break;
*/
        case IDCMP_MOUSEBUTTONS:
        {
          if(data->ypos != data->realypos)
          {
            RETURN(0);
            return(0);
          }

          if((imsg->Code == (IECODE_LBUTTON | IECODE_UP_PREFIX)) && data->mousemove)
          {
            data->mousemove = FALSE;
            RejectInput(data);

            if((data->flags & FLG_ReadOnly) && (data->flags & FLG_AutoClip) && Enabled(data))
              Key_Copy(data);
          }
          else
          {
            if(imsg->Code == IECODE_LBUTTON)
            {
              struct MUI_AreaData *ad = muiAreaData(obj);

              if(((imsg->MouseX >= ad->mad_Box.Left) &&
                 (imsg->MouseX <  ad->mad_Box.Left + ad->mad_Box.Width) &&
                 (imsg->MouseY >= data->ypos) &&
                 (imsg->MouseY <  data->ypos+(data->maxlines * data->height))))
              {
                UWORD last_x = data->CPos_X;
                struct line_node *lastline = data->actualline;

                RequestInput(data);
                data->mousemove = TRUE;
                SetCursor(data->CPos_X, data->actualline, FALSE, data);
                PosFromCursor(imsg->MouseX, imsg->MouseY, data);

                if(imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
                {
                  data->selectmode  = 0;
                }

                if(!(data->blockinfo.enabled && (imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))))
                {
                  // if we already have an enabled block we have to disable it
                  // and clear the marking area with MarkText()
                  if(Enabled(data))
                  {
                    data->blockinfo.enabled = FALSE;
                    MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
                  }

                  data->blockinfo.enabled = TRUE;
                  data->blockinfo.startline = data->actualline;
                  data->blockinfo.startx = data->CPos_X;
                  if(last_x == data->CPos_X && lastline == data->actualline && DoubleClick(data->StartSecs, data->StartMicros, imsg->Seconds, imsg->Micros))
                  {
                    if((data->DoubleClickHook && !CallHook(data->DoubleClickHook, (Object *)data->object, data->actualline->line.Contents, data->CPos_X)) || (!data->DoubleClickHook))
                    {
                      if(!CheckSep(data->actualline->line.Contents[data->CPos_X], data))
                      {
                        if(data->selectmode > 0)
                        {
                          GoStartOfLine(data);
                          data->blockinfo.startx = data->CPos_X;
                          GoEndOfLine(data);

                          // set selectmode to 2 so that PrintLine() knows that the user has tripleclicked
                          // on a line, it will afterwards automatically set to 3 anyway.
                          data->selectmode = 2;

                          // reset the time values
                          data->StartSecs = 0;
                          data->StartMicros = 0;
                        }
                        else
                        {
                          int x = data->CPos_X;

                          while(x > 0 && !CheckSep(*(data->actualline->line.Contents+x-1), data))
                            x--;

                          data->blockinfo.startx = x;
                          data->selectmode  = 1;
                          data->StartSecs = imsg->Seconds;
                          data->StartMicros = imsg->Micros;
                        }
                      }
                      else
                      {
                        // if the user clicked somewhere where we didn't find any separator
                        // we have to check wheter this is already a tripleclick or still a doubleclick
                        // because we ensure that on a tripleclick ALWAYS the whole line is marked
                        // regardless if the user clicked on a actual word that can be separated or not.
                        if(data->selectmode == 1)
                        {
                          GoStartOfLine(data);
                          data->blockinfo.startx = data->CPos_X;
                          GoEndOfLine(data);

                          // set selectmode to 2 so that PrintLine() knows that the user has tripleclicked
                          // on a line, it will afterwards automatically set to 3 anyway.
                          data->selectmode = 2;

                          // reset the time values
                          data->StartSecs = 0;
                          data->StartMicros = 0;
                        }
                        else
                        {
                          if(data->selectmode == 0)
                            data->selectmode = 1;
                          else
                          {
                            data->blockinfo.enabled = FALSE;
                            data->selectmode = 0;
                          }

                          data->StartSecs = imsg->Seconds;
                          data->StartMicros = imsg->Micros;
                        }
                      }
                    }
                  }
                  else
                  {
                    data->blockinfo.enabled = FALSE;
                    data->selectmode  = 0;
                    data->StartSecs = imsg->Seconds;
                    data->StartMicros = imsg->Micros;
                  }
                }
                else
                {
                  if(data->blockinfo.stopline != data->actualline || data->blockinfo.stopx != data->CPos_X)
                    MarkText(data->blockinfo.stopx, data->blockinfo.stopline, data->CPos_X, data->actualline, data);
                }

                data->blockinfo.stopline = data->actualline;
                data->blockinfo.stopx = data->CPos_X;

                SetCursor(data->CPos_X, data->actualline, TRUE, data);
                if(!(data->flags & FLG_ReadOnly))
                  set(_win(obj), MUIA_Window_ActiveObject, obj);
              }
              else
              {
                RETURN(0);
                return(0);
              }
            }
            else
            {
              RETURN(0);
              return(0);
            }
          }
        }
        break;

        default:
        {
          RETURN(0);
          return(0);
        }
      }

      RETURN(MUI_EventHandlerRC_Eat);
      return(MUI_EventHandlerRC_Eat);
    }
    else
    {
      RETURN(0);
      return(0);
    }
  }
  else
  {
    RETURN(0);
    return(0);
  }
}

static VOID DoBlock(BOOL clipboard, BOOL erase, struct InstData *data)
{
  ENTER();

  data->blockinfo.enabled = FALSE;
  CutBlock(data, clipboard, !erase, TRUE);

  LEAVE();
}

static VOID EraseBlock(BOOL clipboard, struct InstData *data)
{
  ENTER();

  if(Enabled(data))
  {
    DoBlock(clipboard, TRUE, data);
    data->HasChanged = TRUE;
  }
  else
  {
    DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NoAreaMarked);
  }

  LEAVE();
}

void Key_Clear(struct InstData *data)
{
  ENTER();

  EraseBlock(FALSE, data);

  LEAVE();
}

enum {Del_BOL = 0, Del_EOL, Del_BOW, Del_EOW};

void Key_DelSomething(ULONG what, struct InstData *data)
{
  ENTER();

  if(data->blockinfo.enabled)
  {
    data->blockinfo.enabled = FALSE;
    MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
  }

  data->blockinfo.startx = data->CPos_X;
  data->blockinfo.startline = data->actualline;

  switch(what)
  {
    case Del_BOL:
      GoStartOfLine(data);
      break;
    case Del_EOL:
      data->flags |= FLG_FreezeCrsr;
      GoEndOfLine(data);
      break;
    case Del_BOW:
      GoPreviousWord(data);
      break;
    case Del_EOW:
      data->flags |= FLG_FreezeCrsr;
      GoNextWord(data);
      break;
  }
  data->blockinfo.stopx = data->CPos_X;
  data->blockinfo.stopline = data->actualline;
  data->blockinfo.enabled = TRUE;
  if(Enabled(data))
      Key_Clear(data);
  else  data->blockinfo.enabled = FALSE;
  data->flags &= ~FLG_FreezeCrsr;

  LEAVE();
}

void Key_DelLine(struct InstData *data)
{
  ENTER();

  if(data->blockinfo.enabled)
  {
    data->blockinfo.enabled = FALSE;
    MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
  }

  if(!data->actualline->next && *data->actualline->line.Contents == '\n')
    GoLeft(data);

  GoStartOfLine(data);

  data->blockinfo.startx = data->CPos_X;
  data->blockinfo.startline = data->actualline;

//  data->flags |= FLG_FreezeCrsr;
  GoEndOfLine(data);
  GoRight(data);
  data->blockinfo.stopx = data->CPos_X;
  data->blockinfo.stopline = data->actualline;
  data->blockinfo.enabled = TRUE;
  if(Enabled(data))
      Key_Clear(data);
  else  data->blockinfo.enabled = FALSE;
//  data->flags &= ~FLG_FreezeCrsr;

  LEAVE();
}


void Key_Cut(struct InstData *data)
{
  ENTER();

  EraseBlock(TRUE, data);

  LEAVE();
}

void Key_Copy (struct InstData *data)
{
  ENTER();

  if(Enabled(data))
  {
    DoBlock(TRUE, FALSE, data);
  }
  else
  {
    DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NoAreaMarked);
  }

  LEAVE();
}

void Key_Paste (struct InstData *data)
{
  BOOL update;
  struct marking block;

  ENTER();

  if(Enabled(data))
    Key_Clear(data);

  block.startline = data->actualline;
  block.startx = data->CPos_X;
//  SetCursor(data->CPos_X, data->actualline, FALSE, data);
  data->update = FALSE;
  update = PasteClip(data->CPos_X, data->actualline, data);
  if(update)
  {
    block.stopline = data->actualline;
    block.stopx = data->CPos_X;
    AddToUndoBuffer(pasteblock, (char *)&block, data);
    data->pixel_x = 0;
  }

  LEAVE();
}

void Key_Tab (struct InstData *data)
{
  ENTER();

  if(Enabled(data))
    Key_Clear(data);

  {
    struct marking block =
    {
      TRUE,
      data->actualline,
      data->CPos_X,
      data->actualline,
      data->CPos_X+data->TabSize
    };

#ifndef ClassAct
    CheckWord(data);
#endif
    AddToUndoBuffer(pasteblock, (char *)&block, data);
    data->CPos_X += data->TabSize;
    PasteChars(data->CPos_X-data->TabSize, data->actualline, data->TabSize, "            ", NULL, data);
  }

  LEAVE();
}

void Key_Return (struct InstData *data)
{
  ENTER();

  if(Enabled(data))
    Key_Clear(data);

#ifndef ClassAct
  CheckWord(data);
#endif
  AddToUndoBuffer(splitline, NULL, data);
  SplitLine(data->CPos_X, data->actualline, TRUE, NULL, data);

  LEAVE();
}

void Key_Backspace (struct InstData *data)
{
  ENTER();

  if(Enabled(data))
  {
    Key_Clear(data);
  }
  else
  {
    if(data->CPos_X > 0)
    {
      AddToUndoBuffer(backspacechar, data->actualline->line.Contents+--data->CPos_X, data);
      RemoveChars(data->CPos_X, data->actualline, 1, data);
    }
    else
    {
      if(data->actualline->previous)
      {
        data->actualline = data->actualline->previous;
        data->CPos_X = data->actualline->line.Length-1;
        AddToUndoBuffer(backspacemerge, NULL, data);
        MergeLines(data->actualline, data);
      }
    }
  }

  LEAVE();
}

void Key_Delete (struct InstData *data)
{
  ENTER();

  if(Enabled(data))
  {
    Key_Clear(data);
  }
  else
  {
    if(data->actualline->line.Length > (ULONG)(data->CPos_X+1))
    {
      AddToUndoBuffer(deletechar, data->actualline->line.Contents+data->CPos_X, data);
      RemoveChars(data->CPos_X, data->actualline, 1, data);
    }
    else
    {
      if(data->actualline->next)
      {
        AddToUndoBuffer(mergelines, NULL, data);
        MergeLines(data->actualline, data);
      }
    }
  }

  LEAVE();
}

/*
void cutcopypasteerase (UBYTE key, ULONG qualifier, struct InstData *data)
{
    UWORD updatefrom;

  data->update = FALSE;
  data->blockinfo.enabled = FALSE;
  updatefrom = CutBlock(data, Clipboard, NoCut);
  if(code != 0x08 && code != 0x7f && (!NoInsert))
  {
    result = MatchVanilla(code, qualifier, data);
  }
  else
  {
      ULONG blocklength = 0;

    if(NoInsert && (!Clipboard))
    {
        int t_updatefrom;
        struct marking block;

      block.startline = data->actualline;
      block.startx = data->CPos_X;
      t_updatefrom = PasteClip(data->CPos_X, data->actualline, data);
      if(t_updatefrom >= 0)
      {
        block.stopline = data->actualline;
        block.stopx = data->CPos_X;
        AddToUndoBuffer(pasteblock, (char *)&block, data);
        if(t_updatefrom < updatefrom)
          updatefrom = t_updatefrom;
      }
    }

    if(!NoCut)
    {
      data->HasChanged = TRUE;
    }
    else
    {
      data->update = TRUE;
      MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
    }
    data->pixel_x = 0;
  }
  data->update = TRUE;
  if(!NoCut)
    DumpText(data->visual_y+updatefrom, updatefrom, stopline, TRUE, data);
}
*/

void Key_Normal(UBYTE key, struct InstData *data)
{
  ENTER();

  if(Enabled(data))
  {
    Key_Clear(data);
  }

#ifndef ClassAct
  if((!IsAlpha(data->mylocale, key)) && key != '-')
    CheckWord(data);
#endif

  AddToUndoBuffer(pastechar, NULL, data);
  PasteChars(data->CPos_X++, data->actualline, 1, (char *)&key, NULL, data);
  if(data->WrapBorder && (data->CPos_X > data->WrapBorder) && (key != ' '))
  {
    ULONG xpos = data->CPos_X;

    while(--xpos && *(data->actualline->line.Contents+xpos) != ' ');
    if(xpos)
    {
        ULONG length = data->CPos_X-xpos;

      data->CPos_X = xpos;
      AddToUndoBuffer(splitline, NULL, data);
      SplitLine(data->CPos_X, data->actualline, TRUE, NULL, data);
      AddToUndoBuffer(deletechar, data->actualline->line.Contents+data->CPos_X, data);
      data->CPos_X = length-1;
      RemoveChars(0, data->actualline, 1, data);
    }
  }

  LEAVE();
}

/*----------------*
 * Convert Rawkey *
 *----------------*/
static LONG ConvertKey(UBYTE key, ULONG qualifier, struct IntuiMessage *imsg, struct InstData *data)
{
  LONG result = TRUE;
  UBYTE code = 0;
#ifndef ClassAct
  struct InputEvent event;

  ENTER();

  event.ie_NextEvent    = NULL;
  event.ie_Class        = IECLASS_RAWKEY;
  event.ie_SubClass     = 0;
  event.ie_Code         = key;
  event.ie_Qualifier    = qualifier;
  event.ie_EventAddress = (APTR *) *((ULONG *)imsg->IAddress);

  if(MapRawKey(&event, (STRPTR)&code, 1, NULL) > 0)
#else
  ENTER();

  // XXX: why shall imsg be an InputEvent!?
  if(MapRawKey((struct InputEvent *)imsg, (STRPTR)&code, 1, NULL) > 0)
#endif
  {
    SHOWVALUE(DBF_INPUT, code);

#ifdef FILTER_NONPRINTABLE
    if((code < 32) || ((code > 126) && (code < 160)))
#else
    if(code < 32)
#endif
    {
      result = FALSE;
    }
    else
    {
      data->pixel_x = 0;
      Key_Normal(code, data);
    }
  }
  else  result = FALSE;

  RETURN(result);
  return(result);
}

/*  data->pixel_x = 0;
  return(MUI_EventHandlerRC_Eat);
*/

static BOOL MatchQual(ULONG input, ULONG match, UWORD action, struct InstData *data)
{
  ENTER();

  if((match & IEQUALIFIER_SHIFT) && (input & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)))
  {
    match &= ~IEQUALIFIER_SHIFT;
    input &= ~(IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT);
  }
  if((match & IEQUALIFIER_ALT) && (input & (IEQUALIFIER_LALT | IEQUALIFIER_RALT)))
  {
    match &= ~IEQUALIFIER_ALT;
    input &= ~(IEQUALIFIER_LALT | IEQUALIFIER_RALT);
  }
  if((match & IEQUALIFIER_COMMAND) && (input & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND)))
  {
    match &= ~IEQUALIFIER_COMMAND;
    input &= ~(IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND);
  }

  LEAVE();
  return(input == match || (((input & ~data->blockqual) == match) && action < kSuggestWord));
}


/*---------------------------------*
 * Function to handle a cursormove *
 *---------------------------------*/
static LONG FindKey (UBYTE key, ULONG qualifier, struct InstData *data)
{
  struct keybindings *t_keys = data->RawkeyBindings;
  BOOL speed = FALSE;

  ENTER();

  if(t_keys == 0)
    t_keys = keys;

#ifdef FAST_SCROLL
  if(qualifier & IEQUALIFIER_REPEAT)
  {
    static ULONG  repeatcount = 0;

    if(repeatcount == 20)
        speed = TRUE;
    else  repeatcount++;
  }
  else  repeatcount = 0;
#endif

  qualifier &= ~(IEQUALIFIER_RELATIVEMOUSE | IEQUALIFIER_REPEAT | IEQUALIFIER_CAPSLOCK);
  while(t_keys->keydata.code != (unsigned short)-1)
  {
    if((key == t_keys->keydata.code) && MatchQual(qualifier, t_keys->keydata.qual, t_keys->keydata.act, data))
    {
      if(data->flags & FLG_ReadOnly)
      {
          LONG new_y = data->visual_y-1;

        switch(t_keys->keydata.act)
        {
          case mTop:
            new_y = 0;
            break;
          case mPreviousPage:
            new_y -= data->maxlines;
            break;
          case mUp:
            new_y -= 1;
            if(speed)
              new_y -= 1;
            break;
          case mBottom:
            new_y = data->totallines-data->maxlines;
            break;
          case mNextPage:
            new_y += data->maxlines;
            break;
          case mDown:
            new_y += 1;
            if(speed)
              new_y += 1;
            break;
          case kCopy:
            Key_Copy(data);
            break;
#ifndef ClassAct
          case kNextGadget:
            set(_win(data->object), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_Next);
            break;
#endif
        }
        if(new_y != data->visual_y-1)
        {
          if(new_y > data->totallines-data->maxlines)
            new_y = data->totallines-data->maxlines;
          if(new_y < 0)
            new_y = 0;
          set(data->object, MUIA_TextEditor_Prop_First, new_y*data->height);

          RETURN(4);
          return(4);
        }
      }
      else
      {
        switch(t_keys->keydata.act)
        {
          case mTop:
            GoTop(data);
            RETURN(TRUE);
            return(TRUE);
          case mPreviousLine:
            GoPreviousLine(data);
            RETURN(TRUE);
            return(TRUE);
          case mPreviousPage:
            GoPreviousPage(data);
            RETURN(FALSE);
            return(FALSE);
          case mUp:
            GoUp(data);
            if(speed)
              GoUp(data);
            RETURN(FALSE);
            return(FALSE);
          case mBottom:
            GoBottom(data);
            RETURN(TRUE);
            return(TRUE);
          case mNextLine:
            GoNextLine(data);
            RETURN(TRUE);
            return(TRUE);
          case mNextPage:
            GoNextPage(data);
            RETURN(FALSE);
            return(FALSE);
          case mDown:
            GoDown(data);
            if(speed)
              GoDown(data);
            RETURN(FALSE);
            return(FALSE);
          case mNextWord:
            GoNextWord(data);
            RETURN(TRUE);
            return(TRUE);
          case mNextSentence:
            GoNextSentence(data);
            RETURN(TRUE);
            return(TRUE);
          case mEndOfLine:
            GoEndOfLine(data);
            RETURN(TRUE);
            return(TRUE);
          case mRight:
            GoRight(data);
            if(speed)
              GoRight(data);
            RETURN(TRUE);
            return(TRUE);
          case mPreviousWord:
            GoPreviousWord(data);
            RETURN(TRUE);
            return(TRUE);
          case mPreviousSentence:
            GoPreviousSentence(data);
            RETURN(TRUE);
            return(TRUE);
          case mStartOfLine:
            GoStartOfLine(data);
            RETURN(TRUE);
            return(TRUE);
          case mLeft:
            GoLeft(data);
            if(speed)
              GoLeft(data);
            RETURN(TRUE);
            return(TRUE);
          case kSuggestWord:
#ifndef ClassAct
            SuggestWord(data);
#endif
            break;
          case kBackspace:
            Key_Backspace(data);
            break;
          case kDelete:
            Key_Delete(data);
            break;
          case kReturn:
            Key_Return(data);
            break;
          case kTab:
            Key_Tab(data);
            break;
          case kUndo:
            Undo(data);
            break;
          case kRedo:
            Redo(data);
            break;
          case kCut:
            Key_Cut(data);
            break;
          case kCopy:
            Key_Copy(data);
            break;
          case kPaste:
            Key_Paste(data);
            break;
          case kDelEOL:
            Key_DelSomething(Del_EOL, data);
            break;
          case kDelBOL:
            Key_DelSomething(Del_BOL, data);
            break;
          case kDelEOW:
            Key_DelSomething(Del_EOW, data);
            break;
          case kDelBOW:
            Key_DelSomething(Del_BOW, data);
            break;
          case kDelLine:
            Key_DelLine(data);
            break;
          case kNextGadget:
#ifndef ClassAct
            set(_win(data->object), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_Next);
#endif
            break;
          case kGotoBookmark1:
            GotoBookmark(0, data);
            RETURN(TRUE);
            return(TRUE);
          case kGotoBookmark2:
            GotoBookmark(1, data);
            RETURN(TRUE);
            return(TRUE);
          case kGotoBookmark3:
            GotoBookmark(2, data);
            RETURN(TRUE);
            return(TRUE);
          case kSetBookmark1:
            SetBookmark(0, data);
            break;
          case kSetBookmark2:
            SetBookmark(1, data);
            break;
          case kSetBookmark3:
            SetBookmark(2, data);
            break;
        }

        RETURN(3);
        return(3);
      }
    }
    t_keys++;
  }

  RETURN(2);
  return(2);
}

static LONG ReactOnRawKey(UBYTE key, ULONG qualifier, struct IntuiMessage *imsg, struct InstData *data)
{
  struct line_node *oldactualline = data->actualline;
  UWORD oldCPos_X = data->CPos_X;
  LONG result = TRUE;

  ENTER();

  if(key <= IECODE_KEY_CODE_LAST)
  {
    LONG dummy = FindKey(key, qualifier, data);
    if(dummy == 1)
    {
      data->pixel_x = 0;
    }

    if(dummy == 1 || dummy == 0)
    {
      if((data->CPos_X != oldCPos_X || oldactualline != data->actualline) || (!(qualifier & data->blockqual) && data->blockinfo.enabled))
      {
        SetCursor(oldCPos_X, oldactualline, FALSE, data);

        if(!(data->flags & FLG_ReadOnly))
        {
          if(qualifier & data->blockqual)
          {
            data->blockinfo.stopline = data->actualline;
            data->blockinfo.stopx = data->CPos_X;
            if(!data->blockinfo.enabled)
            {
              data->blockinfo.enabled = TRUE;
              data->blockinfo.startline = oldactualline;
              data->blockinfo.startx = oldCPos_X;
            }

            MarkText(oldCPos_X, oldactualline, data->CPos_X, data->actualline, data);
          }
          else
          {
            data->flags &= ~FLG_ARexxMark;
            if(data->blockinfo.enabled)
            {
              data->blockinfo.enabled = FALSE;
              MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
            }
          }

          ScrollIntoDisplay(data);
          SetCursor(data->CPos_X, data->actualline, TRUE, data);
        }
      }
    }
    else
    {
      if(dummy == 3)
      {
        data->pixel_x = 0;
      }
      else
      {
        if(((dummy != 4 && ((data->flags & FLG_ReadOnly) || (qualifier & IEQUALIFIER_RCOMMAND))) || !ConvertKey(key, qualifier, imsg, data)))
        {
          result = FALSE;
        }

        if(dummy == 4)
        {
          result = TRUE;
        }
/*
        else
        {
          if(((data->flags & FLG_ReadOnly || qualifier & IEQUALIFIER_RCOMMAND) || !ConvertKey(key, qualifier, imsg, data)))
            result = FALSE;
        }
*/
      }
    }
  }
  else
  {
    result = FALSE;
  }

  if(result)
    ScrollIntoDisplay(data);

  RETURN(result);
  return(result);
}
/*------------------------------------------------------*
 * Make sure that the cursor is inside the visible area *
 *------------------------------------------------------*/
void ScrollIntoDisplay(struct InstData *data)
{
  struct pos_info pos;
  LONG diff;

  ENTER();

  if(data->shown)
  {
    OffsetToLines(data->CPos_X, data->actualline, &pos, data);
    diff = pos.lines + LineToVisual(data->actualline, data) - 1;
    if(diff > data->maxlines)
    {
      data->visual_y += diff-data->maxlines;
      ScrollUp(0, diff-data->maxlines, data);
    }
    if(diff < 1)
    {
      data->visual_y += diff-1;
      ScrollDown(0, (-diff)+1, data);
    }
  }

  LEAVE();
}
/*------------------------*
 * Update the marked area *
 *------------------------*/
void MarkText(LONG x1, struct line_node *line1, LONG x2, struct line_node *line2, struct InstData *data)
{
  struct marking newblock;
  struct marking fakeblock;
  struct line_node *startline;
  struct line_node *stopline;
  struct pos_info pos1;
  struct pos_info pos2;
  LONG startx;
  LONG stopx;
  LONG line_nr1;
  LONG line_nr2;

  ENTER();

  fakeblock.startx = x1;
  fakeblock.startline = line1;
  fakeblock.stopx = x2;
  fakeblock.stopline = line2;

  NiceBlock(&fakeblock, &newblock);
  startx    = newblock.startx;
  stopx     = newblock.stopx;
  startline = newblock.startline;
  stopline  = newblock.stopline;

  line_nr1 = LineToVisual(startline, data) - 1;
  line_nr2 = LineToVisual(stopline, data) - 1;

  OffsetToLines(startx, startline, &pos1, data);
  OffsetToLines(stopx, stopline, &pos2, data);

  data->blockinfo.stopx = x2;
  data->blockinfo.stopline = line2;

  if((line_nr1 += pos1.lines-1) < 0)
    line_nr1 = 0;

  if((line_nr2 += pos2.lines-1) >= data->maxlines)
    line_nr2 = data->maxlines-1;

  if(line_nr1 <= line_nr2)
    DumpText(data->visual_y+line_nr1, line_nr1, line_nr2+1, FALSE, data);

  LEAVE();
}
