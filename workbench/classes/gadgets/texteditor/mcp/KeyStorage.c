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

 $Id: KeyStorage.c,v 1.1 2005/03/28 11:29:49 damato Exp $

***************************************************************************/

#include <math.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <devices/inputevent.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/rdargs.h>
#include <exec/memory.h>
#include <libraries/mui.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/muimaster.h>

#include "private.h"

void ConvertKeyString (STRPTR keystring, UWORD action, struct KeyAction *storage)
{
    static const STRPTR ratemplate =
          "LSHIFT/S,RSHIFT/S,CAPSLOCK/S,CONTROL=CTRL/S,LALT/S,RALT/S,LAMIGA=LCOMMAND/S,RAMIGA=RCOMMAND/S,NUMPAD=NUMERICPAD/S,SHIFT/S,ALT/S,AMIGA=COMMAND/S,"
          "f1/S,f2/S,f3/S,f4/S,f5/S,f6/S,f7/S,f8/S,f9/S,f10/S,"
          "help/S,"
          "up/S,down/S,right/S,left/S,"
          "escape=esc/S,tab/S,return=enter/S,space/S,backspace=bs/S,delete=del/S,key/F";
    LONG    args[43];
    struct RDArgs *ra_result;
    struct RDArgs *myrdargs;
    ULONG count = 0;

  storage->vanilla = FALSE;
  storage->key = 0;
  storage->qualifier = 0;
  storage->action = action;

  while(count < 43)
  {
    args[count++] = 0L;
  }

  if((myrdargs = AllocDosObject(DOS_RDARGS, NULL)))
  {
      ULONG length = strlen(keystring);
      STRPTR buffer;

    if((buffer = AllocMem(length+2, MEMF_ANY)))
    {
      CopyMem(keystring, buffer, length);
      buffer[length] = '\n';
      buffer[length+1] = '\0';
      myrdargs->RDA_Source.CS_Buffer = buffer;
      myrdargs->RDA_Source.CS_Length = length+1;
      myrdargs->RDA_Source.CS_CurChr = 0;
      myrdargs->RDA_Flags |= RDAF_NOPROMPT;

      if((ra_result = ReadArgs(ratemplate, args, myrdargs)))
      {
          ULONG qual = 1;

        /* Scan for qualifier */
        for(count = 0;count < 12;count++)
        {
          if(args[count])
          {
            storage->qualifier |= qual;
          }
          qual = qual << 1;
        }

        /* Scan for F-keys */
        for(;count < 22;count++)
        {
          if(args[count])
            storage->key = count+68;
        }

        if(args[count++])
          storage->key = 95;    /* Help */

        /* Scan for cursor-keys */
        for(;count < 27;count++)
        {
          if(args[count])
            storage->key = count+53;
        }

        if(!storage->key)
        {
          storage->vanilla = TRUE;
          if(args[count++])
            storage->key = 27;  /* Esc */
          if(args[count++])
            storage->key = 9;   /* Tab */
          if(args[count++])
            storage->key = 13;  /* CR */
          if(args[count++])
            storage->key = ' '; /* Space */
          if(args[count++])
            storage->key = 8;   /* Backspace */
          if(args[count++])
            storage->key = 0x7f;  /* Delete */

          if(!storage->key)
          {
            storage->key = (UWORD)*(STRPTR)args[count];
          }
        }
        FreeArgs(ra_result);
      }
      FreeMem(buffer, length+2);
    }
    FreeDosObject(DOS_RDARGS, myrdargs);
  }
}

