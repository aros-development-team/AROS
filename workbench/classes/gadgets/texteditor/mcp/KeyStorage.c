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

#include <math.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <devices/inputevent.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/rdargs.h>
#include <exec/memory.h>
#include <libraries/mui.h>
#include <MUI/HotkeyString_mcc.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/muimaster.h>

#include "private.h"
#ifdef __AROS__
#include "../includes/newmouse.h"
#else
#include "newmouse.h"
#endif

#define KEYS_COUNT (64)

void ConvertKeyString (STRPTR keystring, UWORD action, struct KeyAction *storage)
{
  LONG args[KEYS_COUNT];
  struct RDArgs *ra_result;
  struct RDArgs *myrdargs;
  ULONG count = 0;

  static const STRPTR ratemplate =
    "LSHIFT/S,RSHIFT/S,CAPSLOCK/S,CONTROL=CTRL/S,LALT/S,RALT/S,LAMIGA=LCOMMAND/S,RAMIGA=RCOMMAND/S,NUMPAD=NUMERICPAD/S,SHIFT/S,ALT/S,AMIGA=COMMAND/S,"
    "f1/S,f2/S,f3/S,f4/S,f5/S,f6/S,f7/S,f8/S,f9/S,f10/S,f11/S,f12/S"
    "help/S,"
    "up/S,down/S,right/S,left/S,"
    "home/S,end/S,page_up=pageup/S,page_down=pagedown/S,insert/S,printscreen=prtscr/S,pause=break/S,numlock/S,"
    #if defined(__amigaos4__)
    "menu/S"
    #elif defined(__MORPHOS__)
    "scrolllock=scrlock/S"
    #endif
    "media_stop/S,media_play/S,media_prev/S,media_next/S,media_rewind/S,media_forward/S,"
    "nm_wheel_up/S,nm_wheel_down/S,nm_wheel_left/S,nm_wheel_right/S,nm_wheel_button/S"
    "escape=esc/S,tab/S,return=enter/S,space/S,backspace=bs/S,delete=del/S,key/F";

  storage->vanilla = FALSE;
  storage->key = 0;
  storage->qualifier = 0;
  storage->action = action;

  // clear all args
  while(count < KEYS_COUNT)
    args[count++] = 0L;

  if((myrdargs = AllocDosObject(DOS_RDARGS, NULL)))
  {
    ULONG length = strlen(keystring);
    char *buffer;

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

        // Scan for 12 qualifier keys
        for(count = 0;count < 12;count++)
        {
          if(args[count])
          {
            storage->qualifier |= qual;
          }
          qual = qual << 1;
        }

        // Scan for the 10 standard F-keys (f1-f10)
        for(;count < 22;count++)
        {
          if(args[count])
            storage->key = count+0x44;
        }

        // Scan for the 2 extended f-keys (f11,f12)
        if(args[count++])
          storage->key = RAWKEY_F11;
        if(args[count++])
          storage->key = RAWKEY_F12;

        // Help
        if(args[count++])
          storage->key = RAWKEY_HELP;

        // Scan for cursor-keys
        for(;count < 27;count++)
        {
          if(args[count])
            storage->key = count+0x35;
        }

        // scan for the other extended (non-standard) keys
        if(args[count++])
          storage->key = RAWKEY_HOME;
        if(args[count++])
          storage->key = RAWKEY_END;
        if(args[count++])
          storage->key = RAWKEY_PAGEUP;
        if(args[count++])
          storage->key = RAWKEY_PAGEDOWN;
        if(args[count++])
          storage->key = RAWKEY_INSERT;
        if(args[count++])
          storage->key = RAWKEY_PRINTSCR;
        if(args[count++])
          storage->key = RAWKEY_BREAK;
        if(args[count++])
          storage->key = RAWKEY_NUMLOCK;

        // some keys are mutual excluse on some platforms
        #if defined(__amigso4__)
        if(args[count++])
          storage->key = RAWKEY_MENU;
        #elif defined(__MORPHOS__)
        if(args[count++])
          storage->key = RAWKEY_SCRLOCK;
        #endif

        // lets address the media/CDTV keys as well
        if(args[count++])
          storage->key = RAWKEY_AUD_STOP;
        if(args[count++])
          storage->key = RAWKEY_AUD_PLAY_PAUSE;
        if(args[count++])
          storage->key = RAWKEY_AUD_PREV_TRACK;
        if(args[count++])
          storage->key = RAWKEY_AUD_NEXT_TRACK;
        if(args[count++])
          storage->key = RAWKEY_AUD_SHUFFLE;
        if(args[count++])
          storage->key = RAWKEY_AUD_REPEAT;

        // take respect of the NEWMOUSE RAWKEY based wheel events as well
        if(args[count++])
          storage->key = NM_WHEEL_UP;
        if(args[count++])
          storage->key = NM_WHEEL_DOWN;
        if(args[count++])
          storage->key = NM_WHEEL_LEFT;
        if(args[count++])
          storage->key = NM_WHEEL_RIGHT;
        if(args[count++])
          storage->key = NM_BUTTON_FOURTH;

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

  // lets first put the qualifiers in our buffer string
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

  // then we check wheter this are vanilla key codes or RAWKEY codes
  if(ka->vanilla)
  {
    switch(ka->key)
    {
      case 8:   strcat(buffer, "backspace"); break;
      case 9:   strcat(buffer, "tab"); break;
      case 13:  strcat(buffer, ((ka->qualifier & IEQUALIFIER_NUMERICPAD) ? "enter" : "return")); break;
      case 27:  strcat(buffer, "esc"); break;
      case 32:  strcat(buffer, "space"); break;
      case 0x7f:strcat(buffer, "del"); break;

      default:
      {
        char *p = &buffer[strlen(buffer)];

        *p = ka->key;
        p++;
        *p = '\0';
      }
    }
  }
  else
  {
    switch(ka->key)
    {
      case RAWKEY_CRSRUP:     strcat(buffer, "up"); break;
      case RAWKEY_CRSRDOWN:   strcat(buffer, "down"); break;
      case RAWKEY_CRSRRIGHT:  strcat(buffer, "right"); break;
      case RAWKEY_CRSRLEFT:   strcat(buffer, "left"); break;
      case RAWKEY_F1:         strcat(buffer, "f1"); break;
      case RAWKEY_F2:         strcat(buffer, "f2"); break;
      case RAWKEY_F3:         strcat(buffer, "f3"); break;
      case RAWKEY_F4:         strcat(buffer, "f4"); break;
      case RAWKEY_F5:         strcat(buffer, "f5"); break;
      case RAWKEY_F6:         strcat(buffer, "f6"); break;
      case RAWKEY_F7:         strcat(buffer, "f7"); break;
      case RAWKEY_F8:         strcat(buffer, "f8"); break;
      case RAWKEY_F9:         strcat(buffer, "f9"); break;
      case RAWKEY_F10:        strcat(buffer, "f10"); break;
      case RAWKEY_F11:        strcat(buffer, "f11"); break;
      case RAWKEY_F12:        strcat(buffer, "f12"); break;
      case RAWKEY_HELP:       strcat(buffer, "help"); break;
      case RAWKEY_HOME:       strcat(buffer, "home"); break;
      case RAWKEY_END:        strcat(buffer, "end"); break;
      case RAWKEY_PAGEUP:     strcat(buffer, "page_up"); break;
      case RAWKEY_PAGEDOWN:   strcat(buffer, "page_down"); break;
      case RAWKEY_INSERT:     strcat(buffer, "insert"); break;
      case RAWKEY_PRINTSCR:   strcat(buffer, "printscreen"); break;
      case RAWKEY_BREAK:      strcat(buffer, "pause"); break;
      case RAWKEY_NUMLOCK:    strcat(buffer, "numlock"); break;

      #if defined(__amigaos4__)
      case RAWKEY_MENU:       strcat(buffer, "menu"); break;
      #elif defined(__MORPHOS__)
      case RAWKEY_SCRLOCK:    strcat(buffer, "scrolllock"); break;
      #endif

			case RAWKEY_AUD_STOP:       strcat(buffer, "media_stop"); break;
			case RAWKEY_AUD_PLAY_PAUSE: strcat(buffer, "media_play"); break;
			case RAWKEY_AUD_PREV_TRACK: strcat(buffer, "media_prev"); break;
			case RAWKEY_AUD_NEXT_TRACK: strcat(buffer, "media_next"); break;
			case RAWKEY_AUD_SHUFFLE:    strcat(buffer, "media_rewind"); break;
			case RAWKEY_AUD_REPEAT:     strcat(buffer, "media_forward"); break;

			case NM_WHEEL_UP:           strcat(buffer, "nm_wheel_up"); break;
			case NM_WHEEL_DOWN:         strcat(buffer, "nm_wheel_down"); break;
			case NM_WHEEL_LEFT:         strcat(buffer, "nm_wheel_left"); break;
  		case NM_WHEEL_RIGHT:        strcat(buffer, "nm_wheel_right"); break;
			case NM_BUTTON_FOURTH:      strcat(buffer, "nm_wheel_button"); break;

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
