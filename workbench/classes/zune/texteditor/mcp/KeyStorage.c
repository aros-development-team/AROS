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

#include <math.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <devices/inputevent.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/rdargs.h>
#include <exec/memory.h>
#include <libraries/mui.h>
#include <mui/HotkeyString_mcc.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/muimaster.h>

#include "private.h"
#include "newmouse.h"
#include "Debug.h"

enum Keys
{
    key_lshift = 0,
    key_rshift,
    key_capslock,
    key_control,
    key_lalt,
    key_ralt,
    key_lamiga,
    key_ramiga,
    key_numpad,
    key_shift,
    key_alt,
    key_amiga,
    key_f1,
    key_f2,
    key_f3,
    key_f4,
    key_f5,
    key_f6,
    key_f7,
    key_f8,
    key_f9,
    key_f10,
    key_f11,
    key_f12,
    key_help,
    key_up,
    key_down,
    key_right,
    key_left,
    key_home,
    key_end,
    key_page_up,
    key_page_down,
    key_insert,
    key_prtscr,
    key_pause,
    key_numlock,
#if defined(__amigaos4__)
    key_menu,
#elif defined(__MORPHOS__)
    key_scrlock,
#endif
    key_mm_stop,
    key_mm_play,
    key_mm_prev,
    key_mm_next,
    key_mm_rewind,
    key_mm_forward,
    key_wheel_up,
    key_wheel_down,
    key_wheel_left,
    key_wheel_right,
    key_wheel_button,
    key_escape,
    key_tab,
    key_return,
    key_space,
    key_backspace,
    key_delete,
    key_key,
    key_count,
};

