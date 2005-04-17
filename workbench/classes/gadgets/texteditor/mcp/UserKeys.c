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

 $Id: UserKeys.c,v 1.1 2005/03/28 11:29:49 damato Exp $

***************************************************************************/

#include <math.h>
#include <string.h>

#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/rdargs.h>
#include <devices/inputevent.h>
#include <proto/dos.h>
#include <proto/keymap.h>

#include "TextEditor_mcp.h"
#include "private.h"

void AddKeyBinding(STRPTR keystring, UWORD action, struct KeyAction *storage)
{
    STRPTR  ratemplate =
            "LSHIFT/S,RSHIFT/S,CAPSLOCK/S,CONTROL=CTRL/S,LALT/S,RALT/S,LAMIGA=LCOMMAND/S,RAMIGA=RCOMMAND/S,NUMERICPAD/S,"
            "f1/S,f2/S,f3/S,f4/S,f5/S,f6/S,f7/S,f8/S,f9/S,f10/S,"
            "help/S,"
            "up/S,down/S,right/S,left/S,"
            "esc/S,tab/S,return=enter/S,space/S,backspace/S,del/S,key/F";
    LONG    args[32];

    struct RDArgs *ra_result;
    struct RDArgs *myrdargs;
    ULONG count = 0;
    storage->vanilla = FALSE;
    storage->key = 0;
    storage->qualifier = 0;
    storage->action = action;

  while(count < 32)
  {
    args[count++] = 0;
  }

  if((myrdargs = AllocDosObject(DOS_RDARGS, NULL)))
  {
    myrdargs->RDA_Source.CS_Buffer = keystring;
    myrdargs->RDA_Source.CS_Length = strlen(keystring);
    myrdargs->RDA_Source.CS_CurChr = 0;
    myrdargs->RDA_Flags |= RDAF_NOPROMPT;

    if((ra_result = ReadArgs(ratemplate, args, myrdargs)))
    {
      /* Scan for qualifier */
      for(count = 0;count < 9;count++)
      {
        if(args[count])
          storage->qualifier |= 2^count;
      }

      /* Scan for F-keys */
      for(;count < 19;count++)
      {
        if(args[count])
          storage->key = count+71;
      }

      if(args[count++])
        storage->key = 95;    /* Help */

      /* Scan for cursor-keys */
      for(;count < 24;count++)
      {
        if(args[count])
          storage->key = count+56;
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

/*          if(storage->qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
          {
            storage->qualifier &= ~(IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT);
            storage->key = ToUpper(storage->key);
          }
*/        }

      }
      FreeArgs(ra_result);
    }
    FreeDosObject(DOS_RDARGS, myrdargs);
  }
}
