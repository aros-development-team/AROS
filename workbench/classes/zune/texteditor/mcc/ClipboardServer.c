/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2014 TextEditor.mcc Open Source Team

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
#include <stdlib.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <dos/dostags.h>

// for iffparse.library no global variable definitions are needed
#define __NOLIBBASE__
#define __NOGLOBALIFACE__
#include <proto/iffparse.h>

#include "private.h"
#include "Debug.h"

static struct Library *IFFParseBase = NULL;
#if defined(__amigaos4__)
static struct IFFParseIFace *IIFFParse = NULL;
#endif
static struct SignalSemaphore *serverLock = NULL;
static struct Process *serverProcess = NULL;
static struct MsgPort *serverPort = NULL;
static struct MsgPort replyPort;
static struct Message msg;

struct ServerData
{
  ULONG sd_Command;
  IPTR sd_Session;
  ULONG sd_Mode;
  struct line_node *sd_Line;
  LONG sd_Start;
  LONG sd_Length;
  ULONG sd_Codeset;
  LONG sd_Error;
};

#define SERVER_SHUTDOWN       0xdeadf00d
#define SERVER_START_SESSION  0x00000001
#define SERVER_WRITE_CHARS    0x00000002
#define SERVER_WRITE_LINE     0x00000003
#define SERVER_READ_LINE      0x00000004
#define SERVER_END_SESSION    0x0000000f

#define ID_FORM    MAKE_ID('F','O','R','M')
#define ID_FTXT    MAKE_ID('F','T','X','T')
#define ID_CHRS    MAKE_ID('C','H','R','S')
#define ID_CSET    MAKE_ID('C','S','E','T')

/// ServerStartSession
static IPTR ServerStartSession(ULONG mode)
{
  IPTR result = (IPTR)NULL;
  struct IFFHandle *iff;

  ENTER();

  if((iff = AllocIFF()) != NULL)
  {
    if((iff->iff_Stream = (IPTR)OpenClipboard(0)) != 0)
    {
      InitIFFasClip(iff);

      if(OpenIFF(iff, mode) == 0)
      {
        if(mode == IFFF_WRITE)
        {
          PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN);
        }
        else if(mode == IFFF_READ)
        {
          StopChunk(iff, ID_FTXT, ID_CHRS);
          StopChunk(iff, ID_FTXT, ID_FLOW);
          StopChunk(iff, ID_FTXT, ID_HIGH);
          StopChunk(iff, ID_FTXT, ID_SBAR);
          StopChunk(iff, ID_FTXT, ID_COLS);
          StopChunk(iff, ID_FTXT, ID_STYL);
          StopChunk(iff, ID_FTXT, ID_CSET);
        }

        result = (IPTR)iff;
      }
      else
      {
        CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
        FreeIFF(iff);
      }
    }
    else
      FreeIFF(iff);
  }

  RETURN(result);
  return result;
}

///
/// ServerEndSession
static void ServerEndSession(IPTR session)
{
  struct IFFHandle *iff = (struct IFFHandle *)session;

  ENTER();

  if(iff != NULL)
  {
    CloseIFF(iff);

    if(iff->iff_Stream != 0)
      CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);

    FreeIFF(iff);
  }

  LEAVE();
}

