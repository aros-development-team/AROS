/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2009 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id$

***************************************************************************/

#include <libraries/mui.h>
#include <intuition/classes.h>

#if !defined(__amigaos4__)
#include <clib/alib_protos.h>
#endif
#include <clib/macros.h>

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/locale.h>
#include <proto/utility.h>
#include <proto/layers.h>

#include <stdlib.h>

#include "private.h"

#if !defined(__amigaos4__)
#include "newmouse.h"
#endif

static BOOL ReactOnRawKey(struct IntuiMessage *imsg, struct InstData *data);

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

IPTR HandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
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

    activeobj = (Object *)xget(_win(obj), MUIA_Window_ActiveObject);
    defaultobj = (Object *)xget(_win(obj), MUIA_Window_DefaultObject);

    if(data->CtrlChar && activeobj != obj && defaultobj != obj && RAWToANSI(imsg) == data->CtrlChar)
    {
      set(_win(obj), MUIA_Window_ActiveObject, obj);

      RETURN(MUI_EventHandlerRC_Eat);
      return(MUI_EventHandlerRC_Eat);
    }

    // we check if this is a mousemove input message and if
    // so we check whether the mouse is currently over our
    // texteditor object or not.
    if(imsg->Class == IDCMP_MOUSEMOVE)
    {
      BOOL isOverObject = FALSE;

      if(_isinobject(obj, msg->imsg->MouseX, msg->imsg->MouseY))
      {
        #if defined(__MORPHOS__)
        if (IS_MORPHOS2)
          isOverObject = TRUE;
        #endif

        if (isOverObject == FALSE)
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

      D(DBF_INPUT, "IDCMP_MOUSEMOVE");
    }
    else
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
          D(DBF_INPUT, "HandleInput rawkey code=%02x qual=%04x", imsg->Code, imsg->Qualifier);

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
              LONG visible = xget(obj, MUIA_TextEditor_Prop_Visible);

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
          if(ReactOnRawKey(imsg, data) == FALSE)
          {
            D(DBF_INPUT, "not reacted");
            RETURN(0);
            return(0);
          }
          else
          {
            D(DBF_INPUT, "reacted");
            RETURN(MUI_EventHandlerRC_Eat);
            return(MUI_EventHandlerRC_Eat);
          }
        }
        break;

        #if defined(__amigaos4__)
        case IDCMP_EXTENDEDMOUSE:
        {
          if(data->slider &&
             _isinwholeobject(obj, imsg->MouseX, imsg->MouseY))
          {
            LONG visible = xget(obj, MUIA_TextEditor_Prop_Visible);

            if(visible > 0)
            {
              struct IntuiWheelData *iwd = (struct IntuiWheelData *)imsg->IAddress;

              // we scroll about 1/6 of the displayed text by default
              LONG delta = (visible + 3) / 6;

              // make sure that we scroll at least 1 line
              if(delta < 1)
                delta = 1;

              D(DBF_INPUT, "WheelX: %ld WheelY: %ld", iwd->WheelX, iwd->WheelY);

              if(iwd->WheelY < 0 || iwd->WheelX < 0)
                DoMethod(data->slider, MUIM_Prop_Decrease, delta * abs(MIN(iwd->WheelX, iwd->WheelY)));
              else if(iwd->WheelY > 0 || iwd->WheelX > 0)
                DoMethod(data->slider, MUIM_Prop_Increase, delta * abs(MAX(iwd->WheelX, iwd->WheelY)));
            }

            RETURN(MUI_EventHandlerRC_Eat);
            return MUI_EventHandlerRC_Eat;
          }

          RETURN(0);
          return(0);
        }
        break;
        #endif

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
            // user has pressed the left mousebutton
            if(imsg->Code == IECODE_LBUTTON)
            {
              struct MUI_AreaData *ad = muiAreaData(obj);

              if(((imsg->MouseX >= ad->mad_Box.Left) &&
                 (imsg->MouseX <  ad->mad_Box.Left + ad->mad_Box.Width) &&
                 (imsg->MouseY >= data->ypos) &&
                 (imsg->MouseY <  data->ypos+(data->maxlines * data->height))))
              {
                if((data->flags & FLG_Active) == 0 && (data->flags & FLG_Activated) == 0 &&
                   (data->flags & FLG_ActiveOnClick) != 0 && Enabled(data) && (imsg->Qualifier & IEQUALIFIER_CONTROL))
                {
                  // in case the user hold the control key while pressing in an
                  // inactive object we go and just activate it and let the MUIM_GoActive
                  // function refresh the selected area.
                  set(_win(obj), MUIA_Window_ActiveObject, obj);
                }
                else
                {
                  UWORD last_x = data->CPos_X;
                  struct line_node *lastline = data->actualline;

                  RequestInput(data);
                  data->mousemove = TRUE;

                  data->flags |= FLG_Activated;
                  SetCursor(data->CPos_X, data->actualline, FALSE, data);
                  PosFromCursor(imsg->MouseX, imsg->MouseY, data);

                  if(imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
                    data->selectmode  = 0;

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
                      if((data->DoubleClickHook && !CallHook(data->DoubleClickHook, (Object *)data->object, data->actualline->line.Contents, data->CPos_X, imsg->Qualifier)) || (!data->DoubleClickHook))
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

                  if((data->flags & FLG_ActiveOnClick) != 0)
                    set(_win(obj), MUIA_Window_ActiveObject, obj);
                }
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
  }

  RETURN(0);
  return(0);
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

  ScrollIntoDisplay(data);
  EraseBlock(TRUE, data);

  LEAVE();
}

void Key_Copy(struct InstData *data)
{
  ENTER();

  if(Enabled(data))
  {
    ScrollIntoDisplay(data);
    DoBlock(TRUE, FALSE, data);
  }
  else
    DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NoAreaMarked);

  LEAVE();
}

