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

 $Id: GetSetAttrs.c,v 1.5 2005/04/07 10:10:53 sba Exp $

***************************************************************************/

#include <utility/tagitem.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/diskfont.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/utility.h>

#ifndef ClassAct
#include <proto/muimaster.h>
#include <libraries/mui.h>
#else
#include <images/bevel.h>
#include <intuition/gadgetclass.h>

struct SpecialPens
{
    WORD sp_Version;
    LONG sp_DarkPen;
    LONG sp_LightPen;
};

#endif

#include "TextEditor_mcc.h"
#include "private.h"

#include "rev.h"

ULONG Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  ULONG ti_Data;

  switch(msg->opg_AttrID)
  {
    case MUIA_TextEditor_CursorPosition:
    {
      UWORD xplace, yplace, cursor_width;

      UWORD x = data->CPos_X;

      struct line_node *line = data->actualline;
      LONG line_nr = LineToVisual(line, data) - 1;

      struct pos_info pos;
      OffsetToLines(x, line, &pos, data);

      cursor_width = (data->CursorWidth == 6) ? MyTextLength(data->font, (line->line.Contents[x] == '\n') ? (char *)"n" : (char *)&line->line.Contents[x], 1) : data->CursorWidth;

      xplace  = data->xpos + MyTextLength(data->font, &line->line.Contents[x-pos.x], pos.x);
      xplace += FlowSpace(line->line.Flow, line->line.Contents+pos.bytes, data);
      yplace  = data->ypos + (data->height * (line_nr + pos.lines - 1));

      data->CursorPosition.MinX = xplace;
      data->CursorPosition.MinY = yplace;
      data->CursorPosition.MaxX = xplace + cursor_width - 1;
      data->CursorPosition.MaxY = yplace + data->height - 1;
      ti_Data = (ULONG)&data->CursorPosition;
    }
    break;

    case MUIA_TextEditor_UndoAvailable:
      ti_Data = data->undosize && data->undobuffer != data->undopointer;
    break;

    case MUIA_TextEditor_RedoAvailable:
      ti_Data = data->undosize && *(short *)data->undopointer != 0xff;
    break;

    case MUIA_TextEditor_AutoClip:
      ti_Data = ((data->flags & FLG_AutoClip) ? TRUE : FALSE);
    break;

#ifndef ClassAct
    case MUIA_TextEditor_KeyUpFocus:
      ti_Data = (ULONG)data->KeyUpFocus;
    break;

    case MUIA_Version:
      ti_Data = LIB_VERSION;
    break;

    case MUIA_Revision:
      ti_Data = LIB_REVISION;
    break;

    case MUIA_ControlChar:
      ti_Data = (ULONG)data->CtrlChar;
    break;
#endif
    case MUIA_TextEditor_AreaMarked:
      ti_Data = Enabled(data);
      break;
    case MUIA_TextEditor_CursorX:
      ti_Data = data->CPos_X;
      break;
    case MUIA_TextEditor_CursorY:
      ti_Data = LineNr(data->actualline, data)-1;
      break;
    case MUIA_TextEditor_ExportWrap:
      ti_Data = data->ExportWrap;
      break;
    case MUIA_TextEditor_FixedFont:
      ti_Data = data->use_fixedfont;
      break;
    case MUIA_TextEditor_Pen:
      ti_Data = data->Pen;
      break;
    case MUIA_TextEditor_Flow:
      ti_Data = data->Flow;
      break;
    case MUIA_TextEditor_Separator:
      ti_Data = data->Separator;
      break;
    case MUIA_TextEditor_HasChanged:
      ti_Data = data->HasChanged;
      break;
    case MUIA_TextEditor_HorizontalScroll:
      ti_Data = ((data->flags & FLG_HScroll) ? TRUE : FALSE);
      break;
    case MUIA_TextEditor_ImportWrap:
      ti_Data = data->ImportWrap;
      break;
/*    case MUIA_TextEditor_InsertMode:
      ti_Data = data->InsertMode;
      break;
*/
    case MUIA_TextEditor_Prop_Entries:
      ti_Data = data->totallines;
      break;
    case MUIA_TextEditor_Prop_Visible:
      ti_Data = data->maxlines;
      break;
    case MUIA_TextEditor_Prop_DeltaFactor:
      ti_Data = data->height;
      break;
    case MUIA_TextEditor_Prop_First:
#ifdef ClassAct
      ti_Data = (data->visual_y-1);
#else
      ti_Data = (data->visual_y-1)*data->height;
#endif
    break;
    case MUIA_TextEditor_ReadOnly:
      ti_Data = ((data->flags & FLG_ReadOnly) ? TRUE : FALSE);
      break;
    case MUIA_TextEditor_Quiet:
      ti_Data = ((data->flags & FLG_Quiet) ? TRUE : FALSE);
      break;
    case MUIA_TextEditor_StyleBold:
      ti_Data = ((GetStyle(data->CPos_X, data->actualline) & BOLD) ? TRUE : FALSE);
      break;
    case MUIA_TextEditor_StyleItalic:
      ti_Data = ((GetStyle(data->CPos_X, data->actualline) & ITALIC) ? TRUE : FALSE);
      break;
    case MUIA_TextEditor_StyleUnderline:
      ti_Data = ((GetStyle(data->CPos_X, data->actualline) & UNDERLINE) ? TRUE : FALSE);
      break;
    case MUIA_TextEditor_TypeAndSpell:
      ti_Data = data->TypeAndSpell;
      break;
    case MUIA_TextEditor_WrapBorder:
      ti_Data = data->WrapBorder;
      break;
    default:
      return(DoSuperMethodA(cl, obj, (Msg)msg));
  }
  *msg->opg_Storage = ti_Data;
  return(TRUE);
}

