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

#include <string.h>

#include <exec/memory.h>
#include <intuition/classes.h>
#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/exec.h>

#ifdef __MORPHOS__
#include <proto/alib.h>
#endif

#include <proto/muimaster.h>
#include <libraries/mui.h>

#include "private.h"
#include "Debug.h"

DISPATCHERPROTO(_Dispatcher);

/// ResetDisplay()
void ResetDisplay(struct InstData *data)
{
  struct  line_node *line = data->firstline;
  LONG    lines = 0;

  ENTER();

  data->blockinfo.enabled = FALSE;
  data->visual_y = 1;
  data->CPos_X = 0;
  data->pixel_x = 0;
  data->actualline = line;

  data->cursor_shown = 0;
  if(data->shown == TRUE)
  {
    while(line != NULL)
    {
      LONG c;

      c = VisualHeight(data, line);
      lines += c;
      line->visual = c;
      line = line->next;
    }
    data->totallines = lines;

    data->Pen = GetColor(data->CPos_X, data->actualline);
    data->Flow = data->actualline->line.Flow;
    data->Separator = data->actualline->line.Separator;
    data->NoNotify = TRUE;
    SetAttrs(data->object,  MUIA_TextEditor_Pen,            data->Pen,
                    MUIA_TextEditor_Flow,         data->Flow,
                    MUIA_TextEditor_Separator,        data->Separator,
                    MUIA_TextEditor_Prop_Entries,     lines*data->height,
                    MUIA_TextEditor_Prop_Visible,     data->maxlines*data->height,
                    MUIA_TextEditor_Prop_First,     (data->visual_y-1)*data->height,
                    MUIA_TextEditor_Prop_DeltaFactor, data->height,
                    TAG_DONE);
    data->NoNotify = FALSE;


    UpdateStyles(data);

    DumpText(data, data->visual_y, 0, data->maxlines, FALSE);
  }

  LEAVE();
}

///
/// RequestInput()
void RequestInput(struct InstData *data)
{
  ENTER();

  if(data->scrollaction == FALSE && data->mousemove == FALSE)
    DoMethod(_app(data->object), MUIM_Application_AddInputHandler, &data->ihnode);

  LEAVE();
}

///
/// RejectInput()
void RejectInput(struct InstData *data)
{
  ENTER();

  if(data->scrollaction == FALSE && data->mousemove == FALSE)
    DoMethod(_app(data->object), MUIM_Application_RemInputHandler, &data->ihnode);

  LEAVE();
}

///
/// mNew()
static IPTR mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
  ENTER();

  if((obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg)) != NULL)
  {
    struct InstData *data = INST_DATA(cl, obj);
    data->object = obj;

    #if defined(__amigaos4__)
    data->mypool = AllocSysObjectTags(ASOT_MEMPOOL, ASOPOOL_MFlags, MEMF_SHARED|MEMF_CLEAR,
                                                    ASOPOOL_Puddle, 3*1024,
                                                    ASOPOOL_Threshold, 512,
                                                    ASOPOOL_Name, "TextEditor.mcc pool",
                                                    TAG_DONE);
    #else
    data->mypool = CreatePool(MEMF_ANY|MEMF_CLEAR, 3*1024, 512);
    #endif
    if(data->mypool != NULL)
    {
      if((data->mylocale = OpenLocale(NULL)) != NULL)
      {
        if((data->firstline = AllocLine(data)) != NULL)
        {
          if(Init_LineNode(data, data->firstline, NULL, "\n") == TRUE)
          {
            data->actualline = data->firstline;
            data->update = TRUE;
            data->ImportHook = &ImPlainHook;
            data->ImportWrap = 1023;
            data->WrapBorder = 0;
            data->WrapMode = MUIV_TextEditor_WrapMode_HardWrap;

            data->ExportHook = &ExportHookPlain;
            setFlag(data->flags, FLG_AutoClip);
            setFlag(data->flags, FLG_ActiveOnClick);

            if(FindTagItem(MUIA_Background, msg->ops_AttrList))
              setFlag(data->flags, FLG_OwnBackground);
            if(FindTagItem(MUIA_Frame, msg->ops_AttrList))
              setFlag(data->flags, FLG_OwnFrame);

            // initialize our temporary rastport
            InitRastPort(&data->tmprp);

            // walk through all attributes and check if
            // they were set during OM_NEW
            mSet(cl, obj, (struct opSet *)msg);
            data->visual_y = 1;

            // start with an inactive cursor
            data->currentCursorState = CS_INACTIVE;

            RETURN((IPTR)obj);
            return (IPTR)obj;
          }
        }
      }
    }
    CoerceMethod(cl, obj, OM_DISPOSE);
  }

  RETURN(FALSE);
  return FALSE;
}

