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

 $Id: HandleInput.c,v 1.3 2005/04/04 21:59:02 damato Exp $

***************************************************************************/

#include <clib/alib_protos.h>
#include <libraries/mui.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/locale.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "TextEditor_mcc.h"
#include "private.h"

extern struct keybindings keys[];

ULONG RAWToANSI (struct IntuiMessage *imsg)
{
    struct  InputEvent  event;
    UBYTE   code = 0;

  event.ie_NextEvent      = NULL;
  event.ie_Class          = IECLASS_RAWKEY;
  event.ie_SubClass       = 0;
  event.ie_Code           = imsg->Code;
  event.ie_Qualifier      = imsg->Qualifier;
  event.ie_EventAddress   = (APTR *) *((ULONG *)imsg->IAddress);

  MapRawKey(&event, &code, 1, NULL);
  return(code);
}

ULONG HandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct InstData *data = INST_DATA(cl, obj);
    long  tresult;

  if((msg->muikey == MUIKEY_UP) && data->KeyUpFocus && _win(obj) && (data->firstline == data->actualline))
  {
    set(_win(obj), MUIA_Window_ActiveObject, data->KeyUpFocus);
    return MUI_EventHandlerRC_Eat;
  }
  else if(!(data->flags & FLG_Ghosted) && data->shown && msg->imsg && (msg->muikey != MUIKEY_GADGET_PREV) && (msg->muikey != MUIKEY_GADGET_NEXT))
  {
      ULONG activeobj, defaultobj;

    get(_win(obj), MUIA_Window_ActiveObject, &activeobj);
    get(_win(obj), MUIA_Window_DefaultObject, &defaultobj);

    if(data->CtrlChar && activeobj != (ULONG)obj && defaultobj != (ULONG)obj && RAWToANSI(msg->imsg) == data->CtrlChar)
    {
      set(_win(obj), MUIA_Window_ActiveObject, obj);
      return(MUI_EventHandlerRC_Eat);
    }

    if((msg->imsg->Class == IDCMP_MOUSEBUTTONS) || (activeobj == (ULONG)obj) || (data->flags & FLG_ReadOnly && defaultobj == (ULONG)obj && !activeobj))
    {
      if(!(data->flags & FLG_Draw))
      {
        data->UpdateInfo = msg;
        MUI_Redraw(obj, MADF_DRAWUPDATE);
        return((ULONG)data->UpdateInfo);
      }

      switch(msg->imsg->Class)
      {
        case IDCMP_RAWKEY:
          if(data->ypos != data->realypos)
            return(MUI_EventHandlerRC_Eat);
          tresult = ReactOnRawKey(msg->imsg->Code, msg->imsg->Qualifier, msg->imsg, data);
          if(tresult == 0)
            return(0);
          break;

/*        case IDCMP_INACTIVEWINDOW:
        {
          if(data->mousemove)
          {
            data->mousemove = FALSE;
            RejectInput(data);
          }
        }
        break;
*/
/*        case IDCMP_MOUSEMOVE:
          if(data->mousemove && !data->smooth_wait)
            InputTrigger(cl, data);
          break;
*/        case IDCMP_MOUSEBUTTONS:
          if(data->ypos != data->realypos)
          {
            return(0);
          }
          if((msg->imsg->Code == (IECODE_LBUTTON | IECODE_UP_PREFIX)) && data->mousemove)
          {
            data->mousemove = FALSE;
            RejectInput(data);
            if((data->flags & FLG_ReadOnly) && (data->flags & FLG_AutoClip) && Enabled(data))
              Key_Copy(data);
          }
          else
          {
            if(msg->imsg->Code == IECODE_LBUTTON)
            {
                struct MUI_AreaData *ad = muiAreaData(obj);

              if(((msg->imsg->MouseX >= ad->mad_Box.Left) &&
                 (msg->imsg->MouseX <  ad->mad_Box.Left + ad->mad_Box.Width) &&
                 (msg->imsg->MouseY >= data->ypos) &&
                 (msg->imsg->MouseY <  data->ypos+(data->maxlines * data->height))))
              {
                UWORD last_x = data->CPos_X; struct line_node *lastline = data->actualline;
                RequestInput(data);
                data->mousemove = TRUE;
                SetCursor(data->CPos_X, data->actualline, FALSE, data);
                PosFromCursor(msg->imsg->MouseX, msg->imsg->MouseY, data);

                if(msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
                {
                  data->selectmode  = 0;
                }

                if(!((data->blockinfo.enabled) && (msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))))
                {
                  if(Enabled(data))
                  {
                    data->blockinfo.enabled = FALSE;
                    MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
                  }

                  data->blockinfo.enabled = TRUE;
                  data->blockinfo.startline = data->actualline;
                  data->blockinfo.startx = data->CPos_X;
                  if(last_x == data->CPos_X && lastline == data->actualline && DoubleClick(data->StartSecs, data->StartMicros, msg->imsg->Seconds, msg->imsg->Micros))
                  {
                    if((data->DoubleClickHook && !CallHook(data->DoubleClickHook, (Object *)data->object, data->actualline->line.Contents, data->CPos_X)) || (!data->DoubleClickHook))
                    {
                      if(!CheckSep(data->actualline->line.Contents[data->CPos_X], data))
//                      if(*(data->actualline->line.Contents+data->CPos_X) != ' ' && *(data->actualline->line.Contents+data->CPos_X) != '\n')
                      {
                        if(data->selectmode)
                        {
                          GoStartOfLine(data);
                          data->blockinfo.startx = data->CPos_X;
                          GoEndOfLine(data);
                          data->blockinfo.stopline = data->actualline;
                          data->blockinfo.stopx = data->CPos_X;
                          MarkText(data->blockinfo.stopx, data->blockinfo.stopline, data->CPos_X, data->actualline, data);
                          data->selectmode  = 2;
                          data->StartSecs = 0;
                          data->StartMicros = 0;
                        }
                        else
                        {
                            int x = data->CPos_X;

                          while(x > 0 && !CheckSep(*(data->actualline->line.Contents+x-1), data))
//                          while(x > 0 && *(data->actualline->line.Contents+x-1) != ' ')
                            x--;

                          data->blockinfo.startx = x;
                          data->selectmode  = 1;
                          data->StartSecs = msg->imsg->Seconds;
                          data->StartMicros = msg->imsg->Micros;
                        }
                      }
                      else
                      {
                        data->selectmode  = 0;
                        data->StartSecs = msg->imsg->Seconds;
                        data->StartMicros = msg->imsg->Micros;
                      }
                    }
                  }
                  else
                  {
                    data->selectmode  = 0;
                    data->StartSecs = msg->imsg->Seconds;
                    data->StartMicros = msg->imsg->Micros;
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
                return(0);
              }
            }
            else
            {
              return(0);
            }
          }
          break;
        default:
          return(0);
      }
      return(MUI_EventHandlerRC_Eat);
    }
    else  return(0);
  }
  else  return(0);
}

VOID DoBlock (BOOL clipboard, BOOL erase, struct InstData *data)
{
  data->blockinfo.enabled = FALSE;
  CutBlock(data, clipboard, !erase, TRUE);
}

VOID EraseBlock (BOOL clipboard, struct InstData *data)
{
  if(Enabled(data))
  {
    DoBlock(clipboard, TRUE, data);
    data->HasChanged = TRUE;
  }
  else
  {
    DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NoAreaMarked);
  }
}

