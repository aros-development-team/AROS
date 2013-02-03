/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2006 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/io.h>
#include <libraries/dos.h>
#include <libraries/dosextens.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <workbench/workbench.h>
#include <intuition/intuition.h>
#include <intuition/classusr.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#if !defined(__amigaos4__)
#include <clib/alib_protos.h>
#endif

struct Library *MUIMasterBase = NULL;
#if defined(__AROS__)
struct UtilityBase *UtilityBase = NULL;
#else
struct Library *UtilityBase = NULL;
#endif

#if defined(__amigaos4__)
struct Library *IntuitionBase = NULL;
struct Library *GfxBase = NULL;
#else
struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;
#endif

#if defined(__amigaos4__)
struct GraphicsIFace *IGraphics = NULL;
struct IntuitionIFace *IIntuition = NULL;
struct MUIMasterIFace *IMUIMaster = NULL;
struct UtilityIFace *IUtility = NULL;
#endif

#include <mui/NListview_mcc.h>
#include <mui/NFloattext_mcc.h>

#include "NList-Demo2.h"
#include "NList-Demo3.h"

#include "SDI_compiler.h"

/* *********************************************** */


static APTR APP_Main,
            WIN_Main,
            WIN_2,
            BM_img,
            BM_img2,
            BT_TitleOn,
            BT_TitleOff,
            BT_NoMulti,
            BT_Multidef,
            BT_Multi,
            BT_AllMulti,
            BT_InputOn,
            BT_InputOff,
            BT_Sort,
            BT_DragSortOn,
            BT_DragSortOff,
            BT_SelLine,
            BT_SelChar,
            BT_OpenWin2,
            BT_RemAct,
            BT_RemSel,
            BT_Clear,
            BT_Add,
            LV_Text,
            LI_Text,
            LV_Text2,
            LI_Text2,
            PR_Horiz,
            ST_string;


/* ************ IMG definitions ************** */

const ULONG list_colors[24] =
{
0xadadadad,0xadadadad,0xadadadad,
0x7b7b7b7b,0x7b7b7b7b,0x7b7b7b7b,
0x3b3b3b3b,0x67676767,0xa2a2a2a2,
0xadadadad,0xadadadad,0xadadadad,
0xadadadad,0xadadadad,0xadadadad,
0xadadadad,0xadadadad,0xadadadad,
0xffffffff,0xffffffff,0xffffffff,
0x00000000,0x00000000,0x00000000,
};

const UBYTE list_body[168] = {
0x00,0x00,0x44,0x00,0xff,0xff,0xb8,0x00,0xff,0xff,0xb8,0x00,0x00,0x00,0x00,
0x00,0x80,0x00,0x64,0x00,0xff,0xff,0xb8,0x00,0x3b,0xb8,0x46,0x00,0x80,0x00,
0x64,0x00,0xc4,0x47,0xfc,0x00,0x00,0x00,0x46,0x00,0x80,0x00,0x64,0x00,0xff,
0xff,0xfc,0x00,0x36,0xd8,0x46,0x00,0x80,0x00,0x64,0x00,0xc9,0x27,0xfc,0x00,
0x00,0x00,0x46,0x00,0x80,0x00,0x64,0x00,0xff,0xff,0xfc,0x00,0x3d,0xe8,0x6e,
0x00,0x80,0x00,0x5c,0x00,0xc2,0x17,0xcc,0x00,0x00,0x00,0x46,0x00,0x80,0x00,
0x78,0x00,0xff,0xff,0xf8,0x00,0x00,0x00,0x40,0x00,0x80,0x00,0x64,0x00,0xff,
0xff,0xf8,0x00,0xbf,0xff,0xee,0x00,0x7f,0xff,0xdc,0x00,0x3f,0xff,0xcc,0x00,
0x00,0x44,0x46,0x00,0xff,0xbb,0xb8,0x00,0xff,0xbb,0xb8,0x00,0x00,0x00,0x00,
0x00,0x80,0x66,0x64,0x00,0xff,0xbb,0xb8,0x00,0xbf,0xee,0xee,0x00,0x7f,0xdd,
0xdc,0x00,0x3f,0xcc,0xcc,0x00,0x3f,0xee,0xee,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00, };

#define LIST_WIDTH        23
#define LIST_HEIGHT       14
#define LIST_DEPTH         3
#define LIST_COMPRESSION   0
#define LIST_MASKING       2

#define IMG \
  BodychunkObject,\
    MUIA_FixWidth             , LIST_WIDTH ,\
    MUIA_FixHeight            , LIST_HEIGHT,\
    MUIA_Bitmap_Width         , LIST_WIDTH ,\
    MUIA_Bitmap_Height        , LIST_HEIGHT,\
    MUIA_Bodychunk_Depth      , LIST_DEPTH ,\
    MUIA_Bodychunk_Body       , (UBYTE *) list_body,\
    MUIA_Bodychunk_Compression, LIST_COMPRESSION,\
    MUIA_Bodychunk_Masking    , LIST_MASKING,\
    MUIA_Bitmap_SourceColors  , (ULONG *) list_colors,\
    MUIA_Bitmap_Transparent   , 0,\
  End


