#include <StdIO.h>
#include <String.h>
#include <Exec/Tasks.h>
#include <Libraries/MUI.h>
#include <Rexx/Storage.h>
#include <Utility/Hooks.h>
#include <Proto/DOS.h>
#include <Proto/Exec.h>
#include <Proto/Graphics.h>
#include <Proto/MUIMaster.h>
#include <Proto/Intuition.h>
#include <Proto/RexxSysLib.h>

#include <MUI/InfoText_mcc.h>
#include <MUI/TextEditor_mcc.h>
#include <MUI/Toolbar_mcc.h>

	struct	Library	*MUIMasterBase;
	Object	*app, *window, *editorgad, *StatusLine;
	LONG		cmap[8];

LONG ARexxHookCode (register __a1 struct RexxMsg *rexxmsg, register __a2 Object *app)
{
		LONG result;

	if(result = DoMethod(editorgad, MUIM_TextEditor_ARexxCmd, rexxmsg->rm_Args[0]))
	{
		if(result != TRUE)
		{
			set(app, MUIA_Application_RexxString, result);
			FreeVec((APTR)result);
		}
	}
	return(0);
}
struct Hook ARexxHook = {0, 0, (APTR)ARexxHookCode};

ULONG TextEditor_Dispatcher (register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 struct MUIP_TextEditor_HandleError *msg)
{
	switch(msg->MethodID)
	{
		case MUIM_Show:
		{
				struct ColorMap *cm = muiRenderInfo(obj)->mri_Screen->ViewPort.ColorMap;
				struct Window *procwindow;

			get(muiRenderInfo(obj)->mri_WindowObject, MUIA_Window_Window, &procwindow);
			((struct Process *)FindTask(NULL))->pr_WindowPtr = procwindow;

			cmap[0] = ObtainBestPenA(cm, 0x00<<24, 0x00<<24, 0x00<<24, NULL);
			cmap[1] = ObtainBestPenA(cm, 0xff<<24, 0xff<<24, 0xff<<24, NULL);
			cmap[2] = ObtainBestPenA(cm, 0xff<<24, 0x00<<24, 0x00<<24, NULL);
			cmap[3] = ObtainBestPenA(cm, 0x00<<24, 0xff<<24, 0x00<<24, NULL);
			cmap[4] = ObtainBestPenA(cm, 0x00<<24, 0xff<<24, 0xff<<24, NULL);
			cmap[5] = ObtainBestPenA(cm, 0xff<<24, 0xff<<24, 0x00<<24, NULL);
			cmap[6] = ObtainBestPenA(cm, 0x00<<24, 0x00<<24, 0xff<<24, NULL);
			cmap[7] = ObtainBestPenA(cm, 0xff<<24, 0x00<<24, 0xff<<24, NULL);
			break;
		}

		case MUIM_Hide:
		{
				struct ColorMap *cm = muiRenderInfo(obj)->mri_Screen->ViewPort.ColorMap;
				int c;

			for(c = 0; c < 8; c++)
			{
				if(cmap[c] >= 0)
				{
					ReleasePen(cm, cmap[c]);
				}
			}
			break;
		}

		case MUIM_DragQuery:
		{
			return(TRUE);
		}

/*		case MUIM_DragDrop:
		{
				struct MUIP_DragDrop *drop_msg = (struct MUIP_DragDrop *)msg;
				ULONG active;

			if(GetAttr(MUIA_List_Active, drop_msg->obj, &active))
			{
				DoMethod(obj, MUIM_TextEditor_InsertText, StdEntries[active]);
			}
			break;
		}
*/
		case MUIM_TextEditor_HandleError:
		{
				char *errortxt = NULL;

			switch(msg->errorcode)
			{
				case Error_ClipboardIsEmpty:
					errortxt = "The clipboard is empty.";
					break;
				case Error_ClipboardIsNotFTXT:
					errortxt = "The clipboard does not contain text.";
					break;
				case Error_MacroBufferIsFull:
					break;
				case Error_MemoryAllocationFailed:
					break;
				case Error_NoAreaMarked:
					errortxt = "No area marked.";
					break;
				case Error_NoMacroDefined:
					break;
				case Error_NothingToRedo:
					errortxt = "Nothing to redo.";
					break;
				case Error_NothingToUndo:
					errortxt = "Nothing to undo.";
					break;
				case Error_NotEnoughUndoMem:
					errortxt = "Out of memory!  The undobuffer is lost.";
					break;
				case Error_StringNotFound:
					break;
				case Error_NoBookmarkInstalled:
					errortxt = "There is no bookmark installed!";
					break;
				case Error_BookmarkHasBeenLost:
					errortxt = "Your bookmark has unfortunately been lost.";
					break;
			}
			if(errortxt)
			{
				set(StatusLine, MUIA_InfoText_Contents, errortxt);
			}
			break;
		}
	}
	return(DoSuperMethodA(cl, obj, (Msg)msg));
}


