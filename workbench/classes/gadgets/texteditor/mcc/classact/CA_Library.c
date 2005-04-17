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

 $Id: CA_Library.c,v 1.1 2005/03/28 11:29:49 damato Exp $

***************************************************************************/

#include <StdIO.h>
#include <Exec/Libraries.h>
#include <Libraries/MUI.h>
#include <Proto/Exec.h>
#include <Proto/MUIMaster.h>
#include <Proto/Utility.h>
#include <Proto/DOS.h>
#include <Proto/MathIeeeDoubTrans.h>
#include <Proto/MathIeeeDoubBas.h>
#include <Proto/RexxSysLib.h>
#include <Proto/Layers.h>
#include <Proto/Keymap.h>
#include <Proto/Graphics.h>
#include <Proto/Diskfont.h>
#include <Proto/Intuition.h>
#include <Proto/Locale.h>

#include <Editor.h>

ULONG Dispatcher  (REG(a0) struct IClass *, REG(a2) Object *, REG(a1) Msg);
VOID abortLibInit ();

  struct LibBase
  {
    struct  Library       base;
  };


#pragma libbase LibBase

struct IClass     *TextEditor;
struct ClassLibrary *BevelBase = NULL;
static UBYTE LibVersionString[] = "$VER: texteditor.gadget 15.9 (" __DATE2__ ") © 1998-2000 Allan Odgaard";

VOID INIT_7_InitLib (REG(a6) struct Library *base)
{
  if(BevelBase = (struct ClassLibrary *)OpenLibrary("images/bevel.image", 44L))
  {
    if(TextEditor = MakeClass(NULL, GADGETCLASS, NULL, sizeof(mydata), 0L))
    {
      TextEditor->cl_Dispatcher.h_Entry = (APTR)Dispatcher;
      return;
    }
  }
  abortLibInit();
}

VOID EXIT_7_ExpungeLib ()
{
  if(BevelBase)
  {
    if(TextEditor)
      FreeClass(TextEditor);
    CloseLibrary((struct Library *)BevelBase);
  }
}

struct IClass *MCC_Query (REG(a6) LibBase *base)
{
  return TextEditor;
}
