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

 $Id$

***************************************************************************/

#include <Devices/InputEvent.h>
#include <DOS/DOS.h>
#include <Exec/Types.h>
#include <CLib/alib_protos.h>
#include <Proto/DOS.h>
#include <Proto/Exec.h>
#include <Proto/Intuition.h>
#include <Proto/Graphics.h>
#include <Proto/Utility.h>
#include <Intuition/ClassUsr.h>
#include <Intuition/GadgetClass.h>
#include <Intuition/ICClass.h>

#include <Images/Bevel.h>

#include <Editor.h>
#include <TextEditor_mcc.h>

SetupGadgetIBox(struct Gadget *, struct IBox *, struct IBox *);
ULONG New(struct IClass *, Object *, struct opSet *);
ULONG Dispose(struct IClass *, Object *, Msg);

struct EraseBackground
{
  Object obj;
  LONG left;
  LONG top;
  LONG width;
  LONG height;
  LONG flags;
};

#if 0
struct impDomain
{
    ULONG MethodID;
    struct DrawInfo *imp_DrInfo;  /* DrawInfo */
    struct RastPort *imp_RPort; /* RastPort to layout for */
    LONG          imp_Which;  /* what size - min/nominal/max */
    struct IBox   imp_Domain; /* Resulting domain */
    struct TagItem  *imp_Attrs; /* Additional attributes */
};
#endif

