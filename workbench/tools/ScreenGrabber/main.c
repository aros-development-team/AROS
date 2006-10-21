/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/debug.h>
#include <datatypes/datatypes.h>
#include <datatypes/pictureclass.h>
#include <intuition/classes.h>
#include <dos/bptr.h>
#include <cybergraphx/cybergraphics.h>
#include <libraries/mui.h>
#include <libraries/asl.h>

#include <proto/dos.h>
#include <proto/datatypes.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>

#include <stdio.h>
#include "locale.h"

#define APPNAME "ScreenGrabber"
#define VERSION "ScreenGrabber 0.4 (29.06.2006)"

static const char version[] = "$VER: " VERSION " ©2006 AROS Dev Team";

static Object *app, *MainWindow, *ScreenList, *FilenameString, *SaveButton, *RefreshButton, *GrabButton;
static Object *Size, *Title, *DefTitle, *Delay, *Hide, *Progress;

static Object *DTImage = NULL;

static struct Hook display_hook;
static struct Hook refresh_hook;
static struct Hook select_hook;
static struct Hook grab_hook;
static struct Hook save_hook;

AROS_UFH3(void, display_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(char **, strings, A2),
    AROS_UFHA(struct PubScreenNode *, psn, A1))
{
    AROS_USERFUNC_INIT

    static char buff[200];

    if (psn)
    {
	snprintf(buff, sizeof(buff)-1, "%s", psn->psn_Node.ln_Name);
	strings[0] = buff;
    }

    AROS_USERFUNC_EXIT
}

static void RefreshScreens()
{
    struct List *list;
    struct PubScreenNode *psn;
    
    DoMethod(ScreenList, MUIM_List_Clear);
    
    list = LockPubScreenList();

    ForeachNode(list, psn)
    {
	DoMethod(ScreenList, MUIM_List_InsertSingle, (IPTR)psn, MUIV_List_Insert_Bottom);
    }

    UnlockPubScreenList();
}

