/***************************************************************************

 TextEditor-Demo - Textediting MUI Custom Class Demonstration Program
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2014 TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 To compile this demo of some features of the TextEditor class,
 depending on your OS and compiler :

 - MorphOS GCC : gcc -noixemul -o TextEditor-Demo TextEditor-Demo.c
 - MorphOS VBCC : vc -o TextEditor-Demo TextEditor-Demo.c -lamiga
 - AmigaOS4 GCC : gcc -ggdb -D__USE_BASETYPE__ -D__USE_INLINE__ -o TextEditor-Demo TextEditor-Demo.c
 - AmigaOS 3.x SAS/C : sc TextEditor-Demo.c link to TextEditor-Demo

 Requirements : SDI_headers

 $Id$

***************************************************************************/

#include <stdio.h>
#include <string.h>

#include <libraries/mui.h>
#include <libraries/iffparse.h>

#include <clib/alib_protos.h>

#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/rexxsyslib.h>

#include <mui/TextEditor_mcc.h>

#if defined(__amigaos4__)
#include <dos/obsolete.h>
#endif

#include "SDI_hook.h"

#ifndef __amigaos4__
struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;
struct Library *MUIMasterBase;
#ifdef __MORPHOS__
struct Library *RexxSysBase;
#else
struct RxsLib *RexxSysBase;
#endif
#else
struct Library *GfxBase;
struct Library *IntuitionBase;
struct Library *MUIMasterBase;
struct Library *RexxSysBase;

struct GraphicsIFace *IGraphics;
struct IntuitionIFace *IIntuition;
struct MUIMasterIFace *IMUIMaster;
struct RexxSysIFace *IRexxSys;
#endif

#ifndef MUIA_Application_UsedClasses
#define MUIA_Application_UsedClasses  0x8042e9a7  /* V20 STRPTR * i.. */
#endif
#define MUIV_RunARexxScript           0xad800000

long __stack = 16384;

Object *app, *window, *editorgad;
const char *StdEntries[] = { "Kind regards ", "Yours ", "Mvh ", NULL };
LONG cmap[8];

HOOKPROTONH(ARexxHookCode, LONG, Object *app, struct RexxMsg *rexxmsg)
{
  IPTR result;

  result = DoMethod(editorgad, MUIM_TextEditor_ARexxCmd, rexxmsg->rm_Args[0]);

  if(result)
  {
    if(result != TRUE)
    {
      set(app, MUIA_Application_RexxString, result);
      FreeVec((APTR)result);
    }
  }

  return(0);
}
MakeStaticHook(ARexxHook, ARexxHookCode);