VOID NotifySet (Object *obj, struct GadgetInfo *GInfo, ULONG ti_tag, ULONG ti_data)
{
  if(GInfo)
  {
    struct TagItem taglist[] =
    {
      ti_tag, ti_data,
      TAG_DONE
    };

/*    STRPTR name = NULL;
    switch(ti_tag)
    {
      case MUIA_TextEditor_HasChanged:
        name = "HasChanged";
      break;

      case MUIA_TextEditor_UndoAvailable:
        name = "UndoAvailable";
      break;

      case MUIA_TextEditor_RedoAvailable:
        name = "RedoAvailable";
      break;
    }
    if(name)
      kprintf("OM_NOTIFY: %s = %s\n", name, ti_data ? "TRUE" : "FALSE");
*/
    DoMethod(obj, OM_NOTIFY, taglist, GInfo, 0);
  }
}
extern ULONG Dispatcher (REG(a0) struct IClass *cl, REG(a2) Object *obj, REG(a1) Msg msg)
{
    struct mydata *data = INST_DATA(cl, obj);
    struct GadgetInfo *GInfo = NULL;
    LONG  t_totallines;
    LONG  t_visual_y;
    LONG  t_haschanged;
    BOOL  areamarked;
    BOOL  t_undoavail;
    BOOL  t_redoavail;
    ULONG result;
    BOOL  notify = FALSE;

  switch(msg->MethodID)
  {
    case OM_NEW:
    case OM_DISPOSE:
    case MUIM_DrawBackground:
    break;

    default:
    {
      ObtainSemaphore(&data->semaphore);

      t_totallines = data->totallines;
      t_visual_y   = data->visual_y;
      t_haschanged = data->HasChanged;
      areamarked  = Enabled(data);
      t_undoavail = data->undosize && data->undobuffer != data->undopointer;
      t_redoavail = data->undosize && *(short *)data->undopointer != 0xff;
    }
  }

  switch(msg->MethodID)
  {
    case GM_DOMAIN:
    {
      struct gpDomain *gpd = (struct gpDomain *)msg;

      switch(gpd->gpd_Which)
      {
        case GDOMAIN_NOMINAL:
          gpd->gpd_Domain.Width = 300;
          gpd->gpd_Domain.Height = 80;
        break;

        case GDOMAIN_MAXIMUM:
          gpd->gpd_Domain.Width = (ULONG)-1;
          gpd->gpd_Domain.Height = (ULONG)-1;
        break;

        default:
          gpd->gpd_Domain.Width = 50;
          gpd->gpd_Domain.Height = gpd->gpd_RPort->Font->tf_YSize + (2*data->BevelVert) + 2;
        break;
      }
    }
    break;

    case GM_RENDER:
      {
          struct line_node  *line;
          struct IBox gadbox;
          struct gpRender *gpr = (struct gpRender *)msg;
          LONG lines = 0;

        result = DoSuperMethodA(cl, obj, msg);
        if(data->doublebuffer)
          MUIG_FreeBitMap(data->doublebuffer);

        data->object = obj;
        data->smooth_wait = 0;
        data->scrollaction = FALSE;

        if(gpr->gpr_RPort && gpr->gpr_GInfo && gpr->gpr_Redraw == GREDRAW_REDRAW)
        {
          SetupGadgetIBox((struct Gadget *)obj, &gpr->gpr_GInfo->gi_Domain, &gadbox);

          if(data->Bevel)
          {
            data->Bevel->Width  = gadbox.Width;
            data->Bevel->Height = gadbox.Height;

            SetAttrs(data->Bevel,
              BEVEL_FillPen, data->backgroundcolor,
              TAG_END);

            DrawImageState(gpr->gpr_RPort,
              data->Bevel,
              gadbox.Left, gadbox.Top,
              IDS_NORMAL,
              gpr->gpr_GInfo->gi_DrInfo);
          }
          else
          {
            data->BevelVert  = 0;
            data->BevelHoriz = 0;
          }

          data->textcolor         = gpr->gpr_GInfo->gi_DrInfo->dri_Pens[TEXTPEN];
          data->highlightcolor    = gpr->gpr_GInfo->gi_DrInfo->dri_Pens[HIGHLIGHTTEXTPEN];
          data->cursorcolor       = data->highlightcolor;
          data->cursortextcolor   = data->textcolor;
          data->markedcolor       = gpr->gpr_GInfo->gi_DrInfo->dri_Pens[FILLPEN];
          data->separatorshine    = gpr->gpr_GInfo->gi_DrInfo->dri_Pens[SHINEPEN];
          data->separatorshadow = gpr->gpr_GInfo->gi_DrInfo->dri_Pens[SHADOWPEN];

          if(!data->font)
          {
            if(data->TextAttrPtr)
              data->font = OpenFont(data->TextAttrPtr);
            if(!data->font)
            {
              struct TextAttr ta;
              AskFont(gpr->gpr_RPort, &ta);
              data->font = OpenFont(&ta);
            }
//            data->font        = gpr->gpr_RPort->Font;
          }
          data->rport       = gpr->gpr_RPort;
          data->height      = data->font->tf_YSize;
          data->xpos        = gadbox.Left + data->BevelHoriz + 2;
          data->innerwidth    = gadbox.Width - (2 * data->BevelHoriz) - 4;
          data->maxlines      = (gadbox.Height - (2 * data->BevelVert) - 2) / data->height;
          data->ypos        = gadbox.Top + ((gadbox.Height-(data->maxlines*data->height))/2);
          data->realypos      = data->ypos;

          line = data->firstline;
          while(line)
          {
              LONG c;

            c = VisualHeight(line, data);
            lines += c;
            line->visual = c;
            line = line->next;
          }
          data->totallines = lines;

          InitRastPort(&data->doublerp);
          data->doublebuffer = MUIG_AllocBitMap(data->innerwidth+((data->height-data->font->tf_Baseline+1)>>1)+1, data->height, data->rport->BitMap->Depth, (BMF_CLEAR | BMF_INTERLEAVED), data->rport->BitMap);
          data->doublerp.BitMap = data->doublebuffer;
          SetFont(&data->doublerp, data->font);

          data->copyrp = *data->rport;
          SetFont(&data->copyrp, data->font);

          {
              struct TagItem taglist[] =
              {
                MUIA_TextEditor_Prop_DeltaFactor, 1,
                MUIA_TextEditor_Prop_Entries,
                ((data->totallines-(data->visual_y-1) < data->maxlines) ?
                  ((data->visual_y-1)+data->maxlines) :
                  ((data->maxlines > data->totallines) ?
                    data->maxlines :
                    data->totallines)),
                MUIA_TextEditor_Prop_Visible, data->maxlines,
                MUIA_TextEditor_Prop_First, (data->visual_y-1),
                TAG_DONE
              };

            DoMethod(obj, OM_NOTIFY, taglist, gpr->gpr_GInfo, 0);
          }

          data->shown   = TRUE;
          DumpText(data->visual_y, 0, data->maxlines, FALSE, data);
          notify = TRUE;
        }
        break;
      }

    case GM_GOACTIVE:
      {
          struct gpInput *gpi = (struct gpInput *)msg;

        if(data->flags & FLG_Ghosted)
        {
          ReleaseSemaphore(&data->semaphore);
          return GMR_NOREUSE;
        }

        data->flags |= FLG_Active;
        data->rport = ObtainGIRPort(gpi->gpi_GInfo);
        result = GMR_MEACTIVE;
        if(gpi->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE && gpi->gpi_IEvent->ie_Code == IECODE_LBUTTON)
        {
            UWORD starty = ((data->Bevel->Height-(data->maxlines*data->height))/2);

          if(gpi->gpi_Mouse.X >= data->BevelHoriz+2 && gpi->gpi_Mouse.X < (data->BevelHoriz+2)+data->innerwidth &&
            gpi->gpi_Mouse.Y >= starty && gpi->gpi_Mouse.Y < starty+(data->maxlines*data->height))
          {
            data->mousemove = TRUE;
            PosFromCursor(gpi->gpi_Mouse.X, gpi->gpi_Mouse.Y-starty, data);

            if(!((data->blockinfo.enabled) && (gpi->gpi_IEvent->ie_Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))))
            {
              if(Enabled(data))
              {
                data->blockinfo.enabled = FALSE;
                MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
              }

              data->blockinfo.enabled = TRUE;
              data->blockinfo.startline = data->actualline;
              data->blockinfo.startx = data->CPos_X;
            }
            else
            {
              if(data->blockinfo.stopline != data->actualline || data->blockinfo.stopx != data->CPos_X)
                MarkText(data->blockinfo.stopx, data->blockinfo.stopline, data->CPos_X, data->actualline, data);
            }
            data->selectmode  = 0;
            data->StartSecs = gpi->gpi_IEvent->ie_TimeStamp.tv_secs;
            data->StartMicros = gpi->gpi_IEvent->ie_TimeStamp.tv_micro;
            data->blockinfo.stopline = data->actualline;
            data->blockinfo.stopx = data->CPos_X;
          }
          else
          {
            result = GMR_NOREUSE;
          }
        }

        if(result != GMR_NOREUSE)
          SetCursor(data->CPos_X, data->actualline, TRUE, data);
        ReleaseGIRPort(data->rport);
        break;
      }
    case GM_HANDLEINPUT:
      {
          struct gpInput *gpi = (struct gpInput *)msg;
          BOOL  update = FALSE;
          UWORD starty = ((data->Bevel->Height-(data->maxlines*data->height))/2);

        if(data->flags & FLG_Ghosted)
        {
          ReleaseSemaphore(&data->semaphore);
          return GMR_NOREUSE;
        }

        switch(gpi->gpi_IEvent->ie_Class)
        {
          case IECLASS_GADGETDOWN:
          case IECLASS_GADGETUP:
          case IECLASS_TIMER:
          case IECLASS_RAWMOUSE:
          case IECLASS_RAWKEY:
            result = GMR_MEACTIVE;
            notify = TRUE;
          break;

          default:
            result = GMR_NOREUSE;
          break;
        }

        data->GInfo = GInfo = gpi->gpi_GInfo;
        if(gpi->gpi_IEvent->ie_Class == IECLASS_RAWKEY)
        {
          data->rport = ObtainGIRPort(gpi->gpi_GInfo);
          if(!ReactOnRawKey(gpi->gpi_IEvent->ie_Code, gpi->gpi_IEvent->ie_Qualifier, (struct IntuiMessage *)gpi->gpi_IEvent, data))
          {
            if(gpi->gpi_IEvent->ie_Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND) && !(gpi->gpi_IEvent->ie_Code & IECODE_UP_PREFIX))
            {
              switch(gpi->gpi_IEvent->ie_Code)
              {
                case 0x60: /* LSHIFT */
                case 0x61: /* RSHIFT */
                case 0x62: /* CapsLock */
                case 0x63: /* CONTROL */
                case 0x64: /* LALT */
                case 0x65: /* RALT */
                case 0x66: /* LCOMMAND */
                case 0x67: /* RCOMMAND */
                break;

                default:
                  result = GMR_REUSE;
                break;
              }
            }
          }
          ReleaseGIRPort(data->rport);
        }

        if((gpi->gpi_IEvent->ie_Qualifier & IEQUALIFIER_LEFTBUTTON) && data->mousemove)
        {
            LONG  MouseX = gpi->gpi_Mouse.X,
                MouseY = gpi->gpi_Mouse.Y;
            LONG  oldCPos_X = data->CPos_X;
            struct line_node *oldactualline = data->actualline;
            BOOL  Scroll = TRUE;

          data->rport = ObtainGIRPort(gpi->gpi_GInfo);
          if(MouseY < starty)
          {
              LONG  diff = starty-MouseY;

            if(diff > 30)
              GoUp(data);
            if(diff > 20)
              GoUp(data);
            if(diff > 10)
              GoUp(data);
            GoUp(data);
            MouseY = 0;
          }
          else
          {
              LONG limit = starty;

            if(data->maxlines < (data->totallines-data->visual_y+1))
                limit += (data->maxlines * data->height);
            else  limit += (data->totallines-data->visual_y+1)*data->height;

            if(MouseY >= limit)
            {
                LONG  diff = MouseY - limit;

              if(diff > 30)
                GoDown(data);
              if(diff > 20)
                GoDown(data);
              if(diff > 10)
                GoDown(data);
              GoDown(data);
            }
            else
            {
              PosFromCursor(MouseX, MouseY-starty, data);
              Scroll = FALSE;
            }
          }

          if(data->blockinfo.enabled && Scroll && data->blockinfo.stopline == data->actualline && data->blockinfo.stopx == data->CPos_X)
            PosFromCursor(MouseX, MouseY-starty, data);

          if(data->selectmode != 0)
          {
              struct marking tmpblock;

            NiceBlock(&data->blockinfo, &tmpblock);
            if(data->blockinfo.startx == tmpblock.startx && data->blockinfo.startline == tmpblock.startline)
            {
              if(MouseX > 0)
              {
                if(data->selectmode == 1)
                {
                  while((data->CPos_X < data->actualline->length-1) && (*(data->actualline->contents+data->CPos_X) != ' '))
                  {
                    data->CPos_X++;
                  }
                }
                else
                {
                  GoEndOfLine(data);
                }
              }
            }
            else
            {
                struct pos_info pos;
                ULONG flow;

              OffsetToLines(data->CPos_X, data->actualline, &pos, data);
              flow = FlowSpace(data->actualline->flow, data->actualline->contents+pos.bytes, data);
              if(MouseX <= flow+MyTextLength(data->font, data->actualline->contents+pos.bytes, pos.extra-pos.bytes-1))
              {
                if(data->selectmode == 1)
                {
                  while(data->CPos_X > 0 && *(data->actualline->contents+data->CPos_X-1) != ' ')
                  {
                    data->CPos_X--;
                  }
                }
                else
                {
                  GoStartOfLine(data);
                }
              }
            }
          }

          if(data->blockinfo.enabled)
          {
            if(data->blockinfo.stopline != data->actualline || data->blockinfo.stopx != data->CPos_X)
            {
              MarkText(data->blockinfo.stopx, data->blockinfo.stopline, data->CPos_X, data->actualline, data);
            }
            data->blockinfo.stopline = data->actualline;
            data->blockinfo.stopx = data->CPos_X;
          }
          else
          {
            data->mousemove = FALSE;
          }

          if((oldCPos_X != data->CPos_X) || (oldactualline != data->actualline))
          {
            ScrollIntoDisplay(data);
            PosFromCursor(MouseX, MouseY-starty, data);
          }
          ReleaseGIRPort(data->rport);
        }

        if(gpi->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE && gpi->gpi_IEvent->ie_Code == (IECODE_LBUTTON | IECODE_UP_PREFIX) && data->mousemove)
        {
          data->mousemove = FALSE;
          if((data->flags & FLG_ReadOnly) && Enabled(data))
          {
            data->rport = ObtainGIRPort(gpi->gpi_GInfo);
            Key_Copy(data);
            ReleaseGIRPort(data->rport);
          }
        }

        if(gpi->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE && gpi->gpi_IEvent->ie_Code == IECODE_LBUTTON)
        {
          if(gpi->gpi_Mouse.X >= data->BevelHoriz+2 && gpi->gpi_Mouse.X < (data->BevelHoriz+2)+data->innerwidth &&
            gpi->gpi_Mouse.Y >= starty && gpi->gpi_Mouse.Y < starty+(data->maxlines*data->height))
          {
            UWORD last_x = data->CPos_X; struct line_node *lastline = data->actualline;
            data->rport = ObtainGIRPort(gpi->gpi_GInfo);
/* ----------------------------- */
            data->mousemove = TRUE;
            SetCursor(data->CPos_X, data->actualline, FALSE, data);
            PosFromCursor(gpi->gpi_Mouse.X, gpi->gpi_Mouse.Y-starty, data);

            if(gpi->gpi_IEvent->ie_Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
            {
              data->selectmode  = 0;
            }

            if(!((data->blockinfo.enabled) && (gpi->gpi_IEvent->ie_Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))))
            {
              if(Enabled(data))
              {
                data->blockinfo.enabled = FALSE;
                MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
              }

              data->blockinfo.enabled = TRUE;
              data->blockinfo.startline = data->actualline;
              data->blockinfo.startx = data->CPos_X;
              if(last_x == data->CPos_X && lastline == data->actualline && DoubleClick(data->StartSecs, data->StartMicros, gpi->gpi_IEvent->ie_TimeStamp.tv_secs, gpi->gpi_IEvent->ie_TimeStamp.tv_micro))
              {
                if(data->DoubleClickHook && (!CallHook(data->DoubleClickHook, (Object *)data->object, data->actualline->contents, data->CPos_X)) || (!data->DoubleClickHook))
                {
                  if(*(data->actualline->contents+data->CPos_X) != ' ' && *(data->actualline->contents+data->CPos_X) != '\n')
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
                        LONG x = data->CPos_X;

                      while(x > 0 && *(data->actualline->contents+x-1) != ' ')
                      {
                        x--;
                      }
                      data->blockinfo.startx = x;
                      data->selectmode  = 1;
                      data->StartSecs = gpi->gpi_IEvent->ie_TimeStamp.tv_secs;
                      data->StartMicros = gpi->gpi_IEvent->ie_TimeStamp.tv_micro;
                    }
                  }
                  else
                  {
                    data->selectmode  = 0;
                    data->StartSecs = gpi->gpi_IEvent->ie_TimeStamp.tv_secs;
                    data->StartMicros = gpi->gpi_IEvent->ie_TimeStamp.tv_micro;
                  }
                }
              }
              else
              {
                data->selectmode  = 0;
                data->StartSecs = gpi->gpi_IEvent->ie_TimeStamp.tv_secs;
                data->StartMicros = gpi->gpi_IEvent->ie_TimeStamp.tv_micro;
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
/* ----------------------------- */
            ReleaseGIRPort(data->rport);
          }
          else
          {
            result = GMR_REUSE;
          }
        }
        else if(gpi->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE && gpi->gpi_IEvent->ie_Code == IECODE_RBUTTON)
          result = GMR_REUSE;

        data->GInfo = NULL; /* just for debug'ing */
        break;
      }
    case GM_GOINACTIVE:
      {
          struct gpGoInactive *gpgi = (struct gpGoInactive *)msg;

        if(gpgi->gpgi_GInfo)
        {
          data->rport = ObtainGIRPort(gpgi->gpgi_GInfo);
          data->mousemove = FALSE;
          data->flags &= ~FLG_Active;
          SetCursor(data->CPos_X, data->actualline, FALSE, data);
          ReleaseGIRPort(data->rport);
        }
        break;
      }
    case MUIM_DrawBackground:
      {
          struct EraseBackground *eb = (struct EraseBackground *)msg;

        if(eb->width >= 1 && eb->height >= 1)
        {
          SetAPen(data->rport, data->backgroundcolor);
          SetDrMd(data->rport, JAM1);
          RectFill(data->rport, eb->left, eb->top, eb->left+eb->width-1, eb->top+eb->height-1);
        }
        return TRUE; // so we avoid ReleaseSemaphore();
      }
    case OM_GET:
      result = Get(cl, obj, (struct opGet *)msg);
      break;

    case OM_UPDATE:
      {
          struct opUpdate *opu = (struct opUpdate *)msg;
          struct Task *mytask = FindTask(NULL);
          struct StackSwapStruct  stackswap;
          ULONG  stacksize = (ULONG)mytask->tc_SPUpper-(ULONG)mytask->tc_SPLower+8192;
          void   *newstack;

        newstack = AllocVec(stacksize, 0L);
        stackswap.stk_Lower   = newstack;
        stackswap.stk_Upper   = (ULONG)newstack+stacksize;
        stackswap.stk_Pointer = (void *)stackswap.stk_Upper;
        if(newstack)
        {
          StackSwap(&stackswap);

          if(GInfo = opu->opu_GInfo)
          {
            data->rport = ObtainGIRPort(opu->opu_GInfo);
            result = Set(cl, obj, (struct opSet *)msg);
            ReleaseGIRPort(data->rport);

            if(FindTagItem(MUIA_TextEditor_Prop_First, opu->opu_AttrList))
            {
              struct TagItem taglist[] =
              {
                MUIA_TextEditor_Prop_Entries,
                ((data->totallines-(data->visual_y-1) < data->maxlines) ?
                  ((data->visual_y-1)+data->maxlines) :
                  ((data->maxlines > data->totallines) ?
                    data->maxlines :
                    data->totallines)),
                TAG_DONE
              };
              DoMethod(obj, OM_NOTIFY, taglist, GInfo, 0);
            }
          }

          StackSwap(&stackswap);
          FreeVec(newstack);
        }
        result = DoSuperMethodA(cl, obj, msg);
        break;
      }
    case OM_SET:
      {
          struct opSet *ops = (struct opSet *)msg;
          BOOL rport = FALSE;

        data->rport = &data->copyrp;
        if(GInfo = ops->ops_GInfo)
        {
          rport = TRUE;
          data->rport = ObtainGIRPort(ops->ops_GInfo);
        }
        result = Set(cl, obj, ops);
        notify = TRUE;

        if(rport)
          ReleaseGIRPort(data->rport);
        break;
      }
    case OM_NEW:
      return(New(cl, obj, (struct opSet *)msg));
    case OM_DISPOSE:
      if(data->font)
        CloseFont(data->font);
      if(data->RawkeyBindings)
        MyFreePooled(data->mypool, data->RawkeyBindings);
      if(data->doublebuffer)
        MUIG_FreeBitMap(data->doublebuffer);
      return(Dispose(cl, obj, msg));

    case MUIM_TextEditor_ClearText:
      GInfo = ((struct MUIP_TextEditor_ClearText *)msg)->GInfo;
      if(data->rport = ObtainGIRPort(GInfo))
      {
        result = ClearText(data);
        notify = TRUE;
        ReleaseGIRPort(data->rport);
      }
    break;

    case MUIM_TextEditor_InsertText:
    {
      struct MUIP_TextEditor_InsertText *imsg = (struct MUIP_TextEditor_InsertText *)msg;
      struct marking block;

      GInfo = imsg->GInfo;
      if(data->rport = ObtainGIRPort(GInfo))
      {
        block.startx = data->CPos_X,
        block.startline = data->actualline,
        result = InsertText(data, imsg->text, TRUE);
        block.stopx = data->CPos_X,
        block.stopline = data->actualline,
        AddToUndoBuffer(pasteblock, (char *)&block, data);
        notify = TRUE;
        ReleaseGIRPort(data->rport);
      }
    }
    break;

    case MUIM_TextEditor_ExportText:
      result = (ULONG)ExportText(data->firstline, data->ExportHook, data->ExportWrap);
    break;

    case MUIM_TextEditor_ARexxCmd:
    {
      struct MUIP_TextEditor_ARexxCmd *amsg = (struct MUIP_TextEditor_ARexxCmd *)msg;
      data->GInfo = GInfo = amsg->GInfo;
      if(data->rport = ObtainGIRPort(GInfo))
      {
        result = HandleARexx(data, amsg->command);
        notify = TRUE;
        ReleaseGIRPort(data->rport);
      }
      data->GInfo = NULL; /* just for debug'ing */
    }
    break;

    case MUIM_TextEditor_MarkText:
    {
      struct MUIP_TextEditor_MarkText *mmsg = (struct MUIP_TextEditor_MarkText *)msg;
      GInfo = mmsg->GInfo;
      if(data->rport = ObtainGIRPort(GInfo))
      {
        result = OM_MarkText(mmsg, data);
        ReleaseGIRPort(data->rport);
      }
    }
    break;

    case MUIM_TextEditor_BlockInfo:
      result = OM_BlockInfo((struct MUIP_TextEditor_BlockInfo *)msg, data);
    break;

    case MUIM_TextEditor_Search:
    {
      struct MUIP_TextEditor_Search *smsg = (struct MUIP_TextEditor_Search *)msg;
      GInfo = smsg->GInfo;
      if(data->rport = ObtainGIRPort(GInfo))
      {
        notify = result = OM_Search(smsg, data);
        ReleaseGIRPort(data->rport);
      }
    }
    break;

    case MUIM_TextEditor_Replace:
    {
      struct MUIP_TextEditor_Replace *rmsg = (struct MUIP_TextEditor_Replace *)msg;
      GInfo = rmsg->GInfo;
      if(data->rport = ObtainGIRPort(GInfo))
      {
        result = OM_Replace(obj, rmsg, data);
        ReleaseGIRPort(data->rport);
      }
    }
    break;

    default:
      result = DoSuperMethodA(cl, obj, msg);
    break;
  }
  ReleaseSemaphore(&data->semaphore);

  if(notify)
  {
      struct  Task          *mytask = FindTask(NULL);
      struct  StackSwapStruct stackswap;
      ULONG   stacksize = (ULONG)mytask->tc_SPUpper-(ULONG)mytask->tc_SPLower+8192;
      void    *newstack;

    if(msg->MethodID == OM_SET)
    {
      ULONG newresult = DoSuperMethodA(cl, obj, msg);
      if(result)
          result = newresult;
      else  return newresult;
    }

    newstack = AllocVec(stacksize, 0L);
    stackswap.stk_Lower   = newstack;
    stackswap.stk_Upper   = (ULONG)newstack+stacksize;
    stackswap.stk_Pointer = (APTR)stackswap.stk_Upper;
    if(newstack)
    {
      StackSwap(&stackswap);

      if((data->visual_y != t_visual_y) || (data->totallines != t_totallines))
      {
        if(GInfo)
        {
            struct TagItem taglist[] =
            {
              MUIA_TextEditor_Prop_Entries,
              ((data->totallines-(data->visual_y-1) < data->maxlines) ?
                ((data->visual_y-1)+data->maxlines) :
                ((data->maxlines > data->totallines) ?
                  data->maxlines :
                  data->totallines)),
              MUIA_TextEditor_Prop_First, (data->visual_y-1),
              TAG_DONE
            };

          DoMethod(obj, OM_NOTIFY, taglist, GInfo, 0);
        }
      }

      if(t_undoavail != (data->undosize && data->undobuffer != data->undopointer))
        NotifySet(obj, GInfo, MUIA_TextEditor_UndoAvailable, !t_undoavail);

      if(t_redoavail != (data->undosize && *(short *)data->undopointer != 0xff))
        NotifySet(obj, GInfo, MUIA_TextEditor_RedoAvailable, !t_redoavail);

      if(t_haschanged != data->HasChanged)
        NotifySet(obj, GInfo, MUIA_TextEditor_HasChanged, data->HasChanged);

      if(areamarked != Enabled(data))
        NotifySet(obj, GInfo, MUIA_TextEditor_AreaMarked, Enabled(data));

      if(data->actualline->flow != data->Flow)
      {
        data->Flow = data->actualline->flow;
        NotifySet(obj, GInfo, MUIA_TextEditor_Flow, data->actualline->flow);
      }

      if(data->actualline->separator != data->Separator)
      {
        data->Separator = data->actualline->separator;
        NotifySet(obj, GInfo, MUIA_TextEditor_Separator, data->actualline->separator);
      }

      CA_UpdateStyles(GInfo, data);

      StackSwap(&stackswap);
      FreeVec(newstack);
    }
  }

  return(result);
}