void KeyToString (STRPTR buffer, struct KeyAction *ka)
{
  buffer[0] = '\0';
  if(ka->qualifier & IEQUALIFIER_LSHIFT)
    strcat(buffer, "lshift ");
  if(ka->qualifier & IEQUALIFIER_RSHIFT)
    strcat(buffer, "rshift ");
  if(ka->qualifier & IEQUALIFIER_CAPSLOCK)
    strcat(buffer, "capslock ");
  if(ka->qualifier & IEQUALIFIER_CONTROL)
    strcat(buffer, "control ");
  if(ka->qualifier & IEQUALIFIER_LALT)
    strcat(buffer, "lalt ");
  if(ka->qualifier & IEQUALIFIER_RALT)
    strcat(buffer, "ralt ");
  if(ka->qualifier & IEQUALIFIER_LCOMMAND)
    strcat(buffer, "lcommand ");
  if(ka->qualifier & IEQUALIFIER_RCOMMAND)
    strcat(buffer, "rcommand ");
  if(ka->qualifier & IEQUALIFIER_NUMERICPAD)
    strcat(buffer, "numpad ");

  if(ka->qualifier & IEQUALIFIER_SHIFT)
    strcat(buffer, "shift ");
  if(ka->qualifier & IEQUALIFIER_ALT)
    strcat(buffer, "alt ");
  if(ka->qualifier & IEQUALIFIER_COMMAND)
    strcat(buffer, "command ");

  if(ka->vanilla)
  {
      UBYTE key = ka->key;

    switch(key)
    {
      case 8:
        strcat(buffer, "backspace");
        break;
      case 9:
        strcat(buffer, "tab");
        break;
      case 13:
        strcat(buffer, ((ka->qualifier & IEQUALIFIER_NUMERICPAD) ? "enter" : "return"));
        break;
      case 27:
        strcat(buffer, "esc");
        break;
      case 32:
        strcat(buffer, "space");
        break;
      case 0x7f:
        strcat(buffer, "del");
        break;
      default:
        strncat(buffer, &key, 1);
    }
  }
  else
  {
    switch(ka->key)
    {
      case 76:
        strcat(buffer, "up");
        break;
      case 77:
        strcat(buffer, "down");
        break;
      case 78:
        strcat(buffer, "right");
        break;
      case 79:
        strcat(buffer, "left");
        break;
      case 80:
        strcat(buffer, "f1");
        break;
      case 81:
        strcat(buffer, "f2");
        break;
      case 82:
        strcat(buffer, "f3");
        break;
      case 83:
        strcat(buffer, "f4");
        break;
      case 84:
        strcat(buffer, "f5");
        break;
      case 85:
        strcat(buffer, "f6");
        break;
      case 86:
        strcat(buffer, "f7");
        break;
      case 87:
        strcat(buffer, "f8");
        break;
      case 88:
        strcat(buffer, "f9");
        break;
      case 89:
        strcat(buffer, "f10");
        break;
      case 95:
        strcat(buffer, "help");
        break;
      default:
        strcat(buffer, "???");
    }
  }
}

void ImportKeys(void *config, struct InstData_MCP *data)
{
    void *cfg_data;

  DoMethod(data->keybindings, MUIM_List_Clear);
  set(data->keybindings, MUIA_List_Quiet, TRUE);
  if((cfg_data = (void *)DoMethod(config, MUIM_Dataspace_Find, MUICFG_TextEditor_Keybindings)))
  {
      struct te_key *entries = cfg_data;

    while(entries->code != (UWORD)-1)
    {
      DoMethod(data->keybindings, MUIM_List_InsertSingle, entries++, MUIV_List_Insert_Bottom);
    }
  }
  else
  {
    DoMethod(data->keybindings, MUIM_List_Insert, keybindings, (ULONG)-1, MUIV_List_Insert_Bottom);
  }
  set(data->keybindings, MUIA_List_Quiet, FALSE);

/*  if(cfg_data = (void *)DoMethod(config, MUIM_Dataspace_Find, MUICFG_TextEditor_SuggestKey))
  {
      UBYTE   buffer[100];
      struct  KeyAction *ka = cfg_data;

    KeyToString(buffer, ka);
    set(data->suggestkey, MUIA_Hotkey_Hotkey, buffer);
  }
  else
  {
    set(data->suggestkey, MUIA_Hotkey_Hotkey, "help");
  }*/
}

void ExportKeys(void *config, struct InstData_MCP *data)
{
    ULONG c, size;
    struct te_key *entry;
    struct te_key *entries;

  get(data->keybindings, MUIA_List_Entries, &c);
  size = (c+1) * sizeof(struct te_key);

  if((entries = (struct te_key *)AllocMem(size, MEMF_ANY)))
  {
      struct te_key *buffer = entries+c;

    buffer->code = -1;
    while(c--)
    {
      DoMethod(data->keybindings, MUIM_List_GetEntry, c, &entry);
      buffer--;
      buffer->code = entry->code;
      buffer->qual = entry->qual;
      buffer->act  = entry->act;
    }
    DoMethod(config, MUIM_Dataspace_Add, entries, size, MUICFG_TextEditor_Keybindings);
    FreeMem((APTR)entries, size);
  }
}