ULONG Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct InstData *data = INST_DATA(cl, obj);
    struct TagItem *tags, *tag;
    char  *contents = NULL;
    ULONG result = FALSE;
    ULONG crsr_x = 0xffff, crsr_y = 0xffff;

#ifndef ClassAct
  if(data->shown && !(data->flags & FLG_Draw))
  {
    if((tag = FindTagItem(MUIA_Disabled, msg->ops_AttrList)))
    {
      if(tag->ti_Data)
          data->flags |= FLG_Ghosted;
      else  data->flags &= ~FLG_Ghosted;
    }
    data->UpdateInfo = msg;
    MUI_Redraw(obj, MADF_DRAWUPDATE);
    return((ULONG)data->UpdateInfo);
  }

#endif

  tags = msg->ops_AttrList;
  while((tag = NextTagItem(&tags)))
  {
    ULONG ti_Data = tag->ti_Data;

    switch(tag->ti_Tag)
    {
#ifdef ClassAct
      case GA_TextAttr:
        data->TextAttrPtr = (struct TextAttr *)ti_Data;
        if(!data->font && FindTask(NULL)->tc_Node.ln_Type == NT_PROCESS)
          data->font = OpenDiskFont(data->TextAttrPtr);
      break;

      case GA_Disabled:
#else
      case MUIA_ControlChar:
        data->CtrlChar = (UBYTE)ti_Data;
      break;

      case MUIA_Disabled:
#endif
        if(ti_Data)
            data->flags |= FLG_Ghosted;
        else  data->flags &= ~FLG_Ghosted;
#ifndef ClassAct

        MUI_Redraw(obj, MADF_DRAWOBJECT);
#endif
        break;

      case MUIA_TextEditor_Rows:
        data->Rows = ti_Data;
      break;

      case MUIA_TextEditor_Columns:
        data->Columns = ti_Data;
      break;

      case MUIA_TextEditor_AutoClip:
        if(ti_Data)
            data->flags |= FLG_AutoClip;
        else  data->flags &= ~FLG_AutoClip;
      break;

      case MUIA_TextEditor_ColorMap:
        data->colormap = (ULONG *)ti_Data;
        break;
      case MUIA_TextEditor_InVirtualGroup:
        if(ti_Data)
            data->flags |= FLG_InVGrp;
        else  data->flags &= ~FLG_InVGrp;
        break;
      case MUIA_TextEditor_CursorX:
        if(!data->NoNotify)
          crsr_x = ti_Data;
        break;
      case MUIA_TextEditor_CursorY:
        if(!data->NoNotify)
          crsr_y = ti_Data;
        break;

      case MUIA_TextEditor_Prop_Release:
        data->smooth_wait = ti_Data;
        break;

      case MUIA_TextEditor_Prop_First:
#ifdef ClassAct
        if((data->visual_y-1 != ti_Data) && (data->shown))
        {
            LONG diff;

          diff = data->visual_y - (ti_Data+1);
          data->visual_y = ti_Data+1;
          if(diff > 0)
              ScrollDown(0, diff, data);
          else  ScrollUp(0, -diff, data);
        }
#else
        if(((data->visual_y-1)*data->height+(data->realypos - data->ypos) != (LONG)ti_Data) && (data->shown))
        {
            LONG     diff, smooth;
            LONG     lastpixel = ((data->visual_y-1)*data->height) + (data->realypos - data->ypos);
            LONG     old_visual_y = data->visual_y;
            struct   Hook  *oldhook;
            void    *cliphandle;
            struct  line_node *oldline = data->actualline;

          diff = data->visual_y - ((ti_Data/data->height)+1);

          if(data->flags & FLG_Active)
            SetCursor(data->CPos_X, data->actualline, FALSE, data);
          data->visual_y = (ti_Data/data->height)+1;
          smooth = ti_Data - lastpixel;

          if(smooth > 0)
              data->scr_direction = 1;
          else  data->scr_direction = 0;

          data->actualline = NULL;
          oldhook = InstallLayerHook(data->rport->Layer, LAYERS_NOBACKFILL);
          cliphandle = MUI_AddClipping(muiRenderInfo(data->object), data->xpos, data->realypos, data->innerwidth, data->maxlines*data->height);
          if(smooth > 0 && smooth < data->maxlines*data->height)
          {
              LONG line_nr;

            ScrollRasterBF(data->rport, 0, smooth,
                          data->xpos, data->realypos,
                          data->xpos + data->innerwidth - 1,
                          data->realypos + (data->maxlines * data->height) - 1);

            data->ypos = data->realypos - ti_Data%data->height;
            line_nr = data->maxlines-(smooth/data->height)-1;

            {
                struct Layer *layer = data->rport->Layer;

              if(layer->DamageList && layer->DamageList->RegionRectangle)
              {
                if(MUI_BeginRefresh(muiRenderInfo(data->object),0))
                {
                  MUI_Redraw(data->object, MADF_DRAWOBJECT);
                  MUI_EndRefresh(muiRenderInfo(data->object), 0);
                }
              }
            }
            DumpText(data->visual_y+line_nr, line_nr, data->maxlines+1, FALSE, data);
          }
          else
          {
            if(smooth < 0 && -smooth < data->maxlines*data->height)
            {
                LONG lines;

              ScrollRasterBF(data->rport, 0, smooth,
                            data->xpos, data->realypos,
                            data->xpos + data->innerwidth - 1,
                            data->realypos + (data->maxlines * data->height) - 1);
              data->ypos = data->realypos - ti_Data%data->height;
              lines = (-smooth/data->height)+2;
              {
                  struct Layer *layer = data->rport->Layer;

                if(layer->DamageList && layer->DamageList->RegionRectangle)
                  if(MUI_BeginRefresh(muiRenderInfo(data->object),0))
                  {
                    MUI_Redraw(data->object, MADF_DRAWOBJECT);
                    MUI_EndRefresh(muiRenderInfo(data->object), 0);
                  }
              }
              DumpText(data->visual_y, 0, lines, FALSE, data);
            }
            else
            {
              if(smooth)
              {
                DumpText(data->visual_y, 0, data->maxlines+1, FALSE, data);
              }
            }
          }
          MUI_RemoveClipping(muiRenderInfo(data->object), cliphandle);
          InstallLayerHook(data->rport->Layer, oldhook);
          data->actualline = oldline;

          if(!Enabled(data))
          {
              LONG   move = old_visual_y - data->visual_y;

            if(move > 0)
            {
              while(move--)
                GoUp(data);
            }
            else
            {
              while(move++)
                GoDown(data);
            }
            data->blockinfo.enabled = FALSE;
            data->blockinfo.startline = data->actualline;
            data->blockinfo.startx = data->CPos_X;
          }

          if(!data->scrollaction)
          {
            RequestInput(data);
            data->smooth_wait = 1;
            data->scrollaction = TRUE;
          }
        }
        SetAttrs(obj, MUIA_TextEditor_Prop_Entries,
                  ((data->totallines-(data->visual_y-1) < data->maxlines) ?
                    ((data->visual_y-1)+data->maxlines) :
                    ((data->maxlines > data->totallines) ?
                      data->maxlines :
                      data->totallines))
                    * data->height,
                  TAG_DONE);
#endif
        break;
#ifdef ClassAct
      case GA_ReadOnly:
#endif
      case MUIA_TextEditor_ReadOnly:
        if(ti_Data)
        {
          SetCursor(data->CPos_X, data->actualline, FALSE, data);
          data->flags |= FLG_ReadOnly;
        }
        else
        {
          data->flags &= ~FLG_ReadOnly;
          if(data->shown)
          {
            if(data->flags & FLG_Active)
              SetCursor(data->CPos_X, data->actualline, TRUE, data);
          }
        }
        break;
      case MUIA_TextEditor_PopWindow_Open:
        if(ti_Data)
            data->flags |=  FLG_PopWindow;
        else  data->flags &= ~FLG_PopWindow;
        break;
      case MUIA_TextEditor_Quiet:
        if(ti_Data)
        {
          data->flags |= FLG_Quiet;
        }
        else
        {
          data->flags &= ~FLG_Quiet;
#ifndef ClassAct
          MUI_Redraw(obj, MADF_DRAWOBJECT);
          if(data->maxlines > data->totallines)
              set(data->object, MUIA_TextEditor_Prop_Entries, data->maxlines*data->height);
          else  set(data->object, MUIA_TextEditor_Prop_Entries, data->totallines*data->height);
          set(data->object, MUIA_TextEditor_Prop_First, (data->visual_y-1)*data->height);
#endif
        }
        break;
      case MUIA_TextEditor_StyleBold:
        AddStyle(&data->blockinfo, BOLD, ti_Data, data);
        break;
      case MUIA_TextEditor_StyleItalic:
        AddStyle(&data->blockinfo, ITALIC, ti_Data, data);
        break;
      case MUIA_TextEditor_StyleUnderline:
        AddStyle(&data->blockinfo, UNDERLINE, ti_Data, data);
        break;

      case MUIA_TextEditor_Pen:
        if(!data->NoNotify)
        {
          data->Pen = ti_Data;
          AddColor(&data->blockinfo, (UWORD)ti_Data, data);
          data->HasChanged = TRUE;
        }
        break;

#ifndef ClassAct
      case MUIA_TextEditor_KeyUpFocus:
        data->KeyUpFocus = (Object *)ti_Data;
      break;

      case MUIA_TextEditor_Slider:
        if(!data->shown)
        {
          data->slider = (void *)ti_Data;

          DoMethod(data->slider, MUIM_Notify,
              MUIA_Prop_Release, MUIV_EveryTime,
              obj, 3, MUIM_NoNotifySet, MUIA_TextEditor_Prop_Release, MUIV_TriggerValue);
          DoMethod(data->slider, MUIM_Notify,
              MUIA_Prop_First, MUIV_EveryTime,
              obj, 3, MUIM_NoNotifySet, MUIA_TextEditor_Prop_First, MUIV_TriggerValue);
          DoMethod(obj, MUIM_Notify,
              MUIA_TextEditor_Prop_First, MUIV_EveryTime,
              data->slider, 3, MUIM_NoNotifySet, MUIA_Prop_First, MUIV_TriggerValue);
          DoMethod(obj, MUIM_Notify,
              MUIA_TextEditor_Prop_Entries, MUIV_EveryTime,
              data->slider, 3, MUIM_NoNotifySet, MUIA_Prop_Entries, MUIV_TriggerValue);
          DoMethod(obj, MUIM_Notify,
              MUIA_TextEditor_Prop_Visible, MUIV_EveryTime,
              data->slider, 3, MUIM_NoNotifySet, MUIA_Prop_Visible, MUIV_TriggerValue);
          DoMethod(obj, MUIM_Notify,
              MUIA_TextEditor_Prop_DeltaFactor, MUIV_EveryTime,
              data->slider, 3, MUIM_NoNotifySet, MUIA_Prop_DeltaFactor, MUIV_TriggerValue);
        }
        break;
      case MUIA_TextEditor_FixedFont:
        {
          if(!data->shown)
          {
            if(ti_Data)
            {
              data->font = data->fixedfont;
              data->use_fixedfont = TRUE;
            }
            else
            {
              data->font = data->normalfont;
              data->use_fixedfont = FALSE;
            }
          }
          break;
        }
#else
      case TAG_USER + 0x5000006: //CLASSACT_ChangePrefs:
        {
            ULONG temp;

          SetAttrs(data->Bevel, TAG_USER + 0x5000006, ti_Data, TAG_END);

          GetAttr(BEVEL_VertSize,  data->Bevel, &temp);
          data->BevelHoriz = temp;

          GetAttr(BEVEL_HorizSize, data->Bevel, &temp);
          data->BevelVert  = temp;
        }
        break;
      case TAG_USER + 0x5000007: //CLASSACT_SpecialPens
        {
          SetAttrs(data->Bevel, TAG_USER + 0x5000007, ti_Data, TAG_END);
          data->backgroundcolor = ((struct SpecialPens *)ti_Data)->sp_LightPen;
        }
        break;
#endif
      case MUIA_TextEditor_DoubleClickHook:
        data->DoubleClickHook = (struct Hook *)ti_Data;
        break;
      case MUIA_TextEditor_HasChanged:
        data->HasChanged = ti_Data;
        if(!ti_Data)
          data->flags &= ~FLG_UndoLost;
        break;
      case MUIA_TextEditor_HorizontalScroll:
        if(ti_Data)
            data->flags |=  FLG_HScroll;
        else  data->flags &= ~FLG_HScroll;
        break;
      case MUIA_TextEditor_Contents:
        contents = (char *)ti_Data;
        break;
      case MUIA_TextEditor_ImportHook:
        switch(ti_Data)
        {
          case MUIV_TextEditor_ImportHook_Plain:
            data->ImportHook = &ImPlainHook;
            break;
          case MUIV_TextEditor_ImportHook_EMail:
            data->ImportHook = &ImEMailHook;
            break;
          case MUIV_TextEditor_ImportHook_MIME:
            data->ImportHook = &ImMIMEHook;
            break;
          case MUIV_TextEditor_ImportHook_MIMEQuoted:
            data->ImportHook = &ImMIMEQuoteHook;
            break;
          default:
            data->ImportHook = (struct Hook *)ti_Data;
        }
        break;
      case MUIA_TextEditor_ImportWrap:
        data->ImportWrap = ti_Data;
        break;
      
      case MUIA_TextEditor_ExportHook:
      {
        switch(ti_Data)
        {
          case MUIV_TextEditor_ExportHook_Plain:
          {
            data->ExportHook = &ExportHookPlain;
          }
          break;
          
          case MUIV_TextEditor_ExportHook_EMail:
          {
            data->ExportHook = &ExportHookEMail;
          }
          break;

          default:
            data->ExportHook = (struct Hook *)ti_Data;
        }
      }
      break;

      case MUIA_TextEditor_ExportWrap:
        data->ExportWrap = ti_Data;
        break;
      case MUIA_TextEditor_Flow:
        if(!data->NoNotify)
        {
            LONG    start, lines = 0;

          data->Flow = ti_Data;
          if(Enabled(data))
          {
              struct marking newblock;
              struct line_node *startline;

            NiceBlock(&data->blockinfo, &newblock);
            startline = newblock.startline;
            start = LineToVisual(startline, data);

            do {
              lines += startline->visual;
              startline->line.Flow = ti_Data;
              startline = startline->next;
            } while(startline != newblock.stopline->next);
          }
          else
          {
            start = LineToVisual(data->actualline, data);
            lines = data->actualline->visual;
            data->actualline->line.Flow = ti_Data;
            data->pixel_x = 0;
          }
          if(start < 1)
            start = 1;
          if(start-1+lines > data->maxlines)
            lines = data->maxlines-(start-1);
          DumpText(data->visual_y+start-1, start-1, start-1+lines, TRUE, data);
          data->HasChanged = TRUE;
        }
        break;
      case MUIA_TextEditor_WrapBorder:
        data->WrapBorder = ti_Data;
        break;
      case MUIA_TextEditor_TypeAndSpell:
        data->TypeAndSpell = ti_Data;
        break;
    }
  }

  if(contents)
  {
    struct line_node *newcontents;

    if((newcontents = ImportText(contents, data->mypool, data->ImportHook, data->ImportWrap)))
    {
      FreeTextMem(data->mypool, data->firstline);
      data->firstline = newcontents;
      ResetDisplay(data);
      ResetUndoBuffer(data);
      result = TRUE;
    }
  }

  if(crsr_x != 0xffff || crsr_y != 0xffff)
  {
    SetCursor(data->CPos_X, data->actualline, FALSE, data);
    if(crsr_y != 0xffff)
    {
      data->actualline = LineNode(crsr_y+1, data);
      if(data->actualline->line.Length < data->CPos_X)
        data->CPos_X = data->actualline->line.Length-1;
    }

    if(crsr_x != 0xffff)
    {
      data->CPos_X = (data->actualline->line.Length > (ULONG)crsr_x) ? (UWORD)crsr_x : (UWORD)data->actualline->line.Length-1;
    }
    ScrollIntoDisplay(data);
    if(data->flags & FLG_Active)
      SetCursor(data->CPos_X, data->actualline, TRUE, data);
    data->pixel_x = 0;
    result = TRUE;
  }

  return result;
}
