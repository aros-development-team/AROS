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

 $Id: CA_TextEditor.c,v 1.1 2005/03/28 11:29:49 damato Exp $

***************************************************************************/

#include <StdIO.h>
#include <CLib/alib_protos.h>
#include <Proto/DOS.h>
#include <Proto/Exec.h>
#include <Proto/Intuition.h>
#include <Proto/Layout.h>
#include <Proto/Window.h>
#include <Proto/Scroller.h>

#include <Intuition/ClassUsr.h>
#include <Intuition/GadgetClass.h>
#include <Intuition/ICClass.h>

#include <Classes/Window.h>
#include <Gadgets/Layout.h>
#include <Gadgets/Scroller.h>

#include <Editor.h>
#include <TextEditor_mcc.h>

extern ULONG Dispatcher (REG(a0) struct IClass *cl, REG(a2) Object *obj, REG(a1) Msg msg);

struct ClassLibrary *WindowBase;
struct ClassLibrary *LayoutBase;
struct ClassLibrary *BevelBase;
struct ClassLibrary *ScrollerBase;

void main (void)
{
    struct IClass *cl;
    struct Object *obj;
    struct Object *prop;

  BevelBase = (struct ClassLibrary *)OpenLibrary("images/bevel.image",0L);
  if(BevelBase && (cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(mydata), 0L)))
  {
    cl->cl_Dispatcher.h_Entry = (APTR)Dispatcher;
    if(obj = NewObject(cl, NULL, MUIA_TextEditor_Contents,
            "\33r\33b" __DATE2__ "\33n\n"
            "\n\33cTextEditor.gadget V15.0ß\n"
            "Copyright 1997 by Allan Odgaard\n"
            "\33l\n\33[s:9]\n"
            "For feedback write to: Duff@DIKU.DK\n"
            "For the latest version, try: \33uhttp://www.DIKU.dk/students/duff/texteditor/\33n\n"
            "\n"
            "\33hThis gadget is not \33ifreeware\33n. You may not use it in your own programs without a licence. A licence can be obtained thru the author.\n"
            "\nColor test: \33b\33p[1]SHINE, \33p[2]HALFSHINE, \33p[3]BACKGROUND, \33p[4]HALFSHADOW, \33p[5]SHADOW, \33p[6]TEXT, \33p[7]FILL, \33p[8]MARK\33n\n"
            "\n"
            "\33[s:2]\33c\33u\33b Usage: \33n\n"
            "\33l\n"
            "You can doubleclick a word to select it, if you hold LMB after a doubleclick, then it will only mark \33bcomplete\33n words. Tripleclicking has the same effect, but for lines.\n"
            "You can extend your block by holding down shift while you press LMB where you want the block to end.\n"
            "While you drag to scroll, the farther away from the gadget your mouse pointer is, the faster the gadget will scroll.\n"
            "\n"
            "\33c\33[s:2]\33u\33b Keybindigns \33n\n\33l"
            "\n"
            "Hold down shift and press a navigation key to mark. When something is marked you can use: LAmiga x, c to cut or copy. Delete or Backspace to erase. Or any other key to overwrite.\n"
            "LAmiga + z, Z, v = Undo, Redo, Paste.\n"
            "TAB will insert 3 spaces.\n"
            "\n"
            "  \33u  Navigation combinations:  \33n\n"
            "Ctrl + left, right, up, down = BOL, EOF, Top, Bottm.\n"
            "Alt + left, right, up, down = BOW(PrevWord), NextWord, StartOfPage(PrevPage), EndOfPage(NextPage).\n"
            "Ctrl-Alt + left, right, up, down = PrevSentence, NextSentence, PrevParagraph, NextParagraph.\n"
            "\n"
            "  \33u  Delete combinations:  \33n\n"
            "Ctrl + Backspace, Delete = \"Delele To BOL\", \"Delele To EOL\".\n"
            "Alt + Backspace, Delete = \"Delele To BOW\", \"Delete To NextWord\".\n",
            TAG_DONE))
    {
      Object *Win_Object;
      struct Window *window;

      WindowBase = (struct ClassLibrary *)OpenLibrary("window.class",0L);
      LayoutBase = (struct ClassLibrary *)OpenLibrary("gadgets/layout.gadget",0L);
      ScrollerBase = (struct ClassLibrary *)OpenLibrary("gadgets/scroller.gadget",0L);

      if(ScrollerBase && WindowBase && LayoutBase)
      {
        Win_Object = NewObject( WINDOW_GetClass(), NULL,
          WA_ScreenTitle, "ClassAct Release 2.0",
          WA_Title, "TextEditor example",
          WA_Activate, TRUE,
          WA_DepthGadget, TRUE,
          WA_DragBar, TRUE,
          WA_CloseGadget, TRUE,
          WA_SizeGadget, TRUE,
          WA_Width, 460,
          WA_Height, 300,
          WINDOW_Position, WPOS_CENTERSCREEN,
          WINDOW_ParentGroup, NewObject( LAYOUT_GetClass(), NULL,
            LAYOUT_DeferLayout, TRUE,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_SpaceInner, FALSE,
            LAYOUT_BevelStyle, BVS_GROUP,
            LAYOUT_Label, "TextEditor.gadget",
            LAYOUT_AddChild, obj,
            LAYOUT_AddChild, prop = NewObjectA( SCROLLER_GetClass(), NULL, NULL),
            CHILD_MinWidth, 18,
            CHILD_WeightedWidth, 0,
          TAG_END),
        TAG_END);
      }
      else
      {
//        printf("Failed to open classes/window or gadgets/layout\n");
      }

      {
          static struct TagItem editortoprop[] =
          {
            MUIA_TextEditor_Prop_DeltaFactor, SCROLLER_ArrowDelta,
            MUIA_TextEditor_Prop_Entries,     SCROLLER_Total,
            MUIA_TextEditor_Prop_First,     SCROLLER_Top,
            MUIA_TextEditor_Prop_Visible,     SCROLLER_Visible,
            TAG_DONE
          };
          static struct TagItem proptoed[] =
          {
            SCROLLER_Top, MUIA_TextEditor_Prop_First,
            TAG_DONE
          };

#define ICA_Target (TAG_USER+0x40000L+1)
#define ICA_Map (TAG_USER+0x40000L+2)

        SetAttrs(prop, ICA_Target, obj,
                  ICA_Map, proptoed,
                  TAG_DONE);

        SetAttrs(obj,  ICA_Target, prop,
                  ICA_Map, editortoprop,
                  TAG_DONE);
      }

      if(Win_Object && (window = (struct Window *)DoMethod((APTR)Win_Object, WM_OPEN, NULL)))
      {
          ULONG wait, signal, result, done = FALSE;
          WORD Code;
          WORD flow = 0;

        GetAttr( WINDOW_SigMask, Win_Object, &signal );
        while(!done)
        {
          wait = Wait(signal|SIGBREAKF_CTRL_C);

          if(wait & SIGBREAKF_CTRL_C)
            done = TRUE;
          else
          {
            while((result = DoMethod((APTR)Win_Object, WM_HANDLEINPUT, &Code)) != WMHI_LASTMSG)
            {
              switch(result & WMHI_CLASSMASK)
              {
                case WMHI_ACTIVE:
//                  DoGadgetMethod((struct Gadget *)obj, window, NULL, MUIM_TextEditor_ARexxCmd, 0, "Copy");
/*                  SetGadgetAttrs((struct Gadget *)obj, window, NULL, MUIA_TextEditor_Flow, flow);
                  flow = (flow+1) & 3;
*/                break;

                case WMHI_CLOSEWINDOW:
                  done = TRUE;
                break;
              }
            }
          }
        }
        DisposeObject(Win_Object);
      }
      else
      {
//        printf("Failed to create or open the window\n");
        DisposeObject(obj);
      }

      if(ScrollerBase)
        CloseLibrary((struct Library *)ScrollerBase);
      if(LayoutBase)
        CloseLibrary((struct Library *)LayoutBase);
      if(WindowBase)
        CloseLibrary((struct Library *)WindowBase);
    }
    FreeClass(cl);
  }
  if(BevelBase)
    CloseLibrary((struct Library *)BevelBase);
}
