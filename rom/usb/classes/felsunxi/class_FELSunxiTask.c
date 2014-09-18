/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <clib/alib_protos.h>

#include "felsunxi_intern.h"

/*
    FELSunxiTask
*/
AROS_UFH0(void, FELSunxiTask) {
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct FELSunxiDevice  *FELSunxiDevice;

    struct Library         *ps;
    struct Library         *MUIMasterBase;

    thistask = FindTask(NULL);
    FELSunxiDevice = thistask->tc_UserData;

    ps = FELSunxiDevice->ps;
    MUIMasterBase = FELSunxiDevice->MUIMasterBase;

    mybug(-1,("FELSunxiTask started\n"));

    Object *app, *wnd, *b1, *b2, *b3, *tick1;
    
	app = ApplicationObject,
	SubWindow, wnd = WindowObject,
		MUIA_Window_Title, "FELSunxi",
		MUIA_Window_Activate, TRUE,
			WindowContents, HGroup,
				Child, VGroup,
					MUIA_Weight, 1,
					MUIA_Group_SameWidth, TRUE,
					GroupFrameT("FELSunxi"),
			    	Child, b1 = SimpleButton("DRAM"),
			    	Child, b2 = SimpleButton("NAND"),
			    	Child, b3 = SimpleButton("SERIAL"),
					Child, HGroup,
						Child, tick1 = MUI_MakeObject(MUIO_Checkmark, NULL),
						Child, MUI_MakeObject(MUIO_Label,"Show object box", 0),
					End,
					Child, (IPTR) VSpace(0),

				End,
			End,
		End,
	End;

	if (app) {

        mybug(-1,("Zune app up!\n"));

        mybug(-1,("FELSunxiTask signaling creator\n"));
        Signal(FELSunxiDevice->readysigtask, 1L<<FELSunxiDevice->readysignal);

		ULONG sigs = 0;

		DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR) app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
		set(wnd,MUIA_Window_Open,TRUE);

		while(1) {

            if( DoMethod(app, MUIM_Application_NewInput, (IPTR) &sigs) == MUIV_Application_ReturnID_Quit ) {
                psdReleaseDevBinding(FELSunxiDevice->pd);
            }

			if (sigs) {
				sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
				if(sigs & SIGBREAKF_CTRL_C) break;
				if(sigs & SIGBREAKF_CTRL_D) break;
			}
		}

		MUI_DisposeObject(app);
	}

    psdFreeVec(FELSunxiDevice);
    /* This is why we copied the bases to stack */
	CloseLibrary(MUIMasterBase);
    CloseLibrary(ps);

    AROS_USERFUNC_EXIT
}
