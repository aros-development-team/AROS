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

 $Id: TextEditor-Test.c,v 1.19 2005/08/16 21:21:01 damato Exp $

***************************************************************************/

#include <stdio.h>
#include <string.h>

#include <exec/tasks.h>
#include <libraries/mui.h>
#include <devices/clipboard.h>
#include <libraries/iffparse.h>
#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>

#include "TextEditor_mcc.h"
#include "private.h"

#include "SDI_hook.h"

HOOKPROTONHNO(URLHookCode, LONG, struct ClickMessage *cm)
{
  ULONG pos = cm->ClickPosition;

  while(pos && *(cm->LineContents+pos-1) != ' ' && *(cm->LineContents+pos-1) != '<')
  {
    pos--;
  }

  if(strncmp(cm->LineContents+pos, "http:", 5))
  {
    return(FALSE);
  }

  return(TRUE);
}
MakeStaticHook(URLHook, URLHookCode);

Object *editorgad;

HOOKPROTONHNONP(PosHookCode, void)
{
  ULONG x, y, sx, sy;
  struct Rectangle *crsr;
  if(DoMethod(editorgad, MUIM_TextEditor_BlockInfo, &x, &y, &sx, &sy))
  {
    printf("%ld, %ld, %ld, %ld\n", x, y, sx, sy);
  }

  if(get(editorgad, MUIA_TextEditor_CursorPosition, (APTR)&crsr))
    printf("Cursor: (%d, %d) - (%d, %d)\n", crsr->MinX, crsr->MinY, crsr->MaxX, crsr->MaxY);
}
MakeStaticHook(PosHook, PosHookCode);

#if defined(__amigaos4__)
struct Library *DiskfontBase = NULL;
struct Library *GfxBase = NULL;
struct Library *IntuitionBase = NULL;
struct Library *KeymapBase = NULL;
struct Library *LayersBase = NULL;
struct Library *LocaleBase = NULL;
struct Library *MUIMasterBase = NULL;
struct Library *RexxSysBase = NULL;
struct Library *UtilityBase = NULL;
struct Library *WorkbenchBase = NULL;
#elif defined(__MORPHOS__)
struct Library *DiskfontBase = NULL;
struct Library *GfxBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct Library *KeymapBase = NULL;
struct Library *LayersBase = NULL;
struct Library *LocaleBase = NULL;
struct Library *MUIMasterBase = NULL;
struct Library *RexxSysBase = NULL;
struct Library *UtilityBase = NULL;
struct Library *WorkbenchBase = NULL;
#else
struct Library *DiskfontBase = NULL;
struct Library *GfxBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct Library *KeymapBase = NULL;
struct Library *LayersBase = NULL;
struct Library *LocaleBase = NULL;
struct Library *MUIMasterBase = NULL;
struct Library *RexxSysBase = NULL;
struct Library *UtilityBase = NULL;
struct Library *WorkbenchBase = NULL;
#endif

#if defined(__amigaos4__)
struct DiskfontIFace *IDiskfont = NULL;
struct GraphicsIFace *IGraphics = NULL;
struct IntuitionIFace *IIntuition = NULL;
struct KeymapIFace *IKeymap = NULL;
struct LayersIFace *ILayers = NULL;
struct LocaleIFace *ILocale = NULL;
struct MUIMasterIFace *IMUIMaster = NULL;
struct RexxSysIFace *IRexxSys = NULL;
struct UtilityIFace *IUtility = NULL;
struct WorkbenchIFace *IWorkbench = NULL;
#endif

#ifdef __MORPHOS__
DISPATCHERPROTO(_Dispatcher);
#endif

static char *page_titles[] = { "Shown", "Hidden", NULL };