void Key_Clear (struct InstData *data)
{
  EraseBlock(FALSE, data);
}

enum {Del_BOL = 0, Del_EOL, Del_BOW, Del_EOW};

void Key_DelSomething (ULONG what, struct InstData *data)
{
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
}

void Key_DelLine (struct InstData *data)
{
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
}


void Key_Cut (struct InstData *data)
{
  EraseBlock(TRUE, data);
}

void Key_Copy (struct InstData *data)
{
  if(Enabled(data))
  {
    DoBlock(TRUE, FALSE, data);
  }
  else
  {
    DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NoAreaMarked);
  }
}

void Key_Paste (struct InstData *data)
{
    BOOL  update;
    struct marking block;

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
}

void Key_Tab (struct InstData *data)
{
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
}

void Key_Return (struct InstData *data)
{
  if(Enabled(data))
    Key_Clear(data);

#ifndef ClassAct
  CheckWord(data);
#endif
  AddToUndoBuffer(splitline, NULL, data);
  SplitLine(data->CPos_X, data->actualline, TRUE, NULL, data);
}

void Key_Backspace (struct InstData *data)
{
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
}

void Key_Delete (struct InstData *data)
{
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

void Key_Normal (UBYTE key, struct InstData *data)
{
  if(Enabled(data))
  {
    Key_Clear(data);
  }

#ifndef ClassAct
  if((!IsAlpha(data->mylocale, key)) && key != '-')
    CheckWord(data);
#endif

  AddToUndoBuffer(pastechar, NULL, data);
  PasteChars(data->CPos_X++, data->actualline, 1, &key, NULL, data);
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
}

/*----------------*
 * Convert Rawkey *
 *----------------*/
long  ConvertKey (UBYTE key, ULONG qualifier, struct IntuiMessage *imsg, struct InstData *data)
{
    long    result = TRUE;
    unsigned char        code;
#ifndef ClassAct
    struct   InputEvent  event;

  event.ie_NextEvent      = NULL;
  event.ie_Class          = IECLASS_RAWKEY;
  event.ie_SubClass       = 0;
  event.ie_Code           = key;
  event.ie_Qualifier      = qualifier;
  event.ie_EventAddress   = (APTR *) *((ULONG *)imsg->IAddress);

  if(MapRawKey(&event, &code, 1, NULL) > 0)
#else
  if(MapRawKey((struct InputEvent *)imsg, &code, 1, NULL) > 0)
#endif
  {
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

  return(result);
}

/*  data->pixel_x = 0;
  return(MUI_EventHandlerRC_Eat);

*/

BOOL MatchQual (ULONG input, ULONG match, UWORD action, struct InstData *data)
{
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
  return(input == match || (((input & ~data->blockqual) == match) && action < kSuggestWord));
}


/*---------------------------------*
 * Function to handle a cursormove *
 *---------------------------------*/
long FindKey (unsigned char key, unsigned long qualifier, struct InstData *data)
{
  struct   keybindings *t_keys = data->RawkeyBindings;
  BOOL    speed = FALSE;

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
          return(4);
        }
      }
      else
      {
        switch(t_keys->keydata.act)
        {
          case mTop:
            GoTop(data);
            return(TRUE);
          case mPreviousLine:
            GoPreviousLine(data);
            return(TRUE);
          case mPreviousPage:
            GoPreviousPage(data);
            return(FALSE);
          case mUp:
            GoUp(data);
            if(speed)
              GoUp(data);
            return(FALSE);
          case mBottom:
            GoBottom(data);
            return(TRUE);
          case mNextLine:
            GoNextLine(data);
            return(TRUE);
          case mNextPage:
            GoNextPage(data);
            return(FALSE);
          case mDown:
            GoDown(data);
            if(speed)
              GoDown(data);
            return(FALSE);
          case mNextWord:
            GoNextWord(data);
            return(TRUE);
          case mNextSentence:
            GoNextSentence(data);
            return(TRUE);
          case mEndOfLine:
            GoEndOfLine(data);
            return(TRUE);
          case mRight:
            GoRight(data);
            if(speed)
              GoRight(data);
            return(TRUE);
          case mPreviousWord:
            GoPreviousWord(data);
            return(TRUE);
          case mPreviousSentence:
            GoPreviousSentence(data);
            return(TRUE);
          case mStartOfLine:
            GoStartOfLine(data);
            return(TRUE);
          case mLeft:
            GoLeft(data);
            if(speed)
              GoLeft(data);
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
            return(TRUE);
          case kGotoBookmark2:
            GotoBookmark(1, data);
            return(TRUE);
          case kGotoBookmark3:
            GotoBookmark(2, data);
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
        return(3);
      }
    }
    t_keys++;
  }
  return(2);
}

