
#include <exec/types.h>
//#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include <clib/alib_protos.h>
#include <stdio.h>

#include <mui.h> /* Should be somewhen libraries/mui.h */

struct Library       *MUIMasterBase;

#ifdef _AROS
#warning FIXME: what is the solution for this?
Object *MUI_NewObject(char const *className, ULONG tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = MUI_NewObjectA(className, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
}

Object *MUI_MakeObject(long type, ULONG tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = MUI_MakeObjectA(type, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
}
#endif

#define _U(s) (s)

#ifndef _AROS

/* On AmigaOS we build a fake library base, because it's not compiled as sharedlibrary yet */
#include "muimaster_intern.h"

int openmuimaster(void)
{
    static struct MUIMasterBase_intern MUIMasterBase_instance;
    MUIMasterBase = (struct Library*)&MUIMasterBase_instance;

    MUIMasterBase_instance.sysbase = *((struct ExecBase **)4);
    MUIMasterBase_instance.dosbase = OpenLibrary("dos.library",37);
    MUIMasterBase_instance.utilitybase = OpenLibrary("utility.library",37);
    MUIMasterBase_instance.aslbase = OpenLibrary("asl.library",37);
    MUIMasterBase_instance.gfxbase = OpenLibrary("graphics.library",37);
    MUIMasterBase_instance.layersbase = OpenLibrary("layers.library",37);
    MUIMasterBase_instance.intuibase = OpenLibrary("intuition.library",37);
    MUIMasterBase_instance.cxbase = OpenLibrary("commodities.library",37);
    MUIMasterBase_instance.keymapbase = OpenLibrary("keymap.library",37);
    __zune_prefs_init(&__zprefs);

    return 1;
}

void closemuimaster(void)
{
}

#else

int openmuimaster(void)
{
    if ((MUIMasterBase = OpenLibrary("muimaster.library", 0))) return 1;
    return 0;
}

void closemuimaster(void)
{
    if (MUIMasterBase) CloseLibrary(MUIMasterBase);
}

#endif

ULONG xget(Object *obj, Tag attr)
{
  ULONG storage;
  GetAttr(attr, obj, &storage);
  return storage;
}

Object *SimpleChainedButton (STRPTR label)
{
    Object *obj;

    obj = MUI_MakeObject(MUIO_Button,label);
    set(obj, MUIA_CycleChain, TRUE);
    return obj;
}

Object *ChainedCheckmark (STRPTR label)
{
    Object *obj;

    obj = MUI_MakeObject(MUIO_Checkmark, _U(label));
    set(obj, MUIA_CycleChain, TRUE);
    return obj;
}

int main (int argc, char **argv)
{
    Object *app;
    Object *mainWin;
    Object *radio1;
    Object *radio2;
    int result = 0;

    if (!(openmuimaster())) return 20;

    app = ApplicationObject,
	SubWindow, mainWin = WindowObject,
	    MUIA_Window_Title, "Input modes",
	    WindowContents, VGroup,
	        Child, MUI_MakeObject(MUIO_BarTitle, _U("MUIV_InputMode_RelVerify")),
                Child, SimpleChainedButton("Hello world, \33u\33iyo\n"
					   "\33l\33iHello \33bworld\33n, yo\n"
					   "\33iHello world, yo\n"
					   "_Hello \33uwo\0331r\33bl\33n\33ud\33n, \33iyo\n"
					   "\33cI \33ilove MUI\n"
					   "HelloH \33b\33ihello\33nH"),
	        Child, MUI_MakeObject(MUIO_BarTitle, _U("MUIV_InputMode_Toggle")),
/*                  Child, VSpace(0), */
#warning FIXME: uncomment this when I have images
#if 0
	        Child, HGroup,
                    MUIA_Frame, MUIV_Frame_Group,
                    MUIA_FrameTitle, "The quick brown fox jumps over the lazy dog",
	            MUIA_Background, MUII_GroupBack,
	            Child, ChainedCheckmark("My first checkmark"),
                End,
#endif
	        Child, MUI_MakeObject(MUIO_BarTitle, _U("MUIV_InputMode_Immediate")),
	        Child, HGroup,
	            MUIA_Frame, MUIV_Frame_Group,
  	            MUIA_FrameTitle, "Radio",
  	            MUIA_FixHeight, 30,
	            Child, radio1 = RectangleObject,
                            MUIA_CycleChain, TRUE,
	                    MUIA_ControlChar, 'i',
	                    MUIA_InputMode, MUIV_InputMode_Immediate,
	                    MUIA_Background, MUII_ButtonBack,
                            MUIA_Frame, MUIV_Frame_Button,
	            End,
                    Child, radio2 = RectangleObject,
                            MUIA_CycleChain, TRUE,
	                    MUIA_ControlChar, 'o',
	                    MUIA_InputMode, MUIV_InputMode_Immediate,
	                    MUIA_Background, MUII_ButtonBack,
                            MUIA_Frame, MUIV_Frame_Button,
                    End,
                End,
            End,
        End,
    End;

    if (!app)
    {
	kprintf("can't create application object.\n");
	result = 20;
	goto error;
    }

kprintf("*** MUIM_Notify on MUIA_Window_CloseRequest...\n");
    DoMethod(mainWin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	     _U(app), 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

kprintf("*** set up radio1...\n");
    set(radio1, MUIA_Selected, TRUE);
    DoMethod(radio1, MUIM_Notify, MUIA_Selected, TRUE,
	     _U(radio2), 3, MUIM_Set, MUIA_Selected, FALSE);
    DoMethod(radio2, MUIM_Notify, MUIA_Selected, TRUE,
	     _U(radio1), 3, MUIM_Set, MUIA_Selected, FALSE);

    /*
     * Open window and ALWAYS check.
     */
kprintf("*** open window...\n");
    set(mainWin, MUIA_Window_Open, TRUE);
    if (!xget(mainWin, MUIA_Window_Open))
    {
	MUI_DisposeObject(app);
	kprintf("%s : can't open main window.\n", argv[0]);
	result = 5;
	goto error;
    }

    {
	ULONG sigs = 0;

kprintf("*** main loop...\n");
	while (DoMethod(app, MUIM_Application_NewInput, _U(&sigs))
	       != MUIV_Application_ReturnID_Quit)
	{
	    if (sigs)
	    {
	        sigs = Wait(sigs | SIGBREAKF_CTRL_C);
	        if (sigs & SIGBREAKF_CTRL_C) break;
	    }
	}
    }

kprintf("*** close window...\n");
    set(mainWin, MUIA_Window_Open, FALSE);
kprintf("*** dispose app...\n");
    MUI_DisposeObject(app);

error:

    closemuimaster();
    return result;
}
