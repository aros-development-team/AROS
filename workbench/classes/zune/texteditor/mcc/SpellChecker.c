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

#include <stdio.h>
#include <string.h>

#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/var.h>
#include <rexx/storage.h>
#include <workbench/workbench.h>
#include <clib/alib_protos.h>
#include <proto/rexxsyslib.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/locale.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/wb.h>

#include "private.h"
#include "Debug.h"

#if !defined(__amigaos4__) || (INCLUDE_VERSION < 50)
struct PathNode
{
  BPTR pn_Next;
  BPTR pn_Lock;
};
#endif

/// SelectCode()
HOOKPROTONH(SelectCode, void, void *lvobj, long **parms)
{
  struct InstData *data = (struct InstData *)*parms;
  struct marking block;
  char *entry;

  ENTER();

  DoMethod(lvobj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &entry);

  if(entry != NULL)
  {
    int length = strlen(entry);
    LONG oldpos;

    if(Enabled(data))
      Key_Clear(data);

    block.startline = data->actualline;
    block.startx    = data->CPos_X;
    block.stopline    = data->actualline;
    block.stopx     = data->CPos_X+length;
    AddToUndoBuffer(data, ET_PASTEBLOCK, &block);
    oldpos = data->CPos_X;
    data->CPos_X += length;
    PasteChars(data, oldpos, data->actualline, length, entry, NULL);
  }
  set(data->SuggestWindow, MUIA_Window_Open, FALSE);
  DoMethod(lvobj, MUIM_List_Clear);
  set(data->object, MUIA_TextEditor_AreaMarked, FALSE);

  LEAVE();
}
MakeStaticHook(SelectHook, SelectCode);

///
/// SafePutMsg()
static BOOL SafePutMsg(CONST_STRPTR portName, struct Message *msg)
{
  struct MsgPort *port;
  BOOL success = FALSE;

  ENTER();

  D(DBF_SPELL, "put message 0x%08lx to port '%s'", msg, portName);

  // forbid task switching before we look up the port
  Forbid();

  if((port = FindPort(portName)) != NULL)
  {
    // send off the message while the Forbid() is still active
    PutMsg(port, msg);
    success = TRUE;
  }
  else
    W(DBF_SPELL, "cannot find port '%s'", portName);

  // permit task switching again
  Permit();

  RETURN(success);
  return success;
}

///
/// SendRexx()
static BOOL SendRexx(CONST_STRPTR word, CONST_STRPTR command)
{
  struct MsgPort *clipport;
  BOOL result = FALSE;

  ENTER();

  #if defined(__amigaos4__)
  clipport = AllocSysObjectTags(ASOT_PORT, TAG_DONE);
  #else
  clipport = CreateMsgPort();
  #endif
  if(clipport != NULL)
  {
    struct RexxMsg *rxmsg;

    if((rxmsg = CreateRexxMsg(clipport, NULL, NULL)) != NULL)
    {
      char buffer[512];

      snprintf(buffer, sizeof(buffer), command, word);
      SHOWSTRING(DBF_SPELL, buffer);

      rxmsg->rm_Action = RXCOMM;
       #ifdef __AROS__
      if((rxmsg->rm_Args[0] = (IPTR)CreateArgstring(buffer, strlen(buffer))) != 0)
      #else
      if((rxmsg->rm_Args[0] = CreateArgstring(buffer, strlen(buffer))) != 0)
      #endif
      {
        if(SafePutMsg("REXX", (struct Message *)rxmsg) == TRUE)
        {
          if(Wait((1 << clipport->mp_SigBit) | SIGBREAKF_CTRL_C) != SIGBREAKF_CTRL_C)
          {
            GetMsg(clipport);

            D(DBF_SPELL, "ARexx result1 %ld", rxmsg->rm_Result1);
            if(rxmsg->rm_Result1 == 0)
            {
              result = TRUE;
              DeleteArgstring((APTR)rxmsg->rm_Result2);
            }
          }
        }

        DeleteArgstring((APTR)rxmsg->rm_Args[0]);
      }

      DeleteRexxMsg(rxmsg);
    }

    #if defined(__amigaos4__)
    FreeSysObject(ASOT_PORT, clipport);
    #else
    DeleteMsgPort(clipport);
    #endif
  }

  RETURN(result);
  return result;
}