///
/// mDispose()
static IPTR mDispose(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  struct line_node *line;

  ENTER();

  ResizeUndoBuffer(data, 0);

  data->blockinfo.startline = NULL;
  data->blockinfo.stopline = NULL;

  // free all lines with their contents
  line = data->firstline;
  while(line != NULL)
  {
    struct line_node *next = line->next;

    if(line->line.Styles != NULL)
      FreeVecPooled(data->mypool, line->line.Styles);
    if(line->line.Colors != NULL)
      FreeVecPooled(data->mypool, line->line.Colors);
    if(line->line.Contents != NULL)
      FreeVecPooled(data->mypool, line->line.Contents);
    FreeLine(data, line);

    line = next;
  }
  data->firstline = NULL;

  if(data->mylocale != NULL)
  {
    CloseLocale(data->mylocale);
    data->mylocale = NULL;
  }

  if(data->mypool != NULL)
  {
    #if defined(__amigaos4__)
    FreeSysObject(ASOT_MEMPOOL, data->mypool);
    #else
    DeletePool(data->mypool);
    #endif
    data->mypool = NULL;
  }

  LEAVE();
  return DoSuperMethodA(cl, obj, msg);
}

///
/// mSetup()
static IPTR mSetup(struct IClass *cl, Object *obj, struct MUI_RenderInfo *rinfo)
{
  IPTR result = FALSE;
  struct InstData *data = INST_DATA(cl, obj);

  ENTER();

  // initialize the configuration of our TextEditor
  // object from the configuration set by the user
  InitConfig(data, obj);

  if(DoSuperMethodA(cl, obj, (Msg)rinfo))
  {
    // disable that the object will automatically get a border when
    // the ActiveObjectOnClick option is active
    if(isFlagSet(data->flags, FLG_ActiveOnClick))
      _flags(obj) |= (1<<7);

    // now we check whether we have a valid font or not
    // and if not we take the default one of our muiAreaData
    if(data->font == NULL)
      data->font = muiAreaData(obj)->mad_Font;

    // initialize our temporary rastport
    InitRastPort(&data->tmprp);
    SetFont(&data->tmprp, data->font);

    // make sure we have a proper font setup here and
    // that our spellchecker suggest window object is also
    // correctly initialized.
    if(data->font != NULL && (data->SuggestWindow = SuggestWindow(data)) != NULL)
    {
      DoMethod(_app(obj), OM_ADDMEMBER, data->SuggestWindow);

      data->mousemove = FALSE;
      data->ehnode.ehn_Priority = 0;
      data->ehnode.ehn_Flags    = MUI_EHF_GUIMODE;
      data->ehnode.ehn_Object   = obj;
      data->ehnode.ehn_Class    = cl;
      data->ehnode.ehn_Events   = IDCMP_INACTIVEWINDOW | IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;

      #if defined(__amigaos4__)
      data->ehnode.ehn_Events  |= IDCMP_EXTENDEDMOUSE;
      #endif

      // setup the selection pointer
      if(data->selectPointer == TRUE)
      {
        setFlag(data->ehnode.ehn_Events, IDCMP_MOUSEMOVE);
        SetupSelectPointer(data);
      }

      data->ihnode.ihn_Object   = obj;
      data->ihnode.ihn_Millis   = 20;
      data->ihnode.ihn_Method   = MUIM_TextEditor_InputTrigger;
      data->ihnode.ihn_Flags    = MUIIHNF_TIMER;

      DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);

      data->smooth_wait = 0;
      data->scrollaction = FALSE;

      result = TRUE;
    }
  }

  RETURN(result);
  return result;
}

