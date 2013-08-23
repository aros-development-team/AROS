/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2013 by BetterString.mcc Open Source Team

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

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/locale.h>

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
  STRPTR sd_String;
  LONG sd_Length;
};

#define SERVER_SHUTDOWN   0xdeadf00d
#define SERVER_WRITE      0x00000001
#define SERVER_READ       0x00000002

#define ID_FORM    MAKE_ID('F','O','R','M')
#define ID_FTXT    MAKE_ID('F','T','X','T')
#define ID_CHRS    MAKE_ID('C','H','R','S')
#define ID_CSET    MAKE_ID('C','S','E','T')

#ifndef MEMF_SHARED
#define MEMF_SHARED       MEMF_ANY
#endif

/// StringToClipboard
// copy a string to the clipboard, public callable function
void StringToClipboard(STRPTR str, LONG length)
{
  ENTER();

  // lock out other tasks
  if(AttemptSemaphore(serverLock))
  {
    struct ServerData sd;

    // set up the data packet
    sd.sd_Command = SERVER_WRITE;
    sd.sd_String = str;
    sd.sd_Length = (length > 0) ? length : (LONG)strlen(str);

    if(sd.sd_Length > 0)
    {
      // set up the message, send it and wait for a reply
      msg.mn_Node.ln_Name = (STRPTR)&sd;
      replyPort.mp_SigTask = FindTask(NULL);

      PutMsg(serverPort, &msg);
      Remove((struct Node *)WaitPort(&replyPort));
    }
    else
      DisplayBeep(0);

    // allow other tasks again
    ReleaseSemaphore(serverLock);
  }
  else
    DisplayBeep(0);

  LEAVE();
}

///
/// ClipboardToString
// copy from the clipboard to a string, public callable function
// the string must be FreeVec()ed externally
BOOL ClipboardToString(STRPTR *str, LONG *length)
{
  BOOL success = FALSE;

  ENTER();

  // lock out other tasks
  if(AttemptSemaphore(serverLock))
  {
    struct ServerData sd;

    // set up the data packet
    sd.sd_Command = SERVER_READ;

    // set up the message, send it and wait for a reply
    msg.mn_Node.ln_Name = (STRPTR)&sd;
    replyPort.mp_SigTask = FindTask(NULL);

    PutMsg(serverPort, &msg);
    Remove((struct Node *)WaitPort(&replyPort));

    if(sd.sd_String != NULL)
    {
      // retrieve the values from the server
      *str = sd.sd_String;
      *length = sd.sd_Length;

      success = TRUE;
    }

    // allow other tasks again
    ReleaseSemaphore(serverLock);
  }
  else
    DisplayBeep(0);

  RETURN(success);
  return success;
}

///
/// WriteToClipboard
// write a given string via iffparse.library to the clipboard
// non-public server side function
static void WriteToClipboard(STRPTR str, LONG length)
{
  struct IFFHandle *iff;

  if((iff = AllocIFF()) != NULL)
  {
    if((iff->iff_Stream = (IPTR)OpenClipboard(0)) != 0)
    {
      InitIFFasClip(iff);

      if(OpenIFF(iff, IFFF_WRITE) == 0)
      {
        PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN);
        PushChunk(iff, 0, ID_CHRS, IFFSIZE_UNKNOWN);
        WriteChunkBytes(iff, str, length);
        PopChunk(iff);
        PopChunk(iff);

        CloseIFF(iff);
      }

      CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    }

    FreeIFF(iff);
  }
}

///
/// utf8_to_ansi
// convert an UTF-8 string to ANSI
#if defined(__MORPHOS__)
static void utf8_to_ansi(CONST_STRPTR src, STRPTR dst)
{
  static struct KeyMap *keymap;
  ULONG octets;

  keymap = AskKeyMapDefault();

  do
  {
     WCHAR wc;
     UBYTE c;

     octets = UTF8_Decode(src, &wc);
     c = ToANSI(wc, keymap);

     *dst++ = c;
     src += octets;
  }
  while (octets > 0);
}
#endif

///
/// ReadFromClipboard
// read a string via iffparse.library from the clipboard
// non-public server side function
static void ReadFromClipboard(STRPTR *str, LONG *length)
{
  struct IFFHandle *iff;

  *str = NULL;
  *length = 0;

  if((iff = AllocIFF()) != NULL)
  {
    if((iff->iff_Stream = (IPTR)OpenClipboard(0)) != 0)
    {
      InitIFFasClip(iff);

      if(OpenIFF(iff, IFFF_READ) == 0)
      {
        if(StopChunk(iff, ID_FTXT, ID_CHRS) == 0 && StopChunk(iff, ID_FTXT, ID_CSET) == 0)
        {
          LONG codeset = 0;

          while(TRUE)
          {
            LONG error;
            struct ContextNode *cn;

            error = ParseIFF(iff, IFFPARSE_SCAN);
            if(error == IFFERR_EOC)
              continue;
            else if(error != 0)
              break;

            if((cn = CurrentChunk(iff)) != NULL)
            {
              if(cn->cn_ID == ID_CSET)
              {
                if (cn->cn_Size >= 4)
                {
                  // Only the first four bytes are interesting
                  if(ReadChunkBytes(iff, &codeset, 4) != 4)
                    codeset = 0;
                }
              }
              else if(cn->cn_ID == ID_CHRS && cn->cn_Size > 0)
              {
                ULONG size = cn->cn_Size;
                STRPTR buffer;

                if((buffer = SharedPoolAlloc(size + 1)) != NULL)
                {
                  LONG readBytes;

                  // read the string from the clipboard
                  if((readBytes = ReadChunkBytes(iff, buffer, size)) > 0)
                  {
                    memset(buffer + readBytes, 0, size-readBytes+1);

                    #if defined(__MORPHOS__)
                    if (codeset == CODESET_UTF8)
                    {
                      if (IS_MORPHOS2)
                      {
                        utf8_to_ansi(buffer, buffer);
                        readBytes = strlen(buffer);
                      }
                    }
                    #endif

                    *str = buffer;
                    *length = readBytes;
                  }
                  else
                    E(DBF_ALWAYS, "ReadChunkBytes error! (%ld)", readBytes);
                }
              }
            }
          }
        }

        CloseIFF(iff);
      }

      CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    }

    FreeIFF(iff);
  }
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

  ENTER();

  D(DBF_CLIPBOARD, "clipboard server starting up");

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

      D(DBF_CLIPBOARD, "clipboard server main loop");
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

            case SERVER_WRITE:
            {
              WriteToClipboard(sd->sd_String, sd->sd_Length);
            }
            break;

            case SERVER_READ:
            {
              ReadFromClipboard(&sd->sd_String, &sd->sd_Length);
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

  D(DBF_CLIPBOARD, "clipboard server shutting down");

  Forbid();

  LEAVE();
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
                                      NP_Name, "BetterString.mcc clipboard server",
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