///
/// ServerWriteInfo
static void ServerWriteInfo(IPTR session, struct line_node *line)
{
  struct IFFHandle *iff = (struct IFFHandle *)session;
  LONG error;

  ENTER();

  if(line->line.Flow != MUIV_TextEditor_Flow_Left || line->line.clearFlow == TRUE)
  {
    UWORD uwordBool = line->line.clearFlow;

    D(DBF_CLIPBOARD, "writing FLOW");
    error = PushChunk(iff, 0, ID_FLOW, IFFSIZE_UNKNOWN);
    SHOWVALUE(DBF_CLIPBOARD, error);
    error = WriteChunkBytes(iff, &line->line.Flow, 2);
    SHOWVALUE(DBF_CLIPBOARD, error);
    error = WriteChunkBytes(iff, &uwordBool, 2);
    SHOWVALUE(DBF_CLIPBOARD, error);
    error = PopChunk(iff);
    SHOWVALUE(DBF_CLIPBOARD, error);
  }

  if(line->line.Separator != LNSF_None)
  {
    D(DBF_CLIPBOARD, "writing SBAR");
    error = PushChunk(iff, 0, ID_SBAR, IFFSIZE_UNKNOWN);
    SHOWVALUE(DBF_CLIPBOARD, error);
    error = WriteChunkBytes(iff, &line->line.Separator, 2);
    SHOWVALUE(DBF_CLIPBOARD, error);
    error = PopChunk(iff);
    SHOWVALUE(DBF_CLIPBOARD, error);
  }

  if(line->line.Highlight == TRUE)
  {
    UWORD uwordBool = TRUE;

    D(DBF_CLIPBOARD, "writing HIGH");
    error = PushChunk(iff, 0, ID_HIGH, IFFSIZE_UNKNOWN);
    SHOWVALUE(DBF_CLIPBOARD, error);
    error = WriteChunkBytes(iff, &uwordBool, sizeof(uwordBool));
    SHOWVALUE(DBF_CLIPBOARD, error);
    error = PopChunk(iff);
    SHOWVALUE(DBF_CLIPBOARD, error);
  }

  LEAVE();
}

///
/// ServerWriteChars
static void ServerWriteChars(IPTR session, struct line_node *line, LONG start, LONG length)
{
  struct IFFHandle *iff = (struct IFFHandle *)session;
  LONG error;
  struct LineStyle style = {1, GetStyle(start-1, line)};
  struct LineColor color = {1, 0};
  struct LineColor *colors = line->line.Colors;

  ENTER();

  ServerWriteInfo(session, line);

  if(colors != NULL)
  {
    D(DBF_CLIPBOARD, "writing COLS");
    error = PushChunk(iff, 0, ID_COLS, IFFSIZE_UNKNOWN);
    SHOWVALUE(DBF_CLIPBOARD, error);

    while(colors->column <= start && colors->column != EOC)
    {
      color.color = colors->color;
      colors++;
    }

    if(color.color != 0 && colors->column-start != 1)
    {
      error = WriteChunkBytes(iff, &color, sizeof(color));
      SHOWVALUE(DBF_CLIPBOARD, error);
    }

    if(colors->column != EOC)
    {
      while(colors->column <= start+length)
      {
        color.column = colors->column - start;
        color.color = colors->color;
        colors++;

        error = WriteChunkBytes(iff, &color, sizeof(color));
        SHOWVALUE(DBF_CLIPBOARD, error);
      }
    }

    error = PopChunk(iff);
    SHOWVALUE(DBF_CLIPBOARD, error);
  }

  D(DBF_CLIPBOARD, "writing STYL");
  error = PushChunk(iff, 0, ID_STYL, IFFSIZE_UNKNOWN);
  SHOWVALUE(DBF_CLIPBOARD, error);

  if(style.style != 0)
  {
    UWORD t_style = style.style;

    if(isFlagSet(t_style, BOLD))
    {
      style.style = BOLD;
      error = WriteChunkBytes(iff, &style, sizeof(style));
      SHOWVALUE(DBF_CLIPBOARD, error);
    }
    if(isFlagSet(t_style, ITALIC))
    {
      style.style = ITALIC;
      error = WriteChunkBytes(iff, &style, sizeof(style));
      SHOWVALUE(DBF_CLIPBOARD, error);
    }
    if(isFlagSet(t_style, UNDERLINE))
    {
      style.style = UNDERLINE;
      error = WriteChunkBytes(iff, &style, sizeof(style));
      SHOWVALUE(DBF_CLIPBOARD, error);
    }
  }

  if(line->line.Styles != NULL)
  {
    struct LineStyle *styles = line->line.Styles;

    while(styles->column <= start && styles->column != EOS)
      styles++;

    if(styles->column != EOS)
    {
      while(styles->column <= start+length)
      {
        style.column = styles->column - start;
        style.style = styles->style;
        styles++;
        error = WriteChunkBytes(iff, &style, sizeof(style));
        SHOWVALUE(DBF_CLIPBOARD, error);
      }

      style.column = length+1;
      style.style = GetStyle(start+length-1, line);
      if(style.style != 0)
      {
        UWORD t_style = style.style;

        if(isFlagSet(t_style, BOLD))
        {
          style.style = ~BOLD;
          error = WriteChunkBytes(iff, &style, sizeof(style));
          SHOWVALUE(DBF_CLIPBOARD, error);
        }
        if(isFlagSet(t_style, ITALIC))
        {
          style.style = ~ITALIC;
          error = WriteChunkBytes(iff, &style, sizeof(style));
          SHOWVALUE(DBF_CLIPBOARD, error);
        }
        if(isFlagSet(t_style, UNDERLINE))
        {
          style.style = ~UNDERLINE;
          error = WriteChunkBytes(iff, &style, sizeof(style));
          SHOWVALUE(DBF_CLIPBOARD, error);
        }
      }
    }
  }

  error = PopChunk(iff);
  SHOWVALUE(DBF_CLIPBOARD, error);

  D(DBF_CLIPBOARD, "writing CHRS");
  error = PushChunk(iff, 0, ID_CHRS, IFFSIZE_UNKNOWN);
  SHOWVALUE(DBF_CLIPBOARD, error);
  error = WriteChunkBytes(iff, line->line.Contents + start, length);
  SHOWVALUE(DBF_CLIPBOARD, error);
  error = PopChunk(iff);
  SHOWVALUE(DBF_CLIPBOARD, error);

  LEAVE();
}