long ReactOnRawKey(unsigned char key, ULONG qualifier, struct IntuiMessage *imsg, struct InstData *data)
{
  BOOL result = TRUE;
  long dummy;
  struct line_node *oldactualline = data->actualline;
  UWORD oldCPos_X = data->CPos_X;

  if(key <= IECODE_KEY_CODE_LAST)
  {
    dummy = FindKey(key, qualifier, data);
    if(dummy == TRUE)
    {
      data->pixel_x = 0;
    }
    if(dummy == TRUE || dummy == FALSE)
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
/*        else
        {
          if(((data->flags & FLG_ReadOnly || qualifier & IEQUALIFIER_RCOMMAND) || !ConvertKey(key, qualifier, imsg, data)))
            result = FALSE;
        }
*/      }
    }
  }
  else
  {
    result = FALSE;
  }

  if(result)
    ScrollIntoDisplay(data);

  return(result);
}
/*------------------------------------------------------*
 * Make sure that the cursor is inside the visible area *
 *------------------------------------------------------*/
void  ScrollIntoDisplay (struct InstData *data)
{
    struct   pos_info pos;
    LONG      diff;

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
}
/*------------------------*
 * Update the marked area *
 *------------------------*/
void  MarkText    (LONG x1, struct line_node *line1, LONG x2, struct line_node *line2, struct InstData *data)
{
    struct   marking  newblock, fakeblock;
      LONG   startx, stopx;
    struct   line_node   *startline, *stopline;

  fakeblock.startx = x1;
  fakeblock.startline = line1;
  fakeblock.stopx = x2;
  fakeblock.stopline = line2;

  NiceBlock(&fakeblock, &newblock);
  startx    = newblock.startx;
  stopx     = newblock.stopx;
  startline = newblock.startline;
  stopline  = newblock.stopline;

  {
      struct pos_info pos1, pos2;
      LONG   line_nr1 = LineToVisual(startline, data) - 1,
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
    {
      DumpText(data->visual_y+line_nr1, line_nr1, line_nr2+1, FALSE, data);
    }
  }
}