///

#if !defined(__amigaos4__) && !defined(__MORPHOS__) && !defined(__AROS__)
#undef WorkbenchControl
/// WorkbenchControl()
BOOL WorkbenchControl(STRPTR name, ...)
{
  BOOL ret;
  va_list args;

  ENTER();

  va_start(args, name);
  ret = WorkbenchControlA(name, (struct TagItem *)args);
  va_end(args);

  RETURN(ret);
  return ret;
}

///
#endif

/// CloneSearchPath()
/***********************************************************************
 This returns a duplicated search path (preferable the workbench
 searchpath) usable for NP_Path of SystemTagList().
************************************************************************/
static BPTR CloneSearchPath(void)
{
  BPTR path = 0;

  ENTER();

  if(WorkbenchBase != NULL && WorkbenchBase->lib_Version >= 44)
    WorkbenchControl(NULL, WBCTRLA_DuplicateSearchPath, &path, TAG_DONE);

  // We don't like this evil code in OS4 compile, as we should have
  // a recent enough Workbench available
  #if !defined(__amigaos4__)
  if(path == 0)
  {
    struct Process *pr = (struct Process*)FindTask(NULL);

    if(pr->pr_Task.tc_Node.ln_Type == NT_PROCESS)
    {
      struct CommandLineInterface *cli = BADDR(pr->pr_CLI);

      if(cli != 0)
      {
        BPTR *p = &path;
        BPTR dir = cli->cli_CommandDir;

        while(dir != 0)
        {
          BPTR dir2;
          struct FileLock *lock = BADDR(dir);
          struct PathNode *node;

          dir = lock->fl_Link;
          dir2 = DupLock((BPTR)lock->fl_Key);
          if(dir2 == 0)
            break;

          /* Use AllocVec(), because this memory is freed by FreeVec()
           * by the system later */
          if((node = AllocVec(sizeof(struct PathNode), MEMF_ANY)) == NULL)
          {
            UnLock(dir2);
            break;
          }
          node->pn_Next = 0;
          node->pn_Lock = dir2;
          *p = MKBADDR(node);
          p = &node->pn_Next;
        }
      }
    }
  }
  #endif

  RETURN(path);
  return path;
}

///
/// FreeSearchPath()
/***********************************************************************
 Free the memory returned by CloneSearchPath
************************************************************************/
static void FreeSearchPath(BPTR path)
{
  ENTER();

  if(path != 0)
  {
    #if !defined(__MORPHOS__)
    if(WorkbenchBase != NULL)
    {
      WorkbenchControl(NULL, WBCTRLA_FreeSearchPath, path, TAG_DONE);
    }
    else
    #endif
    {
      #if !defined(__amigaos4__)
      /* This is compatible with WorkbenchControl(NULL, WBCTRLA_FreeSearchPath, ...)
       * in Ambient */
      while(path != 0)
      {
        struct PathNode *node = BADDR(path);

        path = node->pn_Next;
        UnLock(node->pn_Lock);
        FreeVec(node);
      }
      #endif
    }
  }

  LEAVE();
}

///
/// SendCLI()
static BOOL SendCLI(CONST_STRPTR word, CONST_STRPTR command)
{
  char buffer[512];
  BOOL result;
  BPTR path;

  ENTER();

  snprintf(buffer, sizeof(buffer), command, word);

  // path maybe 0, which is allowed
  path = CloneSearchPath();

  if(SystemTags(buffer, NP_Path, path, TAG_DONE) == -1)
  {
    W(DBF_SPELL, "command '%s' failed, error code %ld", buffer, IoErr());
    FreeSearchPath(path);
    result = FALSE;
  }
  else
  {
    result = TRUE;
  }

  RETURN(result);
  return result;
}

