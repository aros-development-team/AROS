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

 $Id: BlockOperators.c,v 1.13 2005/08/06 20:07:48 damato Exp $

***************************************************************************/

#include <exec/io.h>
#include <devices/clipboard.h>
#include <libraries/iffparse.h>

#include <clib/alib_protos.h>
#include <proto/utility.h>
#include <proto/exec.h>

#include "TextEditor_mcc.h"
#include "private.h"

#ifdef __AROS__
#include <aros/macros.h>
#define LONG2BE(x) AROS_LONG2BE(x)
#define BE2LONG(x) AROS_BE2LONG(x)
#define WORD2BE(x) AROS_WORD2BE(x)
#define BE2WORD(x) AROS_BE2WORD(x)
#else
#define LONG2BE(x) x
#define BE2LONG(x) x
#define WORD2BE(x) x
#define BE2WORD(x) x
#endif

VOID RedrawArea(UWORD startx, struct line_node *startline, UWORD stopx, struct line_node *stopline, struct InstData *data)
{
  struct pos_info pos1, pos2;
  LONG line_nr1 = LineToVisual(startline, data) - 1;
  LONG line_nr2 = LineToVisual(stopline, data) - 1;

  ENTER();

  OffsetToLines(startx, startline, &pos1, data);

  if(stopx >= stopline->line.Length)
    stopx = stopline->line.Length-1;
  
  OffsetToLines(stopx, stopline, &pos2, data);

  if((line_nr1 += pos1.lines-1) < 0)
    line_nr1 = 0;
  if((line_nr2 += pos2.lines-1) >= data->maxlines)
    line_nr2 = data->maxlines-1;
  if(line_nr1 <= line_nr2)
  {
    DumpText(data->visual_y+line_nr1, line_nr1, line_nr2+1, TRUE, data);
  }

  LEAVE();
}