/* ************ IMG2 definitions ************** */

const ULONG list2_colors[24] =
{
0xabababab,0xadadadad,0xc5c5c5c5,
0x7b7b7b7b,0x7b7b7b7b,0x7b7b7b7b,
0x3b3b3b3b,0x67676767,0xa2a2a2a2,
0x00000000,0x00000000,0x00000000,
0xffffffff,0xa9a9a9a9,0x97979797,
0xffffffff,0xffffffff,0xffffffff,
0x00000000,0x00000000,0x00000000,
0xadadadad,0xadadadad,0xadadadad,
};

const UBYTE list2_body[156] = {
0x04,0x00,0x04,0x00,0x78,0x00,0x08,0x00,0x30,0x00,0x08,0x00,0x79,0x00,0x02,
0x00,0x8e,0x00,0x1c,0x00,0x3e,0x00,0x1c,0x00,0x7e,0x10,0x04,0x00,0x83,0xe0,
0x08,0x00,0xfe,0x80,0x08,0x00,0x45,0x02,0x00,0x00,0x85,0x9c,0x00,0x00,0xff,
0x70,0x00,0x00,0x7e,0x1c,0x80,0x00,0xc1,0x3b,0x78,0x00,0xfe,0xf9,0x30,0x00,
0x00,0x2f,0xb0,0x00,0xff,0x60,0x4c,0x00,0x7e,0xbf,0x48,0x00,0x7c,0x27,0x68,
0x00,0x03,0xc1,0xa4,0x00,0x01,0x7f,0xb4,0x00,0x01,0x1a,0xd8,0x00,0x03,0xe3,
0xa4,0x00,0x01,0xbf,0xac,0x00,0x00,0x6c,0xaa,0x00,0x04,0x99,0xd4,0x00,0x03,
0x97,0xfc,0x00,0x01,0x76,0x5a,0x00,0x38,0x8d,0xec,0x00,0x1e,0x8b,0x7c,0x00,
0x02,0xeb,0x72,0x00,0x0d,0x04,0xfc,0x00,0x09,0x06,0xf8,0x00,0x01,0x84,0x06,
0x00,0x0e,0x02,0xf8,0x00,0x06,0x01,0xb0,0x00,0x00,0x03,0xfc,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00, };

#define LIST2_WIDTH        23
#define LIST2_HEIGHT       13
#define LIST2_DEPTH         3
#define LIST2_COMPRESSION   0
#define LIST2_MASKING       2

#define IMG2 \
  BodychunkObject,\
    MUIA_FixWidth             , LIST2_WIDTH ,\
    MUIA_FixHeight            , LIST2_HEIGHT,\
    MUIA_Bitmap_Width         , LIST2_WIDTH ,\
    MUIA_Bitmap_Height        , LIST2_HEIGHT,\
    MUIA_Bodychunk_Depth      , LIST2_DEPTH ,\
    MUIA_Bodychunk_Body       , (UBYTE *) list2_body,\
    MUIA_Bodychunk_Compression, LIST2_COMPRESSION,\
    MUIA_Bodychunk_Masking    , LIST2_MASKING,\
    MUIA_Bitmap_SourceColors  , (ULONG *) list2_colors,\
    MUIA_Bitmap_Transparent   , 0,\
  End


/* *********************************************** */


#define ID_LIST2_ACTIVE 1

#define NL "\n"
#define NL2 " "


/* *********************************************** */

const char MainTextString2[] =
{
  "This new list/listview custom class has its own backgrounds" NL2
  "and pens setables from mui prefs with NListviews.mcp !" NL2
  "It doesn't use the same way to handle multiselection" NL2
  "with mouse and keys than standard List/Listview." NL
  "\033C" NL
  "You can horizontally scroll with cursor keys," NL2
  "or going on the right and left of the list" NL2
  "while selecting with the mouse." NL2
  "(When there is no immediate draggin stuff active...)" NL2
  "Try just clicking on the left/right borders !" NL
  "\033C" NL
  "\033r\033uGive some feedback about it ! :-)" NL
  "\033r\033bhttp://www.sf.net/projects/nlist-classes/"
};