///
/// ServerWriteLine
static void ServerWriteLine(IPTR session, struct line_node *line)
{
  struct IFFHandle *iff = (struct IFFHandle *)session;
  LONG error;
  struct LineStyle *styles = line->line.Styles;
  struct LineColor *colors = line->line.Colors;

  ENTER();

  ServerWriteInfo(session, line);

  if(colors != NULL)
  {
    LONG numColors = 0;

    D(DBF_CLIPBOARD, "writing COLS");
    error = PushChunk(iff, 0, ID_COLS, IFFSIZE_UNKNOWN);
    SHOWVALUE(DBF_CLIPBOARD, error);

    while(colors->column != EOC)
    {
      colors++;
      numColors++;
    }

    SHOWVALUE(DBF_CLIPBOARD, numColors);
    error = WriteChunkBytes(iff, line->line.Colors, numColors * sizeof(*colors));
    SHOWVALUE(DBF_CLIPBOARD, error);
    error = PopChunk(iff);
    SHOWVALUE(DBF_CLIPBOARD, error);
  }

  if(styles != NULL)
  {
    LONG numStyles = 0;

    D(DBF_CLIPBOARD, "writing STYL");
    error = PushChunk(iff, 0, ID_STYL, IFFSIZE_UNKNOWN);
    SHOWVALUE(DBF_CLIPBOARD, error);

    while(styles->column != EOS)
    {
      styles++;
      numStyles++;
    }

    SHOWVALUE(DBF_CLIPBOARD, numStyles);
    error = WriteChunkBytes(iff, line->line.Styles, numStyles * sizeof(*styles));
    SHOWVALUE(DBF_CLIPBOARD, error);
    error = PopChunk(iff);
    SHOWVALUE(DBF_CLIPBOARD, error);
  }

  D(DBF_CLIPBOARD, "writing CHRS");
  error = PushChunk(iff, 0, ID_CHRS, IFFSIZE_UNKNOWN);
  SHOWVALUE(DBF_CLIPBOARD, error);
  error = WriteChunkBytes(iff, line->line.Contents, line->line.Length);
  SHOWVALUE(DBF_CLIPBOARD, error);
  error = PopChunk(iff);
  SHOWVALUE(DBF_CLIPBOARD, error);

  LEAVE();
}