void ConvertKeyString(STRPTR keystring, UWORD action, struct KeyAction *storage)
{
  IPTR args[key_count];
  struct RDArgs *ra_result;
  struct RDArgs *myrdargs;

  static const char * const ratemplate =
    "LSHIFT/S,RSHIFT/S,CAPSLOCK/S,CONTROL=CTRL/S,LALT/S,RALT/S,LAMIGA=LCOMMAND/S,RAMIGA=RCOMMAND/S,NUMPAD=NUMERICPAD/S,SHIFT/S,ALT/S,AMIGA=COMMAND/S,"
    "f1/S,f2/S,f3/S,f4/S,f5/S,f6/S,f7/S,f8/S,f9/S,f10/S,f11/S,f12/S,"
    "help/S,"
    "up/S,down/S,right/S,left/S,"
    "home/S,end/S,page_up=pageup/S,page_down=pagedown/S,insert/S,printscreen=prtscr/S,pause=break/S,numlock/S,"
    #if defined(__amigaos4__)
    "menu/S,"
    #elif defined(__MORPHOS__)
    "scrolllock=scrlock/S,"
    #endif
    "media_stop/S,media_play/S,media_prev/S,media_next/S,media_rewind/S,media_forward/S,"
    "nm_wheel_up/S,nm_wheel_down/S,nm_wheel_left/S,nm_wheel_right/S,nm_wheel_button/S,"
    "escape=esc/S,tab/S,return=enter/S,space/S,backspace=bs/S,delete=del/S,key/F";

  storage->vanilla = FALSE;
  storage->key = 0;
  storage->qualifier = 0;
  storage->action = action;

  // clear all args
  memset(args, 0, sizeof(args));

  if((myrdargs = AllocDosObject(DOS_RDARGS, NULL)) != NULL)
  {
    ULONG length = strlen(keystring);
    char *buffer;

    if((buffer = AllocVecShared(length + 2, MEMF_ANY)) != NULL)
    {
      strlcpy(buffer, keystring, length + 1);
      buffer[length] = '\n';
      buffer[length+1] = '\0';
      myrdargs->RDA_Source.CS_Buffer = buffer;
      myrdargs->RDA_Source.CS_Length = length+1;
      myrdargs->RDA_Source.CS_CurChr = 0;
      myrdargs->RDA_Flags |= RDAF_NOPROMPT;

      if((ra_result = ReadArgs(ratemplate, (APTR)args, myrdargs)) != NULL)
      {
        // Scan for 12 qualifier keys
        if(args[key_lshift])
          storage->qualifier |= IEQUALIFIER_LSHIFT;
        if(args[key_rshift])
          storage->qualifier |= IEQUALIFIER_RSHIFT;
        if(args[key_capslock])
          storage->qualifier |= IEQUALIFIER_CAPSLOCK;
        if(args[key_control])
          storage->qualifier |= IEQUALIFIER_CONTROL;
        if(args[key_lalt])
          storage->qualifier |= IEQUALIFIER_LALT;
        if(args[key_ralt])
          storage->qualifier |= IEQUALIFIER_RALT;
        if(args[key_lamiga])
          storage->qualifier |= IEQUALIFIER_LCOMMAND;
        if(args[key_ramiga])
          storage->qualifier |= IEQUALIFIER_RCOMMAND;
        if(args[key_numpad])
          storage->qualifier |= IEQUALIFIER_NUMERICPAD;
        if(args[key_shift])
          storage->qualifier |= IEQUALIFIER_SHIFT;
        if(args[key_alt])
          storage->qualifier |= IEQUALIFIER_ALT;
        if(args[key_amiga])
          storage->qualifier |= IEQUALIFIER_COMMAND;

        // Scan for the 10 standard F-keys (f1-f10)
        if(args[key_f1])
          storage->key = RAWKEY_F1;
        if(args[key_f2])
          storage->key = RAWKEY_F2;
        if(args[key_f3])
          storage->key = RAWKEY_F3;
        if(args[key_f4])
          storage->key = RAWKEY_F4;
        if(args[key_f5])
          storage->key = RAWKEY_F5;
        if(args[key_f6])
          storage->key = RAWKEY_F6;
        if(args[key_f7])
          storage->key = RAWKEY_F7;
        if(args[key_f8])
          storage->key = RAWKEY_F8;
        if(args[key_f9])
          storage->key = RAWKEY_F9;
        if(args[key_f10])
          storage->key = RAWKEY_F10;

        // Scan for the 2 extended f-keys (f11,f12)
        if(args[key_f11])
          storage->key = RAWKEY_F11;
        if(args[key_f12])
          storage->key = RAWKEY_F12;

        // Help
        if(args[key_help])
          storage->key = RAWKEY_HELP;

        // Scan for cursor-keys
        if(args[key_up])
          storage->key = RAWKEY_CRSRUP;
        if(args[key_down])
          storage->key = RAWKEY_CRSRDOWN;
        if(args[key_right])
          storage->key = RAWKEY_CRSRRIGHT;
        if(args[key_left])
          storage->key = RAWKEY_CRSRLEFT;

        // scan for the other extended (non-standard) keys
        if(args[key_home])
          storage->key = RAWKEY_HOME;
        if(args[key_end])
          storage->key = RAWKEY_END;
        if(args[key_page_up])
          storage->key = RAWKEY_PAGEUP;
        if(args[key_page_down])
          storage->key = RAWKEY_PAGEDOWN;
        if(args[key_insert])
          storage->key = RAWKEY_INSERT;
        if(args[key_prtscr])
          storage->key = RAWKEY_PRINTSCR;
        if(args[key_pause])
          storage->key = RAWKEY_BREAK;
        if(args[key_numlock])
          storage->key = RAWKEY_NUMLOCK;

        // some keys are mutual excluse on some platforms
        #if defined(__amigso4__)
        if(args[key_menu])
          storage->key = RAWKEY_MENU;
        #elif defined(__MORPHOS__)
        if(args[key_scrlock])
          storage->key = RAWKEY_SCRLOCK;
        #endif

        // lets address the media/CDTV keys as well
        #if defined(__amigaos4__)
        if(args[key_mm_stop])
          storage->key = RAWKEY_MEDIA_STOP;
        if(args[key_mm_play])
          storage->key = RAWKEY_MEDIA_PLAY_PAUSE;
        if(args[key_mm_prev])
          storage->key = RAWKEY_MEDIA_PREV_TRACK;
        if(args[key_mm_next])
          storage->key = RAWKEY_MEDIA_NEXT_TRACK;
        if(args[key_mm_rewind])
          storage->key = RAWKEY_MEDIA_SHUFFLE;
        if(args[key_mm_forward])
          storage->key = RAWKEY_MEDIA_REPEAT;
        #else
        if(args[key_mm_stop])
          storage->key = RAWKEY_AUD_STOP;
        if(args[key_mm_play])
          storage->key = RAWKEY_AUD_PLAY_PAUSE;
        if(args[key_mm_prev])
          storage->key = RAWKEY_AUD_PREV_TRACK;
        if(args[key_mm_next])
          storage->key = RAWKEY_AUD_NEXT_TRACK;
        if(args[key_mm_rewind])
          storage->key = RAWKEY_AUD_SHUFFLE;
        if(args[key_mm_forward])
          storage->key = RAWKEY_AUD_REPEAT;
        #endif

        // take respect of the NEWMOUSE RAWKEY based wheel events as well
        if(args[key_wheel_up])
          storage->key = NM_WHEEL_UP;
        if(args[key_wheel_down])
          storage->key = NM_WHEEL_DOWN;
        if(args[key_wheel_left])
          storage->key = NM_WHEEL_LEFT;
        if(args[key_wheel_right])
          storage->key = NM_WHEEL_RIGHT;
        if(args[key_wheel_button])
          storage->key = NM_BUTTON_FOURTH;

        if(storage->key == 0)
        {
          storage->vanilla = TRUE;
          if(args[key_escape])
            storage->key = 0x1b;  /* Esc */
          if(args[key_tab])
            storage->key = 0x09;  /* Tab */
          if(args[key_return])
            storage->key = 0x0d;  /* CR */
          if(args[key_space])
            storage->key = 0x20;  /* Space */
          if(args[key_backspace])
            storage->key = 0x08;  /* Backspace */
          if(args[key_delete])
            storage->key = 0x7f;  /* Delete */

          if(storage->key == 0 && args[key_key])
          {
            STRPTR str = (STRPTR)args[key_key];

            storage->key = str[0];
          }
        }
        FreeArgs(ra_result);
      }
      FreeVec(buffer);
    }
    FreeDosObject(DOS_RDARGS, myrdargs);
  }
}