/* *********************************************** */
const char MainTextString[] =
{
  "\033cNList.mcc \033o[0] NListview.mcc \033o[1] NListviews.mcp" NL
  "\033E\033t\033c\033o[2@1]" NL
  "\033c\033nThis new list/listview custom class has its own backgrounds" NL
  "\033cand pens setables from mui prefs with NListviews.mcp !" NL
  "\033E\033t[M7]\033c\033o[3@2]" NL
  "" NL
  "\033cIt doesn't use the same way to handle multiselection" NL
  "\033cwith mouse and keys than standard List/Listview." NL
  "" NL
  "\033cYou can horizontally scroll with cursor keys," NL
  "\033cor going on the right and left of the list" NL
  "\033cwhile selecting with the mouse." NL
  "\033c(When there is no immediate draggin stuff active...)" NL
  "\033cTry just clicking on the left/right borders !" NL
  "" NL
  "\033r\033uGive some feedback about it ! :-)" NL
  "\033r\033bhttp://www.sf.net/projects/nlist-classes/"
};

/* *********************************************** */

const char *MainTextArray[] =
{
  "\033c\033nThis new list/listview custom class has its own backgrounds",
  "\033cand pens setables from mui prefs with NListviews.mcp !",
  " ",
  "\033cIt doesn't use the same way to handle multiselection",
  "\033cwith mouse and keys than standard List/Listview.",
  " ",
  "\033cYou can horizontally scroll with cursor keys,",
  "\033cor going on the right and left of the list",
  "\033cwhile selecting with the mouse.",
  "\033c(When there is no draggin stuff active...)",
  "\033cTry just clicking on the left/right borders !",
  "",
  "\033r\033uGive some feedback about it ! :-)",
  "\033r\033bhttp://www.sf.net/projects/nlist-classes/"
  "",
  "",
  "",
  "You can push column titles as real buttons...",
  "",
  "You can drag column separator bar in",
  "the title to adjust column width...",
  "(or in upper half of first entry if title is off)",
  "",
  "If, while dragging the bar, you press the",
  "menu button, the width of the column will",
  "come back to its default...",
  "",
  "You can click on columns title to sort the list contents.",
  "You can shift-click (or other qualifier if you modify the",
  "default one) to add a secondary sorting column.",
  "",
  "",
  "",
  "\033cF2 to copy selected lines to printer",
  "\033cF3 to copy all lines to file RAM:tmp.txt",
  "",
  "\033cF4 to copy selected lines to clipboard 0.",
  "\033cF5 to copy active line to clipboard 0.",
  "\033cF6 to copy all lines to clipboard 0.",
  "",
  "\033cThe classic RightAmiga+C to copy selected",
  "\033clines to clipboard 0 is made builtin.",
  " ",
  " ",
  "\033cYou can sort the entries by dragging.",
  "\033cYou start a drag using one of the qualifier",
  "\033ckeys which are set in prefs,",
  "\033cor going on the left and right of entries.",
  "",
  "",
  "Example of horizontal line in top of entry",
  "\033TExample of horizontal line in top of entry",
  "Example of horizontal line in top of entry",
  "",
  "Examples of horizontal line and thick line centered on entry",
  "\033C",
  "\033C\033t",
  "\033C\033t[I7]",
  "\033C\033t[M7]",
  "\033C\033t[-5]",
  "\033C\033t[N]",
  "\033C\033t[NI3]",
  "\033C\033t[NM7]",
  "\033C\033t[N-5]",
  "Examples of horizontal line and thick line centered on entry",
  "",
  "Example of horizontal line in bottom of entry",
  "\033BExample of horizontal line in bottom of entry",
  "Example of horizontal line in bottom of entry",
  "",
  "Examples of horizontal line centered on entry visible on left and rigth only",
  "\033c\033EExample of left-right horizontal line",
  "\033ESame, but left aligned",
  "\033r\033ESame, but right aligned",
  "\033t\033c\033ELeft-right horizontal thick line",
  "\033t[N]\033c\033ELeft-right horizontal thick line",
  "Examples of horizontal line centered on entry visible on left and rigth only",
  "",
  "",
  "\033E\033cExamples of bitmap images :",
  "\033c\033o[0]  \033o[1]",
  "which can be used without subclassing the NList class.",
  "",
  "\033E\033cExamples of custom object images : ",
  "\033o[2;9d51ffff;1]  (default width/special 1)",
  "\033o[2;9d51ffff;0,24] (24 pixel width/special 0)",
  "\033o[2;9d51ffff;1,48] (48 pixel width/special 1)",
  "\033o[2;9d51ffff;0,96] (96 pixel width/special 0)",
  "",
  "\033E\033cExamples of custom object images : ",
  "\033o[2;9d51ffff;8,30]  (default width/special 8)",
  "\033o[2;9d51ffff;6,40]  (default width/special 6)",
  "\033o[2;9d51ffff;7,40] (40 pixel width/special 7)",
  "\033o[2;9d51ffff;4,50] (50 pixel width/special 4)",
  "\033o[2;9d51ffff;5,50] (50 pixel width/special 5)",
  "\033o[2;9d51ffff;2,30] (30 pixel width/special 2)",
  "\033o[2;9d51ffff;3,30] (30 pixel width/special 3)",
  "",
  "\033E\033cExamples of direct ImageSpec :",
  "kmel/kmel_arrowdown.image :    \033I[3:kmel/kmel_arrowdown.image]",
  "WD/11pt/ArrowLeft.mf0 :    \033I[3:WD/11pt/ArrowLeft.mf0]",
  "",
  "\033C",
  "",
  "ww\tTabulation test",
  "w\tTabulation test",
  "\033cww\tTabulation test",
  "\033cw\tTabulation test",
  "\033rww\tTabulation test",
  "\033rw\tTabulation test",
  "",
  "",
  "long line for FULLAUTO horizontal scroller test, long line for FullAuto horizontal scroller test, long line for FULLAUTO horizontal scroller test.",
  "",
  "0 just a little \033bline to test\033n stuffs",
  "1 just a little \033bline to test\033n stuffs",
  "2 just a little \033bline to test\033n stuffs",
  "3 just a little \033bline to test\033n stuffs",
  "\033c4 just a little \033uline to test\033n (center)",
  "\033c5 just a little \033uline to test\033n (center)",
  "\033c6 just a little \033uline to test\033n (center)",
  "\033r7 just a little \033uline to test\033n (right)",
  "\033r8 just a little \033uline to test\033n (right)",
  "\033r9 just a little \033uline to test\033n (right)",
  "10 just a little line to test stuffs and bugs",
  "11 just a little line to test stuffs and bugs",
  "12 just a little line to test stuffs and bugs",
  "13 just a little line to test stuffs and bugs",
  "14 just a little line to test stuffs and bugs",
  "15 just a little \033iline to test stuffs\033n and bugs",
  "16 just a little line to test stuffs and bugs",
  "17 just a little line to test stuffs and bugs",
  "18 just a little line to test stuffs and bugs",
  "19 just a little line to test stuffs and bugs",
  "\033c20 just a little line to test stuffs and bugs (center)",
  "\033c21 just a little line to test stuffs and bugs (center)",
  "\033c22 just a little line to test stuffs and bugs (center)",
  "\033c23 just a little line to test stuffs and bugs (center)",
  "24 just a little line to test stuffs and bugs, just a little line",
  "25 just a little line to test stuffs and bugs, just a little line",
  "\033r26 just a little line to test stuffs and bugs (right)",
  "\033r27 just a little line to test stuffs and bugs (right)",
  "\033r28 just a little line to test stuffs and bugs (right)",
  "\033r29 just a little line to test stuffs and bugs (right)",
  "30 just a little line to test stuffs and bugs, just a little line to test",
  "31 just a little line to test stuffs and bugs, just a little line to test",
  "32 \0332just a little line to test stuffs and bugs, just a little line to test",
  "33 \0333just a little line to test stuffs and bugs, just a little line to test",
  "34 \0334just a little line to test stuffs and bugs, just a little line to test",
  "35 \0335just a little line to test stuffs and bugs, just a little line to test",
  "36 \0336just a little line to test stuffs and bugs, just a little line to test",
  "37 \0337just a little line to test stuffs and bugs, just a little line to test",
  "38 \0338just a little line to test stuffs and bugs, just a little line to test",
  "39 \0339just a little line to test stuffs and bugs, just a little line to test",
  "40 \033P[]just a little line to test stuffs and bugs, just a little line to test stuffs ",
  "41 \033P[1]just a little line to test\033P[] stuffs and bugs, just a little line to test stuffs ",
  "42 \033P[2]just a little line to test\033P[] stuffs and bugs, just a little line to test stuffs ",
  "43 \033P[-1]just a little line to test\033P[] stuffs and bugs, just a little line to test stuffs ",
  "44 \033P[-3]just a little line to test\033P[] stuffs and bugs, just a little line to test stuffs ",
  "45 just a little line to test stuffs and bugs, just a little line to test stuffs and bugs",
  "46 just a little line to test stuffs and bugs, just a little line to test stuffs and bugs",
  "47 just a little line to test stuffs and bugs, just a little line to test stuffs and bugs",
  "48 just a little line to test stuffs and bugs, just a little line to test stuffs and bugs",
  "49 just a little line to test stuffs and bugs, just a little line to test stuffs and bugs",
  "50 \033ijust a little line to test stuffs and bugs, just a little line to test stuffs and bugs",
  "51 \033ujust a little line to test stuffs and bugs, just a little line to test stuffs and bugs",
  "52 \033bjust a little line to test stuffs and bugs, just a little line to test stuffs and bugs",
  "53 \033i\033ujust a little line to test stuffs and bugs, just a little line to test stuffs and bugs",
  "54 \033i\033bjust a little line to test stuffs and bugs, just a little line to test stuffs and bugs",
  "55 \033b\033ujust a little line to test stuffs and bugs, just a little line to test, just a little line to test, just a little line to test",
  "56 \033i\033b\033ujust a little line to test stuffs and bugs, just a little line to test, just a little line to test, just a little line to test",
  "57 just a little line to test stuffs and bugs, just a little line to test, just a little line to test, just a little line to test",
  "58 just a little line to test stuffs and bugs, just a little line to test, just a little line to test, just a little line to test",
  "59 just a little line to test stuffs and bugs, just a little line to test, just a little line to test, just a little line to test",
  " ",
  " ",
  "\033c\033uI find it \033bnice\033n  :-)",
  NULL
};