///
/// ServerReadLine
static LONG ServerReadLine(IPTR session, struct line_node **linePtr, ULONG *csetPtr)
{
  struct IFFHandle *iff = (struct IFFHandle *)session;
  struct line_node *line = NULL;
  struct LineStyle *styles = NULL;
  struct LineColor *colors = NULL;
  STRPTR textline;
  LONG codeset = 0;
  UWORD flow = MUIV_TextEditor_Flow_Left;
  BOOL clearFlow = FALSE;
  BOOL highlight = FALSE;
  UWORD separator = LNSF_None;
  LONG error;
  BOOL lineFinished = FALSE;

  ENTER();

  D(DBF_CLIPBOARD, "server reading next line");
  do
  {
    struct ContextNode *cn;

    error = ParseIFF(iff, IFFPARSE_SCAN);
    SHOWVALUE(DBF_CLIPBOARD, error);
    if(error == IFFERR_EOC)
      continue;
    else if(error != 0)
      break;

    if((cn = CurrentChunk(iff)) != NULL)
    {
      switch (cn->cn_ID)
      {
        case ID_CSET:
        {
          D(DBF_CLIPBOARD, "reading CSET");
          SHOWVALUE(DBF_CLIPBOARD, cn->cn_Size);
          if(cn->cn_Size >= (LONG)sizeof(codeset))
          {
            /* Only the first four bytes are interesting */
            if(ReadChunkBytes(iff, &codeset, 4) != 4)
            {
              codeset = 0;
            }
            SHOWVALUE(DBF_CLIPBOARD, codeset);
          }
        }
        break;

        case ID_FLOW:
        {
          UWORD uwordBool;

          D(DBF_CLIPBOARD, "reading FLOW");
          SHOWVALUE(DBF_CLIPBOARD, cn->cn_Size);
          if(cn->cn_Size >= (LONG)(sizeof(flow)+sizeof(uwordBool)))
          {
            if((error = ReadChunkBytes(iff, &flow, sizeof(flow))) == 2)
            {
              if(flow > MUIV_TextEditor_Flow_Right)
                flow = MUIV_TextEditor_Flow_Left;
            }
            SHOWVALUE(DBF_CLIPBOARD, flow);
            SHOWVALUE(DBF_CLIPBOARD, error);
            error = ReadChunkBytes(iff, &uwordBool, 2);
            clearFlow = (uwordBool != 0) ? TRUE : FALSE;
            SHOWVALUE(DBF_CLIPBOARD, clearFlow);
            SHOWVALUE(DBF_CLIPBOARD, error);
          }
        }
        break;

        case ID_HIGH:
        {
          UWORD uwordBool;

          D(DBF_CLIPBOARD, "reading HIGH");
          SHOWVALUE(DBF_CLIPBOARD, cn->cn_Size);
          if(cn->cn_Size >= (LONG)sizeof(uwordBool))
          {
            error = ReadChunkBytes(iff, &uwordBool, 2);
            highlight = (uwordBool != 0) ? TRUE : FALSE;
            SHOWVALUE(DBF_CLIPBOARD, highlight);
            SHOWVALUE(DBF_CLIPBOARD, error);
          }
        }
        break;

        case ID_SBAR:
        {
          D(DBF_CLIPBOARD, "reading SBAR");
          SHOWVALUE(DBF_CLIPBOARD, cn->cn_Size);
          if (cn->cn_Size >= (LONG)sizeof(separator))
          {
            error = ReadChunkBytes(iff, &separator, 2);
            SHOWVALUE(DBF_CLIPBOARD, separator);
            SHOWVALUE(DBF_CLIPBOARD, error);
          }
        }
        break;

        case ID_COLS:
        {
          ULONG numColors = cn->cn_Size / sizeof(struct LineColor);

          D(DBF_CLIPBOARD, "reading COLS");
          SHOWVALUE(DBF_CLIPBOARD, cn->cn_Size);
          if(colors != NULL)
          {
            // forget any duplicate colors
            FreeVec(colors);
            colors = NULL;
          }
          // allocate one word more than the chunk tell us, because we terminate the array with EOC
          if(numColors > 0 && (colors = AllocVecShared((numColors+1) * sizeof(*colors), MEMF_ANY)) != NULL)
          {
            error = ReadChunkBytes(iff, colors, numColors * sizeof(struct LineColor));
            SHOWVALUE(DBF_CLIPBOARD, error);
            colors[numColors].column = EOC;
          }
        }
        break;

        case ID_STYL:
        {
          ULONG numStyles = cn->cn_Size / sizeof(struct LineStyle);

          D(DBF_CLIPBOARD, "reading STYL");
          SHOWVALUE(DBF_CLIPBOARD, cn->cn_Size);
          if(styles != NULL && styles != (APTR)-1)
          {
            // forget any duplicate styles
            FreeVec(styles);
            styles = NULL;
          }
          // allocate one word more than the chunk tell us, because we terminate the array with EOS
          if(numStyles > 0 && (styles = AllocVecShared((numStyles+1) * sizeof(struct LineStyle), MEMF_ANY)) != NULL)
          {
            error = ReadChunkBytes(iff, styles, numStyles * sizeof(struct LineStyle));
            SHOWVALUE(DBF_CLIPBOARD, error);
            styles[numStyles].column = EOS;
          }
        }
        break;

        case ID_CHRS:
        {
          ULONG length = cn->cn_Size;

          D(DBF_CLIPBOARD, "reading CHRS");
          SHOWVALUE(DBF_CLIPBOARD, cn->cn_Size);

          if(length > 0)
          {
            // allocate 2 additional bytes:
            // - one for the trailing LF
            // - another one for the terminating NUL
            if((textline = AllocVecShared(length + 2, MEMF_ANY)) != NULL)
            {
              error = ReadChunkBytes(iff, textline, length);
              SHOWVALUE(DBF_CLIPBOARD, error);
              textline[length] = '\0';
            }

            if(textline != NULL && (line = AllocVecShared(sizeof(*line), MEMF_ANY|MEMF_CLEAR)) != NULL)
            {
              line->line.Contents = textline;
              line->line.Length = length;
              line->line.allocatedContents = length+2;
              line->line.Highlight = highlight;
              line->line.Flow = flow;
              line->line.clearFlow = clearFlow;
              line->line.Separator = separator;
              line->line.Styles = styles;
              line->line.Colors = colors;

              lineFinished = TRUE;
              error = 0;
            }
            else
            {
              if(textline != NULL)
                FreeVec(textline);
              if(styles != NULL)
                FreeVec(styles);
              if(colors != NULL)
                FreeVec(colors);
            }

            styles = NULL;
            colors = NULL;
            flow = MUIV_TextEditor_Flow_Left;
            clearFlow = FALSE;
            highlight = FALSE;
            separator = LNSF_None;
          }
        }
        break;
      }
    }
  }
  while(lineFinished == FALSE);

  // in case we didn't read a complete line but at least some other of our
  // chunks we free them here to avoid leaks.
  if(line == NULL)
  {
    if(styles != NULL)
      FreeVec(styles);
    if(colors != NULL)
      FreeVec(colors);
  }

  *linePtr = line;
  *csetPtr = codeset;

  RETURN(error);
  return error;
}