#define MUIV_RunARexxScript 0xad800000

enum
{
	NEW = 0, OPEN, x1, CUT, COPY, PASTE, UNDO, x2, BOLD, ITALIC, UNDERLINE, COLORED, x3, LEFT, CENTER, RIGHT, x4, AREXX
};


VOID main (VOID)
{
		struct	RDArgs				*args;
		struct	StackSwapStruct	stackswap;
		struct	Task					*mytask = FindTask(NULL);
		Object	*slider;
		LONG		argarray[6]	=		{0,0,0,0,0,0};
		ULONG		stacksize	=		(ULONG)mytask->tc_SPUpper-(ULONG)mytask->tc_SPLower+8192;
		APTR		newstack		=		AllocVec(stacksize, 0L);
		UBYTE		wintitle[64];

	stackswap.stk_Lower   = newstack;
	stackswap.stk_Upper   = (ULONG)newstack+stacksize;
	stackswap.stk_Pointer = (APTR)stackswap.stk_Upper;
	if(newstack)
	{
		StackSwap(&stackswap);

		if(args = ReadArgs("Filename,EMail/S,MIME/S,MIMEQuoted/S,SkipHeader/S,Fixed/S", argarray, NULL))
		{
			if(MUIMasterBase = OpenLibrary("muimaster.library", MUIMASTER_VMIN))
			{
					struct	MUI_CustomClass	*editor_mcc;
					Object	/**color, */*ToolstripObj;
					STRPTR	colors[] = {"Normal", "Black", "White", "Red", "Gren", "Cyan", "Yellow", "Blue", "Magenta", NULL};
					STRPTR	classes[] = {"TextEditor.mcc", "Toolbar.mcc", NULL};

					struct MUIP_Toolbar_Description Toolstrip[] =
					{
						{ TDT_BUTTON,   0, 0L, NULL,	"Clear document contents",			0},
						{ TDT_IGNORE,   0, TDF_GHOSTED, NULL,	"Load text from file",				0},
						{ TDT_SPACE,    0, 0L, NULL, NULL, 0 },
						{ TDT_BUTTON,   0, 0L, NULL,	"Cut marked text",					0},
						{ TDT_BUTTON,   0, 0L, NULL,	"Copy marked text",					0},
						{ TDT_BUTTON,   0, 0L, NULL,	"Paste from clipboard",				0},
						{ TDT_BUTTON,   0, 0L, NULL,	"Undo last change",					0},
						{ TDT_SPACE,    0, 0L, NULL, NULL, 0 },
						{ TDT_BUTTON, 'b', TDF_TOGGLE, NULL,	"Bold text",				0},
						{ TDT_BUTTON, 'i', TDF_TOGGLE, NULL,	"Italic text",				0},
						{ TDT_BUTTON, 'u', TDF_TOGGLE, NULL,	"Underlined text",		0},
						{ TDT_BUTTON, 'r', TDF_TOGGLE, NULL,	"Colored text",			0},
						{ TDT_SPACE,    0, 0L, NULL, NULL, 0 },
						{ TDT_BUTTON, '[', TDF_RADIOTOGGLE | TDF_SELECTED,	NULL,	"Left aligned",	1<<CENTER | 1<<RIGHT},
						{ TDT_BUTTON, '|', TDF_RADIOTOGGLE,						NULL,	"Centered",			1<<LEFT   | 1<<RIGHT},
						{ TDT_BUTTON, ']', TDF_RADIOTOGGLE,						NULL,	"Right aligned",	1<<LEFT   | 1<<CENTER},
//						{ TDT_SPACE,    0, 0L, NULL, NULL, 0 },
						{ TDT_IGNORE,   0, TDF_GHOSTED, NULL,	"Run an ARexx script",				0},
						{ TDT_END,      0, 0L, NULL, NULL, 0 }
					};

				if((editor_mcc = MUI_CreateCustomClass(NULL, "TextEditor.mcc", NULL, 0, (void *)TextEditor_Dispatcher))/* && (toolbar_mcc = MUI_CreateCustomClass(NULL, "Toolbar.mcc", NULL, 0, (void *)Toolbar_Dispatcher))*/)
				{
					app = ApplicationObject,
								MUIA_Application_Author,      "Allan Odgaard",
								MUIA_Application_Base,        "TextEditor-Demo",
								MUIA_Application_Copyright,   "®1997 Allan Odgaard",
								MUIA_Application_Description, "TextEditor.mcc demonstration program",
								MUIA_Application_RexxHook,		&ARexxHook,
								MUIA_Application_Title,       "TextEditor-Demo",
								MUIA_Application_Version,     "$VER: TextEditor-Demo V1.0 (6-Aug-97)",
								MUIA_Application_UsedClasses, classes,
								SubWindow, window = WindowObject,
									MUIA_Window_ID,         'MAIN',
									WindowContents, VGroup,
										Child, VGroup,
											MUIA_Background, MUII_GroupBack,
											MUIA_Frame, MUIV_Frame_Group,

											Child, HGroup,
//												Child, VirtgroupObject,
													Child, ToolstripObj = ToolbarObject,
														MUIA_Toolbar_Description, Toolstrip,
														MUIA_Toolbar_ImageNormal, "ProgDir:Toolstrip.ILBM",
														MUIA_Toolbar_ImageGhost, "ProgDir:Toolstrip_G.ILBM",
														MUIA_Toolbar_ImageType, MUIV_Toolbar_ImageType_File,
														MUIA_Font, MUIV_Font_Tiny,
														End,
//													End,
//												Child, color = MUI_MakeObject(MUIO_Cycle, NULL, colors),
												Child, RectangleObject, End,
												End,

											Child, HGroup,
												Child, HGroup,
													MUIA_Group_Spacing, 0,
													Child, editorgad = NewObject(editor_mcc->mcc_Class, NULL,
														MUIA_TextEditor_Slider, slider,
														MUIA_TextEditor_ColorMap, cmap,
														MUIA_CycleChain, TRUE,
														End,
													Child, slider = ScrollbarObject,
														End,
													End,
												End,
											Child, StatusLine = InfoTextObject,
												End,

											End,
										End,
									End,
								End;

					if(app)
					{
							STRPTR	ARexxPort;
							ULONG		sigs;
							ULONG		running = 1;
							BPTR		fh;

						if(argarray[5])
						{
							set(editorgad, MUIA_TextEditor_FixedFont, TRUE);
						}
						if(argarray[0] && (fh = Open((STRPTR)argarray[0], MODE_OLDFILE)))
						{
								STRPTR	text = AllocVec(300*1024, 0L);
								STRPTR	buffer = text;
								LONG		size;

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

						DoMethod(ToolstripObj, MUIM_Notify, MUIA_Toolbar_HelpString, MUIV_EveryTime, StatusLine,	3, MUIM_Set, MUIA_Text_Contents, MUIV_TriggerValue);

						DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Application, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
						DoMethod(window, MUIM_Notify, MUIA_Window_InputEvent, "f1", MUIV_Notify_Application, 2, MUIM_Application_ReturnID, MUIV_RunARexxScript);

						/* Mew, cut, copy, paste & undo */
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, NEW,   MUIV_Toolbar_Notify_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "CLEAR");
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, NEW,   MUIV_Toolbar_Notify_Pressed, FALSE, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_HasChanged, FALSE);
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, CUT,   MUIV_Toolbar_Notify_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "CUT");
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, COPY,  MUIV_Toolbar_Notify_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "COPY");
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, PASTE, MUIV_Toolbar_Notify_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "PASTE");
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, UNDO,  MUIV_Toolbar_Notify_Pressed, FALSE, editorgad, 2, MUIM_TextEditor_ARexxCmd, "UNDO");

						/* Bold, italic, underline & colored */
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, BOLD,      MUIV_Toolbar_Notify_Pressed, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_StyleBold,      MUIV_TriggerValue);
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, ITALIC,    MUIV_Toolbar_Notify_Pressed, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_StyleItalic,    MUIV_TriggerValue);
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, UNDERLINE, MUIV_Toolbar_Notify_Pressed, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_StyleUnderline, MUIV_TriggerValue);
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, COLORED,   MUIV_Toolbar_Notify_Pressed, TRUE,           editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_Pen,            7);
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, COLORED,   MUIV_Toolbar_Notify_Pressed, FALSE,          editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_Pen,            0);
						DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_StyleBold,      MUIV_EveryTime, ToolstripObj, 4, MUIM_Toolbar_Set, BOLD,      MUIV_Toolbar_Set_Selected, MUIV_TriggerValue);
						DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_StyleItalic,    MUIV_EveryTime, ToolstripObj, 4, MUIM_Toolbar_Set, ITALIC,    MUIV_Toolbar_Set_Selected, MUIV_TriggerValue);
						DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_StyleUnderline, MUIV_EveryTime, ToolstripObj, 4, MUIM_Toolbar_Set, UNDERLINE, MUIV_Toolbar_Set_Selected, MUIV_TriggerValue);
						DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_Pen,            7,              ToolstripObj, 4, MUIM_Toolbar_Set, COLORED,   MUIV_Toolbar_Set_Selected, TRUE);
						DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_Pen,            0,              ToolstripObj, 4, MUIM_Toolbar_Set, COLORED,   MUIV_Toolbar_Set_Selected, FALSE);

						/* Left, center & right */
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, LEFT,   MUIV_Toolbar_Notify_Pressed, TRUE, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_Flow, MUIV_TextEditor_Flow_Left);
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, CENTER, MUIV_Toolbar_Notify_Pressed, TRUE, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_Flow, MUIV_TextEditor_Flow_Center);
						DoMethod(ToolstripObj, MUIM_Toolbar_Notify, RIGHT,  MUIV_Toolbar_Notify_Pressed, TRUE, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_Flow, MUIV_TextEditor_Flow_Right);
						DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_Flow, MUIV_TextEditor_Flow_Left,   ToolstripObj, 4, MUIM_Toolbar_Set, LEFT,   MUIV_Toolbar_Set_Selected, TRUE);
						DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_Flow, MUIV_TextEditor_Flow_Center, ToolstripObj, 4, MUIM_Toolbar_Set, CENTER, MUIV_Toolbar_Set_Selected, TRUE);
						DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_Flow, MUIV_TextEditor_Flow_Right,  ToolstripObj, 4, MUIM_Toolbar_Set, RIGHT,  MUIV_Toolbar_Set_Selected, TRUE);

						/* AreaMarked? UndoAvailable? */
						DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_AreaMarked, MUIV_EveryTime, ToolstripObj, 6, MUIM_Toolbar_MultiSet, MUIV_Toolbar_Set_Ghosted, MUIV_NotTriggerValue, CUT,  COPY, -1);
						DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_UndoAvailable, MUIV_EveryTime, ToolstripObj, 4, MUIM_Toolbar_Set, UNDO, MUIV_Toolbar_Set_Ghosted, MUIV_NotTriggerValue);

						/* Unsupported... */
