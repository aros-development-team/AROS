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
#endif

int main (int argc, char **argv)
{
    Object *app;
    Object *mainWin;

#ifdef AMIGA
    MUIMasterBase = OpenLibrary("muimaster.library", 0);
    if (MUIMasterBase == NULL) return 20;
    IntuitionBase = OpenLibrary("intuition.library", 36);
#else
    MUI_Init(&argc, &argv);
#endif

    app = ApplicationObject,
	SubWindow, mainWin = WindowObject,
	    MUIA_Window_Title, "Input modes",
	    WindowContents, VGroup,
	        Child, SliderObject,
                   MUIA_Numeric_Value, 50,
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
	     app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    set(mainWin, MUIA_Window_Open, TRUE);
    if (!xget(mainWin, MUIA_Window_Open))
    {
	MUI_DisposeObject(app);
	fprintf(stderr, "%s : can't open main window.\n", argv[0]);
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