///
/// ClientStartSession
// copy a line to the clipboard, public callable function
IPTR ClientStartSession(ULONG mode)
{
  IPTR session = (IPTR)NULL;

  // lock out other tasks
  if(AttemptSemaphore(serverLock))
  {
    struct ServerData sd;

    // set up the data packet
    sd.sd_Command = SERVER_START_SESSION;
    sd.sd_Mode = mode;

    // set up the message, send it and wait for a reply
    msg.mn_Node.ln_Name = (STRPTR)&sd;
    replyPort.mp_SigTask = FindTask(NULL);

    PutMsg(serverPort, &msg);
    Remove((struct Node *)WaitPort(&replyPort));

    session = sd.sd_Session;

    // allow other tasks again
    ReleaseSemaphore(serverLock);
  }
  else
    DisplayBeep(0);

  return session;
}

///
/// ClientEndSession
// copy a line to the clipboard, public callable function
void ClientEndSession(IPTR session)
{
  // lock out other tasks
  if(AttemptSemaphore(serverLock))
  {
    struct ServerData sd;

    // set up the data packet
    sd.sd_Command = SERVER_END_SESSION;
    sd.sd_Session = session;

    // set up the message, send it and wait for a reply
    msg.mn_Node.ln_Name = (STRPTR)&sd;
    replyPort.mp_SigTask = FindTask(NULL);

    PutMsg(serverPort, &msg);
    Remove((struct Node *)WaitPort(&replyPort));

    // allow other tasks again
    ReleaseSemaphore(serverLock);
  }
  else
    DisplayBeep(0);
}