int main(void)
{
  void    *slider;
  long    argarray[6] = {0,0,0,0,0,0};
  struct  MUI_CustomClass *mcc;
  struct  RDArgs        *args;

  if((DiskfontBase = OpenLibrary("diskfont.library", 38)) &&
    GETINTERFACE(IDiskfont, DiskfontBase))
  if((GfxBase = OpenLibrary("graphics.library", 38)) &&
    GETINTERFACE(IGraphics, GfxBase))
  if((IntuitionBase = (APTR)OpenLibrary("intuition.library", 38)) &&
    GETINTERFACE(IIntuition, IntuitionBase))
  if((KeymapBase = OpenLibrary("keymap.library", 38)) &&
    GETINTERFACE(IKeymap, KeymapBase))
  if((LayersBase = OpenLibrary("layers.library", 38)) &&
    GETINTERFACE(ILayers, LayersBase))
  if((LocaleBase = OpenLibrary("locale.library", 38)) &&
    GETINTERFACE(ILocale, LocaleBase))
  if((RexxSysBase = OpenLibrary("rexxsyslib.library", 36)) &&
    GETINTERFACE(IRexxSys, RexxSysBase))
  if((UtilityBase = OpenLibrary("utility.library", 38)) &&
    GETINTERFACE(IUtility, UtilityBase))
  {
  	/* Open workbench.library (optional) */
		if ((WorkbenchBase = OpenLibrary("workbench.library",0)))
		{
  		if (!(GETINTERFACE(IWorkbench, WorkbenchBase)))
  		{
  			CloseLibrary(WorkbenchBase);
  			WorkbenchBase = NULL;
  		}
		}

    #if defined(DEBUG)
    SetupDebug();
    #endif

    if((args = ReadArgs("FILENAME,MIME/S,MIMEQUOTED/S,SKIPHEADER/S,FIXED/S,EMAIL/S", argarray, NULL)))
    {
      if((MUIMasterBase = OpenLibrary("muimaster.library", MUIMASTER_VMIN)) &&
        GETINTERFACE(IMUIMaster, MUIMasterBase))
      {
          Object *app, *window, *clear, *cut, *copy, *paste, *erase,
                 *bold, *italic, *underline, *ischanged, *undo, *redo, *string,
                 *xslider, *yslider, *flow, *search, *replace;
          STRPTR  flow_text[] = {"Left", "Center", "Right", NULL};
          STRPTR  classes[] = {"TextEditor.mcc", NULL};

        mcc = MUI_CreateCustomClass(NULL, "Area.mui", NULL, sizeof(struct InstData), ENTRY(_Dispatcher));
        
        app = MUI_NewObject("Application.mui",
              MUIA_Application_Author,    "Allan Odgaard",
              MUIA_Application_Base,      "Editor-Demo",
              MUIA_Application_Copyright, "®1997 Allan Odgaard",
              MUIA_Application_Description, "Editor.mcc demonstration program",
              MUIA_Application_Title,     "Editor-Demo",
              MUIA_Application_Version,   "$VER: Editor-Demo V1.0 (18-Feb-97)",
              MUIA_Application_UsedClasses, classes,

              MUIA_Application_Window,
                window = MUI_NewObject("Window.mui",
                MUIA_Window_Title,    "Editor-Demo",
                MUIA_Window_ID,       MAKE_ID('M','A','I','N'),
//                MUIA_Window_DefaultObject, editorgad,
                MUIA_Window_RootObject,
                  MUI_NewObject("Group.mui",
                  MUIA_Group_Child,
                    MUI_NewObject("Group.mui",
                    MUIA_Background, MUII_GroupBack,
                    MUIA_Frame, MUIV_Frame_Group,

                    MUIA_Group_Child,
                      MUI_NewObject("Group.mui",
                      MUIA_Group_Horiz,     TRUE,

                      Child, clear = MUI_MakeObject(MUIO_Button, "Clear _Text"),
                      Child, cut = MUI_MakeObject(MUIO_Button, "Cut"),
                      Child, copy = MUI_MakeObject(MUIO_Button, "Copy"),
                      Child, paste = MUI_MakeObject(MUIO_Button, "Paste"),
                      Child, erase = MUI_MakeObject(MUIO_Button, "Erase"),
                      Child, undo = MUI_MakeObject(MUIO_Button, "Undo"),
                      Child, redo = MUI_MakeObject(MUIO_Button, "Redo"),
                      Child, search = MUI_MakeObject(MUIO_Button, "Search"),
                      Child, replace = MUI_MakeObject(MUIO_Button, "Replace"),
                      Child, ischanged = MUI_MakeObject(MUIO_Checkmark, "Is changed?"),
                      Child, flow = MUI_MakeObject(MUIO_Cycle, NULL, flow_text),

                      Child, bold = MUI_NewObject("Text.mui",
                        MUIA_Background,    MUII_ButtonBack,
                        MUIA_Frame,       MUIV_Frame_Button,
                        MUIA_Text_PreParse, "\33c",
                        MUIA_Font,        MUIV_Font_Tiny,
                        MUIA_Text_Contents, "\33I[5:ProgDir:Bold.Brush]\n\n\nBold",
                        MUIA_Text_SetVMax,  FALSE,
                        MUIA_FixHeight,   17,
                        MUIA_FixWidth,      24,
                        MUIA_InputMode,   MUIV_InputMode_Toggle,
                        MUIA_ControlChar,   'b',
                        MUIA_CycleChain,    TRUE,
                        End,

                      Child, italic = MUI_NewObject("Text.mui",
                        MUIA_Background,    MUII_ButtonBack,
                        MUIA_Frame,       MUIV_Frame_Button,
                        MUIA_Text_PreParse, "\33c",
                        MUIA_Font,        MUIV_Font_Tiny,
                        MUIA_Text_Contents, "\33I[5:ProgDir:Italic.Brush]\n\n\nItalic",
                        MUIA_Text_SetVMax,  FALSE,
                        MUIA_FixHeight,   17,
                        MUIA_FixWidth,      24,
                        MUIA_InputMode,   MUIV_InputMode_Toggle,
                        MUIA_ControlChar,   'i',
                        MUIA_CycleChain,    TRUE,
                        End,

                      Child, underline = MUI_NewObject("Text.mui",
                        MUIA_Background,    MUII_ButtonBack,
                        MUIA_Frame,       MUIV_Frame_Button,
                        MUIA_Text_PreParse, "\33c",
                        MUIA_Font,        MUIV_Font_Tiny,
                        MUIA_Text_Contents, "\33I[5:ProgDir:Underline.Brush]\n\n\nUnderline",
                        MUIA_Text_SetVMax,  FALSE,
                        MUIA_FixHeight,   17,
                        MUIA_FixWidth,      24,
                        MUIA_InputMode,   MUIV_InputMode_Toggle,
                        MUIA_ControlChar,   'u',
                        MUIA_CycleChain,    TRUE,
                        End,

                      TAG_DONE ),

                    Child, HGroup,


/*                          Child, NewObject(mcc->mcc_Class, NULL,
                            InputListFrame,
//                            MUIA_TextEditor_InVirtualGroup, TRUE,
                            MUIA_TextEditor_Contents, "Jeg er en dulle!",
                            End,
*/
/*                      Child, ScrollgroupObject,
                        MUIA_Scrollgroup_Contents, VirtgroupObject,

*/
/*                      Child, RegisterGroup(titles),
                          Child, HGroup,
*/
/*                            Child, ColGroup(5),
                              Child, NewObject(mcc->mcc_Class, NULL, End,
                              Child, NewObject(mcc->mcc_Class, NULL, End,
                              Child, NewObject(mcc->mcc_Class, NULL, End,
                              Child, NewObject(mcc->mcc_Class, NULL, End,
                              Child, NewObject(mcc->mcc_Class, NULL, End,
                              Child, NewObject(mcc->mcc_Class, NULL, End,
                              Child, NewObject(mcc->mcc_Class, NULL, End,
                              Child, NewObject(mcc->mcc_Class, NULL, End,
                              Child, NewObject(mcc->mcc_Class, NULL, End,
                              Child, NewObject(mcc->mcc_Class, NULL, End,
                            End,
*/
                            Child, RegisterGroup(page_titles),
                            	MUIA_Register_Frame, TRUE,
                              Child,HGroup,
                                MUIA_Group_Spacing, 0,
                                Child, editorgad = NewObject(mcc->mcc_Class, NULL,
//                                MUIA_Frame, "602211",
//                                InputListFrame,
//                                MUIA_Background, MUII_GroupBack,
//                                MUIA_TextEditor_FixedFont, TRUE,
                                  MUIA_TextEditor_AutoClip, FALSE,
//                                MUIA_TextEditor_ReadOnly, TRUE,
                                  MUIA_TextEditor_DoubleClickHook, &URLHook,
//                                MUIA_TextEditor_HorizontalScroll, TRUE,
/*                                MUIA_TextEditor_ImportWrap, 1023,
                                  MUIA_TextEditor_WrapBorder, 80,
                                  MUIA_TextEditor_ExportWrap, 80,*/
//                                MUIA_TextEditor_ExportHook, MUIV_TextEditor_ExportHook_EMail,
//                                MUIA_TextEditor_ImportHook, MUIV_TextEditor_ImportHook_EMail,
                                  MUIA_CycleChain,    TRUE,
//                                MUIA_TextEditor_WrapBorder, 80,
//                                MUIA_TextEditor_ReadOnly, TRUE,
//                                MUIA_TextEditor_InVirtualGroup, TRUE,
//                                MUIA_Disabled, TRUE,
//                                MUIA_TextEditor_Columns,  40,
//                                MUIA_TextEditor_CursorX, 30,
//                                MUIA_TextEditor_CursorY, 7,
                                  MUIA_ControlChar, 'a',
                                  MUIA_TextEditor_Contents,
                                    "\33r\33b" __DATE__ "\33n\n"
                                    "\n\33cTextEditor.gadget V15.0ß\n"
                                    "Copyright 1997 by Allan Odgaard\n"
                                    "\33l\n\33[s:9]\n"
                                    "For feedback write to: Duff@DIKU.DK\n"
                                    "For the latest version, try: \33p[7]\33uhttp://www.DIKU.dk/students/duff/texteditor/\33n\n"
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
                                End,
                                Child, slider = MUI_NewObject("Scrollbar.mui", End,
                              End,
                              Child, VGroup,
	                              Child, TextObject,
                              	  MUIA_Text_Contents, "TextEditor object is now hidden!!!",
                            	  End,
                            	End,
                            End,
                          End,
/*                          End,
                        End,
*/
/*                          Child, RectangleObject, End,
                          End,



                          End,
*/                    Child, HGroup,
                      Child, xslider = MUI_MakeObject(MUIO_Slider, NULL, 0, 1000),
                      Child, yslider = MUI_MakeObject(MUIO_Slider, NULL, 0, 200),
                      End,
                    Child, string = MUI_MakeObject(MUIO_String, NULL, 256),

                    TAG_DONE ),
                  TAG_DONE ),
                TAG_DONE ),
              TAG_DONE );

        if(app)
        {
            unsigned long sigs;
            unsigned long running = 1;
            BPTR  fh;

          set(editorgad, MUIA_TextEditor_Slider, slider);

          if(argarray[4])
          {
            set(editorgad, MUIA_TextEditor_FixedFont, TRUE);
          }

					if (argarray[0])
					{
	          if((fh = Open((char *)argarray[0], MODE_OLDFILE)))
	          {
	              char  *text = AllocVec(50*1024, 0L);
	              char  *buffer = text;
	              int size;
	
	            size = Read(fh, text, (50*1024)-2);
	            text[size] = '\0';
	            Close(fh);
	
	            if(argarray[3])
	            {
	              while(*buffer != '\n' && buffer < &text[size])
	              {
	                while(*buffer++ != '\n');
	              }
	            }
	
	            if(argarray[2])
	              set(editorgad, MUIA_TextEditor_ImportHook, MUIV_TextEditor_ImportHook_MIMEQuoted);
	            else
	              if(argarray[1])
	                set(editorgad, MUIA_TextEditor_ImportHook, MUIV_TextEditor_ImportHook_MIME);
	              else
	                if(argarray[5])
	                  set(editorgad, MUIA_TextEditor_ImportHook, MUIV_TextEditor_ImportHook_EMail);
	
	            SetAttrs(editorgad, MUIA_TextEditor_Contents, buffer,
	                          TAG_DONE);
	            FreeVec(text);
	          }
					}

          set(string, MUIA_CycleChain, TRUE);

          DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

          DoMethod(flow, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_Flow, MUIV_TriggerValue);
          DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_Flow, MUIV_EveryTime, flow, 3, MUIM_NoNotifySet, MUIA_Cycle_Active, MUIV_TriggerValue);

          DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_CursorX, MUIV_EveryTime, xslider, 3, MUIM_NoNotifySet, MUIA_Numeric_Value, MUIV_TriggerValue);
//          DoMethod(xslider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_Position, MUIV_TriggerValue);
          DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_CursorY, MUIV_EveryTime, yslider, 3, MUIM_NoNotifySet, MUIA_Numeric_Value, MUIV_TriggerValue);
          DoMethod(yslider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_CursorY, MUIV_TriggerValue);

          DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_AreaMarked, MUIV_EveryTime, MUIV_Notify_Self, 7, MUIM_MultiSet, MUIA_Disabled, MUIV_NotTriggerValue, cut, copy, erase, NULL);
          DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_UndoAvailable, MUIV_EveryTime, undo, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);
          DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_RedoAvailable, MUIV_EveryTime, redo, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);
          DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_StyleBold, MUIV_EveryTime, bold, 3, MUIM_NoNotifySet, MUIA_Selected, MUIV_TriggerValue);
          DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_StyleItalic, MUIV_EveryTime, italic, 3, MUIM_NoNotifySet, MUIA_Selected, MUIV_TriggerValue);
          DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_StyleUnderline, MUIV_EveryTime, underline, 3, MUIM_NoNotifySet, MUIA_Selected, MUIV_TriggerValue);

          DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_HasChanged, MUIV_EveryTime, ischanged, 3, MUIM_NoNotifySet, MUIA_Selected, MUIV_TriggerValue);
          DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_HasChanged, MUIV_EveryTime, ischanged, 3, MUIM_NoNotifySet, MUIA_Image_State, MUIV_TriggerValue);
          DoMethod(ischanged, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_HasChanged, MUIV_TriggerValue);

          DoMethod(clear, MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_CallHook, &PosHook); //MUIM_TextEditor_ARexxCmd, "Clear");
