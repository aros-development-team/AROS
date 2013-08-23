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

#include <devices/inputevent.h>
#include <libraries/mui.h>
#include <proto/intuition.h>
#include <proto/keymap.h>

#include <newmouse.h>

#include "private.h"

static ULONG ConvertKey (struct IntuiMessage *imsg)
{
  struct InputEvent event;
  unsigned char code = '\0';

  event.ie_NextEvent    = NULL;
  event.ie_Class         = IECLASS_RAWKEY;
  event.ie_SubClass     = 0;
  event.ie_Code          = imsg->Code;
  event.ie_Qualifier    = 0; /* imsg->Qualifier; */
  event.ie_EventAddress  = 0; /* (APTR *) *((ULONG *)imsg->IAddress); */

  MapRawKey(&event, (STRPTR)&code, 1, NULL);

  return code;
}

#define BETWEEN(a, min, max) (a >= min && a <= max)

/* wheel mouse support */
#undef  IECODE_KEY_CODE_LAST
#define  IECODE_KEY_CODE_LAST 0x7e

IPTR HandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  BOOL nokey = FALSE, backspace = FALSE, qual_only;
  IPTR result;

  qual_only = BETWEEN(msg->imsg->Code, 0x60, 0x67); // betwenn LSHIFT/RCOMMAND

  if(qual_only || ((isFlagSet(data->Flags, FLG_Snoop) || isFlagSet(data->Flags, FLG_Active)) &&
     msg->imsg->Class == IDCMP_RAWKEY &&
     msg->imsg->Code <= IECODE_KEY_CODE_LAST &&
     msg->muikey != MUIKEY_GADGET_NEXT &&
     msg->muikey != MUIKEY_GADGET_PREV))
  {
    static const char *qualifier_name[] =
    {
      "lshift", "rshift", "capslock", "control", "lalt",
      "ralt", "lcommand", "rcommand", "numericpad", "repeat",
      NULL
    };

    ULONG qualifier = msg->imsg->Qualifier;
    char buffer[256];
    ULONG i;

    buffer[0] = '\0';

    for(i=0; qualifier_name[i]; i++)
    {
      if(isFlagSet(qualifier, (1U << i)))
      {
        strlcat(buffer, qualifier_name[i], sizeof(buffer));
        strlcat(buffer, " ", sizeof(buffer));
      }
    }

    if(qual_only)
    {
      if(*buffer)
        buffer[strlen(buffer)-1] = '\0';
    }
    else
    {
      UWORD code = msg->imsg->Code;

      if(code >= RAWKEY_F11 && code <= RAWKEY_F10)
      {
        static const char *key_name[] =
        {
          "f11",
          "up", "down", "right", "left",
          "f1", "f2", "f3", "f4", "f5",
          "f6", "f7", "f8", "f9", "f10"
        };

        strlcat(buffer, key_name[code-RAWKEY_F11], sizeof(buffer));
      }
      else switch(code)
      {
        #if defined(__amigaos4__)
        case RAWKEY_MENU:     strlcat(buffer, "menu", sizeof(buffer)); break;
        #elif defined(__MORPHOS__)
        case RAWKEY_SCRLOCK:  strlcat(buffer, "scrlock", sizeof(buffer)); break;
        #endif

        case RAWKEY_NUMLOCK:  strlcat(buffer, "numlock", sizeof(buffer)); break;
        case RAWKEY_INSERT:   strlcat(buffer, "insert", sizeof(buffer)); break;
        case RAWKEY_PAGEUP:   strlcat(buffer, "page_up", sizeof(buffer)); break;
        case RAWKEY_PAGEDOWN: strlcat(buffer, "page_down", sizeof(buffer)); break;
        case RAWKEY_PRINTSCR: strlcat(buffer, "prtscr", sizeof(buffer)); break;
        case RAWKEY_BREAK:    strlcat(buffer, "pause", sizeof(buffer)); break;
        case RAWKEY_F12:      strlcat(buffer, "f12", sizeof(buffer)); break;
        case RAWKEY_HOME:     strlcat(buffer, "home", sizeof(buffer)); break;
        case RAWKEY_END:      strlcat(buffer, "end", sizeof(buffer)); break;

        #if defined(__amigaos4__)
        case RAWKEY_MEDIA_STOP:       strlcat(buffer, "media_stop", sizeof(buffer)); break;
        case RAWKEY_MEDIA_PLAY_PAUSE: strlcat(buffer, "media_play", sizeof(buffer)); break;
        case RAWKEY_MEDIA_PREV_TRACK: strlcat(buffer, "media_prev", sizeof(buffer)); break;
        case RAWKEY_MEDIA_NEXT_TRACK: strlcat(buffer, "media_next", sizeof(buffer)); break;
        case RAWKEY_MEDIA_SHUFFLE:    strlcat(buffer, "media_rewind", sizeof(buffer)); break;
        case RAWKEY_MEDIA_REPEAT:     strlcat(buffer, "media_forward", sizeof(buffer)); break;
        #else
        case RAWKEY_AUD_STOP:       strlcat(buffer, "media_stop", sizeof(buffer)); break;
        case RAWKEY_AUD_PLAY_PAUSE: strlcat(buffer, "media_play", sizeof(buffer)); break;
        case RAWKEY_AUD_PREV_TRACK: strlcat(buffer, "media_prev", sizeof(buffer)); break;
        case RAWKEY_AUD_NEXT_TRACK: strlcat(buffer, "media_next", sizeof(buffer)); break;
        case RAWKEY_AUD_SHUFFLE:    strlcat(buffer, "media_rewind", sizeof(buffer)); break;
        case RAWKEY_AUD_REPEAT:     strlcat(buffer, "media_forward", sizeof(buffer)); break;
        #endif


        case RAWKEY_HELP:
          strlcat(buffer, "help", sizeof(buffer));
        break;

        case NM_WHEEL_UP:
          strlcat(buffer, "nm_wheel_up", sizeof(buffer));
        break;

        case NM_WHEEL_DOWN:
          strlcat(buffer, "nm_wheel_down", sizeof(buffer));
        break;

        case NM_WHEEL_LEFT:
          strlcat(buffer, "nm_wheel_left", sizeof(buffer));
        break;

        case NM_WHEEL_RIGHT:
          strlcat(buffer, "nm_wheel_right", sizeof(buffer));
        break;

        case NM_BUTTON_FOURTH:
          strlcat(buffer, "nm_wheel_button", sizeof(buffer));
        break;

        default:
        {
          const char *append = NULL;
          unsigned char key[2];

          key[0] = ConvertKey(msg->imsg);
          key[1] = 0;

          switch(key[0])
          {
            case 0:
              nokey = TRUE;
            break;

            case 8:
              backspace = TRUE;
              append = "backspace";
            break;

            case 9:
              append = "tab";
            break;

            case 13:
              append = isFlagSet(qualifier, IEQUALIFIER_NUMERICPAD) ? "enter" : "return";
            break;

            case 27:
              append = "esc";
            break;

            case 32:
              append = "space";
            break;

            case 0x7f:
              append = "del";
            break;

            default:
              append = (char *)key;
            break;
          }

          if(append)
            strlcat(buffer, append, sizeof(buffer));
        }
        break;
      }
    }

    if(!nokey)
    {
      if(backspace)
      {
        if(isFlagSet(data->Flags, FLG_Backspace))
        {
          *buffer = '\0';
          clearFlag(data->Flags, FLG_Backspace);
        }
        else
        {
          setFlag(data->Flags, FLG_Backspace);
        }
      }
      else
      {
        clearFlag(data->Flags, FLG_Backspace);
      }
      SetAttrs(obj, MUIA_String_Contents, buffer, TAG_DONE);
    }
    result = MUI_EventHandlerRC_Eat;
  }
  else
    result = 0;

  return result;
}
