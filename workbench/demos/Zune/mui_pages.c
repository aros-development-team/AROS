#include <exec/types.h>
#ifdef _AROS
#define AMIGA
#endif

#ifdef AMIGA
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include <stdio.h>
#else
#include <zune/zune.h>
#endif

#ifdef AMIGA
struct IntuitionBase *IntuitionBase;
struct Library       *MUIMasterBase;

ULONG xget(Object *obj, Tag attr)
{
  ULONG storage;
  GetAttr(attr, obj, &storage);
  return storage;
}

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

int main (int argc, char **argv)
{
    Object *app;
    Object *mainWin;
    Object *pages;
    Object *stepb[2];
    Object *b[3];
    int i;

#ifdef AMIGA
    MUIMasterBase = OpenLibrary("muimaster.library", 0);
    if (MUIMasterBase == NULL) return 20;
    IntuitionBase = OpenLibrary("intuition.library", 36);
#else
    g_print("use '--gdk-debug all --sync' to debug events\n");

    MUI_Init(&argc, &argv);
#endif

    app = ApplicationObject,
	SubWindow, mainWin = WindowObject,
	    MUIA_Window_Title, "Pages",
	    WindowContents, VGroup,
                Child, VGroup,
   		    GroupFrame,
		    Child, HGroup,
			Child, stepb[0] = RectangleObject,
				MUIA_Rectangle_BarTitle, "prev",
	                        MUIA_CycleChain, TRUE,
				MUIA_InnerLeft, 20,
				MUIA_FixHeight, 20,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_Background, MUII_ButtonBack,
				ButtonFrame,
			End,
			Child, stepb[1] = RectangleObject,
				MUIA_Rectangle_BarTitle, "next",
	                        MUIA_CycleChain, TRUE,
				MUIA_InnerLeft, 20,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_Background, MUII_ButtonBack,
				ButtonFrame,
			End,
		    End,

	            Child, MUI_MakeObject(MUIO_HBar, 4),
		    Child, HGroup,
			Child, b[0] = RectangleObject,
				MUIA_Rectangle_BarTitle, "1",
	                        MUIA_CycleChain, TRUE,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_FixHeight, 20,
				MUIA_Background, MUII_ButtonBack,
				ButtonFrame,
			End,
			Child, b[1] = RectangleObject,
				MUIA_Rectangle_BarTitle, "2",
	                        MUIA_CycleChain, TRUE,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_FixHeight, 20,
				MUIA_Background, MUII_ButtonBack,
				ButtonFrame,
			End,
			Child, b[2] = RectangleObject,
				MUIA_Rectangle_BarTitle, "3",
	                        MUIA_CycleChain, TRUE,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_FixHeight, 20,
				MUIA_Background, MUII_ButtonBack,
				ButtonFrame,
			End,
		    End,
                End,
	        Child, pages = HGroup,
	            MUIA_Group_PageMode, TRUE,
	            InputListFrame,
	            MUIA_Background, MUII_PageBack,
	            Child, RectangleObject,
	                    MUIA_FixWidth, 50,
	                    MUIA_Background, MUII_SHADOW,
                            MUIA_Rectangle_BarTitle, "Rect1",
	            End,
                    Child, RectangleObject,
	                    MUIA_FixWidth, 30,
                            MUIA_FixHeight, 30,
	                    MUIA_Background, MUII_SHINE,
                            MUIA_Rectangle_BarTitle, "Rect2",
                    End,
                    Child, RectangleObject,
                            MUIA_FixHeight, 50,
	                    MUIA_Background, MUII_FILL,
                            MUIA_Rectangle_BarTitle, "Rect3",
                    End,
                End,
            End,
        End,
    End;

    if (!app)
    {
	fprintf(stderr, "can't create application object.\n");
	goto error;
    }

    DoMethod(mainWin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	     app, 2,
	     MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(stepb[0], MUIM_Notify, MUIA_Timer, MUIV_EveryTime,
	     pages, 3, MUIM_Set,
	     MUIA_Group_ActivePage, MUIV_Group_ActivePage_Prev);

    DoMethod(stepb[1], MUIM_Notify, MUIA_Timer, MUIV_EveryTime,
	     pages, 3, MUIM_Set,
	     MUIA_Group_ActivePage, MUIV_Group_ActivePage_Next);

    for (i = 0; i < 3; i++)
    {
	DoMethod(b[i], MUIM_Notify, MUIA_Pressed, FALSE,
		 pages, 3, MUIM_Set, MUIA_Group_ActivePage, i);
    }
    /*
     * Open window and ALWAYS check.
     */
    set(mainWin, MUIA_Window_Open, TRUE);
    if (!xget(mainWin, MUIA_Window_Open))
    {
	MUI_DisposeObject(app);
	fprintf(stderr, "can't open main window.\n");
	goto error;
    }

    {
	ULONG sigs = 0;

	while (DoMethod(app, MUIM_Application_NewInput, &sigs)
	       != MUIV_Application_ReturnID_Quit)
	{
#ifdef AMIGA
	    if (sigs)
	    {
	        sigs = Wait(sigs | SIGBREAKF_CTRL_C);
	        if (sigs & SIGBREAKF_CTRL_C) break;
	    }
#endif
	}
    }
    
    set(mainWin, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(app);

error:

#ifdef AMIGA
    CloseLibrary(IntuitionBase);
    CloseLibrary(MUIMasterBase);
#endif

    return 0;
}

/*** EOF ***/