void Key_Paste(struct InstData *data)
{
  BOOL update;
  struct marking block;

  ENTER();

  if(Enabled(data))
    Key_Clear(data);
  else
    ScrollIntoDisplay(data);

  block.startline = data->actualline;
  block.startx = data->CPos_X;
//  SetCursor(data->CPos_X, data->actualline, FALSE, data);
  data->update = FALSE;
  update = PasteClip(data->CPos_X, data->actualline, data);
  if(update)
  {
    block.stopline = data->actualline;
    block.stopx = data->CPos_X;
    AddToUndoBuffer(ET_PASTEBLOCK, (char *)&block, data);
    data->pixel_x = 0;
  }

  LEAVE();
}

void Key_Tab(struct InstData *data)
{
  ENTER();

  if(Enabled(data))
    Key_Clear(data);
  else
    ScrollIntoDisplay(data);

  {
    struct marking block =
    {
      TRUE,
      data->actualline,
      data->CPos_X,
      data->actualline,
      data->CPos_X+data->TabSize
    };

    CheckWord(data);
    AddToUndoBuffer(ET_PASTEBLOCK, (char *)&block, data);
    data->CPos_X += data->TabSize;
    PasteChars(data->CPos_X-data->TabSize, data->actualline, data->TabSize, "            ", NULL, data);
  }

  LEAVE();
}

void Key_Return(struct InstData *data)
{
  ENTER();

  if(Enabled(data))
    Key_Clear(data);
  else
    ScrollIntoDisplay(data);

  CheckWord(data);
  AddToUndoBuffer(ET_SPLITLINE, NULL, data);
  SplitLine(data->CPos_X, data->actualline, TRUE, NULL, data);

  // make sure the cursor is visible
  ScrollIntoDisplay(data);

  LEAVE();
}

void Key_Backspace(struct InstData *data)
{
  ENTER();

  if(Enabled(data))
    Key_Clear(data);
  else
  {
    if(data->CPos_X > 0)
    {
      AddToUndoBuffer(ET_BACKSPACECHAR, data->actualline->line.Contents+--data->CPos_X, data);
      RemoveChars(data->CPos_X, data->actualline, 1, data);
    }
    else
    {
      if(data->actualline->previous)
      {
        data->actualline = data->actualline->previous;
        data->CPos_X = data->actualline->line.Length-1;
        AddToUndoBuffer(ET_BACKSPACEMERGE, NULL, data);
        ScrollIntoDisplay(data);
        MergeLines(data->actualline, data);
      }
    }

    ScrollIntoDisplay(data);
  }

  LEAVE();
}