char *GetBlock (struct marking *block, struct InstData *data)
{
  LONG    startx, stopx;
  struct  line_node *startline, *stopline, *act;
  char    *text = NULL;
  struct  ExportMessage emsg;

  ENTER();

  startx    = block->startx;
  stopx     = block->stopx;
  startline = block->startline;
  stopline  = block->stopline;

  data->CPos_X = startx;
  data->actualline = startline;

  emsg.UserData = NULL;
  emsg.ExportWrap = 0;
  emsg.Last = FALSE;
  emsg.data = data;

  if(startline != stopline)
  {
    /* Create a firstline look-a-like */
    emsg.Contents = (STRPTR)MyAllocPooled(data->mypool, startline->line.Length-startx);
    if(startline->line.Styles && *startline->line.Styles != EOS)
    {
        ULONG startstyle = GetStyle(startx, startline);

      if((emsg.Styles = (UWORD *)MyAllocPooled(data->mypool, *((ULONG *)startline->line.Styles-1)+16)))
      {
          UWORD *styles = emsg.Styles,
              *oldstyles = startline->line.Styles;

        if(startstyle & BOLD)
        {
          *styles++ = 1;  *styles++ = BOLD;
        }
        if(startstyle & ITALIC)
        {
          *styles++ = 1;  *styles++ = ITALIC;
        }
        if(startstyle & UNDERLINE)
        {
          *styles++ = 1;  *styles++ = UNDERLINE;
        }

        while(*oldstyles <= startx)
          oldstyles += 2;

        while(*oldstyles != EOS)
        {
          *styles++ = *oldstyles++ - startx;  *styles++ = *oldstyles++;
        }
        *styles = EOS;
      }
    }
    else
      emsg.Styles = NULL;

    emsg.Colors = NULL;
    if(emsg.Contents)
    {
      CopyMem(startline->line.Contents + startx, emsg.Contents, startline->line.Length - startx);
      emsg.Length = startline->line.Length - startx;
      emsg.Flow = startline->line.Flow;
      emsg.Separator = startline->line.Separator;
      emsg.Highlight = startline->line.Color;
      emsg.UserData = (APTR)CallHookA(&ExportHookPlain, NULL, &emsg);
      MyFreePooled(data->mypool, emsg.Contents);
    }

    if(emsg.Styles)
      MyFreePooled(data->mypool, emsg.Styles);

    /* Start iterating... */
    act = startline->next;
    while(act != stopline)
    {
      emsg.Contents = act->line.Contents;
      emsg.Length   = act->line.Length;
      emsg.Styles   = act->line.Styles;
      emsg.Colors   = act->line.Colors;
      emsg.Flow   = act->line.Flow;
      emsg.Separator = act->line.Separator;
      emsg.Highlight = act->line.Color;
      emsg.UserData = (APTR)CallHookA(&ExportHookPlain, (APTR)NULL, &emsg);
      act = act->next;
    }

    /* Create a Lastline look-a-like */
    emsg.Contents = (STRPTR)MyAllocPooled(data->mypool, stopx);
    if(stopline->line.Styles && *stopline->line.Styles != EOS)
    {
        ULONG stopstyle = GetStyle(stopx, stopline);

      if((emsg.Styles = (UWORD *)MyAllocPooled(data->mypool, *((ULONG *)stopline->line.Styles-1)+16)))
      {
          UWORD *styles = emsg.Styles,
              *oldstyles = stopline->line.Styles;

        while(*oldstyles <= stopx)
        {
          *styles++ = *oldstyles++; *styles++ = *oldstyles++;
        }

        if(stopstyle & BOLD)
        {
          *styles++ = stopx+1;  *styles++ = ~BOLD;
        }
        if(stopstyle & ITALIC)
        {
          *styles++ = stopx+1;  *styles++ = ~ITALIC;
        }
        if(stopstyle & UNDERLINE)
        {
          *styles++ = stopx+1;  *styles++ = ~UNDERLINE;
        }
        *styles = EOS;
      }
    }
    else
      emsg.Styles = NULL;

    emsg.Colors = NULL;
    if(emsg.Contents)
    {
      CopyMem(stopline->line.Contents, emsg.Contents, stopx);
      emsg.Length = stopx;
      emsg.Flow = stopline->line.Flow;
      emsg.Separator = stopline->line.Separator;
      emsg.Highlight = stopline->line.Color;
      emsg.Last = TRUE;
      text = (STRPTR)CallHookA(&ExportHookPlain, NULL, &emsg);
      MyFreePooled(data->mypool, emsg.Contents);
    }

    if(emsg.Styles)
      MyFreePooled(data->mypool, emsg.Styles);
  }
  else
  {
    /* Create a single line */
    emsg.Contents = (STRPTR)MyAllocPooled(data->mypool, stopx-startx);
    if(startline->line.Styles && *startline->line.Styles != EOS)
    {
        ULONG startstyle = GetStyle(startx, startline);
        ULONG stopstyle = GetStyle(stopx, stopline);

      if((emsg.Styles = (UWORD *)MyAllocPooled(data->mypool, *((ULONG *)startline->line.Styles-1))))
      {
          UWORD *styles = emsg.Styles,
              *oldstyles = startline->line.Styles;

        if(startstyle & BOLD)
        {
          *styles++ = 1;  *styles++ = BOLD;
        }
        if(startstyle & ITALIC)
        {
          *styles++ = 1;  *styles++ = ITALIC;
        }
        if(startstyle & UNDERLINE)
        {
          *styles++ = 1;  *styles++ = UNDERLINE;
        }

        while(*oldstyles <= startx)
          oldstyles += 2;

        while(*oldstyles <= stopx)
        {
          *styles++ = *oldstyles++ - startx;
          *styles++ = *oldstyles++;
        }

        if(stopstyle & BOLD)
        {
          *styles++ = stopx-startx+1; *styles++ = ~BOLD;
        }
        if(stopstyle & ITALIC)
        {
          *styles++ = stopx-startx+1; *styles++ = ~ITALIC;
        }
        if(stopstyle & UNDERLINE)
        {
          *styles++ = stopx-startx+1; *styles++ = ~UNDERLINE;
        }
        *styles = EOS;
      }
    }
    else
      emsg.Styles = NULL;

    emsg.Colors = NULL;
    if(emsg.Contents)
    {
      CopyMem(startline->line.Contents+startx, emsg.Contents, stopx-startx);
      emsg.Length = stopx-startx;
      emsg.Flow = startline->line.Flow;
      emsg.Separator = startline->line.Separator;
      emsg.Highlight = startline->line.Color;
      emsg.Last = TRUE;
      text = (STRPTR)CallHookA(&ExportHookPlain, NULL, &emsg);
      MyFreePooled(data->mypool, emsg.Contents);
    }

    if(emsg.Styles)
      MyFreePooled(data->mypool, emsg.Styles);
  }

  RETURN(text);
  return(text);
}