///
/// mCleanup
static IPTR mCleanup(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  IPTR result = 0;

  ENTER();

  // cleanup the selection pointer
  CleanupSelectPointer(data);

  DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);

  if(DoMethod(_app(obj), OM_REMMEMBER, data->SuggestWindow))
    MUI_DisposeObject(data->SuggestWindow);

  // enable that the object will automatically get a border when
  // the ActiveObjectOnClick option is active
  if(isFlagSet(data->flags, FLG_ActiveOnClick))
    _flags(obj) &= ~(1<<7);

  if(data->mousemove == TRUE)
  {
    data->mousemove = FALSE;
    RejectInput(data);
  }

  FreeConfig(data, muiRenderInfo(obj));

  result = DoSuperMethodA(cl, obj, msg);

#ifdef __AROS__
  DeinitRastPort(&data->tmprp);
#endif

  RETURN(result);
  return result;
}

///
/// mAskMinMax()
static IPTR mAskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  struct MUI_MinMax *mi;
  ULONG fontheight;

  ENTER();

  // call the supermethod first
  DoSuperMethodA(cl, obj, (Msg)msg);

  mi = ((struct MUIP_AskMinMax *)msg)->MinMaxInfo;

  if(data->Columns)
  {
    // for the font width we take the nominal font width provided by the tf_XSize attribute
    ULONG width = data->Columns * (data->font ? data->font->tf_XSize : muiAreaData(obj)->mad_Font->tf_XSize);

    mi->MinWidth += width;
    mi->DefWidth += width;
    mi->MaxWidth += width;
  }
  else
  {
    mi->MinWidth += 40;
    mi->DefWidth += 300;
    mi->MaxWidth = MUI_MAXMAX;
  }

  fontheight = data->font ? data->font->tf_YSize : muiAreaData(obj)->mad_Font->tf_YSize;
  if(data->Rows)
  {
    ULONG height = data->Rows * fontheight;

    mi->MinHeight += height;
    mi->DefHeight += height;
    mi->MaxHeight += height;
  }
  else
  {
    mi->MinHeight +=  1 * fontheight;
    mi->DefHeight += 15 * fontheight;
    mi->MaxHeight = MUI_MAXMAX;
  }

  RETURN(0);
  return 0;
}

///
/// mShow()
static IPTR mShow(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  struct line_node  *line;
  struct MUI_AreaData *ad = muiAreaData(obj);
  LONG  lines = 0;

  ENTER();

  DoSuperMethodA(cl, obj, msg);

  // now we check whether we have a valid font or not
  // and if not we take the default one of our muiAreaData
  if(!data->font)
    data->font = ad->mad_Font;

  data->rport       = muiRenderInfo(obj)->mri_RastPort;
  data->height      = data->font->tf_YSize;
  data->xpos        = ad->mad_Box.Left + ad->mad_addleft;
  data->innerwidth  = ad->mad_Box.Width - ad->mad_subwidth;
  data->maxlines    = (ad->mad_Box.Height - ad->mad_subheight) / data->height;
  data->ypos        = ad->mad_Box.Top + ad->mad_addtop;
  data->realypos    = data->ypos;

  line = data->firstline;
  while(line != NULL)
  {
    LONG c;

    c = VisualHeight(data, line);
    lines += c;
    line->visual = c;
    line = line->next;
  }
  data->totallines = lines;

  data->shown = TRUE;
  data->update = FALSE;
  ScrollIntoDisplay(data);
  data->update = TRUE;
  data->shown = FALSE;

  SetAttrs(obj, MUIA_TextEditor_Prop_DeltaFactor, data->height,
            MUIA_TextEditor_Prop_Entries,
              ((lines-(data->visual_y-1) < data->maxlines) ?
                ((data->visual_y-1)+data->maxlines) :
                ((data->maxlines > lines) ?
                  data->maxlines :
                  lines))
                * data->height,
            MUIA_TextEditor_Prop_First,     (data->visual_y-1)*data->height,
            MUIA_TextEditor_Prop_Visible,     data->maxlines*data->height,
            TAG_DONE);

  // initialize the doublebuffering rastport
  InitRastPort(&data->doublerp);
  data->doublebuffer = MUIG_AllocBitMap(data->innerwidth+((data->height-data->font->tf_Baseline+1)>>1)+1, data->height, GetBitMapAttr(data->rport->BitMap, BMA_DEPTH), (BMF_CLEAR | BMF_INTERLEAVED), data->rport->BitMap);
  data->doublerp.BitMap = data->doublebuffer;
  SetFont(&data->doublerp, data->font);

  // initialize the copyrp rastport
  data->copyrp = *muiRenderInfo(obj)->mri_RastPort;
  SetFont(&data->copyrp, data->font);

  // initialize our temporary rastport
  InitRastPort(&data->tmprp);
  SetFont(&data->tmprp, data->font);

  set(data->SuggestWindow, MUIA_Window_Open, isFlagSet(data->flags, FLG_PopWindow));

  data->shown = TRUE;

  RETURN(TRUE);
  return TRUE;
}