DISPATCHER(TextEditor_Dispatcher)
{
  switch(msg->MethodID)
  {
    case MUIM_Setup:
    {
      if(DoSuperMethodA(cl, obj, (Msg)msg))
      {
        struct ColorMap *cm = muiRenderInfo(obj)->mri_Screen->ViewPort.ColorMap;

        cmap[0] = ObtainBestPenA(cm, 0x00<<24, 0x00<<24, 0x00<<24, NULL);
        cmap[1] = ObtainBestPenA(cm, 0xff<<24, 0xff<<24, 0xff<<24, NULL);
        cmap[2] = ObtainBestPenA(cm, 0xff<<24, 0x00<<24, 0x00<<24, NULL);
        cmap[3] = ObtainBestPenA(cm, 0x00<<24, 0xff<<24, 0x00<<24, NULL);
        cmap[4] = ObtainBestPenA(cm, 0x00<<24, 0xff<<24, 0xff<<24, NULL);
        cmap[5] = ObtainBestPenA(cm, 0xff<<24, 0xff<<24, 0x00<<24, NULL);
        cmap[6] = ObtainBestPenA(cm, 0x00<<24, 0x00<<24, 0xff<<24, NULL);
        cmap[7] = ObtainBestPenA(cm, 0xff<<24, 0x00<<24, 0xff<<24, NULL);

        set(obj, MUIA_TextEditor_ColorMap, cmap);

        return TRUE;
      }
      else
        return FALSE;
    }
    break;

    case MUIM_Cleanup:
    {
      struct ColorMap *cm = muiRenderInfo(obj)->mri_Screen->ViewPort.ColorMap;
      int c;

      set(obj, MUIA_TextEditor_ColorMap, NULL);

      for(c = 0; c < 8; c++)
      {
        if(cmap[c] != -1)
          ReleasePen(cm, cmap[c]);
      }
    }
    break;

    case MUIM_DragQuery:
    {
      return TRUE;
    }

    case MUIM_DragDrop:
    {
      struct MUIP_DragDrop *drop_msg = (struct MUIP_DragDrop *)msg;
      ULONG active = 0;

      if(GetAttr(MUIA_List_Active, drop_msg->obj, (IPTR *)&active))
        DoMethod(obj, MUIM_TextEditor_InsertText, StdEntries[active]);
    }
    break;

    case MUIM_TextEditor_HandleError:
    {
      const char *errortxt = NULL;
      struct MUIP_TextEditor_HandleError *msgerr;

      msgerr = (struct MUIP_TextEditor_HandleError *)msg;

      switch(msgerr->errorcode)
      {
        case Error_ClipboardIsEmpty:
          errortxt = "\33cThe clipboard is empty.";
        break;

        case Error_ClipboardIsNotFTXT:
          errortxt = "\33cThe clipboard does not contain text.";
        break;

        case Error_MacroBufferIsFull:
        case Error_MemoryAllocationFailed:
        case Error_NoMacroDefined:
        case Error_StringNotFound:
        break;

        case Error_NoAreaMarked:
          errortxt = "\33cNo area marked.";
        break;

        case Error_NothingToRedo:
          errortxt = "\33cNothing to redo.";
        break;

        case Error_NothingToUndo:
          errortxt = "\33cNothing to undo.";
        break;

        case Error_NotEnoughUndoMem:
          errortxt = "There is not enough memory\nto keep the undo buffer.\n\nThe undobuffer is lost.";
        break;

        case Error_NoBookmarkInstalled:
          errortxt = "There is no bookmark installed!";
        break;

        case Error_BookmarkHasBeenLost:
          errortxt = "Your bookmark has unfortunately been lost.";
        break;
      }

      if(errortxt != NULL)
        MUI_Request(app, window, 0L, NULL, "Continue", errortxt);
    }
    break;
  }

  return DoSuperMethodA(cl, obj, (Msg)msg);
}

static Object *ImageGad(const char *text, UBYTE key)
{
  return(TextObject,
        MUIA_Background,     MUII_ButtonBack,
        MUIA_Frame,          MUIV_Frame_Button,
        MUIA_Text_PreParse,  "\33c",
        MUIA_Font,           MUIV_Font_Tiny,
        MUIA_Text_Contents,  text,
        MUIA_Text_SetVMax,   FALSE,
        MUIA_FixHeight,      17,
        MUIA_FixWidth,       24,
        MUIA_InputMode,      MUIV_InputMode_Toggle,
        MUIA_ControlChar,    key,
        MUIA_CycleChain,     TRUE,
        End);
}