///
/// SuggestWindow()
Object *SuggestWindow(struct InstData *data)
{
  Object *window;
  Object *lvobj;

  window = WindowObject,
        MUIA_Window_Borderless,     TRUE,
        MUIA_Window_CloseGadget,    FALSE,
        MUIA_Window_DepthGadget,    FALSE,
        MUIA_Window_DragBar,        FALSE,
        MUIA_Window_SizeGadget,     FALSE,
        WindowContents, VGroup,
          MUIA_InnerBottom,       0,
          MUIA_InnerLeft,         0,
          MUIA_InnerRight,        0,
          MUIA_InnerTop,          0,
          MUIA_Group_PageMode,      TRUE,
          Child, ListviewObject,
            MUIA_Listview_Input, FALSE,
            MUIA_Listview_List, FloattextObject,
              MUIA_Floattext_Text, "Word is spelled correctly.",
              MUIA_Frame, MUIV_Frame_ReadList,
              MUIA_Background, MUII_ListBack,
              End,
            End,
          Child, lvobj = ListviewObject,
            MUIA_Listview_List, ListObject,
              MUIA_Frame, MUIV_Frame_InputList,
              MUIA_Background, MUII_ListBack,
              MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
              MUIA_List_DestructHook, MUIV_List_DestructHook_String,
              MUIA_List_Pool, data->mypool,
              End,
            End,
          End,
        End;

  DoMethod(lvobj, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
      MUIV_Notify_Self, 3, MUIM_CallHook, &SelectHook);

  DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
      MUIV_Notify_Self, 3, MUIM_Set, MUIA_Window_Open, FALSE);

  DoMethod(window, MUIM_Notify, MUIA_Window_Open, MUIV_EveryTime,
      data->object, 3, MUIM_Set, MUIA_TextEditor_PopWindow_Open, MUIV_TriggerValue);

  data->SuggestListview = lvobj;

  RETURN(window);
  return window;
}

///
/// LookupWord()
static BOOL LookupWord(struct InstData *data, CONST_STRPTR word)
{
  BOOL res;

  ENTER();

  SHOWSTRING(DBF_SPELL, data->LookupCmd);
  SHOWSTRING(DBF_SPELL, word);
  SHOWVALUE(DBF_SPELL, data->LookupSpawn);

  if(data->LookupSpawn == FALSE)
    res = SendCLI(word, data->LookupCmd);
  else
    res = SendRexx(word, data->LookupCmd);

  if(res == TRUE)
  {
    char buf[4];

    buf[0] = '\0';
    if(GetVar("Found", &buf[0], sizeof(buf), GVF_GLOBAL_ONLY) != -1)
    {
      if(buf[0] == '0')
        res = FALSE;
    }
    else
    {
      D(DBF_SPELL, "cannot read ENV variable 'Found', error code %ld", IoErr());
      // don't treat a missing "Found" variable as a failure, at least this
      // is what previous releases did.
    }
  }
  else
  {
    D(DBF_SPELL, "lookup of word '%s' failed", word);
  }

  RETURN(res);
  return res;
}