void NiceBlock (struct marking *realblock, struct marking *newblock)
{
  LONG  startx = realblock->startx, stopx = realblock->stopx;
  struct line_node *startline = realblock->startline;
  struct line_node *stopline = realblock->stopline;

  ENTER();

  if(startline == stopline)
  {
    if(startx > stopx)
    {
        LONG  c_x = startx;

      startx = stopx;
      stopx = c_x;
    }
  }
  else
  {
      struct  line_node *c_startline = startline,
                    *c_stopline = stopline;

    while((c_startline != stopline) && (c_stopline != startline))
    {
      if(c_startline->next)
        c_startline = c_startline->next;
      if(c_stopline->next)
        c_stopline = c_stopline->next;
    }

    if(c_stopline == startline)
    {
        LONG  c_x = startx;

      startx = stopx;
      stopx = c_x;

      c_startline = startline;
      startline = stopline;
      stopline = c_startline;
    }
  }
  newblock->startx    = startx;
  newblock->stopx     = stopx;
  newblock->startline = startline;
  newblock->stopline  = stopline;

  LEAVE();
}

BOOL InitClipboard (struct InstData *data)
{
  ENTER();

  if((data->clipport = CreateMsgPort()))
  {
    if((data->clipboard = (struct IOClipReq*)CreateIORequest(data->clipport, sizeof(struct IOClipReq))))
    {
      if(!OpenDevice("clipboard.device", 0, (struct IORequest*)data->clipboard, 0))
      {
        RETURN(TRUE);
        return TRUE;
      }

      CloseDevice((struct IORequest*)data->clipboard);
    }
    DeleteIORequest((struct IORequest*)data->clipboard);
  }
  DeleteMsgPort(data->clipport);

  RETURN(FALSE);
  return(FALSE);
}

void EndClipSession (struct InstData *data)
{
  ULONG clipheader[] = { LONG2BE(MAKE_ID('F','O','R','M')), 0, LONG2BE(MAKE_ID('F','T','X','T'))};

  ENTER();

  clipheader[1] = data->clipboard->io_Offset-8;
  data->clipboard->io_Offset    = 0;
  data->clipboard->io_Data    = (STRPTR)clipheader;
  data->clipboard->io_Length    = sizeof(clipheader);
  DoIO((struct IORequest*)data->clipboard);

  data->clipboard->io_Command = CMD_UPDATE;
  DoIO((struct IORequest*)data->clipboard);

  CloseDevice((struct IORequest*)data->clipboard);
  DeleteIORequest((struct IORequest*)data->clipboard);
  DeleteMsgPort(data->clipport);

  LEAVE();
}

void ClipInfo (struct line_node *line, struct InstData *data)
{
  ULONG  highlightheader[]  = { LONG2BE(MAKE_ID('H','I','G','H')), LONG2BE(2)};
  ULONG  separatorheader[]  = { LONG2BE(MAKE_ID('S','B','A','R')), LONG2BE(2)};
  ULONG  flowheader[]    = { LONG2BE(MAKE_ID('F','L','O','W')), LONG2BE(2)};

  ENTER();

  if(line->line.Flow != MUIV_TextEditor_Flow_Left)
  {
    data->clipboard->io_Data    = (STRPTR)flowheader;
    data->clipboard->io_Length    = sizeof(flowheader);
    DoIO((struct IORequest*)data->clipboard);
    data->clipboard->io_Data    = (STRPTR)&line->line.Flow;
    data->clipboard->io_Length    = BE2LONG(flowheader[1]);
    DoIO((struct IORequest*)data->clipboard);
  }

  if(line->line.Separator)
  {
    data->clipboard->io_Data    = (STRPTR)separatorheader;
    data->clipboard->io_Length    = sizeof(separatorheader);
    DoIO((struct IORequest*)data->clipboard);
    data->clipboard->io_Data    = (STRPTR)&line->line.Separator;
    data->clipboard->io_Length    = BE2LONG(separatorheader[1]);
    DoIO((struct IORequest*)data->clipboard);
  }

  if(line->line.Color)
  {
    data->clipboard->io_Data    = (STRPTR)highlightheader;
    data->clipboard->io_Length    = sizeof(highlightheader);
    DoIO((struct IORequest*)data->clipboard);
    data->clipboard->io_Data    = (STRPTR)&line->line.Color;
    data->clipboard->io_Length    = BE2LONG(highlightheader[1]);
    DoIO((struct IORequest*)data->clipboard);
  }

  LEAVE();
}

