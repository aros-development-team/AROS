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

#include <dos/dos.h>
#include <dos/notify.h>
#include <dos/dostags.h>
#include <dos/filesystem.h>
#include <intuition/intuitionbase.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#include <proto/intuition.h>
#include <proto/workbench.h>
#include <proto/gadtools.h>
#include <proto/dos.h>
#include <proto/alib.h>

#include <stdlib.h>

static const char version[] = "$VER: LoadWB 0.1 (06.04.2002)\n";
static const char easteregg[] = "Brought to you by the wizzards of AROS!!!";

static ULONG     notifysig;
struct MsgPort * notifyport;
struct Screen  * wbscreen;
struct Window  * wbwindow;
struct Menu *    menus;
APTR vi;
static ULONG     check;


static struct NewMenu nm[] =
{
  {NM_TITLE, "Workbench"              },
    {NM_ITEM,  "Backdrop",           "B", CHECKIT | CHECKED},
    {NM_ITEM,  "Execute Command...", "E"},
    {NM_ITEM,  "Redraw All" },
    {NM_ITEM,  "Update All" },
    {NM_ITEM,  "Last Message" },
    {NM_ITEM,  "Shell",              "Z"},
    {NM_ITEM,  "About...",           "?"},
    {NM_ITEM,  "Quit...",            "Q"},

  {NM_TITLE, "Window",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "New Drawer", "N"},
    {NM_ITEM,  "Open Parent" },
    {NM_ITEM,  "Close", "K"},
    {NM_ITEM,  "Update" },
    {NM_ITEM,  "Select Contents", "A"},
    {NM_ITEM,  "Clean Up", "."},
    {NM_ITEM,  "Snapshot" },
      {NM_SUB, "Window"},
      {NM_SUB, "All"},
    {NM_ITEM,  "Show" },
      {NM_SUB, "Only Icons", CHECKIT | CHECKED, 32},
      {NM_SUB, "All Files", CHECKIT, 16 },
    {NM_ITEM,  "View By" },
      {NM_SUB, "Icon", CHECKIT | CHECKED, 32 + 64 + 128},
      {NM_SUB, "Name",CHECKIT, 16 + 64 + 128},
      {NM_SUB, "Size",CHECKIT, 16 + 32 + 128},
      {NM_SUB, "Date", CHECKIT, 16 + 32 + 64},

  {NM_TITLE, "Icon",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "Open", "O"},
    {NM_ITEM,  "Close","C" },
    {NM_ITEM,  "Rename...", "R"},
    {NM_ITEM,  "Information...", "I" },
    {NM_ITEM,  "Snapshot", "S" },
    {NM_ITEM,  "Unsnapshot", "U" },
    {NM_ITEM,  "Leave Out", "L" },
    {NM_ITEM,  "Put Away", "P" },
    {NM_ITEM, NM_BARLABEL},
    {NM_ITEM,  "Delete..." },
    {NM_ITEM,  "Format Disk..." },
    {NM_ITEM,  "Empty Trash..." },

  {NM_TITLE, "Tools",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "ResetWB" },
    {NM_ITEM,  "Matt Parsons did a good job?" },
      {NM_SUB, "Yeah, Kinda...", CHECKIT | CHECKED, 32},
      {NM_SUB, "Nope, this sucks!", CHECKIT, 16 },
  {NM_END}

};


/*********************************************************************************************/
void Cleanup(STRPTR msg);
void ExecuteCommand();
void backdrop(ULONG);
/*********************************************************************************************/

LONG            __detacher_must_wait_for_signal = SIGBREAKF_CTRL_F;
struct Process *__detacher_process              = NULL;
STRPTR          __detached_name                 = "Workbench";

void DoDetach(void)
{
    kprintf("LoadWB.DoDetach\n");

    /* If there's a detacher, tell it to go away */
    if (__detacher_process)
    {
	Signal((struct Task *)__detacher_process, __detacher_must_wait_for_signal);
    }
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
    WA_ScreenTitle, "AROS Workbench Alpha 0.1 (No file navigation) FreeMem-xxxxxxx",
	WA_Title,	    "AROS Workbench Alpha Version 0.1 (No file navigation)",
	WA_Flags,	   (WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_ACTIVATE) ,
//	WA_Flags,	   (WFLG_BACKDROP | WFLG_ACTIVATE) ,
	WA_IDCMP,	   (IDCMP_MENUPICK),
	WA_Left,	   0,
	WA_Top,		   wbscreen->BarHeight + 1,
	WA_Width,	   wbscreen->Width,
	WA_Height,	   wbscreen->Height - wbscreen->BarHeight -1,
//	WA_Width,	   128,
//	WA_Height,	   96,
	TAG_DONE
	);

    notifysig |= 1L << wbwindow->UserPort->mp_SigBit;

    menus = CreateMenusA(nm, NULL);
    LayoutMenusA(menus, vi, NULL);
    SetMenuStrip(wbwindow, menus);
    OffMenu(wbwindow, FULLMENUNUM(0,0,NOSUB));

    return TRUE;
}