///
/// ClientWriteChars
void ClientWriteChars(IPTR session, struct line_node *line, LONG start, LONG length)
{
  // lock out other tasks
  if(AttemptSemaphore(serverLock))
  {
    struct ServerData sd;

    // set up the data packet
    sd.sd_Command = SERVER_WRITE_CHARS;
    sd.sd_Session = session;
    sd.sd_Line = line;
    sd.sd_Start = start;
    sd.sd_Length = length;

    // set up the message, send it and wait for a reply
    msg.mn_Node.ln_Name = (STRPTR)&sd;
    replyPort.mp_SigTask = FindTask(NULL);

    PutMsg(serverPort, &msg);
    Remove((struct Node *)WaitPort(&replyPort));

    // allow other tasks again
    ReleaseSemaphore(serverLock);
  }
  else
    DisplayBeep(0);
}

///
/// ClientWriteLine
void ClientWriteLine(IPTR session, struct line_node *line)
{
  // lock out other tasks
  if(AttemptSemaphore(serverLock))
  {
    struct ServerData sd;

    // set up the data packet
    sd.sd_Command = SERVER_WRITE_LINE;
    sd.sd_Session = session;
    sd.sd_Line = line;

    // set up the message, send it and wait for a reply
    msg.mn_Node.ln_Name = (STRPTR)&sd;
    replyPort.mp_SigTask = FindTask(NULL);

    PutMsg(serverPort, &msg);
    Remove((struct Node *)WaitPort(&replyPort));

    // allow other tasks again
    ReleaseSemaphore(serverLock);
  }
  else
    DisplayBeep(0);
}

///
/// ClientReadLine
LONG ClientReadLine(IPTR session, struct line_node **line, ULONG *cset)
{
  LONG error = IFFERR_NOTIFF;

  // lock out other tasks
  if(AttemptSemaphore(serverLock))
  {
    struct ServerData sd;

    // set up the data packet
    sd.sd_Command = SERVER_READ_LINE;
    sd.sd_Session = session;

    // set up the message, send it and wait for a reply
    msg.mn_Node.ln_Name = (STRPTR)&sd;
    replyPort.mp_SigTask = FindTask(NULL);

    PutMsg(serverPort, &msg);
    Remove((struct Node *)WaitPort(&replyPort));

    *line = sd.sd_Line;
    *cset = sd.sd_Codeset;
    error = sd.sd_Error;

    // allow other tasks again
    ReleaseSemaphore(serverLock);
  }
  else
    DisplayBeep(0);

  return error;
}

///
/// ClipboardServer
// the clipboard server process
#if defined(__amigaos4__)
static LONG ClipboardServer(UNUSED STRPTR args, UNUSED LONG length, struct ExecBase *SysBase)
#else
static SAVEDS ASM LONG ClipboardServer(UNUSED REG(a0, STRPTR args), UNUSED REG(d0, LONG length))
#endif
{
  struct Process *me;
  struct Message *msg;
  #if defined(__amigaos4__)
  struct ExecIFace *IExec = (struct ExecIFace *)SysBase->MainInterface;
  #endif

  me = (struct Process *)FindTask(NULL);
  WaitPort(&me->pr_MsgPort);
  msg = GetMsg(&me->pr_MsgPort);

  if((IFFParseBase = OpenLibrary("iffparse.library", 36)) != NULL)
  {
    #if defined(__amigaos4__)

    if((IIFFParse = (struct IFFParseIFace *)GetInterface(IFFParseBase, "main", 1, NULL)) != NULL)
    {
    #endif
    struct MsgPort *mp;

    #if defined(__amigaos4__)
    mp = AllocSysObjectTags(ASOT_PORT, TAG_DONE);
    #else
    mp = CreateMsgPort();
    #endif
    if(mp != NULL)
    {
      BOOL running = TRUE;

      // return something as a valid reply
      msg->mn_Node.ln_Name = (STRPTR)mp;
      ReplyMsg(msg);

      do
      {
        WaitPort(mp);

        while((msg = GetMsg(mp)) != NULL)
        {
          struct ServerData *sd = (struct ServerData *)msg->mn_Node.ln_Name;

          switch(sd->sd_Command)
          {
            case SERVER_SHUTDOWN:
            {
              running = FALSE;
            }
            break;

            case SERVER_START_SESSION:
            {
              sd->sd_Session = ServerStartSession(sd->sd_Mode);
            }
            break;

            case SERVER_END_SESSION:
            {
              ServerEndSession(sd->sd_Session);
            }
            break;

            case SERVER_WRITE_CHARS:
            {
              ServerWriteChars(sd->sd_Session, sd->sd_Line, sd->sd_Start, sd->sd_Length);
            }
            break;

            case SERVER_WRITE_LINE:
            {
              ServerWriteLine(sd->sd_Session, sd->sd_Line);
            }
            break;

            case SERVER_READ_LINE:
            {
              sd->sd_Error = ServerReadLine(sd->sd_Session, &sd->sd_Line, &sd->sd_Codeset);
            }
            break;
          }

          ReplyMsg(msg);
        }
      }
      while(running == TRUE);

      #if defined(__amigaos4__)
      FreeSysObject(ASOT_PORT, mp);
      #else
      DeleteMsgPort(mp);
      #endif
    }

    #if defined(__amigaos4__)
    DropInterface((struct Interface *)IIFFParse);
    }
    #endif

    CloseLibrary(IFFParseBase);
  }

  Forbid();

  return 0;
}