///
/// SuggestWord()
void SuggestWord(struct InstData *data)
{
  LONG top;
  LONG left;
  LONG line_nr;
  struct pos_info pos;
  struct line_node *line = data->actualline;

  ENTER();

  if(IsAlpha(data->mylocale, line->line.Contents[data->CPos_X]))
  {
    if(Enabled(data))
    {
      data->blockinfo.enabled = FALSE;
      MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
    }
/*    else
    {
      SetCursor(data, data->CPos_X, line, FALSE);
    }
*/

    while(data->CPos_X != 0 && (IsAlpha(data->mylocale, line->line.Contents[data->CPos_X-1]) || line->line.Contents[data->CPos_X-1] == '-' || line->line.Contents[data->CPos_X-1] == '\''))
    {
      GoPreviousWord(data);
    }

    line = data->actualline;
    data->blockinfo.startx = data->CPos_X;
    data->blockinfo.startline = line;

    line_nr = LineToVisual(data, line) - 1;
    OffsetToLines(data, data->CPos_X, line, &pos);
    left = xget(_win(data->object), MUIA_Window_LeftEdge);
    top = xget(_win(data->object), MUIA_Window_TopEdge);
    left  += _mleft(data->object) + FlowSpace(data, line->line.Flow, line->line.Contents+(data->CPos_X-pos.x)) + TextLength(&data->tmprp, line->line.Contents+(data->CPos_X-pos.x), pos.x);
    top += data->ypos + (data->fontheight * (line_nr + pos.lines));

    while(data->CPos_X < line->line.Length && (IsAlpha(data->mylocale, line->line.Contents[data->CPos_X]) || line->line.Contents[data->CPos_X] == '-' || line->line.Contents[data->CPos_X] == '\''))
    {
      data->CPos_X++;
    }
    data->blockinfo.stopx = data->CPos_X;
    data->blockinfo.stopline = line;

    data->blockinfo.enabled = TRUE;
    MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);

    SetAttrs(data->SuggestWindow, MUIA_Window_Open,     FALSE,
                                  MUIA_Window_LeftEdge, left,
                                  MUIA_Window_TopEdge,  top,
                                  TAG_DONE);

    if(data->blockinfo.stopx-data->blockinfo.startx < 256)
    {
      char word[256];

      strlcpy(word, line->line.Contents+data->blockinfo.startx, data->blockinfo.stopx-data->blockinfo.startx+1);

      set(_win(data->object), MUIA_Window_Sleep, TRUE);

      if(isFlagSet(data->flags, FLG_CheckWords) && LookupWord(data, word) == TRUE)
      {
        Object *group = (Object *)xget(data->SuggestWindow, MUIA_Window_RootObject);

        set(group, MUIA_Group_ActivePage, MUIV_Group_ActivePage_First);

        SetAttrs(data->SuggestWindow, MUIA_Window_Activate, TRUE,
                                      MUIA_Window_DefaultObject, NULL,
                                      MUIA_Window_Open, TRUE,
                                      TAG_DONE);
      }
      else
      {
        BOOL res;

        if(data->SuggestSpawn == FALSE)
          res = SendCLI(word, data->SuggestCmd);
        else
          res = SendRexx(word, data->SuggestCmd);

        if(res == TRUE)
        {
          BPTR fh;

          if((fh = Open("T:Matches", MODE_OLDFILE)) != 0)
          {
            char entry[128];
            Object *group;

            DoMethod(data->SuggestListview, MUIM_List_Clear);
            while(FGets(fh, entry, 128) != NULL)
            {
              entry[strlen(entry)-1] = '\0';
              DoMethod(data->SuggestListview, MUIM_List_InsertSingle, entry, MUIV_List_Insert_Sorted);
            }
            Close(fh);

            group = (Object *)xget(data->SuggestWindow, MUIA_Window_RootObject);
            set(group, MUIA_Group_ActivePage, MUIV_Group_ActivePage_Last);
            SetAttrs(data->SuggestWindow, MUIA_Window_Activate, TRUE,
                                          MUIA_Window_DefaultObject, data->SuggestListview,
                                          MUIA_Window_Open, TRUE,
                                          TAG_DONE);
          }
        }
      }
      set(_win(data->object), MUIA_Window_Sleep, FALSE);
    }
  }
  else
  {
    D(DBF_ALWAYS, "character '%lc' is non-alpha", line->line.Contents[data->CPos_X]);
    DisplayBeep(NULL);
  }

  LEAVE();
}

///
/// CheckWord()
void CheckWord(struct InstData *data)
{
  ENTER();

  if(data->TypeAndSpell == TRUE && data->CPos_X != 0 && IsAlpha(data->mylocale, data->actualline->line.Contents[data->CPos_X-1]))
  {
    LONG start;
    LONG end = data->CPos_X;
    struct line_node *line = data->actualline;

    do
    {
      GoPreviousWord(data);
    }
    while(data->CPos_X != 0 && data->actualline == line && (data->actualline->line.Contents[data->CPos_X-1] == '-' || data->actualline->line.Contents[data->CPos_X-1] == '\''));

    start = data->CPos_X;
    data->CPos_X = end;

    if(start-end < 256 && data->actualline == line)
    {
      char word[256];

      strlcpy(word, &data->actualline->line.Contents[start], end-start+1);

      if(LookupWord(data, word) == FALSE)
        DisplayBeep(NULL);
    }
    else
    {
      data->actualline = line;
    }
  }

  LEAVE();
}

///