//          DoMethod(clear, MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_HasChanged, FALSE);

          DoMethod(cut,   MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "Cut");
          DoMethod(copy,  MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "Copy");
          DoMethod(paste, MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "Paste");
          DoMethod(erase, MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "Erase");
//          DoMethod(undo,  MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 3, MUIM_Set, MUIA_TextEditor_Prop_First, 13*15);
          DoMethod(undo,  MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "Undo");
          DoMethod(redo,  MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "Redo");
          DoMethod(search,  MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 3, MUIM_TextEditor_Search, "is not", 0);//MUIF_TextEditor_Search_CaseSensitive);
          DoMethod(replace, MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 3, MUIM_TextEditor_Replace, "replaced", 0);

          DoMethod(bold,      MUIM_Notify, MUIA_Selected, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_StyleBold, MUIV_TriggerValue);
          DoMethod(italic,    MUIM_Notify, MUIA_Selected, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_StyleItalic, MUIV_TriggerValue);
          DoMethod(underline, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_StyleUnderline, MUIV_TriggerValue);

          DoMethod(string, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, editorgad, 2, MUIM_TextEditor_ARexxCmd, MUIV_TriggerValue);
//          DoMethod(string, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, window, 3, MUIM_Set, MUIA_Window_ActiveObject, string);
//          DoMethod(string, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, editorgad, 3, MUIM_TextEditor_Search, MUIV_TriggerValue, 0L);//MUIF_TextEditor_Search_FromTop);
//          DoMethod(string, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, editorgad, 3, MUIM_TextEditor_Replace, MUIV_TriggerValue, 0L);//MUIF_TextEditor_Search_FromTop);

          set(window, MUIA_Window_ActiveObject, editorgad);
          set(window, MUIA_Window_Open, TRUE);