///
/// mHide()
static IPTR mHide(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = INST_DATA(cl, obj);

  ENTER();

  data->shown = FALSE;
  HideSelectPointer(data, obj);
  nnset(data->SuggestWindow, MUIA_Window_Open, FALSE);
  set(_win(obj), MUIA_Window_DisableKeys, 0L);
  MUIG_FreeBitMap(data->doublebuffer);
#ifdef __AROS__
  DeinitRastPort(&data->doublerp);
#endif
  data->doublerp.BitMap = NULL;
  data->rport = NULL;

  LEAVE();
  return DoSuperMethodA(cl, obj, msg);
}

///
/// mDraw()
static IPTR mDraw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
  struct InstData *data = INST_DATA(cl, obj);

  ENTER();

  DoSuperMethodA(cl, obj, (Msg)msg);

  if(isFlagSet(msg->flags, MADF_DRAWUPDATE) && data->UpdateInfo != NULL)
  {
    setFlag(data->flags, FLG_Draw);
    data->UpdateInfo = (APTR)_Dispatcher(cl, obj, (Msg)data->UpdateInfo);
    clearFlag(data->flags, FLG_Draw);
  }

  if(isFlagSet(msg->flags, MADF_DRAWOBJECT))
  {
    struct MUI_AreaData *ad = muiAreaData(obj);

    SetFont(data->rport, data->font);

/*    This cases crash on simplerefresh,
    when something  covers  window and
    contents  scroll  (gfxcard  only!)
*/ /* data->update = FALSE;
    ScrollIntoDisplay(data);
    data->update = TRUE;
*/

    // we clear the very last part of the gadget
    // content at the very bottom because that one will not be
    // automatically cleared by PrintLine() later on
    DoMethod(obj, MUIM_DrawBackground, data->xpos, data->ypos+(data->height * (data->maxlines)),
                                       data->innerwidth, (ad->mad_Box.Height-ad->mad_subheight-(data->height * (data->maxlines))),
                                       data->xpos, ad->mad_Box.Top+ad->mad_addtop, 0);

    // make sure we ghost out the whole area in case
    // the gadget was flagged as being ghosted.
    if(isFlagSet(data->flags, FLG_Ghosted))
    {
      UWORD *oldPattern = (UWORD *)data->rport->AreaPtrn;
      UBYTE oldSize = data->rport->AreaPtSz;
      UWORD newPattern[] = {0x1111, 0x4444};

      SetDrMd(data->rport, JAM1);
      SetAPen(data->rport, _pens(obj)[MPEN_SHADOW]);
      data->rport->AreaPtrn = newPattern;
      data->rport->AreaPtSz = 1;
      RectFill(data->rport, ad->mad_Box.Left, ad->mad_Box.Top, ad->mad_Box.Left + ad->mad_Box.Width  - 1, ad->mad_Box.Top  + ad->mad_Box.Height - 1);
      data->rport->AreaPtrn = oldPattern;
      data->rport->AreaPtSz = oldSize;
    }

    // dump all text now
    DumpText(data, data->visual_y, 0, data->maxlines, FALSE);
  }

  RETURN(0);
  return 0;
}