/*						DoMethod(color, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, editorgad, 3, MUIM_NoNotifySet, MUIA_TextEditor_Pen, MUIV_TriggerValue);
						DoMethod(editorgad, MUIM_Notify, MUIA_TextEditor_Pen, MUIV_EveryTime, color, 3, MUIM_NoNotifySet, MUIA_Cycle_Active, MUIV_TriggerValue);
*/
						get(app, MUIA_Application_Base, &ARexxPort);
						sprintf(wintitle, "TextEditor-Demo · ArexxPort: %s", ARexxPort);
						SetAttrs(window,	MUIA_Window_ActiveObject, editorgad,
												MUIA_Window_Title, wintitle,
												MUIA_Window_Open, TRUE,
												TAG_DONE);

						do	{
								struct MsgPort *myport = NULL;
								struct RexxMsg *rxmsg;
								ULONG	changed;
								ULONG ReturnID;
								BPTR	rxstdout = NULL;

							while((ReturnID = DoMethod(app, MUIM_Application_NewInput, &sigs)) != MUIV_Application_ReturnID_Quit)
							{
								if(ReturnID == MUIV_RunARexxScript && !myport)
								{
										struct MsgPort *rexxport;
										STRPTR script = "Rexx:TextEditor/Demo.Rexx";

									if((rexxport = FindPort("REXX")) && (myport = CreateMsgPort()))
									{
										if(!rxstdout)
											rxstdout = Open("Con://640/100/TextEditor-Demo, ARexx output:/Close/Wait/Auto/InActive", MODE_READWRITE);
										rxmsg = CreateRexxMsg(myport, NULL, ARexxPort);
										rxmsg->rm_Action = RXCOMM;
										rxmsg->rm_Stdin = rxstdout;
										rxmsg->rm_Stdout = rxstdout;
										rxmsg->rm_Args[0] = CreateArgstring(script, strlen(script));
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
										DeleteArgstring(rxmsg->rm_Args[0]);
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

						} while(running == 0);

						if(running == 2)
						{
								STRPTR text = (STRPTR)DoMethod(editorgad, MUIM_TextEditor_ExportText);

							if(fh = Open((STRPTR)argarray[0], MODE_NEWFILE))
							{
								Write(fh, text, strlen(text));
								Close(fh);
							}
							FreeVec(text);
						}
						MUI_DisposeObject(app);
					}
					else printf("Failed to create application\n");
					MUI_DeleteCustomClass(editor_mcc);
				}
				else printf("Failed to open TextEditor.mcc\n");
				CloseLibrary(MUIMasterBase);
			}
			else printf("Failed to open MUIMaster.Library V%d\n", MUIMASTER_VMIN);
			FreeArgs(args);
		}
		else
		{
				UBYTE	prgname[32];
				LONG	error = IoErr();

			GetProgramName(prgname, 32);
			PrintFault(error, prgname);
		}
		StackSwap(&stackswap);
		FreeVec(newstack);
	}
}

VOID wbmain (VOID)
{
}