void ClipChars (LONG x, struct line_node *line, LONG length, struct InstData *data)
{
  long  colorheader[]   = { LONG2BE(MAKE_ID('C','O','L','S')), 0};
  long  styleheader[]   = { LONG2BE(MAKE_ID('S','T','Y','L')), 0};
  long  textheader[]    = { LONG2BE(MAKE_ID('C','H','R','S')), 0};
  UWORD style[2] = {1, GetStyle(x-1, line)};
  UWORD color[2] = {1, 0};
  ULONG t_offset;
  UWORD *colors = line->line.Colors;

  ENTER();

  ClipInfo(line, data);

  if(colors)
  {
    data->clipboard->io_Offset    += sizeof(colorheader);
    t_offset = data->clipboard->io_Offset;
    data->clipboard->io_Data    = (STRPTR)color;
    data->clipboard->io_Length    = 4;
    while((*colors <= x) && (*colors != 0xffff))
    {
      color[1] = *(colors+1);
      colors += 2;
    }

    if(color[1] != 0 && *colors-x != 1)
    {
      DoIO((struct IORequest*)data->clipboard);
    }

    if(*colors != 0xffff)
    {
      while(*colors <= x+length)
      {
        color[0] = *colors++ - x;
        color[1] = *colors++;
        DoIO((struct IORequest*)data->clipboard);
      }
    }

    colorheader[1] = LONG2BE(data->clipboard->io_Offset - t_offset);
    data->clipboard->io_Offset    = t_offset - sizeof(colorheader);
    if(colorheader[1])
    {
      data->clipboard->io_Data    = (STRPTR)colorheader;
      data->clipboard->io_Length    = sizeof(colorheader);
      DoIO((struct IORequest*)data->clipboard);
      data->clipboard->io_Offset    += BE2LONG(colorheader[1]);
    }
  }

/* --- Styles --- */
  data->clipboard->io_Offset    += sizeof(styleheader);
  t_offset = data->clipboard->io_Offset;

  data->clipboard->io_Data    = (STRPTR)style;
  data->clipboard->io_Length    = 4;
  if(style[1] != 0)
  {
      unsigned short t_style = style[1];

    if(t_style & BOLD)
    {
      style[1] = BOLD;
      DoIO((struct IORequest*)data->clipboard);
    }
    if(t_style & ITALIC)
    {
      style[1] = ITALIC;
      DoIO((struct IORequest*)data->clipboard);
    }
    if(t_style & UNDERLINE)
    {
      style[1] = UNDERLINE;
      DoIO((struct IORequest*)data->clipboard);
    }
  }

  if(line->line.Styles)
  {
      unsigned short *styles = line->line.Styles;

    while((*styles <= x) && (*styles != EOS))
      styles += 2;

    if(*styles != EOS)
    {
      while(*styles <= x+length)
      {
        style[0] = *styles++ - x;
        style[1] = *styles++;
        DoIO((struct IORequest*)data->clipboard);
      }
      style[0] = length+1;
      style[1] = GetStyle(x+length-1, line);
      if(style[1] != 0)
      {
          unsigned short t_style = style[1];

        if(t_style & BOLD)
        {
          style[1] = ~BOLD;
          DoIO((struct IORequest*)data->clipboard);
        }
        if(t_style & ITALIC)
        {
          style[1] = ~ITALIC;
          DoIO((struct IORequest*)data->clipboard);
        }
        if(t_style & UNDERLINE)
        {
          style[1] = ~UNDERLINE;
          DoIO((struct IORequest*)data->clipboard);
        }
      }
    }
  }

  styleheader[1] = LONG2BE(data->clipboard->io_Offset - t_offset);
  data->clipboard->io_Offset    = t_offset - sizeof(styleheader);
  data->clipboard->io_Data    = (STRPTR)styleheader;
  data->clipboard->io_Length    = sizeof(styleheader);
  DoIO((struct IORequest*)data->clipboard);
  data->clipboard->io_Offset    += BE2LONG(styleheader[1]);

  textheader[1] = LONG2BE(length);
  data->clipboard->io_Data    = (STRPTR)textheader;
  data->clipboard->io_Length    = sizeof(textheader);
  DoIO((struct IORequest*)data->clipboard);
  data->clipboard->io_Data    = line->line.Contents+x;
  data->clipboard->io_Length    = length;
  DoIO((struct IORequest*)data->clipboard);

  data->clipboard->io_Offset += data->clipboard->io_Offset & 1;

  LEAVE();
}

