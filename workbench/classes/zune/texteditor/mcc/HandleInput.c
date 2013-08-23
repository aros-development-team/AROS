/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2013 by TextEditor.mcc Open Source Team

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
#include "Debug.h"

#if !defined(__amigaos4__)
#include "newmouse.h"
#endif

/// RAWToANSI()
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
  event.ie_EventAddress = (APTR *) *((IPTR *)imsg->IAddress);

  MapRawKey(&event, (STRPTR)&code, 1, NULL);

  SHOWVALUE(DBF_INPUT, code);

  RETURN(code);
  return(code);
}

///
/// MatchQual()
static BOOL MatchQual(struct InstData *data, ULONG input, ULONG match, UWORD action)
{
  BOOL result = FALSE;

  ENTER();

  if(isFlagSet(match, IEQUALIFIER_SHIFT) && isAnyFlagSet(input, IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
  {
    clearFlag(match, IEQUALIFIER_SHIFT);
    clearFlag(input, IEQUALIFIER_LSHIFT);
    clearFlag(input, IEQUALIFIER_RSHIFT);
  }
  if(isFlagSet(match, IEQUALIFIER_ALT) && isAnyFlagSet(input, IEQUALIFIER_LALT | IEQUALIFIER_RALT))
  {
    clearFlag(match, IEQUALIFIER_ALT);
    clearFlag(input, IEQUALIFIER_LALT);
    clearFlag(input, IEQUALIFIER_RALT);
  }
  if(isFlagSet(match, IEQUALIFIER_COMMAND) && isAnyFlagSet(input, IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND))
  {
    clearFlag(match, IEQUALIFIER_COMMAND);
    clearFlag(input, IEQUALIFIER_LCOMMAND);
    clearFlag(input, IEQUALIFIER_RCOMMAND);
  }

  if(input == match)
    result = TRUE;

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

///
/// Key_DelSomething()
enum { Del_BOL = 0, Del_EOL, Del_BOW, Del_EOW };
static void Key_DelSomething(struct InstData *data, ULONG what)
{
  ENTER();

  if(data->blockinfo.enabled == TRUE)
  {
    data->blockinfo.enabled = FALSE;
    MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
  }

  data->blockinfo.startx = data->CPos_X;
  data->blockinfo.startline = data->actualline;

  switch(what)
  {
    case Del_BOL:
      GoStartOfLine(data);
      break;
    case Del_EOL:
      setFlag(data->flags, FLG_FreezeCrsr);
      GoEndOfLine(data);
      break;
    case Del_BOW:
      GoPreviousWord(data);
      break;
    case Del_EOW:
      setFlag(data->flags, FLG_FreezeCrsr);
      GoNextWord(data);
      break;
  }
  data->blockinfo.stopx = data->CPos_X;
  data->blockinfo.stopline = data->actualline;
  data->blockinfo.enabled = TRUE;
  if(Enabled(data))
    Key_Clear(data);
  else
    data->blockinfo.enabled = FALSE;

  clearFlag(data->flags, FLG_FreezeCrsr);

  LEAVE();
}

///
/// FindKey()
/*---------------------------------*
 * Function to handle a cursormove *
 *---------------------------------*/
static LONG FindKey(struct InstData *data, UBYTE key, ULONG qualifier)
{
  struct te_key *t_keys = data->RawkeyBindings;
  int i;
  UBYTE pressKey = key & ~IECODE_UP_PREFIX;
  UBYTE releaseKey = key | IECODE_UP_PREFIX;

  ENTER();

  if(t_keys == NULL)
    t_keys = (struct te_key *)default_keybindings;

  clearFlag(qualifier, IEQUALIFIER_RELATIVEMOUSE);
  clearFlag(qualifier, IEQUALIFIER_REPEAT);
  clearFlag(qualifier, IEQUALIFIER_CAPSLOCK);

  for(i=0; (WORD)t_keys[i].code != -1; i++)
  {
    struct te_key *curKey = &t_keys[i];

    if(curKey->code == pressKey && MatchQual(data, qualifier, curKey->qual, curKey->act) == TRUE)
    {
      if(key == releaseKey)
      {
        // We have been called for a known shortcut, but this was a release action.
        // This will also be "eaten" to avoid double input
        RETURN(5);
        return 5;
      }

      if(isFlagSet(data->flags, FLG_ReadOnly))
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
            MarkAllBlock(data, &data->blockinfo);
            MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
          }
          break;

          case MUIV_TextEditor_KeyAction_SelectNone:
          {
            data->blockinfo.enabled = FALSE;
            MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
          }
          break;

        }

        if(new_y != data->visual_y-1)
        {
          if(new_y > data->totallines-data->maxlines)
            new_y = data->totallines-data->maxlines;
          if(new_y < 0)
            new_y = 0;
          set(data->object, MUIA_TextEditor_Prop_First, new_y*data->fontheight);

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
            Key_DelSomething(data, Del_EOL);
            break;

          case MUIV_TextEditor_KeyAction_DelBOL:
            Key_DelSomething(data, Del_BOL);
            break;

          case MUIV_TextEditor_KeyAction_DelEOW:
            Key_DelSomething(data, Del_EOW);
            break;

          case MUIV_TextEditor_KeyAction_DelBOW:
            Key_DelSomething(data, Del_BOW);
            break;

          case MUIV_TextEditor_KeyAction_DelLine:
            Key_DelLine(data);
            break;

          case MUIV_TextEditor_KeyAction_NextGadget:
            set(_win(data->object), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_Next);
            break;

          case MUIV_TextEditor_KeyAction_GotoBookmark1:
            GotoBookmark(data, 0);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_GotoBookmark2:
            GotoBookmark(data, 1);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_GotoBookmark3:
            GotoBookmark(data, 2);
            RETURN(TRUE);
            return(TRUE);

          case MUIV_TextEditor_KeyAction_SetBookmark1:
            SetBookmark(data, 0);
            break;

          case MUIV_TextEditor_KeyAction_SetBookmark2:
            SetBookmark(data, 1);
            break;

          case MUIV_TextEditor_KeyAction_SetBookmark3:
            SetBookmark(data, 2);
            break;

          case MUIV_TextEditor_KeyAction_SelectAll:
          {
            MarkAllBlock(data, &data->blockinfo);
            MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
          }
          break;

          case MUIV_TextEditor_KeyAction_SelectNone:
          {
            data->blockinfo.enabled = FALSE;
            MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
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

///
/// Key_Normal()
void Key_Normal(struct InstData *data, char key)
{
  ENTER();

  if(Enabled(data))
    Key_Clear(data);
  else
    ScrollIntoDisplay(data);

  // check if the user wants to do a direct spell checking while
  // writing some text.
  if(data->TypeAndSpell == TRUE && !IsAlpha(data->mylocale, key) && key != '-')
    CheckWord(data);

  // add the pastechar to the undobuffer and paste the current
  // key immediately.
  AddToUndoBuffer(data, ET_PASTECHAR, NULL);
  PasteChars(data, data->CPos_X++, data->actualline, 1, &key, NULL);

  // check if the user selected the texteditor to do an automatic hard word
  // wrapping during writing text and if so we go and perform the hard word
  // wrapping at the correct border.
  if(data->WrapMode == MUIV_TextEditor_WrapMode_HardWrap &&
     data->WrapBorder > 0 &&
     data->CPos_X > data->WrapBorder &&
     key != ' ')
  {
    LONG xpos = data->WrapBorder+1;
    D(DBF_INPUT, "must wrap");

    // now we make sure to wrap *exactly* at the WrapBorder the user
    // specified instead of wrapping right where we are.
    while(xpos > 0 && data->actualline->line.Contents[xpos] != ' ')
      xpos--;

    // if we reached the end we go and find a wrap position after
    // the wrap border
    if(xpos == 0)
    {
      xpos = data->WrapBorder;
      while(xpos < data->CPos_X && data->actualline->line.Contents[xpos] != ' ')
        xpos++;
    }

    D(DBF_INPUT, "xpos=%ld cposx=%ld", xpos, data->CPos_X);
    // now we do the line split operation at the xpos we found
    if(xpos != 0 && xpos < data->CPos_X)
    {
      LONG length = data->CPos_X-xpos;

      data->CPos_X = xpos;
      AddToUndoBuffer(data, ET_SPLITLINE, NULL);
      SplitLine(data, data->CPos_X, data->actualline, TRUE, NULL);
      AddToUndoBuffer(data, ET_DELETECHAR, &data->actualline->line.Contents[data->CPos_X]);
      data->CPos_X = length-1;
      RemoveChars(data, 0, data->actualline, 1);
    }
  }

  LEAVE();
}

///
/// ConvertKey()
/*----------------*
 * Convert Rawkey *
 *----------------*/
static BOOL ConvertKey(struct InstData *data, struct IntuiMessage *imsg)
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
  event.ie_EventAddress = (APTR *) *((IPTR *)imsg->IAddress);

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
      Key_Normal(data, code);

      result = TRUE;
    }
  }

  RETURN(result);
  return(result);
}

///
/// ReactOnRawKey()
static BOOL ReactOnRawKey(struct InstData *data, struct IntuiMessage *imsg)
{
  struct line_node *oldactualline = data->actualline;
  LONG oldCPos_X = data->CPos_X;
  BOOL result = TRUE;
  LONG dummy;

  ENTER();

  dummy = FindKey(data, imsg->Code, imsg->Qualifier);

  D(DBF_INPUT, "FindKey: %ld (%lx)", dummy, imsg->Code);

  if(dummy == 1 || dummy == 0)
  {
    if(dummy == 1)
      data->pixel_x = 0;

    if((data->CPos_X != oldCPos_X || oldactualline != data->actualline) || (!(imsg->Qualifier & data->blockqual) && data->blockinfo.enabled == TRUE))
    {
      SetCursor(data, oldCPos_X, oldactualline, FALSE);

      if(isFlagClear(data->flags, FLG_ReadOnly))
      {
        if(imsg->Qualifier & data->blockqual)
        {
          data->blockinfo.stopline = data->actualline;
          data->blockinfo.stopx = data->CPos_X;
          if(data->blockinfo.enabled == FALSE)
          {
            data->blockinfo.enabled = TRUE;
            data->blockinfo.startline = oldactualline;
            data->blockinfo.startx = oldCPos_X;
          }

          MarkText(data, oldCPos_X, oldactualline, data->CPos_X, data->actualline);
        }
        else
        {
          clearFlag(data->flags, FLG_ARexxMark);
          if(data->blockinfo.enabled == TRUE)
          {
            data->blockinfo.enabled = FALSE;
            MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
          }
        }

        ScrollIntoDisplay(data);
        SetCursor(data, data->CPos_X, data->actualline, TRUE);
      }
      else
        ScrollIntoDisplay(data);
    }
  }
  else if(dummy == 2)
  {
    // we execute the ConvertKey() action which in fact will
    // perform the actual key press reaction
    if(isFlagClear(data->flags, FLG_ReadOnly) && isFlagClear(imsg->Qualifier, IEQUALIFIER_RCOMMAND))
      ConvertKey(data, imsg);
    else
      result = FALSE;
  }
  else if(dummy == 3)
    data->pixel_x = 0;

  RETURN(result);
  return(result);
}

///
/// mHandleInput()
IPTR mHandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  IPTR result = 0;
  BOOL wasActivated;

  ENTER();

  // check if the gadget was activate recently (via TAB key?)
  wasActivated = isFlagSet(data->flags, FLG_Activated);

  // clear the activated flag immediately
  clearFlag(data->flags, FLG_Activated);

  D(DBF_INPUT, "imsg->Code: %lx msg->muikey: %d", msg->imsg != NULL ? msg->imsg->Code : 0, msg->muikey);

  if(msg->muikey != MUIKEY_NONE)
  {
    switch(msg->muikey)
    {
	  case MUIKEY_UP:
	  {
	    if(data->KeyUpFocus != NULL && _win(obj) != NULL && data->actualline == GetFirstLine(&data->linelist))
	    {
		  set(_win(obj), MUIA_Window_ActiveObject, data->KeyUpFocus);
		  result = MUI_EventHandlerRC_Eat;
		}
	  }
	  break;

	  case MUIKEY_COPY:
	  {
	    Key_Copy(data);
	    result = MUI_EventHandlerRC_Eat;
	  }
	  break;

	  case MUIKEY_CUT:
	  {
	    if(isFlagSet(data->flags, FLG_ReadOnly))
	    {
	      Key_Cut(data);
	      result = MUI_EventHandlerRC_Eat;
	    }
	  }
	  break;

	  case MUIKEY_PASTE:
	  {
	    if(isFlagSet(data->flags, FLG_ReadOnly))
	    {
		  Key_Paste(data);
	      result = MUI_EventHandlerRC_Eat;
	    }
	  }
	  break;

	  case MUIKEY_UNDO:
	  {
	    if(isFlagSet(data->flags, FLG_ReadOnly))
	    {
		  Undo(data);
	      result = MUI_EventHandlerRC_Eat;
	    }
	  }
	  break;

	  case MUIKEY_REDO:
	  {
	    if(isFlagSet(data->flags, FLG_ReadOnly))
	    {
		  Redo(data);
	      result = MUI_EventHandlerRC_Eat;
	    }
	  }
	  break;
	}
  }

  if(result != 0)
  {
    RETURN(MUI_EventHandlerRC_Eat);
    return MUI_EventHandlerRC_Eat;
  }
  else if(isFlagClear(data->flags, FLG_Ghosted) &&
          data->shown == TRUE &&
          msg->imsg != NULL)
  {
    Object *activeobj;
    Object *defaultobj;
    struct IntuiMessage *imsg = msg->imsg;

    // here we check if the GADGET_NEXT muikey has been used (usually associated with the
    // TAB key) but if it is not the TAB key we activate the next object in the cyclechain because
    // we need to catch the TAB key to use it for storing either \t or a specific number of
    // spaces. So TE.mcc has to be TAB key aware. (note: MUIA_Window_DisableKeys is used in
    // Dispatcher.c to make this happen)
    if(msg->muikey == MUIKEY_GADGET_NEXT && imsg->Code != 0x42)
    {
      set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_Next);

      RETURN(MUI_EventHandlerRC_Eat);
      return MUI_EventHandlerRC_Eat;
    }

    // next we check if TE.mcc is the currently active object in the window and if not and if
    // it is also not the default object assigned to the window it is embedded we check if the
    // CtrlChar has been pressed which would signal that we need to activate the object accordingly
    // if the developer used MUIA_ControlChar correctly.
    activeobj = (Object *)xget(_win(obj), MUIA_Window_ActiveObject);
    defaultobj = (Object *)xget(_win(obj), MUIA_Window_DefaultObject);

    D(DBF_INPUT, "actobj: %08lx, defobj: %08lx, obj: %08lx", activeobj, defaultobj, obj);
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
        ShowSelectPointer(data, obj);
      else
        HideSelectPointer(data, obj);

      D(DBF_INPUT, "IDCMP_MOUSEMOVE");
    }
    else
    #if defined(__amigaos4__)
    if(imsg->Class == IDCMP_MOUSEBUTTONS ||
       activeobj == obj ||
       (isFlagSet(data->flags, FLG_ReadOnly) && defaultobj == obj && activeobj == NULL) ||
       (imsg->Class == IDCMP_EXTENDEDMOUSE && isFlagSet(imsg->Code, IMSGCODE_INTUIWHEELDATA)))
    #else
    if(imsg->Class == IDCMP_MOUSEBUTTONS ||
       activeobj == obj ||
       (isFlagSet(data->flags, FLG_ReadOnly) && defaultobj == obj && activeobj == NULL) ||
       (imsg->Class == IDCMP_RAWKEY && (imsg->Code == NM_WHEEL_UP || imsg->Code == NM_WHEEL_DOWN || imsg->Code == NM_WHEEL_LEFT || imsg->Code == NM_WHEEL_RIGHT)))
    #endif
    {
      if(isFlagClear(data->flags, FLG_Draw))
      {
        data->UpdateInfo = msg;
        MUI_Redraw(obj, MADF_DRAWUPDATE);

        RETURN((IPTR)data->UpdateInfo);
        return((IPTR)data->UpdateInfo);
      }

      switch(imsg->Class)
      {
        case IDCMP_RAWKEY:
        {
          D(DBF_INPUT, "HandleInput rawkey code=%02x qual=%04x", imsg->Code, imsg->Qualifier);

          if(data->ypos != _mtop(obj) ||
             (wasActivated && (msg->muikey == MUIKEY_GADGET_NEXT || imsg->Code == 0x42))) // ignore TAB key if the gadget was activated recently
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
          if(ReactOnRawKey(data, imsg) == FALSE)
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
          if(data->ypos != _mtop(obj))
          {
            RETURN(0);
            return(0);
          }

          if(imsg->Code == (IECODE_LBUTTON | IECODE_UP_PREFIX) && data->mousemove == TRUE)
          {
            data->mousemove = FALSE;
            RejectInput(data);

            if(isFlagSet(data->flags, FLG_ReadOnly) && isFlagSet(data->flags, FLG_AutoClip) && Enabled(data))
              Key_Copy(data);
          }
          else
          {
            // user has pressed the left mousebutton
            if(imsg->Code == IECODE_LBUTTON)
            {
              if(imsg->MouseX >= _left(obj) &&
                 imsg->MouseX <= _right(obj) &&
                 imsg->MouseY >= data->ypos &&
                 imsg->MouseY <  data->ypos+(data->maxlines * data->fontheight))
              {
                if(isFlagClear(data->flags, FLG_Active) &&
                   isFlagClear(data->flags, FLG_Activated) &&
                   isFlagSet(data->flags, FLG_ActiveOnClick) &&
                   Enabled(data) &&
                   isFlagSet(imsg->Qualifier, IEQUALIFIER_CONTROL))
                {
                  // in case the user hold the control key while pressing in an
                  // inactive object we go and just activate it and let the MUIM_GoActive
                  // function refresh the selected area.
                  set(_win(obj), MUIA_Window_ActiveObject, obj);
                }
                else
                {
                  LONG last_x = data->CPos_X;
                  struct line_node *lastline = data->actualline;

                  RequestInput(data);
                  data->mousemove = TRUE;

                  setFlag(data->flags, FLG_Activated);
                  SetCursor(data, data->CPos_X, data->actualline, FALSE);
                  PosFromCursor(data, imsg->MouseX, imsg->MouseY);

                  if(isFlagSet(imsg->Qualifier, IEQUALIFIER_LSHIFT) || isFlagSet(imsg->Qualifier, IEQUALIFIER_RSHIFT))
                    data->selectmode = 0;

                  if(!(data->blockinfo.enabled == TRUE && (isFlagSet(imsg->Qualifier, IEQUALIFIER_LSHIFT) || isFlagSet(imsg->Qualifier, IEQUALIFIER_RSHIFT))))
                  {
                    // if we already have an enabled block we have to disable it
                    // and clear the marking area with MarkText()
                    if(Enabled(data))
                    {
                      data->blockinfo.enabled = FALSE;
                      MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
                    }

                    data->blockinfo.enabled = TRUE;
                    data->blockinfo.startline = data->actualline;
                    data->blockinfo.startx = data->CPos_X;

                    if(last_x == data->CPos_X && lastline == data->actualline && DoubleClick(data->StartSecs, data->StartMicros, imsg->Seconds, imsg->Micros))
                    {
                      BOOL doubleClickHandled;

                      if(data->DoubleClickHook != NULL)
                      {
                        // we have a user defined hook, let this one decide
                        doubleClickHandled = CallHook(data->DoubleClickHook, obj, data->actualline->line.Contents, data->CPos_X, imsg->Qualifier);
                      }
                      else
                      {
                        // no hook function, treat this double click as unhandled yet
                        doubleClickHandled = FALSE;
                      }

                      if(doubleClickHandled == FALSE)
                      {
                        if(CheckSep(data, data->actualline->line.Contents[data->CPos_X]) == FALSE)
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
                            LONG x = data->CPos_X;

                            while(x > 0 && CheckSep(data, data->actualline->line.Contents[x-1]) == FALSE)
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
                      else
                      {
                        // If the double click was handled externally we immediately assume to
                        // have received the corresponding mouse release event as well.
                        // It might happen that the double click hook launches a program which
                        // steals our focus and leaves us pending in a "mouse pressed" situation.
                        // This will result in an immediate marking action as soon as we regain
                        // the focus. For example this happens when a URL is double clicked in
                        // YAM and if launching the browser takes some seconds.
                        if(data->mousemove == TRUE)
                        {
                          data->mousemove = FALSE;
                          RejectInput(data);

                          if(isFlagSet(data->flags, FLG_ReadOnly) && isFlagSet(data->flags, FLG_AutoClip) && Enabled(data))
                            Key_Copy(data);
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
                      MarkText(data, data->blockinfo.stopx, data->blockinfo.stopline, data->CPos_X, data->actualline);
                  }

                  data->blockinfo.stopline = data->actualline;
                  data->blockinfo.stopx = data->CPos_X;

                  SetCursor(data, data->CPos_X, data->actualline, TRUE);

                  if(isFlagSet(data->flags, FLG_ActiveOnClick))
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
    else
      D(DBF_INPUT, "ignore reached");
  }

  RETURN(0);
  return(0);
}

///
/// DoBlock()
static void DoBlock(struct InstData *data, ULONG flags)
{
  ENTER();

  data->blockinfo.enabled = FALSE;
  CutBlock(data, flags|CUTF_UPDATE);

  LEAVE();
}

///
/// EraseBlock()
static void EraseBlock(struct InstData *data, ULONG flags)
{
  ENTER();

  if(Enabled(data))
  {
    DoBlock(data, flags|CUTF_CUT);
    data->HasChanged = TRUE;
  }
  else
  {
    DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NoAreaMarked);
  }

  LEAVE();
}

///
/// Key_Clear()
void Key_Clear(struct InstData *data)
{
  ENTER();

  EraseBlock(data, 0);

  LEAVE();
}

///
/// Key_DelLine()
void Key_DelLine(struct InstData *data)
{
  ENTER();

  if(data->blockinfo.enabled == TRUE)
  {
    data->blockinfo.enabled = FALSE;
    MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
  }

  if(HasNextLine(data->actualline) == FALSE && data->actualline->line.Contents[0] == '\n')
    GoLeft(data);

  GoStartOfLine(data);

  data->blockinfo.startx = data->CPos_X;
  data->blockinfo.startline = data->actualline;

//  setFlag(data->flags, FLG_FreezeCrsr);
  GoEndOfLine(data);
  GoRight(data);
  data->blockinfo.stopx = data->CPos_X;
  data->blockinfo.stopline = data->actualline;
  data->blockinfo.enabled = TRUE;
  if(Enabled(data))
    Key_Clear(data);
  else
    data->blockinfo.enabled = FALSE;

//  clearFlag(data->flags, FLG_FreezeCrsr);

  LEAVE();
}

///
/// Key_Cut()
void Key_Cut(struct InstData *data)
{
  ENTER();

  ScrollIntoDisplay(data);
  EraseBlock(data, CUTF_CLIPBOARD);

  LEAVE();
}

///
/// Key_Copy()
void Key_Copy(struct InstData *data)
{
  ENTER();

  if(Enabled(data))
  {
    ScrollIntoDisplay(data);
    DoBlock(data, CUTF_CLIPBOARD);
  }
  else
    DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NoAreaMarked);

  LEAVE();
}

///
/// Key_Paste()
void Key_Paste(struct InstData *data)
{
  struct marking block;

  ENTER();

  if(Enabled(data))
    Key_Clear(data);
  else
    ScrollIntoDisplay(data);

  block.startline = data->actualline;
  block.startx = data->CPos_X;
//  SetCursor(data, data->CPos_X, data->actualline, FALSE);
  data->update = FALSE;
  if(PasteClip(data, data->CPos_X, data->actualline) == TRUE)
  {
    block.stopline = data->actualline;
    block.stopx = data->CPos_X;
    AddToUndoBuffer(data, ET_PASTEBLOCK, &block);
    data->pixel_x = 0;
  }
  else
    data->update = TRUE;

  LEAVE();
}

///
/// Key_Tab()
void Key_Tab(struct InstData *data)
{
  ENTER();

  if(Enabled(data))
    Key_Clear(data);
  else
    ScrollIntoDisplay(data);

  if(data->ConvertTabs == FALSE)
  {
	struct marking block =
	{
	  TRUE,
	  data->actualline,
	  data->CPos_X,
	  data->actualline,
	  data->CPos_X+1
	};

	CheckWord(data);
	AddToUndoBuffer(data, ET_PASTEBLOCK, &block);
	data->CPos_X++;
	PasteChars(data, data->CPos_X-1, data->actualline, 1, "\t", NULL);
  }
  else
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
	AddToUndoBuffer(data, ET_PASTEBLOCK, &block);
	data->CPos_X += data->TabSize;
	PasteChars(data, data->CPos_X-data->TabSize, data->actualline, data->TabSize, "            ", NULL);
  }

  LEAVE();
}

///
/// Key_Return()
void Key_Return(struct InstData *data)
{
  ENTER();

  if(Enabled(data))
    Key_Clear(data);
  else
    ScrollIntoDisplay(data);

  CheckWord(data);
  AddToUndoBuffer(data, ET_SPLITLINE, NULL);
  SplitLine(data, data->CPos_X, data->actualline, TRUE, NULL);

  // make sure the cursor is visible
  ScrollIntoDisplay(data);

  LEAVE();
}

///
/// Key_Backspace()
void Key_Backspace(struct InstData *data)
{
  ENTER();

  if(Enabled(data))
    Key_Clear(data);
  else
  {
    if(data->CPos_X > 0)
    {
      struct marking block;

      // move the cursor position one to the left
      data->CPos_X--;

      // define a block which consists of a single character only
      block.enabled = TRUE;
      block.startline = data->actualline;
      block.startx = data->CPos_X;
      block.stopline = data->actualline;
      block.stopx = data->CPos_X+1;

      // add this single character block to the Undo buffer
      AddToUndoBuffer(data, ET_DELETEBLOCK, &block);

      // erase the character
      RemoveChars(data, data->CPos_X, data->actualline, 1);
    }
    else if(HasPrevLine(data->actualline) == TRUE)
    {
      // merge two lines to a single line by appending
      // the current line to the previous
      data->actualline = GetPrevLine(data->actualline);
      data->CPos_X = data->actualline->line.Length-1;
      AddToUndoBuffer(data, ET_BACKSPACEMERGE, NULL);
      ScrollIntoDisplay(data);
      MergeLines(data, data->actualline);
    }

    ScrollIntoDisplay(data);
  }

  LEAVE();
}

///
/// Key_Delete()
void Key_Delete(struct InstData *data)
{
  ENTER();

  if(Enabled(data))
    Key_Clear(data);
  else
  {
    ScrollIntoDisplay(data);

    if(data->CPos_X+1 < data->actualline->line.Length)
    {
      struct marking block;

      // define a block which consists of a single character only
      block.enabled = TRUE;
      block.startline = data->actualline;
      block.startx = data->CPos_X;
      block.stopline = data->actualline;
      block.stopx = data->CPos_X+1;

      // add this single character block to the Undo buffer
      AddToUndoBuffer(data, ET_DELETEBLOCK_NOMOVE, &block);

      // erase the character
      RemoveChars(data, data->CPos_X, data->actualline, 1);
    }
    else if(HasNextLine(data->actualline) == TRUE)
    {
      AddToUndoBuffer(data, ET_MERGELINES, NULL);
      MergeLines(data, data->actualline);
    }
  }

  LEAVE();
}

///
/// ScrollIntoDisplay()
/*------------------------------------------------------*
 * Make sure that the cursor is inside the visible area *
 *------------------------------------------------------*/
void ScrollIntoDisplay(struct InstData *data)
{
  struct pos_info pos;
  LONG diff;

  ENTER();

  if(data->shown == TRUE)
  {
    OffsetToLines(data, data->CPos_X, data->actualline, &pos);
    diff = pos.lines + LineToVisual(data, data->actualline) - 1;
    if(diff > data->maxlines)
    {
      data->visual_y += diff-data->maxlines;
      D(DBF_INPUT,"scrollup: %ld", diff-data->maxlines);
      ScrollUpDown(data);
    }
    else if(diff < 1)
    {
      data->visual_y += diff-1;
      D(DBF_INPUT,"scrolldown: %ld", -diff+1);
      ScrollUpDown(data);
    }
  }

  LEAVE();
}

///
/// MarkText()
/*------------------------*
 * Update the marked area *
 *------------------------*/
void MarkText(struct InstData *data, LONG x1, struct line_node *line1, LONG x2, struct line_node *line2)
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

  line_nr1 = LineToVisual(data, startline) - 1;
  line_nr2 = LineToVisual(data, stopline) - 1;

  OffsetToLines(data, startx, startline, &pos1);
  OffsetToLines(data, stopx, stopline, &pos2);

  data->blockinfo.stopx = x2;
  data->blockinfo.stopline = line2;

  line_nr1 += pos1.lines-1;
  if(line_nr1 < 0)
    line_nr1 = 0;

  line_nr2 += pos2.lines-1;
  if(line_nr2 >= data->maxlines)
    line_nr2 = data->maxlines-1;

  if(line_nr1 <= line_nr2)
    DumpText(data, data->visual_y+line_nr1, line_nr1, line_nr2+1, FALSE);

  LEAVE();
}

///