/*********************************************************************************************/

void Cleanup(STRPTR msg)
{
    kprintf("LoadWB.Cleanup\n");

    if (msg)
    {
        FPuts(Error(), msg);
    }

    CloseWindow(wbwindow);
    UnregisterWorkbench(notifyport);

    DoDetach();

    exit(msg ? RETURN_FAIL : RETURN_OK);
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
//                backdrop()  need to read menu status!!!
      			break;
			    case 1: /* Execute Command... */
				ExecuteCommand();
				break;
			    case 2: /* Shell */
			    {
				BPTR win = Open("CON:10/10/640/480/AROS-Shell/CLOSE", FMF_READ);
				SystemTags
                                (
                                    "",
				    SYS_Asynch,     TRUE,
				    SYS_Background, FALSE,
				    SYS_Input,	    (IPTR)win,
				    SYS_Output,	    (IPTR)NULL,
				    SYS_Error,	    (IPTR)NULL,
				    SYS_UserShell,  TRUE,
				    TAG_DONE
				);

                                break;
 			    }
 			    case 3: /* About... */
			    {
				struct EasyStruct es;

                                es.es_StructSize   = sizeof(es);
				es.es_Flags        = 0;
				es.es_Title        = "About AROS Workbench...";
				es.es_TextFormat   = "Written by Henning Kiel <hkiel@aros.org>\nCopyright © 2002, The AROS Development Team.\nAll rights reserved.\n\nAROS 0.7x ROM (Alpha)\nWe made it...\nThe AROS Development team: Aaron Digulla, Georg Steger,\nNils Henrik Lorentzen, Henning Kiel, Staf Verhaegen,\nHenrik Berglund, Michal Schulz, Iain Templeton,\nFabio Alemagna, Sebastian Heutling, Johan Grip,\nTobias Seiler, Johan Alfredsson, Adam Chodorowski,\nMatt Parsons...\nTo be continued...";
				es.es_GadgetFormat = "Better than ever!";
				EasyRequest ( wbwindow, &es, NULL, NULL, NULL );

                                break;
		            }
			    case 4: /* Quit... */
				Cleanup(NULL);
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
 void backdrop(check)
{
    if (check) = FALSE
    {
        CloseWindow(wbwindow);
             wbwindow = OpenWindowTags(NULL,
    WA_ScreenTitle, "AROS Workbench Alpha 0.1 (No file navigation) FreeMem-xxxxxxx",
	WA_Title,	    "AROS Workbench Alpha Version 0.1 (No file navigation)",
//	WA_Flags,	   (WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_ACTIVATE) ,
	WA_Flags,	   (WFLG_ACTIVATE) ,
	WA_IDCMP,	   (IDCMP_MENUPICK),
	WA_Left,	   0,
	WA_Top,		   wbscreen->BarHeight + 1,
	WA_Width,	   wbscreen->Width,
	WA_Height,	   wbscreen->Height - wbscreen->BarHeight -1,
//	WA_Width,	   128,
//	WA_Height,	   96,
	TAG_DONE
	);

    }
        if (check) = TRUE
    {
        CloseWindow(wbwindow);
             wbwindow = OpenWindowTags(NULL,
    WA_ScreenTitle, "AROS Workbench Alpha 0.1 (No file navigation) FreeMem-xxxxxxx",
	WA_Title,	    "AROS Workbench Alpha Version 0.1 (No file navigation)",
	WA_Flags,	   (WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_ACTIVATE) ,
//	WA_Flags,	   (WFLG_ACTIVATE) ,
	WA_IDCMP,	   (IDCMP_MENUPICK),
	WA_Left,	   0,
	WA_Top,		   wbscreen->BarHeight + 1,
	WA_Width,	   wbscreen->Width,
	WA_Height,	   wbscreen->Height - wbscreen->BarHeight -1,
//	WA_Width,	   128,
//	WA_Height,	   96,
	TAG_DONE
	);

    }

}
/********************************************************************************************/
int main(void)
{
    if( InitWB() == 0 )
	return RETURN_FAIL;

    DoDetach();
    HandleAll();

    Cleanup(NULL);

    return RETURN_OK;
}