void KeyToString(STRPTR buffer, ULONG buffer_len, struct KeyAction *ka)
{
  buffer[0] = '\0';

  // lets first put the qualifiers in our buffer string
  if(ka->qualifier & IEQUALIFIER_LSHIFT)
    strlcat(buffer, "lshift ", buffer_len);
  if(ka->qualifier & IEQUALIFIER_RSHIFT)
    strlcat(buffer, "rshift ", buffer_len);
  if(ka->qualifier & IEQUALIFIER_CAPSLOCK)
    strlcat(buffer, "capslock ", buffer_len);
  if(ka->qualifier & IEQUALIFIER_CONTROL)
    strlcat(buffer, "control ", buffer_len);
  if(ka->qualifier & IEQUALIFIER_LALT)
    strlcat(buffer, "lalt ", buffer_len);
  if(ka->qualifier & IEQUALIFIER_RALT)
    strlcat(buffer, "ralt ", buffer_len);
  if(ka->qualifier & IEQUALIFIER_LCOMMAND)
    strlcat(buffer, "lcommand ", buffer_len);
  if(ka->qualifier & IEQUALIFIER_RCOMMAND)
    strlcat(buffer, "rcommand ", buffer_len);
  if(ka->qualifier & IEQUALIFIER_NUMERICPAD)
    strlcat(buffer, "numpad ", buffer_len);
  if(ka->qualifier & IEQUALIFIER_SHIFT)
    strlcat(buffer, "shift ", buffer_len);
  if(ka->qualifier & IEQUALIFIER_ALT)
    strlcat(buffer, "alt ", buffer_len);
  if(ka->qualifier & IEQUALIFIER_COMMAND)
    strlcat(buffer, "command ", buffer_len);

  // then we check wheter this are vanilla key codes or RAWKEY codes
  if(ka->vanilla)
  {
    switch(ka->key)
    {
      case 0x08: strlcat(buffer, "backspace", buffer_len); break;
      case 0x09: strlcat(buffer, "tab", buffer_len); break;
      case 0x0d: strlcat(buffer, ((ka->qualifier & IEQUALIFIER_NUMERICPAD) ? "enter" : "return"), buffer_len); break;
      case 0x1b: strlcat(buffer, "esc", buffer_len); break;
      case 0x29: strlcat(buffer, "space", buffer_len); break;
      case 0x7f: strlcat(buffer, "del", buffer_len); break;

      default:
      {
        char *p = &buffer[strlen(buffer)];

        *p++ = ka->key;
        *p = '\0';
      }
    }
  }
  else
  {
    switch(ka->key)
    {
      case RAWKEY_CRSRUP:     strlcat(buffer, "up", buffer_len); break;
      case RAWKEY_CRSRDOWN:   strlcat(buffer, "down", buffer_len); break;
      case RAWKEY_CRSRRIGHT:  strlcat(buffer, "right", buffer_len); break;
      case RAWKEY_CRSRLEFT:   strlcat(buffer, "left", buffer_len); break;
      case RAWKEY_F1:         strlcat(buffer, "f1", buffer_len); break;
      case RAWKEY_F2:         strlcat(buffer, "f2", buffer_len); break;
      case RAWKEY_F3:         strlcat(buffer, "f3", buffer_len); break;
      case RAWKEY_F4:         strlcat(buffer, "f4", buffer_len); break;
      case RAWKEY_F5:         strlcat(buffer, "f5", buffer_len); break;
      case RAWKEY_F6:         strlcat(buffer, "f6", buffer_len); break;
      case RAWKEY_F7:         strlcat(buffer, "f7", buffer_len); break;
      case RAWKEY_F8:         strlcat(buffer, "f8", buffer_len); break;
      case RAWKEY_F9:         strlcat(buffer, "f9", buffer_len); break;
      case RAWKEY_F10:        strlcat(buffer, "f10", buffer_len); break;
      case RAWKEY_F11:        strlcat(buffer, "f11", buffer_len); break;
      case RAWKEY_F12:        strlcat(buffer, "f12", buffer_len); break;
      case RAWKEY_HELP:       strlcat(buffer, "help", buffer_len); break;
      case RAWKEY_HOME:       strlcat(buffer, "home", buffer_len); break;
      case RAWKEY_END:        strlcat(buffer, "end", buffer_len); break;
      case RAWKEY_PAGEUP:     strlcat(buffer, "page_up", buffer_len); break;
      case RAWKEY_PAGEDOWN:   strlcat(buffer, "page_down", buffer_len); break;
      case RAWKEY_INSERT:     strlcat(buffer, "insert", buffer_len); break;
      case RAWKEY_PRINTSCR:   strlcat(buffer, "printscreen", buffer_len); break;
      case RAWKEY_BREAK:      strlcat(buffer, "pause", buffer_len); break;
      case RAWKEY_NUMLOCK:    strlcat(buffer, "numlock", buffer_len); break;

      #if defined(__amigaos4__)
      case RAWKEY_MENU:       strlcat(buffer, "menu", buffer_len); break;
      #elif defined(__MORPHOS__)
      case RAWKEY_SCRLOCK:    strlcat(buffer, "scrolllock", buffer_len); break;
      #endif

      #if defined(__amigaos4__)
      case RAWKEY_MEDIA_STOP:       strlcat(buffer, "media_stop", buffer_len); break;
      case RAWKEY_MEDIA_PLAY_PAUSE: strlcat(buffer, "media_play", buffer_len); break;
      case RAWKEY_MEDIA_PREV_TRACK: strlcat(buffer, "media_prev", buffer_len); break;
      case RAWKEY_MEDIA_NEXT_TRACK: strlcat(buffer, "media_next", buffer_len); break;
      case RAWKEY_MEDIA_SHUFFLE:    strlcat(buffer, "media_rewind", buffer_len); break;
      case RAWKEY_MEDIA_REPEAT:     strlcat(buffer, "media_forward", buffer_len); break;
      #else
      case RAWKEY_AUD_STOP:       strlcat(buffer, "media_stop", buffer_len); break;
      case RAWKEY_AUD_PLAY_PAUSE: strlcat(buffer, "media_play", buffer_len); break;
      case RAWKEY_AUD_PREV_TRACK: strlcat(buffer, "media_prev", buffer_len); break;
      case RAWKEY_AUD_NEXT_TRACK: strlcat(buffer, "media_next", buffer_len); break;
      case RAWKEY_AUD_SHUFFLE:    strlcat(buffer, "media_rewind", buffer_len); break;
      case RAWKEY_AUD_REPEAT:     strlcat(buffer, "media_forward", buffer_len); break;
      #endif

      case NM_WHEEL_UP:           strlcat(buffer, "nm_wheel_up", buffer_len); break;
      case NM_WHEEL_DOWN:         strlcat(buffer, "nm_wheel_down", buffer_len); break;
      case NM_WHEEL_LEFT:         strlcat(buffer, "nm_wheel_left", buffer_len); break;
      case NM_WHEEL_RIGHT:        strlcat(buffer, "nm_wheel_right", buffer_len); break;
      case NM_BUTTON_FOURTH:      strlcat(buffer, "nm_wheel_button", buffer_len); break;

      default:
        strlcat(buffer, "???", buffer_len);
        break;
    }
  }
}

void ImportKeys(struct InstData_MCP *data, void *config)
{
  void *cfg_data;
  struct te_key *userkeys;

  ENTER();

  if(config != NULL && (cfg_data = (void *)DoMethod(config, MUIM_Dataspace_Find, MUICFG_TextEditor_Keybindings)))
    userkeys = cfg_data;
  else
    userkeys = (struct te_key *)default_keybindings;

  DoMethod(data->keybindings, MUIM_List_Clear);

  set(data->keybindings, MUIA_List_Quiet, TRUE);

  while((WORD)userkeys->code != -1)
  {
    DoMethod(data->keybindings, MUIM_List_InsertSingle, userkeys, MUIV_List_Insert_Bottom);

    userkeys++;
  }

  set(data->keybindings, MUIA_List_Quiet, FALSE);

  LEAVE();
}

void ExportKeys(struct InstData_MCP *data, void *config)
{
  ULONG c, size;
  struct te_key *entry;
  struct te_key *entries;

  c = xget(data->keybindings, MUIA_List_Entries);
  size = (c+1) * sizeof(struct te_key);

  if((entries = (struct te_key *)AllocVecShared(size, MEMF_ANY)))
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
    FreeVec(entries);
  }
}