/*          {
            ULONG delta;
            get(editorgad, MUIA_TextEditor_Prop_DeltaFactor, &delta);
            printf("Delta: %d\n", delta);
          }
*/
//          DoMethod(editorgad, MUIM_TextEditor_MarkText, 0x000a000f, 0x0030000f);
          do  {
              long  changed;

            while((LONG)DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
            {
              if(sigs)
              {
                sigs = Wait(sigs | SIGBREAKF_CTRL_C);
                if(sigs & SIGBREAKF_CTRL_C)
                  break;
              }
            }

            get(editorgad, MUIA_TextEditor_HasChanged, &changed);
            if(changed && !(sigs & SIGBREAKF_CTRL_C))
              running = MUI_Request(app, window, 0L, "Warning", "*_Proceed|_Save|_Cancel", "\33cText '%s'\n is modified. Save it?", argarray[0]);
          } while(running == 0);

          if(running == 2)
          {
              void  *text = (void *)DoMethod(editorgad, MUIM_TextEditor_ExportText);

            if((fh = Open((char *)argarray[0], MODE_NEWFILE)))
            {
              Write(fh, text, strlen(text));
              Close(fh);
            }
            FreeVec(text);
          }
          MUI_DisposeObject(app);

          if(mcc)
          {
            MUI_DeleteCustomClass(mcc);
          }
        }
        else printf("Failed to create application\n");

        DROPINTERFACE(IMUIMaster);
        CloseLibrary(MUIMasterBase);
        MUIMasterBase = NULL;
      }

      FreeArgs(args);
    }
    else
    {
      char prgname[32];
      long error = IoErr();

      GetProgramName(prgname, 32);
      PrintFault(error, prgname);
    }
  }

	if(WorkbenchBase)
	{
		DROPINTERFACE(IWorkbench);
		WorkbenchBase = NULL;
	}
  if(UtilityBase)
  {
    DROPINTERFACE(IUtility);
    CloseLibrary(UtilityBase);
  }

  if(RexxSysBase)
  {
    DROPINTERFACE(IRexxSys);
    CloseLibrary(RexxSysBase);
  }

  if(LocaleBase)
  {
    DROPINTERFACE(ILocale);
    CloseLibrary(LocaleBase);
  }

  if(LayersBase)
  {
    DROPINTERFACE(ILayers);
    CloseLibrary(LayersBase);
  }

  if(KeymapBase)
  {
    DROPINTERFACE(IKeymap);
    CloseLibrary(KeymapBase);
  }

  if(IntuitionBase)
  {
    DROPINTERFACE(IIntuition);
    CloseLibrary((struct Library *)IntuitionBase);
  }

  if(GfxBase)
  {
    DROPINTERFACE(IGraphics);
    CloseLibrary(GfxBase);
  }

  if(DiskfontBase)
  {
    DROPINTERFACE(IDiskfont);
    CloseLibrary(DiskfontBase);
  }

  return 0;
}