void ClipLine (struct line_node *line, struct InstData *data)
{
  long  colorheader[] = { LONG2BE(MAKE_ID('C','O','L','S')), 0};
  long  styleheader[] = { LONG2BE(MAKE_ID('S','T','Y','L')), 0};
  long  textheader[]  = { LONG2BE(MAKE_ID('C','H','R','S')), 0};
  UWORD *styles = line->line.Styles;
  UWORD *colors = line->line.Colors;

  ENTER();

  ClipInfo(line, data);

  if(colors)
  {
    while(*colors != 0xffff)
    {
      colors += 2;
      colorheader[1] += 4;
    }
    colorheader[1] = LONG2BE(colorheader[1]);
    
    data->clipboard->io_Data    = (STRPTR)colorheader;
    data->clipboard->io_Length    = sizeof(colorheader);
    DoIO((struct IORequest*)data->clipboard);
    data->clipboard->io_Data    = (STRPTR)line->line.Colors;
    data->clipboard->io_Length    = BE2LONG(colorheader[1]);
    DoIO((struct IORequest*)data->clipboard);
  }

  if(styles)
  {
    while(*styles != EOS)
    {
      styles += 2;
      styleheader[1] += 4;
    }
  }
  styleheader[1] = LONG2BE(styleheader[1]);
  
  data->clipboard->io_Data    = (STRPTR)styleheader;
  data->clipboard->io_Length    = sizeof(styleheader);
  DoIO((struct IORequest*)data->clipboard);
  data->clipboard->io_Data    = (STRPTR)line->line.Styles;
  data->clipboard->io_Length    = BE2LONG(styleheader[1]);
  DoIO((struct IORequest*)data->clipboard);

  textheader[1] = line->line.Length;
  data->clipboard->io_Data    = (STRPTR)textheader;
  data->clipboard->io_Length    = sizeof(textheader);
  DoIO((struct IORequest*)data->clipboard);
  data->clipboard->io_Data    = (STRPTR)line->line.Contents;
  data->clipboard->io_Length    = BE2LONG(textheader[1]);
  DoIO((struct IORequest*)data->clipboard);

  data->clipboard->io_Offset += data->clipboard->io_Offset & 1;

  LEAVE();
}

LONG CutBlock (struct InstData *data, long Clipboard, long NoCut, BOOL update)
{
  struct  marking newblock;

  ENTER();

  NiceBlock(&data->blockinfo, &newblock);
  if(!NoCut)
    AddToUndoBuffer(deleteblock, (char *)&newblock, data);

  LEAVE();
  return(CutBlock2(data, Clipboard, NoCut, &newblock, update));
}

#ifdef ClassAct

struct CutArgs
{
  struct InstData *data;
  long Clipboard, NoCut;
  struct marking *newblock;
  BOOL update;

  WORD sigbit;
  struct Task *task;
  LONG res;
};

VOID CutBlockProcess (REG(a0) STRPTR arguments);