///
/// StartClipboardServer
// launch the clipboard server process
// we must use a separate process, because accessing the clipboard via iffparse.library
// allocates 2 signals for every instance of this class. Hence we will run out of signals
// sooner or later. The separate process avoids this situation.
BOOL StartClipboardServer(void)
{
  BOOL success = FALSE;

  ENTER();

  // create a semaphore to protect several concurrent tasks
  #if defined(__amigaos4__)
  serverLock = AllocSysObjectTags(ASOT_SEMAPHORE, TAG_DONE);
  #else
  serverLock = AllocVec(sizeof(*serverLock), MEMF_CLEAR);
  #endif
  if(serverLock != NULL)
  {
    #if defined(__amigaos4__)
    uint32 oldStackSize;
    #else
    InitSemaphore(serverLock);
    #endif

    #if defined(__amigaos4__)
    // set a minimum stack size of 8K, no matter what the user has set
    DosControlTags(DC_MinProcStackR, &oldStackSize,
                   DC_MinProcStackW, 8192,
                   TAG_DONE);
    #endif

    // create the server process
    // this must *NOT* be a child process
    serverProcess = CreateNewProcTags(NP_Entry, ClipboardServer,
                                      NP_Name, "TextEditor.mcc clipboard server",
                                      NP_Priority, 1,
                                      NP_StackSize, 8192,
                                      NP_WindowPtr, ~0L,
                                      #if defined(__amigaos4__)
                                      NP_Child, FALSE,
                                      #elif defined(__MORPHOS__)
                                      NP_CodeType, CODETYPE_PPC,
                                      #endif
                                      TAG_DONE);
    if(serverProcess !=  NULL)
    {
      // we use one global reply port with a static signal bit
      replyPort.mp_Node.ln_Type = NT_MSGPORT;
      NewList(&replyPort.mp_MsgList);
      replyPort.mp_SigBit = SIGB_SINGLE;
      replyPort.mp_SigTask = FindTask(NULL);

      msg.mn_ReplyPort = &replyPort;
      msg.mn_Node.ln_Name = (STRPTR)NULL;

      // send out the startup message and wait for a reply
      PutMsg(&serverProcess->pr_MsgPort, &msg);
      Remove((struct Node *)WaitPort(&replyPort));

      // check whether everything went ok
      if((serverPort = (struct MsgPort *)msg.mn_Node.ln_Name) != NULL)
      {
        success = TRUE;
      }
    }

    #if defined(__amigaos4__)
    // restore the old minimum stack size
    DosControlTags(DC_MinProcStackW, oldStackSize,
                   TAG_DONE);
    #endif
  }

  RETURN(success);
  return success;
}

///
/// ShutdownClipboardServer
// shutdown the server process and clean up
void ShutdownClipboardServer(void)
{
  if(serverPort != NULL)
  {
    struct ServerData sd;

    sd.sd_Command = SERVER_SHUTDOWN;

    msg.mn_Node.ln_Name = (STRPTR)&sd;
    replyPort.mp_SigTask = FindTask(NULL);

    PutMsg(serverPort, &msg);
    WaitPort(&replyPort);

    serverPort = NULL;
  }

  if(serverLock != NULL)
  {
    #if defined(__amigaos4__)
    FreeSysObject(ASOT_SEMAPHORE, serverLock);
    #else
    FreeVec(serverLock);
    #endif

    serverLock = NULL;
  }
}

///