/* *********************************************** */

#if defined(__amigaos4__)
#define GETINTERFACE(iface, base)	(iface = (APTR)GetInterface((struct Library *)(base), "main", 1L, NULL))
#define DROPINTERFACE(iface)			(DropInterface((struct Interface *)iface), iface = NULL)
#else
#define GETINTERFACE(iface, base)	TRUE
#define DROPINTERFACE(iface)
#endif

static VOID fail(APTR APP_Main,const char *str)
{
  if (APP_Main)
    MUI_DisposeObject(APP_Main);
  NLI_Delete();

  if(MUIMasterBase)
  {
    DROPINTERFACE(IMUIMaster);
    CloseLibrary(MUIMasterBase);
  }

  if(IntuitionBase)
  {
    DROPINTERFACE(IIntuition);
    CloseLibrary((struct Library *)IntuitionBase);
  }

  if(GfxBase)
  {
    DROPINTERFACE(IGraphics);
    CloseLibrary((struct Library *)GfxBase);
  }

  if(UtilityBase)
  {
    DROPINTERFACE(IUtility);
    CloseLibrary((struct Library *)UtilityBase);
  }

  if (str)
  { puts(str);
    exit(20);
  }
  exit(0);
}

static VOID init(VOID)
{
  APP_Main = NULL;

  if((UtilityBase = (APTR)OpenLibrary("utility.library", 36)) &&
     GETINTERFACE(IUtility, UtilityBase))
  {
    if((GfxBase = (APTR)OpenLibrary("graphics.library", 36)) &&
       GETINTERFACE(IGraphics, GfxBase))
    {
      if((IntuitionBase = (APTR)OpenLibrary("intuition.library", 36)) &&
         GETINTERFACE(IIntuition, IntuitionBase))
      {
        if((MUIMasterBase = OpenLibrary("muimaster.library", 19)) &&
           GETINTERFACE(IMUIMaster, MUIMasterBase))
        {

          if(!NLI_Create())
            fail(NULL,"Failed to create NLI class.");
        }
        else
          fail(NULL,"Failed to open muimaster.library.");
      }
      else
        fail(NULL,"Failed to open intuition.library.");
    }
    else
      fail(NULL,"Failed to open graphics.library.");
  }
  else
    fail(NULL,"Failed to open utility.library.");

}