AROS_UFH3(void, refresh_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(char **, dummy, A2),
    AROS_UFHA(struct Screen *, dummy2, A1))
{
    AROS_USERFUNC_INIT

    RefreshScreens();

    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, select_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    ULONG active = XGET(object, MUIA_List_Active);

    if (active != MUIV_List_Active_Off)
    {
	struct PubScreenNode *psn;
	struct Screen *screen;
	char buff[200];
	
	DoMethod(object, MUIM_List_GetEntry, active, (IPTR)&psn);
	screen=psn->psn_Screen;

	snprintf(buff, sizeof(buff)-1, _(MSG_SCREEN_PARM),
	    screen->Width, screen->Height, GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH));
	set(Size, MUIA_Text_Contents, buff);
	set(Title, MUIA_Text_Contents, screen->Title);
	set(DefTitle, MUIA_Text_Contents, screen->DefaultTitle);

	set(GrabButton, MUIA_Disabled, FALSE);
    }
    else
    {
	set(GrabButton, MUIA_Disabled, TRUE);
	set(Size, MUIA_Text_Contents, "");
	set(Title, MUIA_Text_Contents, "");
	set(DefTitle, MUIA_Text_Contents, "");
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, grab_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(char **, dummy, A2),
    AROS_UFHA(struct Screen *, dummy2, A1))
{
    AROS_USERFUNC_INIT

    ULONG delay,active;
    BOOL hide_win;
    struct PubScreenNode *psn;
    struct Screen *screen;

    delay = XGET(Delay, MUIA_Numeric_Value) * 10;
    hide_win = XGET(Hide, MUIA_Selected);
   
    active = XGET(ScreenList, MUIA_List_Active);
    DoMethod(ScreenList, MUIM_List_GetEntry, active, (IPTR)&psn);
    screen = LockPubScreen(psn->psn_Node.ln_Name);

    D(bug("delay=%d, hide=%d\n", delay, hide_win));

    if (screen)
    {
	APTR dst;

	if (delay)
	{
	    set(Progress, MUIA_Gauge_Current, delay);
	    set(Progress, MUIA_Gauge_Max, delay);
	}
	else if (hide_win) set(MainWindow, MUIA_Window_Open, FALSE);

	while(delay) {
	    delay--;
	    set(Progress, MUIA_Gauge_Current, delay);
	    Delay(5);
	    if ((delay < 5) && hide_win) set(MainWindow, MUIA_Window_Open, FALSE);
	}

	dst = AllocVec(4*screen->Width, MEMF_ANY);
	if (dst)
	{

	    if (DTImage) DisposeDTObject(DTImage);

	    DTImage = NewDTObject((APTR)NULL,
		    DTA_SourceType, DTST_RAM,
		    DTA_BaseName, (IPTR)"jpeg",
		    PDTA_DestMode, PMODE_V43,
		    TAG_DONE);

	    if (DTImage)
	    {
		struct BitMapHeader *bmhd;

		if ((GetDTAttrs(DTImage, PDTA_BitMapHeader, (IPTR)&bmhd, TAG_DONE)))
		{
		    ULONG y;
		    struct pdtBlitPixelArray dtb;
		    dtb.MethodID = PDTM_WRITEPIXELARRAY;
		    dtb.pbpa_PixelData = dst;
		    dtb.pbpa_PixelFormat = PBPAFMT_ARGB;
		    dtb.pbpa_PixelArrayMod = screen->Width;
		    dtb.pbpa_Left = 0;
		    dtb.pbpa_Width = screen->Width;
		    dtb.pbpa_Height = 1;

		    bmhd->bmh_Width = screen->Width;
		    bmhd->bmh_Height = screen->Height;
		    bmhd->bmh_Depth = 24;
		    bmhd->bmh_PageWidth = 320;
		    bmhd->bmh_PageHeight = 240;

		    for (y=0; y < screen->Height; y++)
		    {
			ReadPixelArray(dst, 0, 0, 4*screen->Width, &screen->RastPort, 0, y, screen->Width, 1, RECTFMT_ARGB);
			dtb.pbpa_Top = y;

			DoMethodA(DTImage, (Msg) &dtb);
		    }

		    set(SaveButton, MUIA_Disabled, FALSE);
		}
	    }
	    else
	    {
		MUI_Request(app, MainWindow, 0, _(MSG_ERROR_TITLE), _(MSG_GAD_OK), _(MSG_ERR_DATATYPE), NULL );
	    }

	    FreeVec(dst);
	}
	else
	{
	    MUI_Request(app, MainWindow, 0, _(MSG_ERROR_TITLE), _(MSG_GAD_OK), _(MSG_NORAM), NULL);
	}


	if (hide_win)
	    set(MainWindow, MUIA_Window_Open, TRUE);

	UnlockPubScreen(NULL, screen);
    }
    
    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, save_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(char **, dummy, A2),
    AROS_UFHA(struct Screen *, dummy2, A1))
{
    AROS_USERFUNC_INIT

    struct dtWrite dtw;
    UBYTE *filename;
    UBYTE *infofilename;
    BPTR fh;
    BPTR oldfile;
    BPTR oldinfofile;

    filename = (UBYTE*)XGET(FilenameString, MUIA_String_Contents);

    // Does file or icon already exist?
    infofilename = AllocVec(strlen(filename) + 6, MEMF_ANY);
    if ( infofilename == NULL )
    {
	MUI_Request(app, MainWindow, 0, _(MSG_ERROR_TITLE), _(MSG_GAD_OK), _(MSG_NORAM), NULL);
	return;
    }
    strcpy(infofilename, filename);
    strcat(infofilename, ".info");

    oldfile = Open(filename, MODE_OLDFILE);
    oldinfofile = Open(infofilename, MODE_OLDFILE);
    if (oldfile || oldinfofile)
    {
	if (oldfile) Close(oldfile);
	if (oldinfofile) Close(oldinfofile);
	if (MUI_Request(app, MainWindow, 0, _(MSG_ERROR_TITLE), _(MSG_GAD_YESNO), _(MSG_OVERWRITE), filename ) == 0)
	{
	    FreeVec(infofilename);
	    return;
	}
    }
    FreeVec(infofilename);
    
    if ((fh = Open(filename, MODE_NEWFILE)))
    {
	dtw.MethodID = DTM_WRITE;
	dtw.dtw_GInfo = NULL;
	dtw.dtw_FileHandle = fh;
	dtw.dtw_Mode = DTWM_RAW;
	dtw.dtw_AttrList = NULL;

	DoMethodA(DTImage, (Msg) &dtw);
	if (IoErr())
	{
	    MUI_Request(app, MainWindow, 0, _(MSG_ERROR_TITLE), _(MSG_GAD_OK), _(MSG_ERR_DATATYPEIO), filename, IoErr() );
	}

	Close(fh);
    }
    else
    {
	MUI_Request(app, MainWindow, 0, _(MSG_ERROR_TITLE), _(MSG_GAD_OK), _(MSG_ERR_OPENFILE), filename);
    }

    AROS_USERFUNC_EXIT
}