LONG CutBlock2 (struct InstData *data, long Clipboard, long NoCut, struct marking *newblock, BOOL update)
{
  struct CutArgs args = { data, Clipboard, NoCut, newblock, update, AllocSignal(-1), FindTask(NULL) };
  if(args.sigbit != -1)
  {
    UBYTE str_args[10];
    sprintf(str_args, "%lx", &args);

    ReleaseGIRPort(data->rport);
    ReleaseSemaphore(&data->semaphore);
    if(CreateNewProcTags(NP_Entry, CutBlockProcess, NP_Name, "Texteditor slave", NP_StackSize, 2*4096, NP_Arguments, str_args, TAG_DONE))
      Wait(1 << args.sigbit);
    FreeSignal(args.sigbit);
    ObtainSemaphore(&data->semaphore);
    data->rport = ObtainGIRPort(data->GInfo);
  }
  return args.res;
}

VOID CutBlockProcess (REG(a0) STRPTR arguments)
{
  struct CutArgs *args;
  if(sscanf(arguments, "%x", &args))
  {
    struct InstData *data = args->data;
    long Clipboard = args->Clipboard, NoCut = args->NoCut;
    struct marking *newblock = args->newblock;
    BOOL update = args->update;
#else
LONG CutBlock2 (struct InstData *data, long Clipboard, long NoCut, struct marking *newblock, BOOL update)
{
#endif
  LONG  tvisual_y;
  LONG  startx, stopx;
  LONG  res = 0;
  struct  line_node *startline, *stopline;

  ENTER();

#ifdef ClassAct
  ObtainSemaphore(&data->semaphore);
  data->rport = ObtainGIRPort(data->GInfo);
#endif

  startx    = newblock->startx;
  stopx     = newblock->stopx;
  startline = newblock->startline;
  stopline  = newblock->stopline;
  if(startline != stopline)
  {
      struct  line_node *c_startline = startline->next;
    data->update = FALSE;
    if(Clipboard)
    {
      if(InitClipboard(data))
      {
        data->clipboard->io_ClipID    = 0;
        data->clipboard->io_Command = CMD_WRITE;
        data->clipboard->io_Offset    = 12;
        ClipChars(startx, startline, startline->line.Length-startx, data);
      }
      else
      {
        Clipboard = FALSE;
      }
    }

    while(c_startline != stopline)
    {
      if(Clipboard)
      {
        ClipLine(c_startline, data);
      }

      if(!NoCut)
      {
          struct  line_node *cc_startline = c_startline;

        MyFreePooled(data->mypool, c_startline->line.Contents);
        if(c_startline->line.Styles)
          MyFreePooled(data->mypool, c_startline->line.Styles);
        data->totallines -= c_startline->visual;
        c_startline = c_startline->next;

        FreeLine(cc_startline, data);
      }
      else  c_startline = c_startline->next;
    }

    if(Clipboard)
    {
      if(stopx)
        ClipChars(0, stopline, stopx, data);

      EndClipSession(data);
    }

    if(!NoCut)
    {
      startline->next = stopline;
      stopline->previous = startline;

      RemoveChars(startx, startline, startline->line.Length-startx-1, data);
      if(stopx)
      {
        RemoveChars(0, stopline, stopx, data);
      }

      data->CPos_X = startx;
      data->actualline = startline;
      MergeLines(startline, data);
    }
  }
  else
  {
    if(Clipboard)
    {
      if(InitClipboard(data))
      {
        data->clipboard->io_ClipID    = 0;
        data->clipboard->io_Command = CMD_WRITE;
        data->clipboard->io_Offset    = 12;
        ClipChars(startx, startline, stopx-startx, data);
        EndClipSession(data);
      }
      if(update && NoCut)
      {
        MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
          goto end;
      }
    }

    if(!NoCut)
    {
      data->CPos_X = startx;
      RemoveChars(startx, startline, stopx-startx, data);
      if(update)
        goto end;
    }
  }

  tvisual_y = LineToVisual(startline, data)-1;
  if(tvisual_y < 0 || tvisual_y > data->maxlines)
  {
    ScrollIntoDisplay(data);
    tvisual_y = 0;
  }

  if(update)
  {
    data->update = TRUE;
    DumpText(data->visual_y+tvisual_y, tvisual_y, data->maxlines, TRUE, data);
  }
  res = tvisual_y;

end:
#ifdef ClassAct
    args->res = res;
    Forbid();
    ReleaseGIRPort(data->rport);
    ReleaseSemaphore(&data->semaphore);
    Signal(args->task, 1 << args->sigbit);
  }
#else

  RETURN(res);
  return res;
#endif
}
