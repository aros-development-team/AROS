/*
    Copyright © 2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ExecuteCommand functionality
    Lang: English
*/

/*********************************************************************************************/

#define   DEBUG  0
#include  <aros/debug.h>

#include  <dos/dos.h>
#include  <dos/filesystem.h>
#include  <intuition/intuitionbase.h>
#include  <intuition/intuition.h>
#include  <libraries/gadtools.h>

#include  <proto/intuition.h>
#include  <proto/gadtools.h>
#include  <proto/graphics.h>
#include  <proto/dos.h>
#include  <proto/alib.h>

/*********************************************************************************************/

extern struct Screen *wbscreen;
extern APTR vi;

/*********************************************************************************************/
#define ID_CMDSTRING 0
struct NewGadget gt_cmdstringgad =
{
  80, 40, 300, 20,
  "Command:", NULL,
  ID_CMDSTRING, PLACETEXT_LEFT, NULL, NULL
};
#define ID_CMDOK 1
struct NewGadget gt_cmdokgad =
{
  10, 65, 50, 15,
  "Ok", NULL,
  ID_CMDOK, PLACETEXT_IN, NULL, NULL
};
#define ID_CMDCANCEL 2
struct NewGadget gt_cmdcancelgad =
{
  340, 65, 50, 15,
  "Cancel", NULL,
  ID_CMDCANCEL, PLACETEXT_IN, NULL, NULL
};
/*********************************************************************************************/

void ExecuteCommand()
{
    struct Window *cmdwin;
    struct RastPort *rp;
    struct Gadget *gad, *cmdglist;
    struct IntuiMessage *msg;
    int finish = 0;
    char *cmdstring;
    
kprintf("LoadWB.ExecuteCommand\n");

#define EW_WIDTH 400
#define EW_HEIGHT 88

    cmdwin = OpenWindowTags( NULL,
	WA_Title,	"Execute Command...",
	WA_Flags,	(WFLG_ACTIVATE | WFLG_DRAGBAR) ,
	WA_IDCMP,	(IDCMP_GADGETUP),
	WA_Left,	0,
	WA_Top,		(wbscreen->Height-EW_HEIGHT) / 2,
	WA_Left,	(wbscreen->Width-EW_WIDTH) / 2,
	WA_Height,	EW_HEIGHT,
	WA_Width,	EW_WIDTH,
	TAG_DONE
    );

    rp = cmdwin->RPort;

    gt_cmdstringgad.ng_VisualInfo = vi;
    gt_cmdokgad.ng_VisualInfo = vi;
    gt_cmdcancelgad.ng_VisualInfo = vi;

    gad = CreateContext(&cmdglist);
    gad = CreateGadget( BUTTON_KIND, gad, &gt_cmdokgad, TAG_DONE);
    gad = CreateGadget( BUTTON_KIND, gad, &gt_cmdcancelgad, TAG_DONE);
    gad = CreateGadget( STRING_KIND, gad, &gt_cmdstringgad,
	GTST_String,	"",
	GTST_MaxChars,	128,
	GTTX_Border,	TRUE,
	TAG_DONE);

    AddGList(cmdwin, cmdglist, -1, -1, NULL);
    RefreshGList(cmdglist, cmdwin, NULL, -1);
    GT_RefreshWindow(cmdwin, NULL);

    Move(rp, cmdwin->BorderLeft+10, cmdwin->BorderTop+cmdwin->IFont->tf_YSize);
    SetAPen(rp, 1);
    Text(rp, "Enter Command and its Arguments:", 32);

    while(finish == 0)
    {
	WaitPort(cmdwin->UserPort);
	while((msg = GT_GetIMsg(cmdwin->UserPort)))
	{
	    switch(msg->Class)
	    {
		case IDCMP_GADGETUP:
		    switch( ((struct Gadget*)(msg->IAddress))->GadgetID )
		    {
			case ID_CMDSTRING:
			    break;

			case ID_CMDOK:
			    finish = 1;
			    break;

			case ID_CMDCANCEL:
			    finish = -1;
			    break;

			default:
			    break;
		    }
		    break;

		default:
		    break;
	    }
	}
    }
    if(finish == 1)
    {
	BPTR input;
	GT_GetGadgetAttrs(gad, cmdwin, NULL, GTST_String, &cmdstring, TAG_DONE);
	input = Open("CON:////AROS Shell/CLOSE/AUTO", FMF_READ);
	SystemTags(cmdstring,
	    SYS_Asynch,		TRUE,
	    SYS_Input,		input,
	    SYS_Output,		(IPTR)NULL,
	    SYS_Error,		(IPTR)NULL,
	    TAG_DONE);

    }

    RemoveGList(cmdwin, cmdglist, -1);
    FreeGadgets(cmdglist);
    CloseWindow(cmdwin);
}

/*********************************************************************************************/