BOOL GUIInit()
{
    BOOL retval = FALSE;
    D(bug("GUIInit()\n"));
    
    app = ApplicationObject,
	    MUIA_Application_Title, (IPTR)APPNAME,
	    MUIA_Application_Version, (IPTR)VERSION,
	    MUIA_Application_Copyright, (IPTR)"© 2004-2006, The AROS Development Team",
	    MUIA_Application_Author, (IPTR)"Michal Schulz",
	    MUIA_Application_Description, _(MSG_WINDOW_TITLE),
	    MUIA_Application_Base, (IPTR)"SCREENGRABBER",

	    SubWindow, MainWindow = WindowObject,
		MUIA_Window_Title, _(MSG_WINDOW_TITLE),
		MUIA_Window_ID, MAKE_ID('S','G','W','N'),
		WindowContents, HGroup,
		    MUIA_Group_SameWidth, FALSE,
		    Child, VGroup,
			MUIA_Weight, 100,
			Child, ListviewObject,
			    MUIA_Listview_List, ScreenList = ListObject,
				InputListFrame,
				MUIA_List_AdjustWidth, FALSE,
				MUIA_List_DisplayHook, &display_hook,
				End,
			    End,
			Child, RefreshButton = MUI_MakeObject(MUIO_Button, _(MSG_REFRESH)),
		        End,    // VGroup
		    Child, VGroup,
			MUIA_Weight, 200,
			Child, VGroup,
			    TextFrame,
			    MUIA_Background, MUII_PageBack,
			    InnerSpacing(5,5),
			    Child, VGroup, GroupFrameT(_(MSG_SCREEN_INFO)),
				Child, ColGroup(2),
				    Child, Label(_(MSG_SIZE)),
				    Child, Size = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "",
				    End,
				End,
				Child, ColGroup(2),
				    Child, Label(_(MSG_TITLE)),
				    Child, Title = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "",
				    End,
				End,
				Child, ColGroup(2),
				    Child, Label(_(MSG_DEFAULT_TITLE)),
				    Child, DefTitle = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "",
				    End,
				End,
			    End,
			    Child, VGroup, GroupFrameT(_(MSG_GRABBING_OPTIONS)),
				Child, ColGroup(2),
				    Child, Label(_(MSG_DELAY)),
				    Child, Delay = SliderObject,
					MUIA_Numeric_Min, 0,
					MUIA_Numeric_Max, 30,
					MUIA_Numeric_Value, 0,
				    End,
				End,
				Child, HGroup,
				    Child, HVSpace,
				    Child, Label(_(MSG_HIDE)),
				    Child, Hide = MUI_MakeObject(MUIO_Checkmark, "aaaa"),
				End,
			    End,
			    Child, VGroup, GroupFrameT(_(MSG_SAVE_OPTIONS)),
				Child, PopaslObject,
				    ASLFR_DoSaveMode, TRUE,
				    MUIA_Popstring_String, FilenameString = MUI_MakeObject(MUIO_String, NULL, 200),
				    MUIA_Popstring_Button, PopButton(MUII_PopFile),
				End,
				Child, SaveButton = MUI_MakeObject(MUIO_Button, _(MSG_SAVE_FILE)),
			    End,
			    Child, Progress = GaugeObject,
				GaugeFrame,
				MUIA_Gauge_Horiz, TRUE,
				MUIA_FixHeight, 8,
				MUIA_Gauge_Current, 0,
			    End,
			End,
		    
			Child, GrabButton = MUI_MakeObject(MUIO_Button, _(MSG_SCREENSHOT)),
		    End,
		End, // WindowContents
	    End, // WindowObject
	End; // ApplicationObject

    if (app)
    {
	D(bug("app=%p\n",app));
	set(SaveButton, MUIA_Disabled, TRUE);
	set(GrabButton, MUIA_Disabled, TRUE);

	DoMethod(RefreshButton, MUIM_Notify, MUIA_Pressed, FALSE,
	    (IPTR)app, 2, MUIM_CallHook, (IPTR)&refresh_hook);

	DoMethod(GrabButton, MUIM_Notify, MUIA_Pressed, FALSE,
	    (IPTR)app, 2, MUIM_CallHook, (IPTR)&grab_hook);

	DoMethod(SaveButton, MUIM_Notify, MUIA_Pressed, FALSE,
	    (IPTR)app, 2, MUIM_CallHook, (IPTR)&save_hook);

	DoMethod(ScreenList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
	    (IPTR)ScreenList, 2, MUIM_CallHook, (IPTR)&select_hook);

	DoMethod(MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	    (IPTR)app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	retval = TRUE;
    }

    return retval;
}

int main()
{
    display_hook.h_Entry = (APTR)display_function;
    refresh_hook.h_Entry = (APTR)refresh_function;
    select_hook.h_Entry = (APTR)select_function;
    grab_hook.h_Entry = (APTR)grab_function;
    save_hook.h_Entry = (APTR)save_function;
    if (GUIInit())
    {
	set(MainWindow, MUIA_Window_Open, TRUE);
	if (XGET(MainWindow, MUIA_Window_Open))
	{
	    RefreshScreens();
	    DoMethod(app, MUIM_Application_Execute);
	}
	set(MainWindow, MUIA_Window_Open, FALSE);
	MUI_DisposeObject(app);
	if (DTImage) DisposeDTObject(DTImage);
	DTImage = NULL;
    }
    else
    {
	MUI_Request(NULL, NULL, 0, _(MSG_ERROR_TITLE), _(MSG_GAD_OK), _(MSG_ERR_APP), NULL);
	return RETURN_FAIL;
    }

    return RETURN_OK;
}