static ULONG OpenLibs(void)
{
  GfxBase = (APTR)OpenLibrary("graphics.library", 39);
  IntuitionBase = (APTR)OpenLibrary("intuition.library", 39);
  MUIMasterBase = OpenLibrary("muimaster.library", 11);  //MUIMASTER_VMIN)
  RexxSysBase = (APTR)OpenLibrary("rexxsyslib.library", 36);

  if(GfxBase && MUIMasterBase && IntuitionBase && RexxSysBase)
  {
    #ifdef __amigaos4__
    IGraphics = (struct GraphicsIFace *)GetInterface((struct Library *)GfxBase, "main", 1, NULL);
    IIntuition = (struct IntuitionIFace *)GetInterface((struct Library *)IntuitionBase, "main", 1, NULL);
    IMUIMaster = (struct MUIMasterIFace *)GetInterface(MUIMasterBase, "main", 1, NULL);
    IRexxSys = (struct RexxSysIFace *)GetInterface(RexxSysBase, "main", 1, NULL);
    #endif

    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

void CloseLibs(void)
{
  #ifdef __amigaos4__
  if (IRexxSys) DropInterface((struct Interface *)IRexxSys);
  if (IMUIMaster) DropInterface((struct Interface *)IMUIMaster);
  if (IIntuition) DropInterface((struct Interface *)IIntuition);
  if (IGraphics) DropInterface((struct Interface *)IGraphics);
  #endif

  if (RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
  if (MUIMasterBase) CloseLibrary(MUIMasterBase);
  if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
  if (GfxBase) CloseLibrary((struct Library *)GfxBase);
}

int main(VOID)
{
  struct RDArgs *args;
  Object *slider;
  IPTR argarray[6] = { 0,0,0,0,0,0 };

  if((args = ReadArgs("Filename/F,EMail/S,MIME/S,MIMEQuoted/S,SkipHeader/S,Fixed/S", (IPTR *)argarray, NULL)))
  {
    if(OpenLibs() == TRUE)
    {
      struct MUI_CustomClass *editor_mcc;
      Object *clear, *cut, *copy, *paste, *erase,
             *bold, *italic, *underline, *ischanged, *undo, *redo,
             *flow, *separator, *color, *config;
      const char *flow_text[] = { "Left", "Center", "Right", NULL };
      const char *colors[] = { "Normal", "Black", "White", "Red", "Green", "Cyan", "Yellow", "Blue", "Magenta", NULL };
      const char *classes[] = { "TextEditor.mcc" };

      if((editor_mcc = MUI_CreateCustomClass(NULL, "TextEditor.mcc", NULL, 0, ENTRY(TextEditor_Dispatcher))))
      {
        app = ApplicationObject,
                MUIA_Application_Author,      "TextEditor.mcc Open Source Team",
                MUIA_Application_Base,        "TextEditor-Demo",
                MUIA_Application_Copyright,   "(c) 2005-2010 by TextEditor.mcc Open Source Team",
                MUIA_Application_Description, "TextEditor.mcc demonstration program",
                MUIA_Application_RexxHook,    &ARexxHook,
                MUIA_Application_Title,       "TextEditor-Demo",
                MUIA_Application_Version,     "$VER: TextEditor-Demo (" __DATE__ ")",
                MUIA_Application_UsedClasses, classes,
                SubWindow, window = WindowObject,
                  MUIA_Window_Title,      "TextEditor-Demo",
                  MUIA_Window_ID,         MAKE_ID('M','A','I','N'),
                  WindowContents, VGroup,
                    Child, VGroup,
                      MUIA_Background, MUII_GroupBack,
                      MUIA_Frame, MUIV_Frame_Group,

                      Child, HGroup,

                        Child, RowGroup(2),
                          Child, cut = MUI_MakeObject(MUIO_Button, "Cut"),
                          Child, paste = MUI_MakeObject(MUIO_Button, "Paste"),
                          Child, undo = MUI_MakeObject(MUIO_Button, "Undo"),
                          Child, flow = MUI_MakeObject(MUIO_Cycle, NULL, flow_text),
                          Child, separator = MUI_MakeObject(MUIO_Button, "Separator"),
                          Child, clear = MUI_MakeObject(MUIO_Button, "Clear _Text"),

                          Child, copy = MUI_MakeObject(MUIO_Button, "Copy"),
                          Child, erase = MUI_MakeObject(MUIO_Button, "Erase"),
                          Child, redo = MUI_MakeObject(MUIO_Button, "Redo"),
                          Child, color = MUI_MakeObject(MUIO_Cycle, NULL, colors),
                          Child, config = MUI_MakeObject(MUIO_Button, "Config..."),
                          Child, HGroup,
                            Child, ischanged = MUI_MakeObject(MUIO_Checkmark, "Is changed?"),
                            Child, TextObject,
                              MUIA_Text_Contents, "Is changed?",
                            End,
                          End,
                        End,

                        Child, RectangleObject, End,

                        Child, bold = ImageGad("\33I[5:ProgDir:Bold.Brush]\n\n\nBold", 'b'),
                        Child, italic = ImageGad("\33I[5:ProgDir:Italic.Brush]\n\n\nItalic", 'i'),
                        Child, underline = ImageGad("\33I[5:ProgDir:Underline.Brush]\n\n\nUnderline", 'u'),
                      End,

                      Child, HGroup,

                        Child, HGroup,
                          MUIA_Group_Spacing, 0,
                          Child, editorgad = NewObject(editor_mcc->mcc_Class, NULL,
                            MUIA_CycleChain, TRUE,
                          End,
                          Child, slider = ScrollbarObject,
                          End,
                        End,

                        Child, VGroup,
                          Child, ListviewObject,
                            MUIA_Listview_DragType, MUIV_Listview_DragType_Immediate,
                            MUIA_Listview_List, ListObject,
                              InputListFrame,
                              MUIA_Background, MUII_ListBack,
                              MUIA_List_AdjustWidth, TRUE,
                              MUIA_List_AdjustHeight, TRUE,
                              MUIA_List_SourceArray, StdEntries,
                            End,
                          End,
                          Child, RectangleObject,
                          End,
                        End,
                      End,
                    End,
                  End,
                End,
              End;

            if(app)
            {
              ULONG sigs;
              ULONG running = 1;
              BPTR  fh;

              if(argarray[5])
                set(editorgad, MUIA_TextEditor_FixedFont, TRUE);

              if(argarray[0] && (fh = Open((STRPTR)argarray[0], MODE_OLDFILE)))
              {
                STRPTR  text = AllocVec(300*1024, 0L);
                STRPTR  buffer = text;
                LONG    size;

                if(text)
                {
                  size = Read(fh, text, (300*1024)-2);
                  text[size] = '\0';
                  Close(fh);

                  if(argarray[4])
                  {
                    while(*buffer != '\n' && buffer < &text[size])
                    {
                      while(*buffer++ != '\n');
                    }
                  }

                  if(argarray[3])
                  {
                    SetAttrs(editorgad,
                        MUIA_TextEditor_ImportHook, MUIV_TextEditor_ImportHook_MIMEQuoted,
                        MUIA_TextEditor_ImportWrap, 80,
                        MUIA_TextEditor_ExportHook, MUIV_TextEditor_ExportHook_EMail,
                        TAG_DONE);
                  }
                  else
                  {
                    if(argarray[2])
                    {
                      SetAttrs(editorgad,
                          MUIA_TextEditor_ImportHook, MUIV_TextEditor_ImportHook_MIME,
                          MUIA_TextEditor_ExportHook, MUIV_TextEditor_ExportHook_EMail,
                          TAG_DONE);
                    }
                    else
                    {
                      if(argarray[1])
                      {
                        SetAttrs(editorgad,
                            MUIA_TextEditor_ImportHook, MUIV_TextEditor_ImportHook_EMail,
                            MUIA_TextEditor_ExportHook, MUIV_TextEditor_ExportHook_EMail,
                            TAG_DONE);
                      }
                    }
                  }

                  set(editorgad, MUIA_TextEditor_Contents, buffer);
                  FreeVec(text);
                  set(editorgad, MUIA_TextEditor_ImportHook, MUIV_TextEditor_ImportHook_Plain);
                }
              }

              DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Application, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

              DoMethod(window, MUIM_Notify, MUIA_Window_InputEvent, "f1", MUIV_Notify_Application, 2, MUIM_Application_ReturnID, MUIV_RunARexxScript);

              DoMethod(flow, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_Flow, MUIV_TriggerValue);
              DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_Flow, MUIV_EveryTime, flow, 3, MUIM_NoNotifySet, MUIA_Cycle_Active, MUIV_TriggerValue);

              DoMethod(color, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_Pen, MUIV_TriggerValue);
              DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_Pen, MUIV_EveryTime, color, 3, MUIM_NoNotifySet, MUIA_Cycle_Active, MUIV_TriggerValue);

              DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_StyleBold,      MUIV_EveryTime, bold,      3, MUIM_NoNotifySet, MUIA_Selected, MUIV_TriggerValue);
              DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_StyleItalic,    MUIV_EveryTime, italic,    3, MUIM_NoNotifySet, MUIA_Selected, MUIV_TriggerValue);
              DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_StyleUnderline, MUIV_EveryTime, underline, 3, MUIM_NoNotifySet, MUIA_Selected, MUIV_TriggerValue);

              DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_HasChanged, MUIV_EveryTime, ischanged, 3, MUIM_NoNotifySet, MUIA_Selected, MUIV_TriggerValue);
              DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_HasChanged, MUIV_EveryTime, ischanged, 3, MUIM_NoNotifySet, MUIA_Image_State, MUIV_TriggerValue);
              DoMethod(ischanged, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_HasChanged, MUIV_TriggerValue);

              DoMethod(separator, MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_InsertText, "\n\33c\33[s:2]\n");

              DoMethod(config, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Application, 3, MUIM_Application_OpenConfigWindow, 1, "TextEditor.mcc");

              DoMethod(clear, MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "Clear");
              DoMethod(clear, MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_HasChanged, FALSE);

              DoMethod(cut,   MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "Cut");
              DoMethod(copy,  MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "Copy");
              DoMethod(paste, MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "Paste");
              DoMethod(erase, MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "Erase");
              DoMethod(undo,  MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "Undo");
              DoMethod(redo,  MUIM_Notify, MUIA_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "Redo");

              DoMethod(bold,      MUIM_Notify, MUIA_Selected, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_StyleBold,      MUIV_TriggerValue);
              DoMethod(italic,    MUIM_Notify, MUIA_Selected, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_StyleItalic,    MUIV_TriggerValue);
              DoMethod(underline, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_StyleUnderline, MUIV_TriggerValue);

              set(editorgad, MUIA_TextEditor_Slider, slider);

              SetAttrs(window,  MUIA_Window_ActiveObject, editorgad,
                                MUIA_Window_Open, TRUE,
                      TAG_DONE);

              do
              {
                struct MsgPort *myport = NULL;
                struct RexxMsg *rxmsg = NULL;
                ULONG changed;
                ULONG ReturnID;
                BPTR  rxstdout = 0L;

                while((LONG)(ReturnID = DoMethod(app, MUIM_Application_NewInput, &sigs)) != (LONG)MUIV_Application_ReturnID_Quit)
                {
                  if(ReturnID == MUIV_RunARexxScript && !myport)
                  {
                    struct MsgPort *rexxport;
                    const char *script = "Rexx:TextEditor/Demo.Rexx";

                    if((rexxport = FindPort((STRPTR)"REXX")) && (myport = CreateMsgPort()))
                    {
                      if(!rxstdout)
                        rxstdout = Open("CON://640/100/TextEditor-Demo, ARexx output:/Close/Wait/Auto/InActive", MODE_READWRITE);

                      rxmsg = CreateRexxMsg(myport, NULL, "TEXTEDITOR-DEMO.1");
                      rxmsg->rm_Action = RXCOMM;
                      rxmsg->rm_Stdin = rxstdout;
                      rxmsg->rm_Stdout = rxstdout;
                      rxmsg->rm_Args[0] = (IPTR)CreateArgstring((STRPTR)script, strlen(script));
                      PutMsg(rexxport, (struct Message *)rxmsg);
                    }
                  }

                  if(sigs)
                  {
                    sigs = Wait(sigs | SIGBREAKF_CTRL_C | (myport ? 1<<myport->mp_SigBit : 0));
                    if(myport && (sigs & 1<<myport->mp_SigBit))
                    {
                      GetMsg(myport);
                      if(!rxmsg->rm_Result1 && rxmsg->rm_Result2)
                        DeleteArgstring((STRPTR)rxmsg->rm_Result2);

                      DeleteArgstring((UBYTE *)rxmsg->rm_Args[0]);
                      DeleteRexxMsg(rxmsg);
                      DeleteMsgPort(myport);
                      myport = NULL;
                    }

                    if(sigs & SIGBREAKF_CTRL_C)
                      break;
                  }
                }

                if(rxstdout)
                  Close(rxstdout);

                get(editorgad, MUIA_TextEditor_HasChanged, &changed);
                if(argarray[0] && changed && !(sigs & SIGBREAKF_CTRL_C))
                  running = MUI_Request(app, window, 0L, "Warning", "_Proceed|*_Save|_Cancel", "\33cText '%s'\n is modified. Save it?", argarray[0]);
              }
              while(running == 0);

              if(running == 2)
              {
                STRPTR text = (STRPTR)DoMethod(editorgad, MUIM_TextEditor_ExportText);

                if(argarray[0] && (fh = Open((STRPTR)argarray[0], MODE_NEWFILE)))
                {
                  Write(fh, text, strlen(text));
                  Close(fh);
                }
                FreeVec(text);
              }
              MUI_DisposeObject(app);
            }
            else
              printf("Failed to create application\n");

        MUI_DeleteCustomClass(editor_mcc);
      }
      else
        printf("Failed to open TextEditor.mcc\n");

      CloseLibs();
    }
    else
      printf("Failed to open MUIMaster.Library V%d\n", MUIMASTER_VMIN);

    FreeArgs(args);
  }
  else
  {
    char prgname[32];
    LONG error = IoErr();

    GetProgramName(prgname, 32);
    PrintFault(error, prgname);
  }

  return 0;
}