void Key_Delete (struct InstData *data)
{
  ENTER();

  if(Enabled(data))
    Key_Clear(data);
  else
  {
    ScrollIntoDisplay(data);

    if(data->actualline->line.Length > (ULONG)(data->CPos_X+1))
    {
      AddToUndoBuffer(ET_DELETECHAR, data->actualline->line.Contents+data->CPos_X, data);
      RemoveChars(data->CPos_X, data->actualline, 1, data);
    }
    else
    {
      if(data->actualline->next)
      {
        AddToUndoBuffer(ET_MERGELINES, NULL, data);
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
        AddToUndoBuffer(ET_PASTEBLOCK, (char *)&block, data);
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
    Key_Clear(data);
  else
    ScrollIntoDisplay(data);

  // check if the user wants to do a direct spell checking while
  // writing some text.
  if(data->TypeAndSpell && (!IsAlpha(data->mylocale, key)) && key != '-')
    CheckWord(data);

  // add the pastechar to the undobuffer and paste the current
  // key immediatly.
  AddToUndoBuffer(ET_PASTECHAR, NULL, data);
  PasteChars(data->CPos_X++, data->actualline, 1, (char *)&key, NULL, data);

  // check if the user selected the texteditor to do an automatic hard word
  // wrapping during writing text and if so we go and perform the hard word
  // wrapping at the correct border.
  if(data->WrapMode == MUIV_TextEditor_WrapMode_HardWrap &&
     data->WrapBorder > 0 && (data->CPos_X > data->WrapBorder) && (key != ' '))
  {
    ULONG xpos = data->WrapBorder+1;
    D(DBF_INPUT, "must wrap");


    // now we make sure to wrap *exactly* at the WrapBorder the user
    // specified instead of wrapping right where we are.
    while(xpos > 0 && *(data->actualline->line.Contents+xpos) != ' ')
      xpos--;

    // if we reached the end we go and find a wrap position after
    // the wrap border
    if(xpos == 0)
    {
      xpos = data->WrapBorder;
      while(xpos < data->CPos_X && *(data->actualline->line.Contents+xpos) != ' ')
        xpos++;
    }

    D(DBF_INPUT, "xpos=%ld cposx=%ld", xpos, data->CPos_X);
    // now we do the line split operation at the xpos we found
    if(xpos != 0 && xpos < data->CPos_X)
    {
      ULONG length = data->CPos_X-xpos;

      data->CPos_X = xpos;
      AddToUndoBuffer(ET_SPLITLINE, NULL, data);
      SplitLine(data->CPos_X, data->actualline, TRUE, NULL, data);
      AddToUndoBuffer(ET_DELETECHAR, data->actualline->line.Contents+data->CPos_X, data);
      data->CPos_X = length-1;
      RemoveChars(0, data->actualline, 1, data);
    }
  }

  LEAVE();
}

/*----------------*
 * Convert Rawkey *
 *----------------*/
static BOOL ConvertKey(struct IntuiMessage *imsg, struct InstData *data)
{
  BOOL result = FALSE;
  UBYTE code = 0;
  struct InputEvent event;

  ENTER();

  event.ie_NextEvent    = NULL;
  event.ie_Class        = IECLASS_RAWKEY;
  event.ie_SubClass     = 0;
  event.ie_Code         = imsg->Code;
  event.ie_Qualifier    = imsg->Qualifier;
  event.ie_EventAddress = (APTR *) *((ULONG *)imsg->IAddress);

  if(MapRawKey(&event, (STRPTR)&code, 1, NULL) > 0)
  {
    SHOWVALUE(DBF_INPUT, code);

#ifdef FILTER_NONPRINTABLE
    if((code >= 32 && code <= 126) || code >= 160)
#else
    if(code >= 32)
#endif
    {
      data->pixel_x = 0;

      // now we perform the key action
      Key_Normal(code, data);

      result = TRUE;
    }
  }

  RETURN(result);
  return(result);
}

static BOOL MatchQual(ULONG input, ULONG match, UWORD action, struct InstData *data)
{
  BOOL result = FALSE;
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

  result = (input == match);

  if(result == FALSE && ((input & ~data->blockqual) == match))
  {
    if(action == MUIV_TextEditor_KeyAction_Up           ||
       action == MUIV_TextEditor_KeyAction_Down         ||
       action == MUIV_TextEditor_KeyAction_Left         ||
       action == MUIV_TextEditor_KeyAction_Right        ||
       action == MUIV_TextEditor_KeyAction_PageUp       ||
       action == MUIV_TextEditor_KeyAction_PageDown     ||
       action == MUIV_TextEditor_KeyAction_StartOfLine  ||
       action == MUIV_TextEditor_KeyAction_EndOfLine    ||
       action == MUIV_TextEditor_KeyAction_Top          ||
       action == MUIV_TextEditor_KeyAction_Bottom       ||
       action == MUIV_TextEditor_KeyAction_PrevWord     ||
       action == MUIV_TextEditor_KeyAction_NextWord     ||
       action == MUIV_TextEditor_KeyAction_PrevLine     ||
       action == MUIV_TextEditor_KeyAction_NextLine     ||
       action == MUIV_TextEditor_KeyAction_PrevSentence ||
       action == MUIV_TextEditor_KeyAction_NextSentence)
    {
      result = TRUE;
    }
  }

  RETURN(result);
  return result;
}


/*---------------------------------*
 * Function to handle a cursormove *
 *---------------------------------*/
static LONG FindKey(UBYTE key, ULONG qualifier, struct InstData *data)
{
  struct te_key *t_keys = data->RawkeyBindings;
  int i;
  UBYTE pressKey = key & ~IECODE_UP_PREFIX;
  UBYTE releaseKey = key | IECODE_UP_PREFIX;

  ENTER();

  if(t_keys == NULL)
    t_keys = (struct te_key *)default_keybindings;

  qualifier &= ~(IEQUALIFIER_RELATIVEMOUSE | IEQUALIFIER_REPEAT | IEQUALIFIER_CAPSLOCK);
  for(i=0; (WORD)t_keys[i].code != -1; i++)
  {
    struct te_key *curKey = &t_keys[i];

    if(curKey->code == pressKey && MatchQual(qualifier, curKey->qual, curKey->act, data))
    {
      if(key == releaseKey)
      {
        // We have been called for a known shortcut, but this was a release action.
        // This will also be "eaten" to avoid double input
        RETURN(5);
        return 5;
      }

      if(data->flags & FLG_ReadOnly)
      {
        LONG new_y = data->visual_y-1;

        switch(curKey->act)
        {
          case MUIV_TextEditor_KeyAction_Top:
            new_y = 0;
            break;

          case MUIV_TextEditor_KeyAction_PageUp:
            new_y -= data->maxlines;
            break;

          case MUIV_TextEditor_KeyAction_Up:
            new_y -= 1;
            break;

          case MUIV_TextEditor_KeyAction_Bottom:
            new_y = data->totallines-data->maxlines;
            break;

          case MUIV_TextEditor_KeyAction_PageDown:
            new_y += data->maxlines;
            break;

          case MUIV_TextEditor_KeyAction_Down:
            new_y += 1;
            break;

          case MUIV_TextEditor_KeyAction_Copy:
            Key_Copy(data);
            break;

          case MUIV_TextEditor_KeyAction_NextGadget:
            set(_win(data->object), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_Next);
            break;

          case MUIV_TextEditor_KeyAction_SelectAll:
          {
            struct line_node *actual = data->firstline;

            data->blockinfo.startline = actual;
            data->blockinfo.startx = 0;

            while(actual->next)
              actual = actual->next;

            data->blockinfo.stopline = actual;
            data->blockinfo.stopx = data->blockinfo.stopline->line.Length-1;
            data->blockinfo.enabled = TRUE;
            MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
          }
          break;

          case MUIV_TextEditor_KeyAction_SelectNone:
          {
            data->blockinfo.enabled = FALSE;
            MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
          }
          break;

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
        D(DBF_INPUT, "curKey->act: %ld", curKey->act);

        switch(curKey->act)
        {
          case MUIV_TextEditor_KeyAction_Top:
            GoTop(data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_PrevLine:
            GoPreviousLine(data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_PageUp:
            GoPreviousPage(data);
            RETURN(FALSE);
            return(FALSE);

          case MUIV_TextEditor_KeyAction_Up:
            GoUp(data);
            RETURN(FALSE);
            return(FALSE);

          case MUIV_TextEditor_KeyAction_Bottom:
            GoBottom(data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_NextLine:
            GoNextLine(data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_PageDown:
            GoNextPage(data);
            RETURN(FALSE);
            return(FALSE);

          case MUIV_TextEditor_KeyAction_Down:
            GoDown(data);
            RETURN(FALSE);
            return(FALSE);

          case MUIV_TextEditor_KeyAction_NextWord:
            GoNextWord(data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_NextSentence:
            GoNextSentence(data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_EndOfLine:
            GoEndOfLine(data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_Right:
            GoRight(data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_PrevWord:
            GoPreviousWord(data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_PrevSentence:
            GoPreviousSentence(data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_StartOfLine:
            GoStartOfLine(data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_Left:
            GoLeft(data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_SuggestWord:
            SuggestWord(data);
            break;

          case MUIV_TextEditor_KeyAction_Backspace:
            Key_Backspace(data);
            break;

          case MUIV_TextEditor_KeyAction_Delete:
            Key_Delete(data);
            break;

          case MUIV_TextEditor_KeyAction_Return:
            Key_Return(data);
            break;

          case MUIV_TextEditor_KeyAction_Tab:
            Key_Tab(data);
            break;

          case MUIV_TextEditor_KeyAction_Undo:
            Undo(data);
            break;

          case MUIV_TextEditor_KeyAction_Redo:
            Redo(data);
            break;

          case MUIV_TextEditor_KeyAction_Cut:
            Key_Cut(data);
            break;

          case MUIV_TextEditor_KeyAction_Copy:
            Key_Copy(data);
            break;

          case MUIV_TextEditor_KeyAction_Paste:
            Key_Paste(data);
            break;

          case MUIV_TextEditor_KeyAction_DelEOL:
            Key_DelSomething(Del_EOL, data);
            break;

          case MUIV_TextEditor_KeyAction_DelBOL:
            Key_DelSomething(Del_BOL, data);
            break;

          case MUIV_TextEditor_KeyAction_DelEOW:
            Key_DelSomething(Del_EOW, data);
            break;

          case MUIV_TextEditor_KeyAction_DelBOW:
            Key_DelSomething(Del_BOW, data);
            break;

          case MUIV_TextEditor_KeyAction_DelLine:
            Key_DelLine(data);
            break;

          case MUIV_TextEditor_KeyAction_NextGadget:
            set(_win(data->object), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_Next);
            break;

          case MUIV_TextEditor_KeyAction_GotoBookmark1:
            GotoBookmark(0, data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_GotoBookmark2:
            GotoBookmark(1, data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_GotoBookmark3:
            GotoBookmark(2, data);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_SetBookmark1:
            SetBookmark(0, data);
            break;

          case MUIV_TextEditor_KeyAction_SetBookmark2:
            SetBookmark(1, data);
            break;

          case MUIV_TextEditor_KeyAction_SetBookmark3:
            SetBookmark(2, data);
            break;

          case MUIV_TextEditor_KeyAction_SelectAll:
          {
            struct line_node *actual = data->firstline;

            data->blockinfo.startline = actual;
            data->blockinfo.startx = 0;

            while(actual->next)
              actual = actual->next;

            data->blockinfo.stopline = actual;
            data->blockinfo.stopx = data->blockinfo.stopline->line.Length-1;
            data->blockinfo.enabled = TRUE;
            MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
          }
          break;

          case MUIV_TextEditor_KeyAction_SelectNone:
          {
            data->blockinfo.enabled = FALSE;
            MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
          }
          break;
        }

        RETURN(3);
        return(3);
      }
      break;
    }
  }

  RETURN(2);
  return(2);
}

static BOOL ReactOnRawKey(struct IntuiMessage *imsg, struct InstData *data)
{
  struct line_node *oldactualline = data->actualline;
  UWORD oldCPos_X = data->CPos_X;
  BOOL result = TRUE;
  LONG dummy;

  ENTER();

  dummy = FindKey(imsg->Code, imsg->Qualifier, data);

  D(DBF_INPUT, "FindKey: %ld", dummy);
  if(dummy == 1 || dummy == 0)
  {
    if(dummy == 1)
      data->pixel_x = 0;

    if((data->CPos_X != oldCPos_X || oldactualline != data->actualline) || (!(imsg->Qualifier & data->blockqual) && data->blockinfo.enabled))
    {
      SetCursor(oldCPos_X, oldactualline, FALSE, data);

      if(!(data->flags & FLG_ReadOnly))
      {
        if(imsg->Qualifier & data->blockqual)
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
      else
        ScrollIntoDisplay(data);
    }
  }
  else if(dummy == 2)
  {
    // we execute the ConvertKey() action which in fact will
    // perform the actual key press reaction
    if((data->flags & FLG_ReadOnly) == 0 && (imsg->Qualifier & IEQUALIFIER_RCOMMAND) == 0)
      ConvertKey(imsg, data);
    else
      result = FALSE;
  }
  else if(dummy == 3)
    data->pixel_x = 0;

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
      D(DBF_INPUT,"scrollup: %ld", diff-data->maxlines);
    }

    if(diff < 1)
    {
      data->visual_y += diff-1;
      ScrollDown(0, (-diff)+1, data);
      D(DBF_INPUT,"scrolldown: %ld", -diff+1);
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