/* *********************************************** */


int main(UNUSED int argc, UNUSED char *argv[])
{
  LONG win_opened = 0;
  LONG result;
  static const char *const UsedClasses[] = { 
    "NList.mcc",
    "NListviews.mcc",
    NULL
  };

  init();

  APP_Main = ApplicationObject,
    MUIA_Application_Title      , "NList-Demo",
    MUIA_Application_Version    , "$VER: NList-Demo 1.0 (" __DATE__ ")",
    MUIA_Application_Copyright  , "Copyright (C) 2001-2006 by NList Open Source Team",
    MUIA_Application_Author     , "NList Open Source Team",
    MUIA_Application_Description, "NList-Demo",
    MUIA_Application_Base       , "NList-Demo",
    MUIA_Application_UsedClasses, UsedClasses,

    SubWindow, WIN_Main = WindowObject,
      MUIA_Window_Title, "NList-Demo 1996-2006",
      MUIA_Window_ID   , MAKE_ID('T','W','I','N'),
      WindowContents, VGroup,
        Child, HGroup,
          Child, BT_InputOn = SimpleButtonCycle("InputOn"),
          Child, BT_InputOff = SimpleButtonCycle("InputOff"),
          Child, BT_SelLine = SimpleButtonCycle("SelLine"),
          Child, BT_SelChar = SimpleButtonCycle("SelChar"),
          Child, BT_Clear = SimpleButtonCycle("Clear"),
          Child, BT_Add = SimpleButtonCycle("Add"),
        End,
        Child, HGroup,
          Child, BT_NoMulti = SimpleButtonCycle("NoMulti"),
          Child, BT_Multidef = SimpleButtonCycle("Def"),
          Child, BT_Multi = SimpleButtonCycle("Multi"),
          Child, BT_AllMulti = SimpleButtonCycle("AllMulti"),
          Child, BM_img = IMG,
          Child, BT_RemAct = SimpleButtonCycle("RemAct"),
          Child, BT_RemSel = SimpleButtonCycle("RemSel"),
        End,
        Child, HGroup,
          Child, BT_DragSortOn = SimpleButtonCycle("DragSortOn"),
          Child, BT_DragSortOff = SimpleButtonCycle("DragSortOff"),
          Child, BM_img2 = IMG2,
          Child, BT_TitleOn = SimpleButtonCycle("TitleOn"),
          Child, BT_TitleOff = SimpleButtonCycle("TitleOff"),
          Child, BT_Sort = SimpleButtonCycle("Sort"),
          Child, BT_OpenWin2 = SimpleButtonCycle("Win2"),
        End,
        Child, LV_Text = NListviewObject,
          MUIA_CycleChain, 1,

          MUIA_NListview_NList, LI_Text = NListObject,
            MUIA_ObjectID, MAKE_ID('N','L','0','1'),
            MUIA_NList_DefaultObjectOnClick, TRUE,
            MUIA_NList_MultiSelect, MUIV_NList_MultiSelect_None,
            MUIA_NList_DisplayHook2, &DisplayLI_TextHook,
            MUIA_NList_CompareHook2, &CompareLI_TextHook,
            MUIA_NList_ConstructHook2, &ConstructLI_TextHook,
            MUIA_NList_DestructHook2, &DestructLI_TextHook,
            MUIA_NList_Format, "BAR W=-1,BAR W=-1,BAR",
            MUIA_NList_SourceArray, MainTextArray,
            MUIA_NList_AutoVisible, TRUE,
            MUIA_NList_TitleSeparator, TRUE,
            MUIA_NList_Title, TRUE,
            MUIA_NList_EntryValueDependent, TRUE,
            MUIA_NList_MinColSortable, 0,
            MUIA_NList_Imports, MUIV_NList_Imports_All,
            MUIA_NList_Exports, MUIV_NList_Exports_All,
          End,
          MUIA_ShortHelp, "The nice multicolumn\ndraggable list\nwith char selection\npossibility :)",
        End,
      End,
    End,
    SubWindow, WIN_2 = WindowObject,
      MUIA_Window_Title, "NList-Demo 1996-2006 Win 2",
      MUIA_Window_ID   , MAKE_ID('W','I','N','2'),
      MUIA_Window_UseBottomBorderScroller, TRUE,
      WindowContents, VGroup,
        Child, NFloattext(MainTextString2),
        Child, BalanceObject, End,
        Child, VGroup,
          MUIA_Group_VertSpacing, 1,
          Child, LV_Text2 = NListviewObject,
            MUIA_CycleChain, 1,
            MUIA_NListview_Horiz_ScrollBar, MUIV_NListview_HSB_None,
            MUIA_NListview_NList, LI_Text2 = NListObject,
              MUIA_NList_DefaultObjectOnClick, TRUE,
              MUIA_NList_ConstructHook, MUIV_NList_ConstructHook_String,
              MUIA_NList_DestructHook, MUIV_NList_DestructHook_String,
              MUIA_NList_SourceString, MainTextString,
            End,
            MUIA_ShortHelp, "The 2nd list",
          End,
          Child, ST_string = StringObject,
            MUIA_CycleChain, 1,
            StringFrame,
          End,
          Child, PR_Horiz = ScrollbarObject,
            MUIA_Group_Horiz, TRUE,
            MUIA_Prop_UseWinBorder,MUIV_Prop_UseWinBorder_Bottom,
          End,
        End,
      End,
    End,
  End;

  if(!APP_Main) fail(APP_Main,"Failed to create Application.");


  /* ********** MAIN LIST NOTIFIES ********** */

  DoMethod(LI_Text, MUIM_Notify, MUIA_NList_TitleClick,MUIV_EveryTime,
    LI_Text, 4, MUIM_NList_Sort3, MUIV_TriggerValue, MUIV_NList_SortTypeAdd_2Values, MUIV_NList_Sort3_SortType_Both);
  DoMethod(LI_Text, MUIM_Notify, MUIA_NList_TitleClick2,MUIV_EveryTime,
    LI_Text, 4, MUIM_NList_Sort3, MUIV_TriggerValue, MUIV_NList_SortTypeAdd_2Values, MUIV_NList_Sort3_SortType_2);
  DoMethod(LI_Text, MUIM_Notify, MUIA_NList_SortType,MUIV_EveryTime,
    LI_Text, 3, MUIM_Set,MUIA_NList_TitleMark,MUIV_TriggerValue);
  DoMethod(LI_Text, MUIM_Notify, MUIA_NList_SortType2,MUIV_EveryTime,
    LI_Text, 3, MUIM_Set,MUIA_NList_TitleMark2,MUIV_TriggerValue);


  /* ********** MAIN LIST IMAGES ********** */

  DoMethod(LI_Text,MUIM_NList_UseImage,BM_img,0,0);
  DoMethod(LI_Text,MUIM_NList_UseImage,BM_img2,1,0);

  DoMethod(LI_Text,MUIM_NList_UseImage,NImageObject,2,~0L);


  /* ********** BUTTONS NOTIFIES ********** */

  DoMethod(BT_TitleOn, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 3, MUIM_Set,MUIA_NList_Title,"\033cThis is that list title !   :-)");
  DoMethod(BT_TitleOff, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 3, MUIM_Set,MUIA_NList_Title,NULL);
  DoMethod(BT_NoMulti, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 3, MUIM_Set,MUIA_NList_MultiSelect,MUIV_NList_MultiSelect_None);
  DoMethod(BT_Multidef, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 3, MUIM_Set,MUIA_NList_MultiSelect,MUIV_NList_MultiSelect_Default);
  DoMethod(BT_Multi, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 3, MUIM_Set,MUIA_NList_MultiSelect,MUIV_NList_MultiSelect_Shifted);
  DoMethod(BT_AllMulti, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 3, MUIM_Set,MUIA_NList_MultiSelect,MUIV_NList_MultiSelect_Always);
  DoMethod(BT_InputOn, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 3, MUIM_Set,MUIA_NList_Input,TRUE);
  DoMethod(BT_InputOff, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 3, MUIM_Set,MUIA_NList_Input,FALSE);
  DoMethod(BT_DragSortOn, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 3, MUIM_Set,MUIA_NList_DragSortable,TRUE);
  DoMethod(BT_DragSortOff, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 3, MUIM_Set,MUIA_NList_DragSortable,FALSE);
  DoMethod(BT_RemAct, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 2, MUIM_NList_Remove,MUIV_NList_Remove_Active);
  DoMethod(BT_RemSel, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 2, MUIM_NList_Remove,MUIV_NList_Remove_Selected);
  DoMethod(BT_Clear, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 1, MUIM_NList_Clear);
  DoMethod(BT_Add, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 4, MUIM_NList_Insert,MainTextArray,15,MUIV_NList_Insert_Bottom);
  DoMethod(BT_Sort, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 1, MUIM_NList_Sort);
  DoMethod(BT_SelLine, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 3, MUIM_Set,MUIA_NList_TypeSelect,MUIV_NList_TypeSelect_Line);
  DoMethod(BT_SelChar, MUIM_Notify, MUIA_Pressed,FALSE,
    LI_Text, 3, MUIM_Set,MUIA_NList_TypeSelect,MUIV_NList_TypeSelect_Char);
  DoMethod(BT_OpenWin2, MUIM_Notify, MUIA_Pressed,FALSE,
    WIN_2, 3, MUIM_Set,MUIA_Window_Open,TRUE);


  /* ********** WINDOW1 KEYS NOTIFIES ********** */

  DoMethod(WIN_Main,MUIM_Notify, MUIA_Window_InputEvent, "f2",
    LI_Text, 4,MUIM_NList_CopyTo,MUIV_NList_CopyTo_Selected,"PRT:",&result,NULL);
  DoMethod(WIN_Main,MUIM_Notify, MUIA_Window_InputEvent, "f3",
    LI_Text, 4,MUIM_NList_CopyTo,MUIV_NList_CopyTo_All,"RAM:tmp.txt",&result,NULL);
  DoMethod(WIN_Main,MUIM_Notify, MUIA_Window_InputEvent, "f4",
    LI_Text, 4,MUIM_NList_CopyToClip,MUIV_NList_CopyToClip_Selected,0L,NULL,NULL);
  DoMethod(WIN_Main,MUIM_Notify, MUIA_Window_InputEvent, "f5",
    LI_Text, 4,MUIM_NList_CopyToClip,MUIV_NList_CopyToClip_Active,0L,NULL,NULL);
  DoMethod(WIN_Main,MUIM_Notify, MUIA_Window_InputEvent, "f6",
    LI_Text, 4,MUIM_NList_CopyToClip,MUIV_NList_CopyToClip_All,0L,NULL,NULL);


  /* ********** WINDOW2 LIST NOTIFIES ********** */

  DoMethod(LI_Text2,MUIM_Notify,MUIA_NList_ButtonClick, 1,
    APP_Main, 6, MUIM_Application_PushMethod,
    WIN_2, 3, MUIM_Set,MUIA_Window_Open,FALSE);

  DoMethod(LI_Text2,MUIM_Notify,MUIA_NList_ButtonClick, 2,
    LI_Text, 1, MUIM_NList_Clear);


  DoMethod(LI_Text2, MUIM_Notify, MUIA_NList_Horiz_Entries,MUIV_EveryTime,
    PR_Horiz, 3, MUIM_NoNotifySet,MUIA_Prop_Entries,MUIV_TriggerValue);
  DoMethod(LI_Text2, MUIM_Notify, MUIA_NList_Horiz_Visible,MUIV_EveryTime,
    PR_Horiz, 3, MUIM_NoNotifySet,MUIA_Prop_Visible,MUIV_TriggerValue);
  DoMethod(LI_Text2, MUIM_Notify, MUIA_NList_Horiz_First,MUIV_EveryTime,
    PR_Horiz, 3, MUIM_NoNotifySet,MUIA_Prop_First,MUIV_TriggerValue);
  DoMethod(PR_Horiz, MUIM_Notify, MUIA_Prop_First,MUIV_EveryTime,
    LI_Text2, 3, MUIM_NoNotifySet,MUIA_NList_Horiz_First,MUIV_TriggerValue);
  DoMethod(LI_Text2, MUIM_Notify, MUIA_NList_HorizDeltaFactor,MUIV_EveryTime,
    PR_Horiz, 3, MUIM_NoNotifySet,MUIA_Prop_DeltaFactor,MUIV_TriggerValue);

  set(ST_string, MUIA_String_AttachedList, LI_Text2);
  DoMethod(LI_Text2,MUIM_Notify,MUIA_NList_SelectChange,TRUE,APP_Main,2,MUIM_Application_ReturnID,ID_LIST2_ACTIVE);
  DoMethod(LI_Text2,MUIM_Notify,MUIA_NList_Active,MUIV_EveryTime,APP_Main,2,MUIM_Application_ReturnID,ID_LIST2_ACTIVE);
  set(LI_Text2, MUIA_NList_Active, 2);


  /* ********** WINDOW2 LIST IMAGES ********** */

  DoMethod(LI_Text2,MUIM_NList_UseImage,BM_img,0,0);
  DoMethod(LI_Text2,MUIM_NList_UseImage,BM_img2,1,0);

  DoMethod(LI_Text2,MUIM_NList_UseImage,SimpleButtonTiny("Click me to close"),2,~0L);
  DoMethod(LI_Text2,MUIM_NList_UseImage,SimpleButtonTiny("Clear main list"),3,~0L);


  /* ********** WINDOW2 NOTIFIES ********** */

  DoMethod(WIN_Main,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,
    APP_Main, 5, MUIM_Application_PushMethod,
    APP_Main,2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);

  DoMethod(WIN_2,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,
    APP_Main, 6, MUIM_Application_PushMethod,
    WIN_2, 3, MUIM_Set,MUIA_Window_Open,FALSE);

  set(WIN_Main, MUIA_Window_DefaultObject, LV_Text);
  set(WIN_Main, MUIA_Window_ActiveObject, LV_Text);
  set(WIN_2, MUIA_Window_ActiveObject, ST_string);
  set(WIN_2, MUIA_Window_DefaultObject, LV_Text2);


  /* ************************************** */

  DoMethod(APP_Main,MUIM_Application_Load,MUIV_Application_Load_ENVARC);

  /* *** If need to be sorted, sort then restore the active and first *** */
  /* ***            which were set in the Application_Load            *** */
  {
    ULONG active = 0,first = 0,sorttype = 0;
    get(LI_Text,MUIA_NList_SortType,&sorttype);
    if (sorttype != MUIV_NList_SortType_None)
    { get(LI_Text,MUIA_NList_Active,&active);
      get(LI_Text,MUIA_NList_First,&first);
      DoMethod(LI_Text, MUIM_NList_Sort);
      set(LI_Text,MUIA_NList_Active,active);
      set(LI_Text,MUIA_NList_First,first);
    }
  }

  set(WIN_2,MUIA_Window_Open,TRUE);
  set(WIN_Main,MUIA_Window_Open,TRUE);

  get(WIN_Main,MUIA_Window_Open,&win_opened);
  if (win_opened)
  {
    LONG id;
    ULONG sigs = 0;
    char *line;

    while ((id = DoMethod(APP_Main,MUIM_Application_NewInput,&sigs)) != (LONG)MUIV_Application_ReturnID_Quit)
    {
      if (id == ID_LIST2_ACTIVE)
      { DoMethod(LI_Text2, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, &line);
        set(ST_string, MUIA_String_Contents, line);
        set(WIN_2, MUIA_Window_ActiveObject, ST_string);
      }
      if (sigs)
      { sigs = Wait(sigs | SIGBREAKF_CTRL_C);
        if (sigs & SIGBREAKF_CTRL_C) break;
      }
    }
  }
  else
    printf("failed to open main window !\n");

  DoMethod(APP_Main,MUIM_Application_Save,MUIV_Application_Save_ENVARC);

  set(WIN_2,MUIA_Window_Open,FALSE);
  set(WIN_Main,MUIA_Window_Open,FALSE);

  DoMethod(LI_Text,MUIM_NList_UseImage,NULL,-1,0);
  DoMethod(LI_Text2,MUIM_NList_UseImage,NULL,-1,0);

  fail(APP_Main,NULL);

  return 0;
}