///
/// mGoActive
IPTR mGoActive(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  IPTR result;

  ENTER();

  // set the gadgets flags to active and also "activated" so that
  // other functions know that the gadget was activated recently.
  setFlag(data->flags, FLG_Active);
  setFlag(data->flags, FLG_Activated);

  if(data->shown == TRUE)
  {
    SetCursor(data, data->CPos_X, data->actualline, TRUE);

    // in case we ought to show a selected area in a different
    // color than in inactive state we call MarkText()
    if(isFlagSet(data->flags, FLG_ActiveOnClick) && Enabled(data))
      MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);

    if(isFlagClear(data->flags, FLG_ReadOnly))
      set(_win(obj), MUIA_Window_DisableKeys, MUIKEYF_GADGET_NEXT);
  }

  if(data->BlinkSpeed == 1)
  {
    DoMethod(_app(obj), MUIM_Application_AddInputHandler, &data->blinkhandler);
    data->BlinkSpeed = 2;
  }

  result = DoSuperMethodA(cl, obj, msg);

  RETURN(result);
  return result;
}

///
/// mGoInactive
IPTR mGoInactive(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  IPTR result;

  ENTER();

  // clear the active and activated flag so that others know about it
  clearFlag(data->flags, FLG_Active);
  clearFlag(data->flags, FLG_Activated);

  if(data->shown == TRUE)
    set(_win(obj), MUIA_Window_DisableKeys, 0L);

  if(data->mousemove == TRUE)
  {
    data->mousemove = FALSE;
    RejectInput(data);
  }

  if(data->scrollaction == TRUE)
    data->smooth_wait = 1;

  if(data->BlinkSpeed == 2)
  {
    DoMethod(_app(obj), MUIM_Application_RemInputHandler, &data->blinkhandler);
    data->BlinkSpeed = 1;
  }

  SetCursor(data, data->CPos_X, data->actualline, FALSE);

  // in case we ought to show a selected area in a different
  // color than in inactive state we call MarkText()
  if(isFlagSet(data->flags, FLG_ActiveOnClick) && Enabled(data))
    MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);

  result = DoSuperMethodA(cl, obj, msg);

  RETURN(result);
  return result;
}

///
/// mInsertText
IPTR mInsertText(struct IClass *cl, Object *obj, struct MUIP_TextEditor_InsertText *msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  struct marking block;
  IPTR result;

  ENTER();

  switch(msg->pos)
  {
    case MUIV_TextEditor_InsertText_Top:
    {
      GoTop(data);
    }
    break;

    case MUIV_TextEditor_InsertText_Bottom:
    {
      GoBottom(data);
    }
    break;
  }

  block.startx = data->CPos_X;
  block.startline = data->actualline;
  result = InsertText(data, msg->text, TRUE);
  block.stopx = data->CPos_X;
  block.stopline = data->actualline;
  AddToUndoBuffer(data, ET_PASTEBLOCK, &block);

  RETURN(result);
  return result;
}

///
/// mExport
IPTR mExport(UNUSED struct IClass *cl, Object *obj, struct MUIP_Export *msg)
{
  ULONG id;

  ENTER();

  if((id = (muiNotifyData(obj)->mnd_ObjectID)) != 0)
  {
    STRPTR contents;

    if((contents = (STRPTR)DoMethod(obj, MUIM_TextEditor_ExportText)) != NULL)
    {
      DoMethod(msg->dataspace, MUIM_Dataspace_Add, contents, strlen(contents)+1, id);
      FreeVec(contents);
    }
  }

  RETURN(0);
  return 0;
}

///
/// mImport
IPTR mImport(UNUSED struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
  ULONG id;

  ENTER();

  if((id = (muiNotifyData(obj)->mnd_ObjectID)) != 0)
  {
    STRPTR contents = (STRPTR)DoMethod(msg->dataspace, MUIM_Dataspace_Find, id);

    set(obj, MUIA_TextEditor_Contents, contents != NULL ? (ULONG)contents : (ULONG)"");
  }

  RETURN(0);
  return 0;
}

