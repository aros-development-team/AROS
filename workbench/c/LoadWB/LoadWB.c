/*
    Copyright © 2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Load the Workbench
    Lang: English
*/

/******************************************************************************

    NAME

	LoadWB

    SYNOPSIS

    LOCATION

	Workbench:C

    FUNCTION

	Loads a *very* simple "Workbench".
 
    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#define   DEBUG  0
#include  <aros/debug.h>

#include  <dos/dos.h>
#include  <dos/notify.h>
#include  <dos/dostags.h>
#include  <dos/filesystem.h>
#include  <intuition/intuitionbase.h>
#include  <intuition/intuition.h>
#include  <libraries/gadtools.h>

#include  <proto/intuition.h>
#include  <proto/workbench.h>
#include  <proto/gadtools.h>
#include  <proto/dos.h>


static const char version[] = "$VER: LoadWB 0.1 (06.04.2002)\n";


static ULONG     notifysig;
struct MsgPort * notifyport;
struct Screen  * wbscreen;
struct Window  * wbwindow;
struct Menu *    menus;
APTR vi;

static struct NewMenu nm[] =
{
  {NM_TITLE, "Workbench"},
      {NM_ITEM, "Backdrop","B"},
      {NM_ITEM, "Execute Command...", "E"},
      {NM_ITEM, "Shell", "Z"},
      {NM_ITEM, "About...", "?"},
      {NM_ITEM, "Quit...", "Q"}
};


/*********************************************************************************************/
void Cleanup();
void ExecuteCommand();
/*********************************************************************************************/

LONG __detacher_must_wait_for_signal = SIGBREAKF_CTRL_F;
struct Process *__detacher_process = NULL;

void DoDetach(void)
{
kprintf("LoadWB.DoDetach\n");

    /* If there's a detacher, tell it to go away */
    if (__detacher_process)
    {
	Signal((struct Task *)__detacher_process, __detacher_must_wait_for_signal);
    }
    __detacher_process = NULL;
}

/*********************************************************************************************/

int InitWB()
{
kprintf("LoadWB.InitWB\n");

    notifyport = CreateMsgPort();

    if (!notifyport)
	Cleanup("Can't create notification msg port!\n");
	    
    notifysig = 1L << notifyport->mp_SigBit;

#warning RegisterWorkbench() crashes the system!
//    RegisterWorkbench(notifyport);

    wbscreen = LockPubScreen(NULL);
    vi = GetVisualInfoA(wbscreen, NULL);

    wbwindow = OpenWindowTags(NULL,
	WA_Title,	"AROS Workbench",
//	WA_Flags,	(WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_ACTIVATE) ,
	WA_Flags,	(WFLG_BACKDROP | WFLG_ACTIVATE) ,
	WA_IDCMP,	(IDCMP_MENUPICK),
	WA_Left,	0,
	WA_Top,		wbscreen->BarHeight + 1,
	WA_Width,	wbscreen->Width,
	WA_Height,	wbscreen->Height - wbscreen->BarHeight -1,
//	WA_Width,	128,
//	WA_Height,	96,
	TAG_DONE
	);

    notifysig |= 1L << wbwindow->UserPort->mp_SigBit;

    menus = CreateMenusA(nm, NULL);
    LayoutMenusA(menus, vi, NULL);
    SetMenuStrip(wbwindow, menus);
    OffMenu(wbwindow, FULLMENUNUM(0,0,NOSUB));
    OffMenu(wbwindow, FULLMENUNUM(0,1,NOSUB));

    return TRUE;
}

/*********************************************************************************************/

void Cleanup()
{
kprintf("LoadWB.Cleanup\n");

    CloseWindow(wbwindow);
    UnregisterWorkbench(notifyport);
}

/*********************************************************************************************/

void HandleNotify()
{
struct IntuiMessage *msg;
struct MenuItem *item;
ULONG class;
UWORD code;

kprintf("LoadWB.HandleNotify\n");

    msg = (struct IntuiMessage *) GetMsg( wbwindow->UserPort );
    class = msg->Class;
    code = msg->Code;

    switch(class)
    {
	case IDCMP_MENUPICK :
	    while(code != MENUNULL)
	    {
//		kprintf("Menu: %d %d %d\n", MENUNUM(code), ITEMNUM(code), SUBNUM(code));
		switch(MENUNUM(code))
		{
		    case 0: /* Workbench */
			switch(ITEMNUM(code))
			{
			    case 0: /* Backdrop */
				break;
			    case 1: /* Execute Command... */
				ExecuteCommand();
				break;
			    case 2: /* Shell */
				{
				BPTR win = Open("CON:10/10/640/480/AROS-Shell/CLOSE", FMF_READ);
				SystemTags("",
				    SYS_Asynch, 	TRUE,
				    SYS_Background,	FALSE,
				    SYS_Input,		win,
				    SYS_Output,		(IPTR)NULL,
				    SYS_Error,		(IPTR)NULL,
				    SYS_UserShell,	TRUE,
				    TAG_DONE
				    );
				}
				break;
			    case 3: /* About... */
				{
				struct EasyStruct es;
				es.es_StructSize = sizeof(es);
				es.es_Flags = 0;
				es.es_Title = "About AROS Workbench...";
				es.es_TextFormat = "Written by Henning Kiel <hkiel@aros.org>\nCopyright © 2002, The AROS Development Team.\nAll rights reserved.";
				es.es_GadgetFormat = "Ok";
				EasyRequest ( wbwindow, &es, NULL, NULL, NULL );
				}
				break;
			    case 4: /* Quit... */
				Cleanup();
				exit(RETURN_OK);
				break;
			}
		}
		if ( (item = ItemAddress ( menus, code ) ) != NULL )
		{
		    code = item->NextSelect;
		}
		{
		    code = MENUNULL;
		}

	    }
	    break;

	default:
	    break;

    }
    ReplyMsg ( (struct Message *)msg );

}

/*********************************************************************************************/

void HandleAll()
{
    ULONG sigs;

kprintf("LoadWB.HandleAll\n");

    for(;;)
    {
	sigs = Wait(notifysig | SIGBREAKF_CTRL_C);

	if (sigs & SIGBREAKF_CTRL_C) break;

	if (sigs & notifysig) HandleNotify();
    }
}

/*********************************************************************************************/

void ExecuteCommand()
{
kprintf("LoadWB.ExecuteCommand\n");
}

/*********************************************************************************************/

int main(void)
{
    if( InitWB() == 0 )
	return RETURN_FAIL;

    DoDetach();

    HandleAll();

    Cleanup();
    
    return RETURN_OK;
}