///
/// _Dispatcher()
DISPATCHER(_Dispatcher)
{
  struct InstData *data;
  LONG t_totallines;
  LONG t_visual_y;
  BOOL t_haschanged;
  UWORD t_pen;
  BOOL areamarked;
  IPTR result = 0;

  ENTER();

  //kprintf("Method: 0x%lx\n", msg->MethodID);
//  D(DBF_STARTUP, "Stack usage: %ld %lx", (ULONG)FindTask(NULL)->tc_SPUpper - (ULONG)FindTask(NULL)->tc_SPReg);

  // this one must be catched before we try to obtain the instance data, because nobody
  // will guarantee that the pointer returned by INST_DATA() is valid if no object has
  // been created yet!!
  if(msg->MethodID == OM_NEW)
  {
    result = mNew(cl, obj, (struct opSet *)msg);

    RETURN(result);
    return result;
  }

  // now get the instance data
  if((data = INST_DATA(cl, obj)) == NULL)
  {
    ASSERT(data != NULL);

    RETURN(0);
    return 0;
  }

  // set some variables:
  t_totallines = data->totallines;
  t_visual_y = data->visual_y;
  t_haschanged = data->HasChanged;
  t_pen = data->Pen;
  areamarked = Enabled(data);

//  D(DBF_STARTUP, "cont...");

  if(data->shown == TRUE && isFlagClear(data->flags, FLG_Draw))
  {
    switch(msg->MethodID)
    {
      case MUIM_TextEditor_ARexxCmd:
      case MUIM_TextEditor_InsertText:
      case MUIM_TextEditor_InputTrigger:
      case MUIM_TextEditor_ToggleCursor:
      case MUIM_TextEditor_MarkText:
      case MUIM_TextEditor_ClearText:
      case MUIM_TextEditor_SetBlock:
      case MUIM_HandleEvent:
      case MUIM_GoInactive:
      case MUIM_GoActive:
      {
        data->UpdateInfo = msg;
        MUI_Redraw(obj, MADF_DRAWUPDATE);
        result = (ULONG)data->UpdateInfo;
        data->UpdateInfo = NULL;

        RETURN(result);
        return result;
      }
    }
  }

  switch(msg->MethodID)
  {
    case OM_DISPOSE:                     result = mDispose(cl, obj, msg);                  RETURN(result); return result; break;
    case OM_GET:                         result = mGet(cl, obj, (APTR)msg);                RETURN(result); return result; break;
    case OM_SET:                         result = mSet(cl, obj, (APTR)msg);                                               break;
    case MUIM_Setup:                     result = mSetup(cl, obj, (APTR)msg);              RETURN(result); return result; break;
    case MUIM_Show:                      result = mShow(cl, obj, msg);                     RETURN(result); return result; break;
    case MUIM_AskMinMax:                 result = mAskMinMax(cl, obj, (APTR)msg);          RETURN(result); return result; break;
    case MUIM_Draw:                      result = mDraw(cl, obj, (APTR)msg);               RETURN(result); return result; break;
    case MUIM_Hide:                      result = mHide(cl, obj, msg);                     RETURN(result); return result; break;
    case MUIM_Cleanup:                   result = mCleanup(cl, obj, msg);                  RETURN(result); return result; break;
    case MUIM_Export:                    result = mExport(cl, obj, (APTR)msg);                                            break;
    case MUIM_Import:                    result = mImport(cl, obj, (APTR)msg);                                            break;
    case MUIM_GoActive:                  result = mGoActive(cl, obj, msg);                 RETURN(result); return result; break;
    case MUIM_GoInactive:                result = mGoInactive(cl, obj, msg);               RETURN(result); return result; break;
    case MUIM_HandleEvent:
    {
      ULONG oldx = data->CPos_X;
      struct line_node *oldy = data->actualline;

      // process all input events
      result = mHandleInput(cl, obj, (struct MUIP_HandleEvent *)msg);

      // see if the cursor was moved and if so we go and notify
      // others
      data->NoNotify = TRUE;

      if(data->CPos_X != oldx)
        set(obj, MUIA_TextEditor_CursorX, data->CPos_X);

      if(data->actualline != oldy)
        set(obj, MUIA_TextEditor_CursorY, LineNr(data, data->actualline)-1);

      data->NoNotify = FALSE;

      // if the HandleInput() function didn't return
      // an MUI_EventHandlerRC_Eat we can return immediately
      if(result == 0)
      {
        RETURN(0);
        return 0;
      }
    }
    break;

    case MUIM_TextEditor_ClearText:      result = mClearText(cl, obj, msg);                                               break;
    case MUIM_TextEditor_ToggleCursor:   result = mToggleCursor(cl, obj, msg);             RETURN(result); return result; break;
    case MUIM_TextEditor_InputTrigger:   result = mInputTrigger(cl, obj, msg);                                            break;
    case MUIM_TextEditor_InsertText:     result = mInsertText(cl, obj, (APTR)msg);                                        break;
    case MUIM_TextEditor_ExportBlock:    result = mExportBlock(cl, obj, (APTR)msg);        RETURN(result); return result; break;
    case MUIM_TextEditor_ExportText:     result = mExportText(cl, obj, (APTR)msg);         RETURN(result); return result; break;
    case MUIM_TextEditor_ARexxCmd:       result = mHandleARexx(cl, obj, (APTR)msg);                                       break;
    case MUIM_TextEditor_MarkText:       result = mMarkText(data, (APTR)msg);                                             break;
    case MUIM_TextEditor_BlockInfo:      result = mBlockInfo(data, (APTR)msg);                                            break;
    case MUIM_TextEditor_Search:         result = mSearch(cl, obj, (APTR)msg);                                            break;
    case MUIM_TextEditor_Replace:        result = mReplace(cl, obj, (APTR)msg);                                           break;
    case MUIM_TextEditor_QueryKeyAction: result = mQueryKeyAction(cl, obj, (APTR)msg);                                    break;
    case MUIM_TextEditor_SetBlock:       result = mSetBlock(data, (APTR)msg);              RETURN(result); return result; break;
    default:                             result = DoSuperMethodA(cl, obj, msg);            RETURN(result); return result; break;
  }

  if(t_haschanged != data->HasChanged)
    set(obj, MUIA_TextEditor_HasChanged, data->HasChanged);

  if(msg->MethodID == OM_SET)
  {
    ULONG newresult = DoSuperMethodA(cl, obj, msg);

    if(result)
      result = newresult;
    else
    {
      RETURN(newresult);
      return newresult;
    }
  }

  if(data->visual_y != t_visual_y || data->totallines != t_totallines)
  {
    SetAttrs(obj, MUIA_TextEditor_Prop_Entries,
              ((data->totallines-(data->visual_y-1) < data->maxlines) ?
                ((data->visual_y-1)+data->maxlines) :
                ((data->maxlines > data->totallines) ?
                  data->maxlines :
                  data->totallines))
                * data->height,
              MUIA_TextEditor_Prop_First, (data->visual_y-1)*data->height,
              TAG_DONE);
  }

  data->NoNotify = TRUE;
  if(Enabled(data))
  {
    struct marking newblock;

    NiceBlock(&data->blockinfo, &newblock);
    data->Pen = GetColor(data->blockinfo.stopx - ((data->blockinfo.stopx && newblock.startx == data->blockinfo.startx && newblock.startline == data->blockinfo.startline) ? 1 : 0), data->blockinfo.stopline);
  }
  else
    data->Pen = GetColor(data->CPos_X, data->actualline);

  if(t_pen != data->Pen)
    set(obj, MUIA_TextEditor_Pen, data->Pen);

  if(data->actualline->line.Flow != data->Flow)
  {
    data->Flow = data->actualline->line.Flow;
    set(obj, MUIA_TextEditor_Flow, data->actualline->line.Flow);
  }

  if(data->actualline->line.Separator != data->Separator)
  {
    data->Separator = data->actualline->line.Separator;
    set(obj, MUIA_TextEditor_Separator, data->actualline->line.Separator);
  }

  if(areamarked != Enabled(data))
    set(obj, MUIA_TextEditor_AreaMarked, Enabled(data));

  data->NoNotify = FALSE;
  UpdateStyles(data);

  RETURN(result);
  return result;
}

///
